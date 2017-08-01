#include <string.h>
#include <stdlib.h>
#include "urpc.h"

#if !defined(URPC_VERSION) || URPC_VERSION!=0
    #error u-RPC header file mismatch
#endif

//Private type redefitions
typedef __urpc_stream_t urpc_stream_t;
typedef __urpc_cb_pair_t urpc_cb_pair_t;
//u-RPC message handler type
typedef urpc_status_t (*urpc_msg_handler_t)(
    urpc_t*,
    urpc_stream_t*,
    uint16_t
);

//u-RPC version
const static uint8_t urpc_version = 0;
//u-RPC magic ("ur")
const static uint16_t urpc_magic = 29301;

//u-RPC type to size mapping
const static uint8_t urpc_type_size[] = {
    1, //URPC_TYPE_I8
    1, //URPC_TYPE_U8
    2, //URPC_TYPE_I16
    2, //URPC_TYPE_U16
    4, //URPC_TYPE_I32
    4, //URPC_TYPE_U32
    8, //URPC_TYPE_I64
    8, //URPC_TYPE_U64
    0, //URPC_TYPE_VARY
    2 //URPC_TYPE_FUNC
};
//u-RPC message handlers
const static urpc_msg_handler_t urpc_msg_handlers[];

/**
 * Marshall objects and write marshalled data to stream
 *
 * @param self u-RPC instance
 * @param strean Stream to hold marshalled data
 * @param sig_args Arguments signature
 * @param args Arguments
 * @return Operation status code
 */
static urpc_status_t urpc_marshall(
    urpc_t* self,
    urpc_stream_t* stream,
    urpc_sig_t sig_args,
    const void** args
) {
    uint8_t n_args = sig_args[0];

    for (uint8_t i=0;i<n_args;i++) {
        urpc_type_t type = sig_args[i+1];
        const void* arg = args[i];

        switch (type) {
            case URPC_TYPE_CALLBACK:
                return URPC_ERR_NO_SUPPORT;
            //Variable length data
            case URPC_TYPE_VARY: {
                urpc_vary_t* vary_arg = (urpc_vary_t*)arg;
                uint8_t vary_size;

                //Check data size
                if (vary_arg->size>=256)
                    return URPC_ERR_NO_MEMORY;
                vary_size = vary_arg->size;
                //Write data
                WIO_TRY(wio_write(stream, &vary_size, 1))
                WIO_TRY(wio_write(stream, vary_arg->data, vary_size))

                break;
            }
            //Other data types
            default: {
                WIO_TRY(wio_write(stream, arg, urpc_type_size[type]))
                break;
            }
        }
    }

    return WIO_OK;
}

/**
 * Unmarshall objects from input stream
 *
 * @param self u-RPC instance
 * @param in_stream Stream that provides marshalled data
 * @param out_stream Stream to store pointers to data
 * @param sig_args Arguments signature
 * @param args Unmarshalled arguments
 * @return Operation status code
 */
static urpc_status_t urpc_unmarshall(
    urpc_t* self,
    urpc_stream_t* in_stream,
    urpc_stream_t* out_stream,
    urpc_sig_t sig_args,
    void*** args
) {
    uint8_t n_args = sig_args[0];
    void** ptrs;

    //Reserve space for pointer table
    WIO_TRY(wio_alloc(
        out_stream,
        n_args*sizeof(void*),
        (uint8_t**)(&ptrs)
    ))

    for (uint8_t i=0;i<n_args;i++) {
        urpc_type_t type = sig_args[i+1];

        switch (type) {
            case URPC_TYPE_CALLBACK:
                return URPC_ERR_NO_SUPPORT;
            //Variable length data
            case URPC_TYPE_VARY: {
                urpc_vary_t* vary_arg;
                uint8_t vary_size;

                WIO_TRY(wio_alloc(out_stream, sizeof(urpc_vary_t), &vary_arg))
                //Size of variable length data
                WIO_TRY(wio_read(in_stream, &vary_size, 1))
                vary_arg->size = vary_size;
                //Data
                vary_arg->data = in_stream->buffer+in_stream->pos_a;
                in_stream->pos_a += vary_size;

                //Set pointer for current argument
                ptrs[i] = vary_arg;

                break;
            }
            //Other data types
            default: {
                //Set pointer for current argument
                ptrs[i] = in_stream->buffer+in_stream->pos_a;
                //Update cursor
                in_stream->pos_a += urpc_type_size[type];

                break;
            }
        }
    }

    //Pointers to data
    *args = ptrs;

    return WIO_OK;
}

/**
 * Build u-RPC message and write to stream.
 *
 * @param stream u-RPC stream
 * @param msg_type Message type
 * @param counter Message ID counter
 * @return Operation status code
 */
static urpc_status_t urpc_build_header(
    urpc_stream_t* stream,
    urpc_msg_t msg_type,
    uint16_t* counter
) {
    //Write magic and version
    WIO_TRY(wio_write(stream, &urpc_magic, 2))
    WIO_TRY(wio_write(stream, &urpc_version, 1))
    //Write message ID and type
    WIO_TRY(wio_write(stream, counter, 2))
    WIO_TRY(wio_write(stream, &msg_type, 1))
    //Update counter
    (*counter)++;

    return WIO_OK;
}

/**
 * Add a callback for given message ID.
 *
 * @param self u-RPC instance
 * @param msg_id Message ID
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status code
 */
static urpc_status_t urpc_add_callback(
    urpc_t* self,
    uint16_t msg_id,
    void* cb_data,
    urpc_callback_t cb
) {
    for (size_t i=0;i<self->_cb_size;i++) {
        urpc_cb_pair_t* pair = self->_cb_list+i;

        if (!pair->cb) {
            pair->msg_id = msg_id;
            pair->cb_data = cb_data;
            pair->cb = cb;

            return WIO_OK;
        }
    }

    return URPC_ERR_NO_MEMORY;
}

/**
 * Invoke and remove callback.
 *
 * @param self u-RPC instance
 * @param msg_id Message ID
 * @param status Operation status code
 * @param result Pointer to result data
 */
static urpc_status_t urpc_invoke_callback(
    urpc_t* self,
    uint16_t msg_id,
    urpc_status_t status,
    void* result
) {
    for (size_t i=0;i<self->_cb_size;i++) {
        urpc_cb_pair_t* pair = self->_cb_list+i;

        if (pair->msg_id==msg_id) {
            pair->cb(pair->cb_data, status, result);
            //Remove callback after invocation
            pair->cb = NULL;

            return WIO_OK;
        }
    }

    return URPC_ERR_BROKEN_MSG;
}

/**
 * {@inheritDoc}
 */
WIO_CALLBACK(urpc_on_recv) {
    //Temporary variable
    uint8_t tmp_u8;
    uint16_t tmp_u16;

    //Self
    urpc_t* self = (urpc_t*)data;
    //Message stream
    urpc_stream_t* msg_stream = (urpc_stream_t*)result;

    //Error occured when receiving
    if (status)
        return status;

    //Read and compare magic
    WIO_TRY(wio_read(msg_stream, &tmp_u16, 2))
    if (tmp_u16!=urpc_magic)
        return URPC_ERR_BROKEN_MSG;
    //Read and compare version
    WIO_TRY(wio_read(msg_stream, &tmp_u8, 1))
    if (tmp_u8!=urpc_version)
        return URPC_ERR_NO_SUPPORT;

    //Read message ID and type
    WIO_TRY(wio_read(msg_stream, &tmp_u16, 2))
    WIO_TRY(wio_read(msg_stream, &tmp_u8, 1))
    //Invoke corresponding message handler
    urpc_msg_handler_t handler = urpc_msg_handlers[tmp_u8];
    return handler(self, msg_stream, tmp_u16);
}

static urpc_status_t urpc_handle_error(
    urpc_t* self,
    urpc_stream_t* msg_stream,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    urpc_status_t status;

    //Read request message ID and error code
    WIO_TRY(wio_read(msg_stream, &req_msg_id, 2))
    WIO_TRY(wio_read(msg_stream, &status, 1))
    //Invoke callback
    WIO_TRY(urpc_invoke_callback(self, req_msg_id, status, NULL))

    return WIO_OK;
}

static urpc_status_t urpc_handle_func_resp(
    urpc_t* self,
    urpc_stream_t* msg_stream,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    urpc_func_t handle;

    //Request message ID
    WIO_TRY(wio_read(msg_stream, &req_msg_id, 2))
    //Remote function handle
    WIO_TRY(wio_read(msg_stream, &handle, 2))
    //Invoke callback
    WIO_TRY(urpc_invoke_callback(self, req_msg_id, WIO_OK, &handle))

    return WIO_OK;
}

static urpc_status_t urpc_handle_call_result(
    urpc_t* self,
    urpc_stream_t* msg_stream,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    uint8_t n_result;
    urpc_sig_t sig_rets;
    void** results;

    //Request message ID
    WIO_TRY(wio_read(msg_stream, &req_msg_id, 2))
    //Result signature
    WIO_TRY(wio_read(msg_stream, &n_result, 1))
    sig_rets = msg_stream->buffer+msg_stream->pos_a;
    msg_stream->pos_a += n_result;

    //Reset temporary buffer
    WIO_TRY(wio_reset(&self->_tmp_stream))
    //Unmarshall result
    WIO_TRY(urpc_unmarshall(self, msg_stream, &self->_tmp_stream, sig_rets, &results))

    //Invoke callback
    WIO_TRY(urpc_invoke_callback(self, req_msg_id, WIO_OK, results))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
urpc_status_t urpc_init(
    urpc_t* self,
    size_t funcs_size,
    size_t send_buf_size,
    size_t tmp_buf_size,
    void* send_func_data,
    urpc_status_t (*send_func)(void*, uint8_t*, size_t),
    size_t cb_size
) {
    //Send and temporary buffer
    uint8_t* send_buf;
    //Temporary buffer
    uint8_t* tmp_buf;
    //Callback functions list
    __urpc_cb_pair_t* cb_list;

    //Send and receive message counter
    self->_send_counter = 0;
    self->_recv_counter = 0;

    //Size of the function table
    self->_funcs_size = funcs_size;
    //TODO: Functions store initialization

    //Send stream
    send_buf = malloc(send_buf_size);
    if (!send_buf)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_send_stream, send_buf, send_buf_size))
    //Temporary stream for memory allocation
    tmp_buf = malloc(tmp_buf_size);
    if (!tmp_buf)
        return WIO_ERR_NO_MEMORY;
    WIO_TRY(wio_buf_init(&self->_tmp_stream, tmp_buf, tmp_buf_size))

    //Send function
    self->_send_func_data = send_func_data;
    self->_send_func = send_func;

    self->_cb_size = cb_size;
    //Callback pairs
    self->_cb_list = malloc(cb_size*sizeof(__urpc_cb_pair_t));
    if (!self->_cb_list)
        return WIO_ERR_NO_MEMORY;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
urpc_status_t urpc_get_func(
    urpc_t* self,
    const char* name,
    void* cb_data,
    urpc_callback_t cb
) {
    urpc_stream_t* send_stream = &self->_send_stream;
    uint16_t msg_id = self->_send_counter;
    uint8_t name_len;

    //Build header
    WIO_TRY(urpc_build_header(send_stream, URPC_MSG_FUNC_QUERY, &self->_send_counter))
    //Write function name
    name_len = (uint8_t)strlen(name);
    WIO_TRY(wio_write(send_stream, &name_len, 1))
    WIO_TRY(wio_write(send_stream, name, name_len))

    //Set callback data and function
    WIO_TRY(urpc_add_callback(self, msg_id, cb_data, cb))
    //TODO: Send data

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
urpc_status_t urpc_call(
    urpc_t* self,
    urpc_func_t handle,
    urpc_sig_t sig_args,
    const void** args,
    void* cb_data,
    urpc_callback_t cb
) {
    urpc_stream_t* send_stream = &self->_send_stream;
    uint16_t msg_id = self->_send_counter;

    //Build header
    WIO_TRY(urpc_build_header(send_stream, URPC_MSG_CALL, &self->_send_counter))
    //Write function handle, arguments and signature
    WIO_TRY(wio_write(send_stream, &handle, 2))
    WIO_TRY(wio_write(send_stream, sig_args, 1))
    WIO_TRY(wio_write(send_stream, sig_args+1, sig_args[0]))
    WIO_TRY(urpc_marshall(self, send_stream, sig_args, args))

    //Set callback data and callback function
    WIO_TRY(urpc_add_callback(self, msg_id, cb_data, cb))
    //TODO: Send data

    return WIO_OK;
}

//u-RPC message handlers
const static urpc_msg_handler_t urpc_msg_handlers[] = {
    urpc_handle_error, //URPC_MSG_ERROR
    NULL, //URPC_MSG_FUNC_QUERY
    urpc_handle_func_resp, //URPC_MSG_FUNC_RESP
    NULL, //URPC_MSG_CALL
    urpc_handle_call_result, //URPC_MSG_CALL_RESULT
};

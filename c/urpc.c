#include <string.h>
#include "urpc.h"

#if !defined(URPC_VERSION) || URPC_VERSION!=0
    #error u-RPC header file mismatch
#endif

//Simple exception handling marco
//(Usage is similar to rust's "try!")
#define URPC_TRY(expr) { \
        urpc_status_t status = expr; \
        if (status) \
            return status; \
    }

//Private type redefitions
typedef __urpc_stream_t urpc_stream_t;
typedef __urpc_cb_pair_t urpc_cb_pair_t;
//u-RPC message handler type
typedef urpc_status_t (*urpc_msg_handler_t)(
    urpc_t*,
    urpc_stream_t*,
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
 * Allocate memory of given size from stream.
 *
 * @param stream u-RPC stream
 * @param size Size of memory to reserve
 * @param mem Pointer to reserved memory
 * @returns Operation status code
 */
static urpc_status_t urpc_alloc_mem(
    urpc_stream_t* stream,
    size_t size,
    uint8_t** mem
) {
    //Check if remaining space if enough
    if (stream->pos+size>stream->size)
        return URPC_ERR_NO_MEMORY;
    //Set pointer and update position
    *mem = stream->buffer+stream->pos;
    stream->pos += size;

    return URPC_OK;
}

/**
 * Write variable length data to stream.
 *
 * @param stram u-RPC stream
 * @param data Data to write
 * @param size Size of the data
 * @returns Operation status code
 */
static inline urpc_status_t urpc_write_vary(
    urpc_stream_t* stream,
    const void* data,
    uint8_t size
) {
    uint8_t* begin;

    //Allocate space from stream
    URPC_TRY(urpc_alloc_mem(stream, size, &begin))
    //Write data
    memmove(begin, data, size);

    return URPC_OK;
}

/**
 * Write data to send stream.
 *
 * @param stream u-RPC stream
 * @param data Data to write
 * @param type Data type
 * @returns Operation status code
 */
static inline urpc_status_t urpc_write_data(
    urpc_stream_t* stream,
    const void* data,
    urpc_type_t type
) {
    return urpc_write_vary(stream, data, urpc_type_size[type]);
}

/**
 * Read variable length data from stream.
 *
 * @param stream u-RPC stream
 * @param data Data to read
 * @param size Size of the data
 * @return Operation status code
 */
static inline urpc_status_t urpc_read_vary(
    urpc_stream_t* stream,
    void* data,
    size_t size
) {
    uint8_t* begin = stream->buffer+stream->pos;

    //Reading range exceeds buffer area
    if (stream->pos+size>stream->size)
        return URPC_ERR_BROKEN_MSG;
    //Copy data
    memmove(data, begin, size);
    //Update position
    stream->pos += size;

    return URPC_OK;
}

/**
 * Read data from stream.
 *
 * @param stream u-RPC stream
 * @param data Data to read
 * @param type Data type
 * @return Operation status code
 */
static inline urpc_status_t urpc_read_data(
    urpc_stream_t* stream,
    void* data,
    urpc_type_t type
) {
    return urpc_read_vary(stream, data, urpc_type_size[type]);
}

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
                //Write data
                URPC_TRY(urpc_write_data(stream, &vary_size, URPC_TYPE_U8))
                URPC_TRY(urpc_write_vary(stream, vary_arg->data, vary_size))

                break;
            }
            //Other data types
            default: {
                URPC_TRY(urpc_write_data(stream, arg, urpc_type_size[type]))
                break;
            }
        }
    }

    return URPC_OK;
}

/**
 * Unmarshall objects from input stream
 *
 * @param self u-RPC instance
 * @param in_stream Stream that provides marshalled data
 * @param out_stream Stream to store unmarshalled data
 * @param sig_args Arguments signature
 * @param args Unmarshalled arguments
 * @return Operation status code
 */
static urpc_status_t urpc_unmarshall(
    urpc_t* self,
    urpc_stream_t* in_stream,
    urpc_stream_t* out_stream,
    urpc_sig_t sig_args,
    void** args
) {
    uint8_t n_args = sig_args[0];
    void** ptrs;

    //Reserve space for pointer table
    URPC_TRY(urpc_alloc_mem(
        out_stream,
        n_args*sizeof(void*),
        (uint8_t**)(&ptrs)
    ))

    for (uint8_t i=0;i<n_args;i++) {
        urpc_type_t type = sig_args[i+1];

        //TODO: Unmarshall arguments with type info
        //Should we copy data from WISP firmware buffer?
    }

    return URPC_OK;
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
    URPC_TRY(urpc_write_data(stream, &urpc_magic, URPC_TYPE_U16))
    URPC_TRY(urpc_write_data(stream, &urpc_version, URPC_TYPE_U8))
    //Write message ID and type
    URPC_TRY(urpc_write_data(stream, counter, URPC_TYPE_U16))
    URPC_TRY(urpc_write_data(stream, &msg_type, URPC_TYPE_U8))
    //Update counter
    (*counter)++;

    return URPC_OK;
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
    for (size_t i=0;i<self->_cb_max;i++) {
        urpc_cb_pair_t* pair = self->_cb_list+i;

        if (!pair->cb) {
            pair->msg_id = msg_id;
            pair->cb_data = cb_data;
            pair->cb = cb;

            return URPC_OK;
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
    for (size_t i=0;i<self->_cb_max;i++) {
        urpc_cb_pair_t* pair = self->_cb_list+i;

        if (pair->msg_id==msg_id) {
            pair->cb(pair->cb_data, status, result);
            //Remove callback after invocation
            pair->cb = NULL;

            return URPC_OK;
        }
    }

    return URPC_ERR_BROKEN_MSG;
}

static urpc_status_t urpc_handle_msg(
    urpc_t* self,
    urpc_stream_t* req,
    urpc_stream_t* res
) {
    uint8_t tmp_u8;
    uint16_t tmp_u16;

    //Read and compare magic
    URPC_TRY(urpc_read_data(req, &tmp_u16, URPC_TYPE_U16))
    if (tmp_u16!=urpc_magic)
        return URPC_ERR_BROKEN_MSG;
    //Read and compare version
    URPC_TRY(urpc_read_data(req, &tmp_u8, URPC_TYPE_U8))
    if (tmp_u8!=urpc_version)
        return URPC_ERR_NO_SUPPORT;

    //Read message ID and type
    URPC_TRY(urpc_read_data(req, &tmp_u16, URPC_TYPE_U16))
    URPC_TRY(urpc_read_data(req, &tmp_u8, URPC_TYPE_U8))
    //Invoke corresponding message handler
    urpc_msg_handler_t handler = urpc_msg_handlers[tmp_u8];
    return handler(self, req, res, tmp_u16);
}

static urpc_status_t urpc_handle_error(
    urpc_t* self,
    urpc_stream_t* req,
    urpc_stream_t* res,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    urpc_status_t status;

    //Read request message ID and error code
    URPC_TRY(urpc_read_data(req, &req_msg_id, URPC_TYPE_U16))
    URPC_TRY(urpc_read_data(req, &status, URPC_TYPE_U8))
    //Invoke callback
    URPC_TRY(urpc_invoke_callback(self, req_msg_id, status, NULL))

    return URPC_OK;
}

static urpc_status_t urpc_handle_func_resp(
    urpc_t* self,
    urpc_stream_t* req,
    urpc_stream_t* res,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    urpc_func_t handle;

    //Request message ID
    URPC_TRY(urpc_read_data(req, &req_msg_id, URPC_TYPE_U16))
    //Remote function handle
    URPC_TRY(urpc_read_data(req, &handle, URPC_TYPE_U16))
    //Invoke callback
    URPC_TRY(urpc_invoke_callback(self, req_msg_id, URPC_OK, &handle))

    return URPC_OK;
}

static urpc_status_t urpc_handle_call_result(
    urpc_t* self,
    urpc_stream_t* req,
    urpc_stream_t* tmp_buf,
    uint16_t msg_id
) {
    uint16_t req_msg_id;
    uint8_t n_result;
    urpc_sig_t sig_rets;
    void** results;

    //Request message ID
    URPC_TRY(urpc_read_data(req, &req_msg_id, URPC_TYPE_U16))
    //Result signature
    URPC_TRY(urpc_read_data(req, &n_result, URPC_TYPE_U8))
    URPC_TRY(urpc_alloc_mem(tmp_buf, n_result+1, &sig_rets))
    *sig_rets = n_result;
    URPC_TRY(urpc_read_vary(req, sig_rets+1, n_result))
    //Unmarshall result
    URPC_TRY(urpc_unmarshall(self, req, tmp_buf, sig_rets, results))

    //Invoke callback
    URPC_TRY(urpc_invoke_callback(self, req_msg_id, URPC_OK, results))

    return URPC_OK;
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
    URPC_TRY(urpc_build_header(send_stream, URPC_MSG_FUNC_QUERY, &self->_send_counter))
    //Write function name
    name_len = (uint8_t)strlen(name);
    URPC_TRY(urpc_write_data(send_stream, &name_len, URPC_TYPE_U8))
    URPC_TRY(urpc_write_vary(send_stream, name, name_len))

    //Set callback data and function
    URPC_TRY(urpc_add_callback(self, msg_id, cb_data, cb));
    //TODO: Send data

    return URPC_OK;
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
    URPC_TRY(urpc_build_header(send_stream, URPC_MSG_CALL, &self->_send_counter))
    //Write function handle, arguments and signature
    URPC_TRY(urpc_write_data(send_stream, &handle, URPC_TYPE_U16))
    URPC_TRY(urpc_write_data(send_stream, sig_args, URPC_TYPE_U8))
    URPC_TRY(urpc_write_vary(send_stream, sig_args+1, sig_args[0]))
    URPC_TRY(urpc_marshall(self, send_stream, sig_args, args))

    //Set callback data and callback function
    URPC_TRY(urpc_add_callback(self, msg_id, cb_data, cb))
    //TODO: Send data

    return URPC_OK;
}

//u-RPC message handlers
const static urpc_msg_handler_t urpc_msg_handlers[] = {
    urpc_handle_error, //URPC_MSG_ERROR
    NULL, //URPC_MSG_FUNC_QUERY
    urpc_handle_func_resp, //URPC_MSG_FUNC_RESP
    NULL, //URPC_MSG_CALL
    urpc_handle_call_result, //URPC_MSG_CALL_RESULT
};

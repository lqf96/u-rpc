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
//u-RPC magic
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
    2, //URPC_TYPE_OBJ
    2 //URPC_TYPE_FUNC
};
//u-RPC message handlers
const static urpc_msg_handler_t urpc_msg_handlers[];

/**
 * Write data to send stream.
 *
 * @param stream u-RPC stream
 * @param data Data to write
 * @param type Data type
 * @returns Operation status code
 */
static urpc_status_t urpc_write_data(
    urpc_stream_t* stream,
    const void* data,
    urpc_type_t type
) {
    size_t data_size = urpc_type_size[type];
    uint8_t* begin = stream->buffer+stream->pos;

    //Check if there is enough space to write
    if (stream->pos+data_size>stream->size)
        return URPC_ERR_NO_MEMORY;
    //Write data
    memmove(begin, data, data_size);
    //Update position
    stream->pos += data_size;

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
static urpc_status_t urpc_write_vary(
    urpc_stream_t* stream,
    const void* data,
    size_t size
) {
    uint8_t* begin = stream->buffer+stream->pos;

    //Check data size
    if (size>=256)
        return URPC_ERR_NO_MEMORY;
    //Check if there is enough space to write
    if (stream->pos+size>=stream->size)
        return URPC_ERR_NO_MEMORY;
    //Write data length
    *begin = (uint8_t)size;
    //Write data
    memmove(begin+1, data, size);
    //Update position
    stream->pos += size+1;

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
static urpc_status_t urpc_read_data(
    urpc_stream_t* stream,
    void* data,
    urpc_type_t type
) {
    uint8_t* begin = stream->buffer+stream->pos;
    uint8_t size = urpc_type_size[type];

    //Reading range exceeds buffer area
    if (stream->pos+size>=stream->size)
        return URPC_ERR_BROKEN_MSG;
    //Copy data
    memmove(data, begin, size);
    //Update position
    stream->pos += size;

    return URPC_OK;
}

/**
 * Read variable length data from stream.
 *
 * @param stream u-RPC stream
 * @param data Data to read
 * @return Operation status code
 */
static urpc_status_t urpc_read_vary(
    urpc_stream_t* stream,
    void* data
) {
    uint8_t* begin = stream->buffer+stream->pos;
    size_t size = *begin;

    //Reading range exceeds buffer area
    if (stream->pos+size>=stream->size)
        return URPC_ERR_BROKEN_MSG;
    //Copy data
    memmove(data, begin+1, size);
    //Update position
    stream->pos += size;

    return URPC_OK;
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
            case URPC_TYPE_OBJ:
            case URPC_TYPE_CALLBACK:
                return URPC_ERR_NO_SUPPORT;
            //Variable length data
            case URPC_TYPE_VARY: {
                urpc_vary_t* vary_arg = (urpc_vary_t*)arg;

                URPC_TRY(urpc_write_vary(stream, vary_arg->data, vary_arg->size))
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
 * Unmarshall objects from buffer
 *
 * @param self u-RPC instance
 * @param in_stream Stream that provides marshalled data
 * @param out_stream Stream to store unmarshalled data
 * @param sig_args Arguments signature
 * @return Operation status code
 */
static urpc_status_t urpc_unmarshall(
    urpc_t* self,
    urpc_stream_t* in_stream,
    urpc_stream_t* out_stream,
    urpc_sig_t sig_args
) {
    uint8_t n_args = sig_args[0];
    void** ptrs;

    //Check space
    size_t ptr_size = n_args*sizeof(void*);
    if (out_stream->pos+ptr_size>out_stream->size)
        return URPC_ERR_NO_MEMORY;
    //Reserve space for pointer table
    ptrs = (void**)(out_stream->buffer+out_stream->pos);
    out_stream->pos += ptr_size;

    for (uint8_t i=0;i<n_args;i++) {
        urpc_type_t type = sig_args[i+1];

        //TODO: Unmarshall arguments with type info
        //Should we copy data from WISP firmware buffer?
    }

    return URPC_OK;
}

static urpc_status_t urpc_build_header(
    urpc_stream_t* buf,
    urpc_msg_t msg_type,
    uint16_t* counter
) {
    //Write magic and version
    URPC_TRY(urpc_write_data(buf, &urpc_magic, URPC_TYPE_U16))
    URPC_TRY(urpc_write_data(buf, &urpc_version, URPC_TYPE_U8))
    //Write message ID and type
    URPC_TRY(urpc_write_data(buf, counter, URPC_TYPE_U16))
    URPC_TRY(urpc_write_data(buf, &msg_type, URPC_TYPE_U8))
    //Update counter
    (*counter)++;

    return URPC_OK;
}

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

    //Read request message ID
    URPC_TRY(urpc_read_data(req, &req_msg_id, URPC_TYPE_U16))
    //TODO: urpc_read_vary() issue

    return URPC_OK;
}

static urpc_status_t urpc_handle_call_result(
    urpc_t* self,
    urpc_stream_t* req,
    urpc_stream_t* res,
    uint16_t msg_id
) {
    uint16_t req_msg_id;

    //Read request message ID
    URPC_TRY(urpc_read_data(req, &req_msg_id, URPC_TYPE_U16))
    //TODO: urpc_read_vary() issue
    //TODO: Unmarshall

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
    urpc_stream_t* send_buf = &self->_send_buf;
    uint16_t msg_id = self->_send_counter;

    //Build header
    URPC_TRY(urpc_build_header(send_buf, URPC_MSG_FUNC_QUERY, &self->_send_counter))
    //Write function name
    URPC_TRY(urpc_write_vary(send_buf, name, strlen(name)))

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
    urpc_stream_t* send_buf = &self->_send_buf;
    uint16_t msg_id = self->_send_counter;

    //Build header
    URPC_TRY(urpc_build_header(send_buf, URPC_MSG_CALL, &self->_send_counter))
    //Write function handle, arguments and signature
    URPC_TRY(urpc_write_data(send_buf, &handle, URPC_TYPE_U16))
    URPC_TRY(urpc_write_vary(send_buf, sig_args+1, sig_args[0]))
    URPC_TRY(urpc_marshall(self, send_buf, sig_args, args))

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

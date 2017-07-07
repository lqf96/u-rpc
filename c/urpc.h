#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//u-RPC version 0
#define URPC_VERSION 0
//u-RPC function signature builder
#define URPC_SIG(n_objs, ...) ((urpc_type_t[]){n_objs, __VA_ARGS__})

//Operation status code type
typedef uint8_t urpc_status_t;
//Message type
typedef uint8_t urpc_msg_t;
//Data type type
typedef uint8_t urpc_type_t;
//Remote function type
typedef uint16_t urpc_func_t;
//Remote object type
typedef uint16_t urpc_obj_t;
//Signature type
typedef uint8_t* urpc_sig_t;
//Callback type
typedef void (*urpc_callback_t)(void*, urpc_status_t, void*);

//u-RPC variable length data container type
typedef struct urpc_vary {
    //Size of data
    size_t size;
    //Pointer to data
    void* data;
} urpc_vary_t;

//u-RPC buffer type
typedef struct __urpc_buf {
    //Buffer
    uint8_t* buffer;
    //Position of next available byte
    size_t pos;
    //Send buffer size
    size_t size;
} __urpc_stream_t;

//u-RPC callback pair type
typedef struct __urpc_cb_pair {
    //Message ID
    uint16_t msg_id;
    //Callback function closure data
    void* cb_data;
    //Callback function
    urpc_callback_t cb;
} __urpc_cb_pair_t;

//u-RPC instance type
typedef struct urpc {
    //Send message counter
    uint16_t _send_counter;
    //Receive message counter
    uint16_t _recv_counter;

    //Index of next available item
    size_t _funcs_begin;
    //Size of the table
    size_t _funcs_size;
    //Function allocation table
    void* _funcs_store;

    //Send buffer
    __urpc_stream_t _send_buf;
    //Receive buffer
    __urpc_stream_t _recv_buf;

    //Send function closure data
    void* _send_func_data;
    //Send function
    urpc_status_t (*_send_func)(void*, uint8_t*, size_t);

    //Callback pairs
    __urpc_cb_pair_t* _cb_list;
    //Number of callback pairs
    size_t _cb_size;
    //Maximum amount of callback pairs
    size_t _cb_max;
} urpc_t;

//Signed 8-bit data
const urpc_type_t URPC_TYPE_I8 = 0x00;
//Unsigned 8-bit data
const urpc_type_t URPC_TYPE_U8 = 0x01;
//Signed 16-bit data
const urpc_type_t URPC_TYPE_I16 = 0x02;
//Signed 8-bit data
const urpc_type_t URPC_TYPE_U16 = 0x03;
//Signed 32-bit data
const urpc_type_t URPC_TYPE_I32 = 0x04;
//Unsigned 32-bit data
const urpc_type_t URPC_TYPE_U32 = 0x05;
//Signed 64-bit data
const urpc_type_t URPC_TYPE_I64 = 0x06;
//Unsigned 64-bit data
const urpc_type_t URPC_TYPE_U64 = 0x07;
//Variable length data
const urpc_type_t URPC_TYPE_VARY = 0x08;
//Remote object handle
const urpc_type_t URPC_TYPE_OBJ = 0x09;
//Callback handle
const urpc_type_t URPC_TYPE_CALLBACK = 0x0a;

//Operation successfully completed
const urpc_status_t URPC_OK = 0x00;
//Incorrect function signature
const urpc_status_t URPC_ERR_SIG_INCORRECT = 0x01;
//Nonexist handle
const urpc_status_t URPC_ERR_NONEXIST = 0x02;
//Operation not supported
const urpc_status_t URPC_ERR_NO_SUPPORT = 0x03;
//Out of memory; message too long
const urpc_status_t URPC_ERR_NO_MEMORY = 0x04;
//Broken u-RPC message
const urpc_status_t URPC_ERR_BROKEN_MSG = 0x05;
//Function call throws exception
const urpc_status_t URPC_ERR_EXCEPTION = 0x06;

//Error message
const urpc_msg_t URPC_MSG_ERROR = 0x00;
//Function query message
const urpc_msg_t URPC_MSG_FUNC_QUERY = 0x01;
//Function query response message
const urpc_msg_t URPC_MSG_FUNC_RESP = 0x02;
//Function call message
const urpc_msg_t URPC_MSG_CALL = 0x03;
//Function call result message
const urpc_msg_t URPC_MSG_CALL_RESULT = 0x04;

//TODO: Init function

/**
 * Callback function for incoming u-RPC messages.
 *
 * @param self u-RPC instance
 * @param data Incoming data
 * @param size Size of the data
 * @return Operation status code
 */
extern urpc_status_t urpc_on_recv(
    urpc_t* self,
    const char* data,
    size_t size
);

/**
 * Get handle of remote function
 *
 * @param self u-RPC instance
 * @param name Name of the function
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status code if failed
 */
extern urpc_status_t urpc_get_func(
    urpc_t* self,
    const char* name,
    void* cb_data,
    urpc_callback_t cb
);

/**
 * Do u-RPC call
 *
 * @param self u-RPC instance
 * @param handle Remote function handle
 * @param sig_args Arguments signature
 * @param args Arguments
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Operation status code if failed
 */
extern urpc_status_t urpc_call(
    urpc_t* self,
    urpc_func_t handle,
    urpc_sig_t sig_args,
    const void** args,
    void* cb_data,
    urpc_callback_t cb
);

#ifdef __cplusplus
}
#endif

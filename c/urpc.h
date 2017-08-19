#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "wio-shim.h"

//=== u-RPC helper marcos definitions ===
//u-RPC version 0
#define URPC_VERSION 0
//u-RPC function signature builder
#define URPC_SIG(n_objs, ...) ((urpc_type_t[]){n_objs, __VA_ARGS__})
//u-RPC arguments builder
#define URPC_ARG(...) ((const void*[]){__VA_ARGS__})

//=== u-RPC types ===
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

//Status code type
typedef wio_status_t urpc_status_t;
//Callback type
typedef wio_callback_t urpc_callback_t;
//u-RPC stream & buffer type
typedef wio_buf_t __urpc_stream_t;

//Send function type
typedef urpc_status_t (*urpc_send_func_t)(
    void*,
    uint8_t*,
    uint16_t,
    void*,
    urpc_callback_t
);

//u-RPC callback pair type
typedef struct __urpc_cb_pair {
    //Message ID
    uint16_t msg_id;
    //Callback function closure data
    void* cb_data;
    //Callback function
    urpc_callback_t cb;
} __urpc_cb_pair_t;

//u-RPC variable length data type
typedef struct urpc_vary {
    //Data
    uint8_t* data;
    //Size
    uint16_t size;
} urpc_vary_t;

//u-RPC instance type
typedef struct urpc {
    //Send message counter
    uint16_t _send_counter;
    //Receive message counter
    uint16_t _recv_counter;

    //Index of next available item
    uint16_t _funcs_begin;
    //Size of the table
    uint16_t _funcs_size;
    //Function allocation table
    void* _funcs_store;

    //Send stream
    __urpc_stream_t _send_stream;
    //Temporary stream for memory allocation
    __urpc_stream_t _tmp_stream;

    //Send function closure data
    void* _send_func_data;
    //Send function
    urpc_send_func_t _send_func;

    //Callback pairs
    __urpc_cb_pair_t* _cb_list;
    //Capacity of callback pairs
    uint16_t _cb_size;
} urpc_t;

//Signed 8-bit data
static const urpc_type_t URPC_TYPE_I8 = 0x00;
//Unsigned 8-bit data
static const urpc_type_t URPC_TYPE_U8 = 0x01;
//Signed 16-bit data
static const urpc_type_t URPC_TYPE_I16 = 0x02;
//Signed 8-bit data
static const urpc_type_t URPC_TYPE_U16 = 0x03;
//Signed 32-bit data
static const urpc_type_t URPC_TYPE_I32 = 0x04;
//Unsigned 32-bit data
static const urpc_type_t URPC_TYPE_U32 = 0x05;
//Signed 64-bit data
static const urpc_type_t URPC_TYPE_I64 = 0x06;
//Unsigned 64-bit data
static const urpc_type_t URPC_TYPE_U64 = 0x07;
//Variable length data
static const urpc_type_t URPC_TYPE_VARY = 0x08;
//Callback handle
static const urpc_type_t URPC_TYPE_CALLBACK = 0x09;

//Incorrect function signature
static const urpc_status_t URPC_ERR_SIG_INCORRECT = 0x20;
//Nonexist handle
static const urpc_status_t URPC_ERR_NONEXIST = 0x21;
//Operation not supported
static const urpc_status_t URPC_ERR_NO_SUPPORT = 0x22;
//Out of memory; message too long
static const urpc_status_t URPC_ERR_NO_MEMORY = 0x23;
//Broken u-RPC message
static const urpc_status_t URPC_ERR_BROKEN_MSG = 0x24;
//Function call throws exception
static const urpc_status_t URPC_ERR_EXCEPTION = 0x25;

//Error message
static const urpc_msg_t URPC_MSG_ERROR = 0x00;
//Function query message
static const urpc_msg_t URPC_MSG_FUNC_QUERY = 0x01;
//Function query response message
static const urpc_msg_t URPC_MSG_FUNC_RESP = 0x02;
//Function call message
static const urpc_msg_t URPC_MSG_CALL = 0x03;
//Function call result message
static const urpc_msg_t URPC_MSG_CALL_RESULT = 0x04;

/**
 * Initialize u-RPC instance.
 */
extern urpc_status_t urpc_init(
    urpc_t* self,
    uint16_t funcs_size,
    uint16_t send_buf_size,
    uint16_t tmp_buf_size,
    void* send_func_data,
    urpc_send_func_t send_func,
    uint16_t cb_size
);

/**
 * Callback function for incoming u-RPC messages.
 */
extern WIO_CALLBACK(urpc_on_recv);

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

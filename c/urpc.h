#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wio-shim.h"

//=== u-RPC helper marcos definitions ===
/// u-RPC version (Currently version 1)
#define URPC_VERSION 1
/**
 * @brief u-RPC arguments and results signature builder.
 */
#define URPC_SIG(n_objs, ...) ((urpc_type_t[]){n_objs, __VA_ARGS__})
/// u-RPC call arguments builder
#define URPC_ARG(...) ((const void*[]){__VA_ARGS__})
///
#define URPC_VARY(data, size) &((urpc_vary_t){data, size})
#define URPC_VARY_CONST(data, size) &((const urpc_vary_t){data, size})

//=== u-RPC types ===
/// u-RPC message type type
typedef uint8_t urpc_msg_t;
/// u-RPC data type type
typedef uint8_t urpc_type_t;
/// u-RPC remote function handle type
typedef uint16_t urpc_func_t;
/// u-RPC signature type
typedef uint8_t* urpc_sig_t;

/// u-RPC status code type
typedef wio_status_t urpc_status_t;
/// u-RPC callback type
typedef wio_callback_t urpc_callback_t;
/// u-RPC data stream and buffer type
typedef wio_buf_t __urpc_stream_t;

/// u-RPC send message function type
typedef urpc_status_t (*urpc_send_func_t)(
    void*,
    uint8_t*,
    uint16_t,
    void*,
    urpc_callback_t
);

/// u-RPC callback pair type
typedef struct __urpc_cb_pair {
    /// Message ID
    uint16_t msg_id;
    /// Callback function closure data
    void* cb_data;
    /// Callback function
    urpc_callback_t cb;
} __urpc_cb_pair_t;

/// u-RPC variable length data type
typedef struct urpc_vary {
    // Data
    void* data;
    // Size
    uint16_t size;
} urpc_vary_t;

/// u-RPC instance type
typedef struct urpc {
    /// Send message counter
    uint16_t _send_counter;
    /// Receive message counter
    uint16_t _recv_counter;

    /// Index of next available item
    uint16_t _funcs_begin;
    /// Size of the function table
    uint16_t _funcs_size;
    /// Function allocation table
    void* _funcs_store;

    /// Send stream
    __urpc_stream_t _send_stream;
    /// Temporary stream for memory allocation
    __urpc_stream_t _tmp_stream;

    /// Send function closure data
    void* _send_func_data;
    /// Send function
    urpc_send_func_t _send_func;

    /// Callback pairs
    __urpc_cb_pair_t* _cb_list;
    /// Capacity of callback pairs
    uint16_t _cb_size;
} urpc_t;

//=== u-RPC data types ===
/// Signed 8-bit data
static const urpc_type_t URPC_TYPE_I8 = 0x00;
/// Unsigned 8-bit data
static const urpc_type_t URPC_TYPE_U8 = 0x01;
/// Signed 16-bit data
static const urpc_type_t URPC_TYPE_I16 = 0x02;
/// Unsigned 16-bit data
static const urpc_type_t URPC_TYPE_U16 = 0x03;
/// Signed 32-bit data
static const urpc_type_t URPC_TYPE_I32 = 0x04;
/// Unsigned 32-bit data
static const urpc_type_t URPC_TYPE_U32 = 0x05;
/// Signed 64-bit data
static const urpc_type_t URPC_TYPE_I64 = 0x06;
/// Unsigned 64-bit data
static const urpc_type_t URPC_TYPE_U64 = 0x07;
/// Variable length data
static const urpc_type_t URPC_TYPE_VARY = 0x08;

//=== u-RPC error codes ===
/// Incorrect function signature
static const urpc_status_t URPC_ERR_SIG_INCORRECT = 0x20;
/// Nonexist handle
static const urpc_status_t URPC_ERR_NONEXIST = 0x21;
/// Operation not supported
static const urpc_status_t URPC_ERR_NO_SUPPORT = 0x22;
/// Broken u-RPC message
static const urpc_status_t URPC_ERR_BROKEN_MSG = 0x23;
/// Function call throws exception
static const urpc_status_t URPC_ERR_EXCEPTION = 0x24;

//=== u-RPC message types ===
/// Error message
static const urpc_msg_t URPC_MSG_ERROR = 0x00;
/// Function query message
static const urpc_msg_t URPC_MSG_FUNC_QUERY = 0x01;
/// Function query response message
static const urpc_msg_t URPC_MSG_FUNC_RESP = 0x02;
/// Function call message
static const urpc_msg_t URPC_MSG_CALL = 0x03;
/// Function call result message
static const urpc_msg_t URPC_MSG_CALL_RESULT = 0x04;

/**
 * @brief Initialize u-RPC instance.
 *
 * @param self u-RPC instance.
 * @param funcs_size Size of the function table.
 * @param send_buf_size Size of the sending buffer.
 * @param tmp_buf_size Size of the temporary buffer.
 * @param send_func_data Closure data of send function.
 * @param send_func Send function.
 * @param cb_size Maximum amount of u-RPC callbacks.
 * @return Error code if failed, otherwise WIO_OK.
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
 * @brief Get handle of remote function.
 *
 * @param self u-RPC instance.
 * @param name Name of the function.
 * @param cb_data Callback closure data.
 * @param cb Callback function.
 * @return Error code if failed, otherwise WIO_OK.
 */
extern urpc_status_t urpc_get_func(
    urpc_t* self,
    const char* name,
    void* cb_data,
    urpc_callback_t cb
);

/**
 * @brief Do u-RPC call
 *
 * @param self u-RPC instance
 * @param handle Remote function handle
 * @param sig_args Arguments signature
 * @param args Arguments
 * @param cb_data Callback closure data
 * @param cb Callback function
 * @return Error code if failed, otherwise WIO_OK.
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

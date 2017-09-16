#pragma once

#include <stdint.h>

//=== WIO helper marcos ===
/**
 * @brief WIO error handling marco.
 *
 * The marco continues execution if expression returns WIO_OK,
 * or immediately return from current function is expression returns something else.
 * Its usage is similar to rust's "try!" marco.
 */
#define WIO_TRY(expr) { \
        wio_status_t __status = expr; \
        if (__status) \
            return __status; \
    }
/// Shorthand for declaring a WIO callback function.
#define WIO_CALLBACK(name) \
    wio_status_t name(void* data, wio_status_t status, void* result)
/// Create an anonymous variable on stack and return a pointer to it.
#define WIO_INST_PTR(type) \
    &((type){})

//=== WIO type definitions ===
/// WIO status type
typedef uint8_t wio_status_t;
/// WIO callback type
typedef wio_status_t (*wio_callback_t)(void*, wio_status_t, void*);

/// WIO buffer type
typedef struct wio_buf {
    /// Buffer memory
    uint8_t* buffer;
    /// Buffer size
    uint16_t size;

    /// Cursor A (Read cursor)
    uint16_t pos_a;
    /// Cursor B (Write cursor)
    uint16_t pos_b;
} wio_buf_t;

//=== WIO error codes ===
/// No error
static const wio_status_t WIO_OK = 0x00;
/// Out of range
static const wio_status_t WIO_ERR_OUT_OF_RANGE = 0x01;
/// No memory
static const wio_status_t WIO_ERR_NO_MEMORY = 0x02;
/// Already in use
static const wio_status_t WIO_ERR_ALREADY = 0x03;
/// Invalid parameter
static const wio_status_t WIO_ERR_INVALID = 0x04;
/// Empty data structure
static const wio_status_t WIO_ERR_EMPTY = 0x05;

//=== WIO buffer functions ===
/**
 * @brief Initialize WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param buffer Underlying buffer.
 * @param size Size of underlying buffer.
 * @return WIO_OK.
 */
extern wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    uint16_t size
);

/**
 * @brief Initialize WIO buffer with dynamically allocated memory.
 *
 * @param self WIO buffer instance.
 * @param size Size of underlying buffer.
 * @return WIO_ERR_NO_MEMORY if allocation failed, otherwise WIO_OK.
 */
extern wio_status_t wio_buf_alloc_init(
    wio_buf_t* self,
    uint16_t size
);

/**
 * @brief Read data from WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param data Pointer to memory for holding read data.
 * @param size Size of data to read.
 * @return WIO_ERR_OUT_OF_RANGE if read beyond buffer range, otherwise WIO_OK.
 */
extern wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    uint16_t size
);

/**
 * @brief Write data to WIO buffer.
 *
 * @param self WIO buffer instance.
 * @param data Pointer to write data.
 * @param size Size of data to write.
 * @return WIO_ERR_OUT_OF_RANGE if write beyond buffer range, otherwise WIO_OK.
 */
extern wio_status_t wio_write(
    wio_buf_t* self,
    const void* data,
    uint16_t size
);

/**
 * @brief Copy data from one WIO buffer to another.
 *
 * @param from Source WIO buffer.
 * @param to Target WIO buffer.
 * @param size Size of data to copy.
 * @return WIO_ERR_OUT_OF_RANGE if out-of-range read or write happens, otherwise WIO_OK.
 */
extern wio_status_t wio_copy(
    wio_buf_t* from,
    wio_buf_t* to,
    uint16_t size
);

/**
 * @brief Allocate memory from WIO buffer in a circlular manner.
 *
 * @param self WIO buffer instance.
 * @param size Size of the memory to allocate.
 * @param _ptr Pointer to memory for holding pointer to allocated memory.
 * @return WIO_ERR_NO_MEMORY if no memory available for allocation, otherwise WIO_OK.
 */
extern wio_status_t wio_alloc(
    wio_buf_t* self,
    uint16_t size,
    void* _ptr
);

/**
 * @brief Free memory from WIO buffer in a circular manner.
 *
 * @param self WIO buffer instance.
 * @param size Size of the memory to free.
 * @return WIO_OK.
 */
extern wio_status_t wio_free(
    wio_buf_t* self,
    uint16_t size
);

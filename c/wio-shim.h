#pragma once

#include <stdint.h>

//(Usage is similar to rust's "try!")
#define WIO_TRY(expr) { \
        wio_status_t status = expr; \
        if (status) \
            return status; \
    }
//Callback declaration helper
#define WIO_CALLBACK(name) \
    wio_status_t name(void* data, wio_status_t status, void* result)

//WIO status code type
typedef uint8_t wio_status_t;
//WIO callback type
typedef wio_status_t (*wio_callback_t)(void*, wio_status_t, void*);

//WIO buffer type
typedef struct wio_buf {
    //Buffer
    uint8_t* buffer;
    //Buffer size
    uint16_t size;

    //Cursor A (Reading)
    uint16_t pos_a;
    //Cursor B (Writing)
    uint16_t pos_b;
} wio_buf_t;

//No error
const wio_status_t WIO_OK = 0x00;
//Out of range
const wio_status_t WIO_ERR_OUT_OF_RANGE = 0x01;
//No memory
const wio_status_t WIO_ERR_NO_MEMORY = 0x02;
//Already in use
const wio_status_t WIO_ERR_ALREADY = 0x03;

/**
 * Initialize WIO buffer.
 *
 * @param self WIO buffer instance
 * @param buffer Underlying buffer
 * @param size Size of underlying buffer
 * @return Operation status
 */
extern wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    uint16_t size
);

/**
 * Read data from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Read data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    uint16_t size
);

/**
 * Write data to WIO buffer.
 *
 * @param self WIO buffer instance
 * @param data Write data
 * @param size Size of data to read
 * @return Operation status
 */
extern wio_status_t wio_write(
    wio_buf_t* self,
    const void* data,
    uint16_t size
);

/**
 * Copy data from one WIO buffer to another
 *
 * @param from Source WIO buffer
 * @param to Target WIO buffer
 * @param size Size of data to copy
 */
extern wio_status_t wio_copy(
    wio_buf_t* from,
    wio_buf_t* to,
    uint16_t size
);

/**
 * Allocate memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @param ptr Pointer to allocated memory
 * @return Operation status
 */
extern wio_status_t wio_alloc(
    wio_buf_t* self,
    uint16_t size,
    void* _ptr
);

/**
 * Free memory from WIO buffer.
 *
 * @param self WIO buffer instance
 * @param size Size of the memory
 * @return Operation status
 */
extern wio_status_t wio_free(
    wio_buf_t* self,
    uint16_t size
);

/**
 * Reset WIO buffer.
 *
 * @param self WIO buffer instance
 * @return WIO_OK
 */
extern wio_status_t wio_reset(
    wio_buf_t* self
);

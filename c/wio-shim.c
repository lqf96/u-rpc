#include <string.h>
#include <stdlib.h>
#include "wio-shim.h"

/**
 * {@inheritDoc}
 */
wio_status_t wio_buf_init(
    wio_buf_t* self,
    uint8_t* buffer,
    uint16_t size
) {
    //Set up buffer and size
    self->buffer = buffer;
    self->size = size;

    //Initialize cursors
    self->pos_a = 0;
    self->pos_b = 0;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_buf_alloc_init(
    wio_buf_t* self,
    uint16_t size
) {
    //Allocate memory for buffer
    uint8_t* mem = malloc(size);
    if (!mem)
        return WIO_ERR_NO_MEMORY;
    //Initialize buffer
    WIO_TRY(wio_buf_init(self, mem, size))

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_read(
    wio_buf_t* self,
    void* data,
    uint16_t size
) {
    //Out of range check
    if (self->pos_a+size>self->size)
        return WIO_ERR_OUT_OF_RANGE;
    //Copy data
    memcpy(data, self->buffer+self->pos_a, size);
    //Update cursor
    self->pos_a += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_write(
    wio_buf_t* self,
    const void* data,
    uint16_t size
) {
    //Out of range check
    if (self->pos_b+size>self->size)
        return WIO_ERR_OUT_OF_RANGE;
    //Copy data
    memcpy(self->buffer+self->pos_b, data, size);
    //Update cursor
    self->pos_b += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_copy(
    wio_buf_t* from,
    wio_buf_t* to,
    uint16_t size
) {
    //Out of range check
    if (from->pos_a+size>from->size)
        return WIO_ERR_OUT_OF_RANGE;

    //Copy data
    WIO_TRY(wio_write(to, from->buffer+from->pos_a, size))
    //Update read cursor
    from->pos_a += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_alloc(
    wio_buf_t* self,
    uint16_t size,
    void* _ptr
) {
    void** ptr = (void**)_ptr;

    //Move to begin of the buffer
    if (self->pos_b+size>=self->size)
        self->pos_b = 0;
    //Try to allocate buffer
    //TODO: Out of memory check

    //Set pointer
    *ptr = self->buffer+self->pos_b;
    //Update cursor
    self->pos_b += size;

    return WIO_OK;
}

/**
 * {@inheritDoc}
 */
wio_status_t wio_free(
    wio_buf_t* self,
    uint16_t size
) {
    //Move to begin of the buffer
    if (self->size-self->pos_a<size)
        self->pos_a = 0;
    //Update cursor
    self->pos_a += size;

    return WIO_OK;
}

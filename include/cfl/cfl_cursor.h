/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  CFL
 *  ===
 *  Copyright (C) 2022-2026 The CFL Authors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * Bounds-checked byte cursor for decoding untrusted binary input.
 *
 * A cursor is a borrowed, read-only view over a buffer. It does not own or
 * copy the bytes, so the buffer must outlive the cursor and every pointer
 * handed out by it.
 *
 * Rules shared by every read function:
 *
 * - It returns CFL_TRUE when the requested bytes were available, and the
 *   cursor advances past them.
 * - It returns CFL_FALSE when they were not. The cursor does not move, the
 *   output argument is left untouched and the cursor enters a sticky error
 *   state where every later read also fails.
 *
 * The sticky error allows a decoder to chain several reads and check the
 * outcome once:
 *
 *     struct cfl_cursor cursor;
 *     uint16_t type;
 *     uint16_t length;
 *     const unsigned char *body;
 *
 *     cfl_cursor_init(&cursor, buffer, size);
 *
 *     if (!cfl_cursor_read_u16_be(&cursor, &type) ||
 *         !cfl_cursor_read_u16_be(&cursor, &length) ||
 *         !cfl_cursor_read_bytes(&cursor, length, &body)) {
 *         return -1;
 *     }
 *
 * A parser that has to wait for more data should query
 * cfl_cursor_remaining() before reading instead of triggering the error.
 *
 * See DECODING.md for the rationale and more examples.
 */

#ifndef CFL_CURSOR_H
#define CFL_CURSOR_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <cfl/cfl_checked.h>

struct cfl_cursor {
    const unsigned char *data;   /* start of the borrowed buffer */
    size_t size;                 /* total number of bytes in the buffer */
    size_t offset;               /* read position, never beyond size */
    int error;                   /* sticky: set by the first failed read */
};

/*
 * Start reading 'size' bytes at 'data'. A NULL 'data' is valid only for an
 * empty buffer; with a non-zero 'size' the cursor starts in error state.
 */
static inline void cfl_cursor_init(struct cfl_cursor *cursor,
                                   const void *data, size_t size)
{
    cursor->offset = 0;
    cursor->error = CFL_FALSE;

    if (data == NULL) {
        /* keep a valid pointer so no arithmetic is done on NULL */
        cursor->data = (const unsigned char *) "";
        cursor->size = 0;

        if (size > 0) {
            cursor->error = CFL_TRUE;
        }

        return;
    }

    cursor->data = (const unsigned char *) data;
    cursor->size = size;
}

/* Number of bytes that have not been read yet */
static inline size_t cfl_cursor_remaining(const struct cfl_cursor *cursor)
{
    return cursor->size - cursor->offset;
}

/* Number of bytes consumed since cfl_cursor_init() */
static inline size_t cfl_cursor_offset(const struct cfl_cursor *cursor)
{
    return cursor->offset;
}

/* CFL_TRUE when every byte was consumed and no read has failed */
static inline int cfl_cursor_is_done(const struct cfl_cursor *cursor)
{
    if (cursor->error || cursor->offset != cursor->size) {
        return CFL_FALSE;
    }

    return CFL_TRUE;
}

/* CFL_TRUE once any read has failed */
static inline int cfl_cursor_failed(const struct cfl_cursor *cursor)
{
    if (cursor->error) {
        return CFL_TRUE;
    }

    return CFL_FALSE;
}

/*
 * Borrow the next 'length' bytes without copying them: on success 'out'
 * points into the original buffer. Every other read is built on top of this
 * function, which holds the only bounds check.
 */
CFL_MUST_USE static inline int cfl_cursor_read_bytes(struct cfl_cursor *cursor,
                                                     size_t length,
                                                     const unsigned char **out)
{
    if (cursor->error || length > cursor->size - cursor->offset) {
        cursor->error = CFL_TRUE;
        return CFL_FALSE;
    }

    *out = cursor->data + cursor->offset;
    cursor->offset += length;

    return CFL_TRUE;
}

/* Copy the next 'length' bytes into 'out', which must hold that many */
CFL_MUST_USE static inline int cfl_cursor_copy_bytes(struct cfl_cursor *cursor,
                                                     void *out, size_t length)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, length, &bytes)) {
        return CFL_FALSE;
    }

    if (length > 0) {
        memcpy(out, bytes, length);
    }

    return CFL_TRUE;
}

/* Advance over the next 'length' bytes */
CFL_MUST_USE static inline int cfl_cursor_skip(struct cfl_cursor *cursor,
                                               size_t length)
{
    const unsigned char *bytes;

    return cfl_cursor_read_bytes(cursor, length, &bytes);
}

/*
 * Split off the next 'length' bytes as an independent cursor and advance
 * past them. Use it for nested, length-prefixed content: the decoder that
 * receives 'out' cannot read beyond the bytes that belong to it.
 */
CFL_MUST_USE static inline int cfl_cursor_sub(struct cfl_cursor *cursor,
                                              size_t length,
                                              struct cfl_cursor *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, length, &bytes)) {
        return CFL_FALSE;
    }

    out->data = bytes;
    out->size = length;
    out->offset = 0;
    out->error = CFL_FALSE;

    return CFL_TRUE;
}

/* Read the next byte without advancing */
CFL_MUST_USE static inline int cfl_cursor_peek_u8(struct cfl_cursor *cursor,
                                                  uint8_t *out)
{
    if (cursor->error || cursor->offset == cursor->size) {
        cursor->error = CFL_TRUE;
        return CFL_FALSE;
    }

    *out = cursor->data[cursor->offset];

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u8(struct cfl_cursor *cursor,
                                                  uint8_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 1, &bytes)) {
        return CFL_FALSE;
    }

    *out = bytes[0];

    return CFL_TRUE;
}

/*
 * Fixed-width unsigned integers. The '_be' functions decode big-endian
 * (network order) values and the '_le' functions little-endian ones. They
 * read byte by byte, so the input does not need to be aligned.
 */
CFL_MUST_USE static inline int cfl_cursor_read_u16_be(struct cfl_cursor *cursor,
                                                      uint16_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 2, &bytes)) {
        return CFL_FALSE;
    }

    *out = (uint16_t) (((uint16_t) bytes[0] << 8) | (uint16_t) bytes[1]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u16_le(struct cfl_cursor *cursor,
                                                      uint16_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 2, &bytes)) {
        return CFL_FALSE;
    }

    *out = (uint16_t) (((uint16_t) bytes[1] << 8) | (uint16_t) bytes[0]);

    return CFL_TRUE;
}

/* 24-bit lengths are common in framing formats; the result is zero extended */
CFL_MUST_USE static inline int cfl_cursor_read_u24_be(struct cfl_cursor *cursor,
                                                      uint32_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 3, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint32_t) bytes[0] << 16) |
           ((uint32_t) bytes[1] << 8)  |
           ((uint32_t) bytes[2]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u24_le(struct cfl_cursor *cursor,
                                                      uint32_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 3, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint32_t) bytes[2] << 16) |
           ((uint32_t) bytes[1] << 8)  |
           ((uint32_t) bytes[0]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u32_be(struct cfl_cursor *cursor,
                                                      uint32_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 4, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint32_t) bytes[0] << 24) |
           ((uint32_t) bytes[1] << 16) |
           ((uint32_t) bytes[2] << 8)  |
           ((uint32_t) bytes[3]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u32_le(struct cfl_cursor *cursor,
                                                      uint32_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 4, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint32_t) bytes[3] << 24) |
           ((uint32_t) bytes[2] << 16) |
           ((uint32_t) bytes[1] << 8)  |
           ((uint32_t) bytes[0]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u64_be(struct cfl_cursor *cursor,
                                                      uint64_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 8, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint64_t) bytes[0] << 56) |
           ((uint64_t) bytes[1] << 48) |
           ((uint64_t) bytes[2] << 40) |
           ((uint64_t) bytes[3] << 32) |
           ((uint64_t) bytes[4] << 24) |
           ((uint64_t) bytes[5] << 16) |
           ((uint64_t) bytes[6] << 8)  |
           ((uint64_t) bytes[7]);

    return CFL_TRUE;
}

CFL_MUST_USE static inline int cfl_cursor_read_u64_le(struct cfl_cursor *cursor,
                                                      uint64_t *out)
{
    const unsigned char *bytes;

    if (!cfl_cursor_read_bytes(cursor, 8, &bytes)) {
        return CFL_FALSE;
    }

    *out = ((uint64_t) bytes[7] << 56) |
           ((uint64_t) bytes[6] << 48) |
           ((uint64_t) bytes[5] << 40) |
           ((uint64_t) bytes[4] << 32) |
           ((uint64_t) bytes[3] << 24) |
           ((uint64_t) bytes[2] << 16) |
           ((uint64_t) bytes[1] << 8)  |
           ((uint64_t) bytes[0]);

    return CFL_TRUE;
}

#endif

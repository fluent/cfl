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

#include <stdio.h>
#include <stdlib.h>

#include <cfl/cfl_cursor.h>

#include "cfl_tests_internal.h"

/*
 * Copy a test vector into an exactly sized heap block, so sanitizers and
 * Valgrind report any read beyond the end of the input.
 */
static unsigned char *exact_copy(const void *data, size_t size)
{
    unsigned char *copy;

    copy = malloc(size > 0 ? size : 1);
    if (copy != NULL && size > 0) {
        memcpy(copy, data, size);
    }

    return copy;
}

static void init_and_state()
{
    struct cfl_cursor cursor;
    unsigned char data[4] = {1, 2, 3, 4};
    uint8_t byte = 0;

    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 4);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_FALSE);

    TEST_CHECK(cfl_cursor_skip(&cursor, 4) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 0);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 4);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    /* an empty buffer is valid, with or without a pointer */
    cfl_cursor_init(&cursor, data, 0);
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 0);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_skip(&cursor, 0) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);

    cfl_cursor_init(&cursor, NULL, 0);
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 0);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_skip(&cursor, 0) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u8(&cursor, &byte) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);

    /* a NULL buffer that claims to hold data starts in error state */
    cfl_cursor_init(&cursor, NULL, 16);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 0);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_skip(&cursor, 0) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u8(&cursor, &byte) == CFL_FALSE);
}

static void read_integers()
{
    struct cfl_cursor cursor;
    unsigned char *data;
    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    uint64_t u64 = 0;
    const unsigned char vector[] = {
        0x01,
        0x02, 0x03,
        0x04, 0x05, 0x06,
        0x07, 0x08, 0x09, 0x0a,
        0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12
    };

    data = exact_copy(vector, sizeof(vector));
    if (!TEST_CHECK(data != NULL)) {
        return;
    }

    /* big endian */
    cfl_cursor_init(&cursor, data, sizeof(vector));

    TEST_CHECK(cfl_cursor_read_u8(&cursor, &u8) == CFL_TRUE);
    TEST_CHECK(u8 == 0x01);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 1);

    TEST_CHECK(cfl_cursor_read_u16_be(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0x0203);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 3);

    TEST_CHECK(cfl_cursor_read_u24_be(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0x040506);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 6);

    TEST_CHECK(cfl_cursor_read_u32_be(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0x0708090a);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 10);

    TEST_CHECK(cfl_cursor_read_u64_be(&cursor, &u64) == CFL_TRUE);
    TEST_CHECK(u64 == UINT64_C(0x0b0c0d0e0f101112));
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    /* little endian */
    cfl_cursor_init(&cursor, data, sizeof(vector));

    TEST_CHECK(cfl_cursor_skip(&cursor, 1) == CFL_TRUE);

    TEST_CHECK(cfl_cursor_read_u16_le(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0x0302);

    TEST_CHECK(cfl_cursor_read_u24_le(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0x060504);

    TEST_CHECK(cfl_cursor_read_u32_le(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0x0a090807);

    TEST_CHECK(cfl_cursor_read_u64_le(&cursor, &u64) == CFL_TRUE);
    TEST_CHECK(u64 == UINT64_C(0x1211100f0e0d0c0b));
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    free(data);
}

static void read_integers_high_bits()
{
    struct cfl_cursor cursor;
    unsigned char data[8];
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    uint64_t u64 = 0;

    /* every byte has the sign bit set: no sign extension may leak in */
    memset(data, 0xff, sizeof(data));

    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_read_u16_be(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0xffff);

    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_read_u24_le(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0x00ffffff);

    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_read_u32_be(&cursor, &u32) == CFL_TRUE);
    TEST_CHECK(u32 == 0xffffffff);

    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_read_u64_le(&cursor, &u64) == CFL_TRUE);
    TEST_CHECK(u64 == UINT64_MAX);

    data[0] = 0x80;
    data[1] = 0x00;
    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_read_u16_be(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0x8000);
}

/*
 * For every integer width, an input one byte short must fail without
 * reading, without moving and without touching the output.
 */
static void read_integers_truncated()
{
    struct cfl_cursor cursor;
    unsigned char *data;
    size_t width;
    int ret;
    int i;
    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    uint64_t u64 = 0;
    const unsigned char vector[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    for (i = 0; i < 9; i++) {
        switch (i) {
        case 0:
            width = 1;
            break;
        case 1:
        case 2:
            width = 2;
            break;
        case 3:
        case 4:
            width = 3;
            break;
        case 5:
        case 6:
            width = 4;
            break;
        default:
            width = 8;
            break;
        }

        data = exact_copy(vector, width - 1);
        if (!TEST_CHECK(data != NULL)) {
            return;
        }

        cfl_cursor_init(&cursor, data, width - 1);

        u8 = 0xa5;
        u16 = 0xa5a5;
        u32 = 0xa5a5a5a5;
        u64 = UINT64_C(0xa5a5a5a5a5a5a5a5);

        switch (i) {
        case 0:
            ret = cfl_cursor_read_u8(&cursor, &u8);
            break;
        case 1:
            ret = cfl_cursor_read_u16_be(&cursor, &u16);
            break;
        case 2:
            ret = cfl_cursor_read_u16_le(&cursor, &u16);
            break;
        case 3:
            ret = cfl_cursor_read_u24_be(&cursor, &u32);
            break;
        case 4:
            ret = cfl_cursor_read_u24_le(&cursor, &u32);
            break;
        case 5:
            ret = cfl_cursor_read_u32_be(&cursor, &u32);
            break;
        case 6:
            ret = cfl_cursor_read_u32_le(&cursor, &u32);
            break;
        case 7:
            ret = cfl_cursor_read_u64_be(&cursor, &u64);
            break;
        default:
            ret = cfl_cursor_read_u64_le(&cursor, &u64);
            break;
        }

        TEST_CHECK(ret == CFL_FALSE);
        TEST_MSG("case %i, width %zu", i, width);
        TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);
        TEST_CHECK(cfl_cursor_offset(&cursor) == 0);
        TEST_CHECK(u8 == 0xa5);
        TEST_CHECK(u16 == 0xa5a5);
        TEST_CHECK(u32 == 0xa5a5a5a5);
        TEST_CHECK(u64 == UINT64_C(0xa5a5a5a5a5a5a5a5));

        free(data);
    }
}

static void read_bytes()
{
    struct cfl_cursor cursor;
    unsigned char *data;
    const unsigned char *bytes = NULL;
    const unsigned char *sentinel;

    data = exact_copy("abcdef", 6);
    if (!TEST_CHECK(data != NULL)) {
        return;
    }

    cfl_cursor_init(&cursor, data, 6);

    /* a zero length read succeeds and borrows nothing */
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 0, &bytes) == CFL_TRUE);
    TEST_CHECK(bytes == data);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);

    /* the returned pointer aliases the original buffer */
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 2, &bytes) == CFL_TRUE);
    TEST_CHECK(bytes == data);
    TEST_CHECK(memcmp(bytes, "ab", 2) == 0);

    /* exact fit of what remains */
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 4, &bytes) == CFL_TRUE);
    TEST_CHECK(bytes == data + 2);
    TEST_CHECK(memcmp(bytes, "cdef", 4) == 0);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    /* a zero length read at the very end is still valid */
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 0, &bytes) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);

    /* one byte too many */
    cfl_cursor_init(&cursor, data, 6);
    sentinel = (const unsigned char *) "sentinel";
    bytes = sentinel;
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 7, &bytes) == CFL_FALSE);
    TEST_CHECK(bytes == sentinel);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);

    /*
     * A hostile length close to SIZE_MAX must not wrap around when it is
     * compared against the read position.
     */
    cfl_cursor_init(&cursor, data, 6);
    TEST_CHECK(cfl_cursor_skip(&cursor, 4) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, SIZE_MAX, &bytes) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 4);

    cfl_cursor_init(&cursor, data, 6);
    TEST_CHECK(cfl_cursor_skip(&cursor, 4) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, SIZE_MAX - 3,
                                     &bytes) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_skip(&cursor, SIZE_MAX) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 4);

    free(data);
}

static void copy_bytes()
{
    struct cfl_cursor cursor;
    unsigned char *data;
    char out[8];

    data = exact_copy("abcdef", 6);
    if (!TEST_CHECK(data != NULL)) {
        return;
    }

    cfl_cursor_init(&cursor, data, 6);

    memset(out, 'x', sizeof(out));
    TEST_CHECK(cfl_cursor_copy_bytes(&cursor, out, 3) == CFL_TRUE);
    TEST_CHECK(memcmp(out, "abcxxxxx", 8) == 0);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 3);

    /* nothing is written when the input is short */
    memset(out, 'x', sizeof(out));
    TEST_CHECK(cfl_cursor_copy_bytes(&cursor, out, 4) == CFL_FALSE);
    TEST_CHECK(memcmp(out, "xxxxxxxx", 8) == 0);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 3);

    /* a zero length copy does not need a destination */
    cfl_cursor_init(&cursor, data, 6);
    TEST_CHECK(cfl_cursor_copy_bytes(&cursor, NULL, 0) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);

    free(data);
}

static void peek()
{
    struct cfl_cursor cursor;
    unsigned char data[2] = {0x7f, 0x80};
    uint8_t byte = 0;

    cfl_cursor_init(&cursor, data, sizeof(data));

    byte = 0;
    TEST_CHECK(cfl_cursor_peek_u8(&cursor, &byte) == CFL_TRUE);
    TEST_CHECK(byte == 0x7f);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);

    /* peeking twice returns the same byte */
    TEST_CHECK(cfl_cursor_peek_u8(&cursor, &byte) == CFL_TRUE);
    TEST_CHECK(byte == 0x7f);

    TEST_CHECK(cfl_cursor_skip(&cursor, 1) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_peek_u8(&cursor, &byte) == CFL_TRUE);
    TEST_CHECK(byte == 0x80);

    TEST_CHECK(cfl_cursor_skip(&cursor, 1) == CFL_TRUE);

    byte = 0xa5;
    TEST_CHECK(cfl_cursor_peek_u8(&cursor, &byte) == CFL_FALSE);
    TEST_CHECK(byte == 0xa5);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);
}

static void sticky_error()
{
    struct cfl_cursor cursor;
    struct cfl_cursor sub;
    unsigned char data[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    const unsigned char *bytes = NULL;
    uint8_t u8 = 0;
    uint16_t u16 = 0;
    uint32_t u32 = 0;
    char out[2];

    cfl_cursor_init(&sub, NULL, 0);
    cfl_cursor_init(&cursor, data, sizeof(data));

    TEST_CHECK(cfl_cursor_read_u16_be(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_skip(&cursor, 100) == CFL_FALSE);

    /*
     * Six bytes are still in the buffer, but after one failure every read
     * is refused, including those that would fit.
     */
    TEST_CHECK(cfl_cursor_remaining(&cursor) == 6);
    TEST_CHECK(cfl_cursor_read_u8(&cursor, &u8) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_peek_u8(&cursor, &u8) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u16_le(&cursor, &u16) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u32_be(&cursor, &u32) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 1, &bytes) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_bytes(&cursor, 0, &bytes) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_copy_bytes(&cursor, out, 2) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_skip(&cursor, 0) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_sub(&cursor, 1, &sub) == CFL_FALSE);

    TEST_CHECK(cfl_cursor_offset(&cursor) == 2);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_FALSE);

    /* a chain of reads can therefore be checked once, at the end */
    cfl_cursor_init(&cursor, data, 3);
    if (cfl_cursor_read_u16_be(&cursor, &u16) &&
        cfl_cursor_read_u32_be(&cursor, &u32) &&
        cfl_cursor_read_u8(&cursor, &u8)) {
        TEST_CHECK(0);
        TEST_MSG("a truncated chain must not succeed");
    }
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 2);

    /* cfl_cursor_init() is the way to start over */
    cfl_cursor_init(&cursor, data, sizeof(data));
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u8(&cursor, &u8) == CFL_TRUE);
    TEST_CHECK(u8 == 1);
}

static void sub_cursor()
{
    struct cfl_cursor cursor;
    struct cfl_cursor sub;
    struct cfl_cursor untouched;
    unsigned char *data;
    uint8_t u8 = 0;
    uint16_t u16 = 0;
    const unsigned char vector[] = {0xaa, 0x01, 0x02, 0x03, 0xbb, 0xcc};

    data = exact_copy(vector, sizeof(vector));
    if (!TEST_CHECK(data != NULL)) {
        return;
    }

    cfl_cursor_init(&sub, NULL, 0);
    cfl_cursor_init(&cursor, data, sizeof(vector));
    TEST_CHECK(cfl_cursor_read_u8(&cursor, &u8) == CFL_TRUE);

    /* the parent advances past the nested content right away */
    TEST_CHECK(cfl_cursor_sub(&cursor, 3, &sub) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 4);
    TEST_CHECK(cfl_cursor_remaining(&sub) == 3);
    TEST_CHECK(cfl_cursor_offset(&sub) == 0);

    /* the nested decoder cannot see the bytes that follow its content */
    TEST_CHECK(cfl_cursor_read_u16_be(&sub, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0x0102);
    TEST_CHECK(cfl_cursor_read_u16_be(&sub, &u16) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_failed(&sub) == CFL_TRUE);

    /* and its failure does not affect the parent */
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_FALSE);
    TEST_CHECK(cfl_cursor_read_u16_be(&cursor, &u16) == CFL_TRUE);
    TEST_CHECK(u16 == 0xbbcc);
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    /* an empty nested range is valid */
    cfl_cursor_init(&cursor, data, sizeof(vector));
    TEST_CHECK(cfl_cursor_sub(&cursor, 0, &sub) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_is_done(&sub) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_offset(&cursor) == 0);

    /* so is one that takes everything */
    TEST_CHECK(cfl_cursor_sub(&cursor, sizeof(vector), &sub) == CFL_TRUE);
    TEST_CHECK(cfl_cursor_remaining(&sub) == sizeof(vector));
    TEST_CHECK(cfl_cursor_is_done(&cursor) == CFL_TRUE);

    /* a declared length larger than the parent leaves the output alone */
    cfl_cursor_init(&cursor, data, sizeof(vector));
    cfl_cursor_init(&untouched, data, 1);
    sub = untouched;
    TEST_CHECK(cfl_cursor_sub(&cursor, sizeof(vector) + 1, &sub) == CFL_FALSE);
    TEST_CHECK(sub.data == untouched.data);
    TEST_CHECK(sub.size == untouched.size);
    TEST_CHECK(sub.offset == untouched.offset);
    TEST_CHECK(sub.error == untouched.error);
    TEST_CHECK(cfl_cursor_failed(&cursor) == CFL_TRUE);

    free(data);
}

/*
 * A type-length-value stream as used by several network protocols: a two
 * byte type, a two byte length that covers the four byte header, and the
 * value. It exercises the error paths a hand written parser has to get
 * right one by one.
 */
#define PART_STRING  0x0001
#define PART_NUMBER  0x0002

static int decode_parts(const unsigned char *data, size_t size,
                        size_t *strings, uint64_t *sum)
{
    struct cfl_cursor cursor;
    struct cfl_cursor part;
    const unsigned char *text = NULL;
    uint16_t type = 0;
    uint16_t length = 0;
    uint64_t number = 0;
    size_t body;

    *strings = 0;
    *sum = 0;

    cfl_cursor_init(&cursor, data, size);

    while (cfl_cursor_remaining(&cursor) > 0) {
        if (!cfl_cursor_read_u16_be(&cursor, &type) ||
            !cfl_cursor_read_u16_be(&cursor, &length) ||
            !cfl_checked_size_sub(length, 4, &body) ||
            !cfl_cursor_sub(&cursor, body, &part)) {
            return -1;
        }

        if (type == PART_STRING) {
            body = cfl_cursor_remaining(&part);

            /* strings must be NUL terminated inside their own part */
            if (body == 0 ||
                !cfl_cursor_read_bytes(&part, body, &text) ||
                text[body - 1] != '\0') {
                return -1;
            }
            (*strings)++;
        }
        else if (type == PART_NUMBER) {
            if (!cfl_cursor_read_u64_be(&part, &number) ||
                !cfl_cursor_is_done(&part)) {
                return -1;
            }
            *sum += number;
        }
    }

    return 0;
}

static int decode_parts_exact(const void *data, size_t size,
                              size_t *strings, uint64_t *sum)
{
    unsigned char *copy;
    int ret;

    copy = exact_copy(data, size);
    if (!TEST_CHECK(copy != NULL)) {
        return -2;
    }

    ret = decode_parts(copy, size, strings, sum);
    free(copy);

    return ret;
}

static void decode_tlv()
{
    size_t strings;
    uint64_t sum;
    size_t size;
    const unsigned char valid[] = {
        0x00, 0x01, 0x00, 0x07, 'f', 'o', '\0',
        0x00, 0x02, 0x00, 0x0c, 0, 0, 0, 0, 0, 0, 0x01, 0x00,
        0x00, 0x09, 0x00, 0x05, 0xff,
        0x00, 0x02, 0x00, 0x0c, 0, 0, 0, 0, 0, 0, 0x00, 0x05
    };
    /* declared length is smaller than the header that carries it */
    const unsigned char short_length[] = {0x00, 0x01, 0x00, 0x03, 'a', '\0'};
    /* declared length points past the end of the input */
    const unsigned char long_length[] = {0x00, 0x01, 0xff, 0xff, 'a', '\0'};
    /* a number part that is too short to hold a 64-bit value */
    const unsigned char short_number[] = {0x00, 0x02, 0x00, 0x08, 0, 0, 0, 1};
    /* a string part without its terminator, followed by a zero byte */
    const unsigned char no_terminator[] = {
        0x00, 0x01, 0x00, 0x06, 'a', 'b',
        0x00, 0x09, 0x00, 0x04
    };
    /* a string part with an empty body */
    const unsigned char empty_string[] = {0x00, 0x01, 0x00, 0x04};

    TEST_CHECK(decode_parts_exact(valid, sizeof(valid), &strings, &sum) == 0);
    TEST_CHECK(strings == 1);
    TEST_CHECK(sum == 0x105);

    TEST_CHECK(decode_parts_exact(valid, 0, &strings, &sum) == 0);
    TEST_CHECK(strings == 0);
    TEST_CHECK(sum == 0);

    /* every truncation of the valid input is either complete or rejected */
    for (size = 1; size < sizeof(valid); size++) {
        if (size == 7 || size == 19 || size == 24) {
            TEST_CHECK(decode_parts_exact(valid, size, &strings, &sum) == 0);
        }
        else {
            TEST_CHECK(decode_parts_exact(valid, size, &strings, &sum) == -1);
        }
        TEST_MSG("truncated at %zu bytes", size);
    }

    TEST_CHECK(decode_parts_exact(short_length, sizeof(short_length),
                                  &strings, &sum) == -1);
    TEST_CHECK(decode_parts_exact(long_length, sizeof(long_length),
                                  &strings, &sum) == -1);
    TEST_CHECK(decode_parts_exact(short_number, sizeof(short_number),
                                  &strings, &sum) == -1);
    TEST_CHECK(decode_parts_exact(no_terminator, sizeof(no_terminator),
                                  &strings, &sum) == -1);
    TEST_CHECK(decode_parts_exact(empty_string, sizeof(empty_string),
                                  &strings, &sum) == -1);
}

TEST_LIST = {
    {"init_and_state",          init_and_state},
    {"read_integers",           read_integers},
    {"read_integers_high_bits", read_integers_high_bits},
    {"read_integers_truncated", read_integers_truncated},
    {"read_bytes",              read_bytes},
    {"copy_bytes",              copy_bytes},
    {"peek",                    peek},
    {"sticky_error",            sticky_error},
    {"sub_cursor",              sub_cursor},
    {"decode_tlv",              decode_tlv},
    { 0 }
};

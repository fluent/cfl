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

#include <cfl/cfl_checked.h>

#include "cfl_tests_internal.h"

#define SENTINEL_SIZE ((size_t) 0x5a5a)
#define SENTINEL_U64  ((uint64_t) 0x5a5a5a5a)

static void size_add()
{
    size_t out;

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_add(0, 0, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_size_add(40, 2, &out) == CFL_TRUE);
    TEST_CHECK(out == 42);

    /* exact fit at the upper boundary */
    TEST_CHECK(cfl_checked_size_add(SIZE_MAX - 1, 1, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    TEST_CHECK(cfl_checked_size_add(SIZE_MAX, 0, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    /* one past the boundary: the output must not be modified */
    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_add(SIZE_MAX, 1, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(cfl_checked_size_add(1, SIZE_MAX, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(cfl_checked_size_add(SIZE_MAX, SIZE_MAX, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);
}

static void size_sub()
{
    size_t out;

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_sub(10, 4, &out) == CFL_TRUE);
    TEST_CHECK(out == 6);

    TEST_CHECK(cfl_checked_size_sub(4, 4, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_size_sub(SIZE_MAX, SIZE_MAX, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    /* a declared length smaller than its own header must not wrap around */
    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_sub(3, 4, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(cfl_checked_size_sub(0, 1, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);
}

static void size_mul()
{
    size_t out;
    size_t half;

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_mul(0, 0, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_size_mul(SIZE_MAX, 0, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_size_mul(0, SIZE_MAX, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_size_mul(6, 7, &out) == CFL_TRUE);
    TEST_CHECK(out == 42);

    TEST_CHECK(cfl_checked_size_mul(SIZE_MAX, 1, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    /* largest product of two that still fits */
    half = SIZE_MAX / 2;
    TEST_CHECK(cfl_checked_size_mul(half, 2, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX - 1);

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_mul(half + 1, 2, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(cfl_checked_size_mul(SIZE_MAX, 2, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(cfl_checked_size_mul(SIZE_MAX, SIZE_MAX, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);
}

static void size_mul_add()
{
    size_t out;

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_mul_add(4, 10, 2, &out) == CFL_TRUE);
    TEST_CHECK(out == 42);

    TEST_CHECK(cfl_checked_size_mul_add(SIZE_MAX / 10, 10,
                                        SIZE_MAX % 10, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    /* the addition overflows */
    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_size_mul_add(SIZE_MAX / 10, 10,
                                        (SIZE_MAX % 10) + 1,
                                        &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    /* the multiplication overflows */
    TEST_CHECK(cfl_checked_size_mul_add((SIZE_MAX / 10) + 1, 10, 0,
                                        &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);
}

/* Accumulate decimal digits the way a length-prefix parser does */
static int parse_decimal(const char *text, size_t *out)
{
    size_t value;

    value = 0;

    while (*text >= '0' && *text <= '9') {
        if (!cfl_checked_size_mul_add(value, 10, (size_t) (*text - '0'),
                                      &value)) {
            return CFL_FALSE;
        }
        text++;
    }

    *out = value;

    return CFL_TRUE;
}

static void size_digit_accumulation()
{
    size_t out;
    char text[64];

    out = SENTINEL_SIZE;
    TEST_CHECK(parse_decimal("0", &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(parse_decimal("1234567", &out) == CFL_TRUE);
    TEST_CHECK(out == 1234567);

    /* SIZE_MAX itself is representable */
    snprintf(text, sizeof(text), "%llu", (unsigned long long) SIZE_MAX);
    TEST_CHECK(parse_decimal(text, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    /* one more digit is not */
    out = SENTINEL_SIZE;
    snprintf(text, sizeof(text), "%llu0", (unsigned long long) SIZE_MAX);
    TEST_CHECK(parse_decimal(text, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);

    TEST_CHECK(parse_decimal("99999999999999999999999999999999999999",
                             &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_SIZE);
}

static void u64_add()
{
    uint64_t out;

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_add(40, 2, &out) == CFL_TRUE);
    TEST_CHECK(out == 42);

    TEST_CHECK(cfl_checked_u64_add(UINT64_MAX - 1, 1, &out) == CFL_TRUE);
    TEST_CHECK(out == UINT64_MAX);

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_add(UINT64_MAX, 1, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);

    TEST_CHECK(cfl_checked_u64_add(1, UINT64_MAX, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);
}

static void u64_sub()
{
    uint64_t out;

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_sub(UINT64_MAX, 1, &out) == CFL_TRUE);
    TEST_CHECK(out == UINT64_MAX - 1);

    TEST_CHECK(cfl_checked_u64_sub(7, 7, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_sub(7, 8, &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);
}

static void u64_mul()
{
    uint64_t out;

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_mul(UINT64_MAX, 0, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    /* seconds to nanoseconds, the usual timestamp conversion */
    TEST_CHECK(cfl_checked_u64_mul(1700000000, 1000000000, &out) == CFL_TRUE);
    TEST_CHECK(out == UINT64_C(1700000000000000000));

    TEST_CHECK(cfl_checked_u64_mul(UINT64_MAX / 2, 2, &out) == CFL_TRUE);
    TEST_CHECK(out == UINT64_MAX - 1);

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_mul((UINT64_MAX / 2) + 1, 2,
                                   &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);

    TEST_CHECK(cfl_checked_u64_mul(UINT64_C(0x100000000),
                                   UINT64_C(0x100000000), &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);
}

static void u64_mul_add()
{
    uint64_t out;

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_mul_add(0xff, 0x10, 0xf, &out) == CFL_TRUE);
    TEST_CHECK(out == 0xfff);

    TEST_CHECK(cfl_checked_u64_mul_add(UINT64_MAX / 16, 16, 15,
                                       &out) == CFL_TRUE);
    TEST_CHECK(out == UINT64_MAX);

    out = SENTINEL_U64;
    TEST_CHECK(cfl_checked_u64_mul_add(UINT64_MAX / 16, 16, 16,
                                       &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);

    TEST_CHECK(cfl_checked_u64_mul_add((UINT64_MAX / 16) + 1, 16, 0,
                                       &out) == CFL_FALSE);
    TEST_CHECK(out == SENTINEL_U64);
}

static void u64_to_size()
{
    size_t out;

    out = SENTINEL_SIZE;
    TEST_CHECK(cfl_checked_u64_to_size(0, &out) == CFL_TRUE);
    TEST_CHECK(out == 0);

    TEST_CHECK(cfl_checked_u64_to_size((uint64_t) SIZE_MAX, &out) == CFL_TRUE);
    TEST_CHECK(out == SIZE_MAX);

    if (sizeof(size_t) < sizeof(uint64_t)) {
        out = SENTINEL_SIZE;
        TEST_CHECK(cfl_checked_u64_to_size(UINT64_MAX, &out) == CFL_FALSE);
        TEST_CHECK(out == SENTINEL_SIZE);
    }
    else {
        TEST_CHECK(cfl_checked_u64_to_size(UINT64_MAX, &out) == CFL_TRUE);
        TEST_CHECK(out == SIZE_MAX);
    }
}

static void str_equals()
{
    /* 'name' followed by bytes that are not part of the string */
    char buffer[8] = {'n', 'a', 'm', 'e', 's', 'p', 'a', 'c'};
    char *exact;

    TEST_CHECK(cfl_checked_str_equals(buffer, 4, "name") == CFL_TRUE);

    /* a shared prefix is not a match, in either direction */
    TEST_CHECK(cfl_checked_str_equals(buffer, 8, "name") == CFL_FALSE);
    TEST_CHECK(cfl_checked_str_equals(buffer, 4, "namespace") == CFL_FALSE);
    TEST_CHECK(cfl_checked_str_equals(buffer, 3, "name") == CFL_FALSE);

    TEST_CHECK(cfl_checked_str_equals(buffer, 4, "nome") == CFL_FALSE);

    /* empty strings */
    TEST_CHECK(cfl_checked_str_equals(buffer, 0, "") == CFL_TRUE);
    TEST_CHECK(cfl_checked_str_equals(NULL, 0, "") == CFL_TRUE);
    TEST_CHECK(cfl_checked_str_equals(buffer, 4, "") == CFL_FALSE);
    TEST_CHECK(cfl_checked_str_equals(buffer, 0, "name") == CFL_FALSE);

    /* invalid arguments never match */
    TEST_CHECK(cfl_checked_str_equals(NULL, 4, "name") == CFL_FALSE);
    TEST_CHECK(cfl_checked_str_equals(buffer, 4, NULL) == CFL_FALSE);

    /*
     * An exactly sized heap block without a NUL terminator: sanitizers and
     * Valgrind report any read beyond its four bytes.
     */
    exact = malloc(4);
    if (!TEST_CHECK(exact != NULL)) {
        return;
    }
    memcpy(exact, "name", 4);

    TEST_CHECK(cfl_checked_str_equals(exact, 4, "name") == CFL_TRUE);
    TEST_CHECK(cfl_checked_str_equals(exact, 4, "namespace") == CFL_FALSE);
    TEST_CHECK(cfl_checked_str_equals(exact, 4, "nam") == CFL_FALSE);

    free(exact);
}

TEST_LIST = {
    {"size_add",                size_add},
    {"size_sub",                size_sub},
    {"size_mul",                size_mul},
    {"size_mul_add",            size_mul_add},
    {"size_digit_accumulation", size_digit_accumulation},
    {"u64_add",                 u64_add},
    {"u64_sub",                 u64_sub},
    {"u64_mul",                 u64_mul},
    {"u64_mul_add",             u64_mul_add},
    {"u64_to_size",             u64_to_size},
    {"str_equals",              str_equals},
    { 0 }
};

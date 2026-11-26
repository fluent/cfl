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
 * Overflow-checked arithmetic for sizes, lengths and counters that come from
 * untrusted input, plus a length-aware string comparison.
 *
 * Every arithmetic helper returns CFL_TRUE and stores the result in 'out' on
 * success. On overflow, underflow or truncation it returns CFL_FALSE and
 * leaves 'out' untouched. The result must be checked: the helpers are marked
 * with CFL_MUST_USE so compilers that support it warn when it is ignored.
 *
 *     size_t bytes;
 *
 *     if (!cfl_checked_size_mul(count, sizeof(struct entry), &bytes)) {
 *         return -1;
 *     }
 *     entries = malloc(bytes);
 *
 * See DECODING.md for the rationale and more examples.
 */

#ifndef CFL_CHECKED_H
#define CFL_CHECKED_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef CFL_FALSE
#define CFL_FALSE   0
#endif

#ifndef CFL_TRUE
#define CFL_TRUE    !CFL_FALSE
#endif

/*
 * Mark a function whose return value must not be ignored. It expands to
 * nothing on compilers without an equivalent attribute.
 */
#ifndef CFL_MUST_USE
#if defined(__GNUC__) || defined(__clang__)
#define CFL_MUST_USE __attribute__((warn_unused_result))
#else
#define CFL_MUST_USE
#endif
#endif

/*
 * Use the compiler overflow builtins when they exist (GCC >= 5, Clang). All
 * other compilers, and builds that define CFL_CHECKED_NO_BUILTINS, use the
 * portable C99 comparisons. Both paths have identical semantics.
 */
#if !defined(CFL_CHECKED_NO_BUILTINS)
#if defined(__has_builtin)
#if __has_builtin(__builtin_add_overflow) && \
    __has_builtin(__builtin_sub_overflow) && \
    __has_builtin(__builtin_mul_overflow)
#define CFL_CHECKED_HAVE_BUILTINS
#endif
#elif defined(__GNUC__) && (__GNUC__ >= 5)
#define CFL_CHECKED_HAVE_BUILTINS
#endif
#endif

/* out = a + b */
CFL_MUST_USE static inline int cfl_checked_size_add(size_t a, size_t b,
                                                    size_t *out)
{
#ifdef CFL_CHECKED_HAVE_BUILTINS
    size_t result;

    if (__builtin_add_overflow(a, b, &result)) {
        return CFL_FALSE;
    }

    *out = result;
#else
    if (a > SIZE_MAX - b) {
        return CFL_FALSE;
    }

    *out = a + b;
#endif

    return CFL_TRUE;
}

/* out = a - b, fails when b is greater than a */
CFL_MUST_USE static inline int cfl_checked_size_sub(size_t a, size_t b,
                                                    size_t *out)
{
    if (b > a) {
        return CFL_FALSE;
    }

    *out = a - b;

    return CFL_TRUE;
}

/* out = a * b */
CFL_MUST_USE static inline int cfl_checked_size_mul(size_t a, size_t b,
                                                    size_t *out)
{
#ifdef CFL_CHECKED_HAVE_BUILTINS
    size_t result;

    if (__builtin_mul_overflow(a, b, &result)) {
        return CFL_FALSE;
    }

    *out = result;
#else
    if (b != 0 && a > SIZE_MAX / b) {
        return CFL_FALSE;
    }

    *out = a * b;
#endif

    return CFL_TRUE;
}

/*
 * out = (a * b) + c
 *
 * Covers the two usual shapes of size arithmetic: a header followed by an
 * array of elements, and the accumulation of a number digit by digit
 * (value = value * base + digit).
 */
CFL_MUST_USE static inline int cfl_checked_size_mul_add(size_t a, size_t b,
                                                        size_t c, size_t *out)
{
    size_t result;

    if (!cfl_checked_size_mul(a, b, &result)) {
        return CFL_FALSE;
    }

    if (!cfl_checked_size_add(result, c, &result)) {
        return CFL_FALSE;
    }

    *out = result;

    return CFL_TRUE;
}

/* out = a + b */
CFL_MUST_USE static inline int cfl_checked_u64_add(uint64_t a, uint64_t b,
                                                   uint64_t *out)
{
#ifdef CFL_CHECKED_HAVE_BUILTINS
    uint64_t result;

    if (__builtin_add_overflow(a, b, &result)) {
        return CFL_FALSE;
    }

    *out = result;
#else
    if (a > UINT64_MAX - b) {
        return CFL_FALSE;
    }

    *out = a + b;
#endif

    return CFL_TRUE;
}

/* out = a - b, fails when b is greater than a */
CFL_MUST_USE static inline int cfl_checked_u64_sub(uint64_t a, uint64_t b,
                                                   uint64_t *out)
{
    if (b > a) {
        return CFL_FALSE;
    }

    *out = a - b;

    return CFL_TRUE;
}

/* out = a * b */
CFL_MUST_USE static inline int cfl_checked_u64_mul(uint64_t a, uint64_t b,
                                                   uint64_t *out)
{
#ifdef CFL_CHECKED_HAVE_BUILTINS
    uint64_t result;

    if (__builtin_mul_overflow(a, b, &result)) {
        return CFL_FALSE;
    }

    *out = result;
#else
    if (b != 0 && a > UINT64_MAX / b) {
        return CFL_FALSE;
    }

    *out = a * b;
#endif

    return CFL_TRUE;
}

/* out = (a * b) + c */
CFL_MUST_USE static inline int cfl_checked_u64_mul_add(uint64_t a, uint64_t b,
                                                       uint64_t c,
                                                       uint64_t *out)
{
    uint64_t result;

    if (!cfl_checked_u64_mul(a, b, &result)) {
        return CFL_FALSE;
    }

    if (!cfl_checked_u64_add(result, c, &result)) {
        return CFL_FALSE;
    }

    *out = result;

    return CFL_TRUE;
}

/*
 * Convert a 64-bit length, as found in wire formats, to size_t. It fails
 * when the value does not fit, which can only happen where size_t is
 * narrower than 64 bits.
 */
CFL_MUST_USE static inline int cfl_checked_u64_to_size(uint64_t value,
                                                       size_t *out)
{
    if (value > (uint64_t) SIZE_MAX) {
        return CFL_FALSE;
    }

    *out = (size_t) value;

    return CFL_TRUE;
}

/*
 * Compare a length-delimited string, which is not required to be NUL
 * terminated, against a C string. It returns CFL_TRUE only on an exact
 * match: the lengths are compared first, so it never reads more than
 * 'length' bytes from 'data' and a common prefix is not a match.
 *
 * A NULL 'data' is only valid together with a zero 'length'.
 */
static inline int cfl_checked_str_equals(const char *data, size_t length,
                                         const char *str)
{
    size_t str_length;

    if (str == NULL) {
        return CFL_FALSE;
    }

    str_length = strlen(str);

    if (str_length != length) {
        return CFL_FALSE;
    }

    if (length == 0) {
        return CFL_TRUE;
    }

    if (data == NULL) {
        return CFL_FALSE;
    }

    if (memcmp(data, str, length) != 0) {
        return CFL_FALSE;
    }

    return CFL_TRUE;
}

#endif

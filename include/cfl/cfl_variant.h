/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  CFL
 *  ===
 *  Copyright (C) 2022 The CFL Authors
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

#ifndef CFL_VARIANT_H
#define CFL_VARIANT_H

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#define CFL_VARIANT_BOOL       1
#define CFL_VARIANT_INT        2
#define CFL_VARIANT_UINT       3
#define CFL_VARIANT_DOUBLE     4
#define CFL_VARIANT_NULL       5
#define CFL_VARIANT_REFERENCE  6
#define CFL_VARIANT_STRING     7
#define CFL_VARIANT_BYTES      8
#define CFL_VARIANT_ARRAY      9
#define CFL_VARIANT_KVLIST    10

struct cfl_array;
struct cfl_kvlist;

struct cfl_variant {
    int type;
    size_t size;

    /*
     * indicate if 'data' is being referenced for 'as_string' and 'as_bytes':
     *
     * CFL_TRUE:  data is being referenced
     * CFL_FALSE: data is owned by the variant
     *
     * Referenced data uses less memory, when the variant is not referenced we use
     * a copy of the original data.
     */
    uint8_t referenced;

    /* the data */
    union {
        cfl_sds_t as_string;
        cfl_sds_t as_bytes;
        unsigned int as_bool;
        int64_t as_int64;
        uint64_t as_uint64;
        double as_double;
        void *as_reference;
        struct cfl_array *as_array;
        struct cfl_kvlist *as_kvlist;
    } data;
};

int cfl_variant_print(FILE *fp, struct cfl_variant *val);
struct cfl_variant *cfl_variant_create_from_string(char *value);
struct cfl_variant *cfl_variant_create_from_string_s(char *value, size_t value_length, int referenced);
struct cfl_variant *cfl_variant_create_from_bytes(char *value, size_t length, int referenced);
struct cfl_variant *cfl_variant_create_from_bool(int value);
struct cfl_variant *cfl_variant_create_from_int64(int64_t value);
struct cfl_variant *cfl_variant_create_from_uint64(uint64_t value);
struct cfl_variant *cfl_variant_create_from_double(double value);
struct cfl_variant *cfl_variant_create_from_array(struct cfl_array *value);
struct cfl_variant *cfl_variant_create_from_null();
struct cfl_variant *cfl_variant_create_from_kvlist(struct cfl_kvlist *value);
struct cfl_variant *cfl_variant_create_from_reference(void *value);
struct cfl_variant *cfl_variant_create();

void cfl_variant_destroy(struct cfl_variant *instance);

#endif

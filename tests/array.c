/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  CFL
 *  ===
 *  Copyright (C) 2023 The CFL Authors
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

#include <cfl/cfl.h>
#include "cfl_tests_internal.h"

static void test_create_destroy()
{
    struct cfl_array *array = NULL;

    array = cfl_array_create(10);
    if (!TEST_CHECK(array != NULL)) {
        TEST_MSG("cfl_array_create failed");
        return;
    }

    cfl_array_destroy(array);
}

static void test_append_fetch()
{
    ssize_t index;
    int ret;
    size_t array_size = 10;
    struct cfl_array *array = NULL;
    struct cfl_variant *val = NULL;

    array = cfl_array_create(array_size);
    if (!TEST_CHECK(array != NULL)) {
        TEST_MSG("cfl_array_create failed");
        return;
    }

    for (index=0; index<(ssize_t)array_size; index++) {
        ret = cfl_array_append_int64(array, index);
        if (!TEST_CHECK(ret == 0)) {
            TEST_MSG("%zd: cfl_array_append_int64 failed.",index);
            cfl_array_destroy(array);
            return;
        }
    }

    for (index=0; index<(ssize_t)cfl_array_size(array); index++) {
        val = cfl_array_fetch_by_index(array, index);
        if (!TEST_CHECK(val != NULL)) {
            TEST_MSG("%zd: val is NULL",index);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(val->type == CFL_VARIANT_INT)) {
            TEST_MSG("%zd: type is not int. type=%d",index, val->type);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(index == (ssize_t)val->data.as_int64)) {
            TEST_MSG("%zd: mismatch. got=%" PRId64 " expect=%zd",index, val->data.as_int64, index);
            cfl_array_destroy(array);
            return;
        }
    }

    cfl_array_destroy(array);
}

static void test_remove_by_index()
{
    ssize_t index;
    int ret;
    int64_t expect[] = {0,1,2,3,4,6,7,8,9}; /* remove 5 */
    size_t array_size = 10;
    struct cfl_array *array = NULL;
    struct cfl_variant *val = NULL;

    array = cfl_array_create(array_size);
    if (!TEST_CHECK(array != NULL)) {
        TEST_MSG("cfl_array_create failed");
        return;
    }

    for (index=0; index<(ssize_t)array_size; index++) {
        ret = cfl_array_append_int64(array, index);
        if (!TEST_CHECK(ret == 0)) {
            TEST_MSG("%zd: cfl_array_append_int64 failed.",index);
            cfl_array_destroy(array);
            return;
        }
    }

    ret = cfl_array_remove_by_index(array, 5);
    if (!TEST_CHECK(ret == 0)) {
        TEST_MSG("cfl_array_remove_by_index(5) failed.");
        cfl_array_destroy(array);
        return;
    }

    for (index=0; index<(ssize_t)cfl_array_size(array); index++) {
        val = cfl_array_fetch_by_index(array, index);
        if (!TEST_CHECK(val != NULL)) {
            TEST_MSG("%zd: val is NULL",index);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(val->type == CFL_VARIANT_INT)) {
            TEST_MSG("%zd: type is not int. type=%d",index, val->type);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(expect[index] == val->data.as_int64)) {
            TEST_MSG("%zd: mismatch. got=%" PRId64 " expect=" PRId64,index, val->data.as_int64, expect[index]);
            cfl_array_destroy(array);
            return;
        }
    }

    cfl_array_destroy(array);
}

static void test_remove_by_reference()
{
    ssize_t index;
    int ret;
    int expect[] = {0,1,2,3,4,6,7,8,9}; /* remove 5 */
    size_t array_size = 10;
    struct cfl_array *array = NULL;
    struct cfl_variant *val = NULL;

    array = cfl_array_create(array_size);
    if (!TEST_CHECK(array != NULL)) {
        TEST_MSG("cfl_array_create failed");
        return;
    }

    for (index=0; index<(ssize_t)array_size; index++) {
        ret = cfl_array_append_int64(array, index);
        if (!TEST_CHECK(ret == 0)) {
            TEST_MSG("%zd: cfl_array_append_int64 failed.",index);
            cfl_array_destroy(array);
            return;
        }
    }

    val = cfl_array_fetch_by_index(array, 5);
    if (!TEST_CHECK(val != NULL)) {
        TEST_MSG("cfl_array_fetch_by_index(5) failed.");
        cfl_array_destroy(array);
        return;
    }
    ret = cfl_array_remove_by_reference(array, val);
    if (!TEST_CHECK(ret == 0)) {
        TEST_MSG("cfl_array_remove_by_reference failed.");
        cfl_array_destroy(array);
        return;
    }

    for (index=0; index<(ssize_t)cfl_array_size(array); index++) {
        val = cfl_array_fetch_by_index(array, index);
        if (!TEST_CHECK(val != NULL)) {
            TEST_MSG("%zd: val is NULL",index);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(val->type == CFL_VARIANT_INT)) {
            TEST_MSG("%zd: type is not int. type=%d",index, val->type);
            cfl_array_destroy(array);
            return;
        }
        else if (!TEST_CHECK(expect[index] == val->data.as_int64)) {
            TEST_MSG("%zd: mismatch. got=%" PRId64 " expect=%" PRId64,index, val->data.as_int64, expect[index]);
            cfl_array_destroy(array);
            return;
        }
    }

    cfl_array_destroy(array);
}

TEST_LIST = {
    { "create_destroy" , test_create_destroy},
    { "append_fetch" , test_append_fetch},
    { "remove_by_index" , test_remove_by_index},
    { "remove_by_reference" , test_remove_by_reference},
    { 0 }
};

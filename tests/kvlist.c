/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  CFL
 *  ===
 *  Copyright (C) 2022-2024 The CFL Authors
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

static void test_basics()
{
    int ret;
    struct cfl_kvlist *list;
    struct cfl_kvlist *list2;
    struct cfl_array *array;
    struct cfl_variant *variant;

    list = cfl_kvlist_create();
    TEST_CHECK(list != NULL);

    /*
     * Inserts
     */
    ret = cfl_kvlist_insert_string(list, "key1", "value1");
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_bytes(list, "key2", "value2", 6);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_reference(list, "key3", (void *) 0xdeadbeef);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_bool(list, "key4", 1);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_int64(list, "key5", 1234567890);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_uint64(list, "key6", 1234567890);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_double(list, "key7", 1234567890.1234567890);
    TEST_CHECK(ret == 0);

    array = cfl_array_create(2);
    TEST_CHECK(array != NULL);

    ret = cfl_kvlist_insert_array(list, "key8", array);
    TEST_CHECK(ret == 0);

    ret = cfl_kvlist_insert_new_array(list, "key9", 0);
    TEST_CHECK(ret == 0);

    list2 = cfl_kvlist_create();
    ret = cfl_kvlist_insert_kvlist(list, "key10", list2);
    TEST_CHECK(ret == 0);

    variant = cfl_variant_create_from_string("value1");
    TEST_CHECK(variant != NULL);

    ret = cfl_kvlist_insert(list, "key11", variant);
    TEST_CHECK(ret == 0);

    /* Count elements */
    ret = cfl_kvlist_count(list);
    TEST_CHECK(ret == 11);

    /* Fetch */
    variant = cfl_kvlist_fetch(list, "key8");
    TEST_CHECK(variant != NULL);
    TEST_CHECK(variant->type == CFL_VARIANT_ARRAY);

    /* Check if entry exists */
    ret = cfl_kvlist_contains(list, "key7");
    TEST_CHECK(ret == 1);

    ret = cfl_kvlist_contains(list, "key12");
    TEST_CHECK(ret == 0);

    /* Remove entry */
    ret = cfl_kvlist_remove(list, "key5");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_count(list);
    TEST_CHECK(ret == 10);

    /* Remove all entries one by one (for memory sanitizer) */
    ret = cfl_kvlist_remove(list, "key1");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key2");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key3");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key4");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key6");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key7");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key8");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key9");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key10");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_remove(list, "key11");
    TEST_CHECK(ret == CFL_TRUE);

    ret = cfl_kvlist_count(list);
    TEST_CHECK(ret == 0);

    /* failure scenarios */
    ret = cfl_kvlist_insert_string(list, NULL, "value1");
    TEST_CHECK(ret < 0);

    cfl_kvlist_destroy(list);
}

TEST_LIST = {
    {"basics", test_basics},
    { 0 }
};

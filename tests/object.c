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

static int compare(FILE *fp, char *expect)
{
    size_t len;
    size_t ret_fp;
    char buf[128] = {0};

    len = strlen(expect);

    if (fseek(fp, 0, SEEK_SET) != 0) {
        return -1;
    }

    ret_fp = fread(&buf[0], 1, sizeof(buf) - 1, fp);
    if (ret_fp == 0 && ferror(fp)) {
        return -1;
    }

    if (strlen(buf) != len) {
        return -1;
    }

    return strncmp(expect, &buf[0], len);
}

static void test_basics()
{
    int ret;
    struct cfl_object *object;
    struct cfl_kvlist *list;
    struct cfl_array *array;

    object = cfl_object_create();
    TEST_CHECK(object != NULL);

    /*
     * Inserts
     */
    list = cfl_kvlist_create();
    TEST_CHECK(list != NULL);

    ret = cfl_object_set(object, CFL_OBJECT_KVLIST, list);
    TEST_CHECK(ret == 0);

    array = cfl_array_create(2);
    TEST_CHECK(array != NULL);

    ret = cfl_kvlist_insert_array(list, "key1", array);
    TEST_CHECK(ret == 0);

    /*
     * Destroy
     */
    cfl_object_destroy(object);
}

static void test_replace_and_print()
{
    int ret;
    FILE *fp;
    struct cfl_object *object;
    struct cfl_variant *variant;

    object = cfl_object_create();
    TEST_CHECK(object != NULL);

    variant = cfl_variant_create_from_string("first");
    TEST_CHECK(variant != NULL);

    ret = cfl_object_set(object, CFL_OBJECT_VARIANT, variant);
    TEST_CHECK(ret == 0);

    variant = cfl_variant_create_from_string("second");
    TEST_CHECK(variant != NULL);

    ret = cfl_object_set(object, CFL_OBJECT_VARIANT, variant);
    TEST_CHECK(ret == 0);

    ret = cfl_object_set(object, CFL_OBJECT_VARIANT, NULL);
    TEST_CHECK(ret == -1);

    fp = tmpfile();
    TEST_CHECK(fp != NULL);

    ret = cfl_object_print(fp, object);
    TEST_CHECK(ret == 0);

    ret = compare(fp, "\"second\"\n");
    TEST_CHECK(ret == 0);

    fclose(fp);
    cfl_object_destroy(object);
}

TEST_LIST = {
    { "test_basics", test_basics },
    { "test_replace_and_print", test_replace_and_print },
    { 0 }
};

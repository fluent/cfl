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

#include <cfl/cfl.h>
#include "cfl_tests_internal.h"

static void test_sds_usage()
{
    cfl_sds_t s;

    s = cfl_sds_create("test");
    TEST_CHECK(s != NULL);
    TEST_CHECK(cfl_sds_len(s) == 4);
    TEST_CHECK(cfl_sds_alloc(s) == 4);
    TEST_CHECK(strcmp("test", s) == 0);

    s = cfl_sds_cat(s, ",cat message", 12);
    TEST_CHECK(strcmp("test,cat message", s) == 0);

    cfl_sds_destroy(s);
}

static void test_sds_printf()
{
    int len;
    cfl_sds_t s;
    cfl_sds_t tmp;
    char *str = "0123456789ABCDEFGHIJQLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvewxyz";

    s = cfl_sds_create_size(10);
    tmp = cfl_sds_printf(&s, "%s=%s", str, str);

    len = (strlen(str) * 2) + 1;
    TEST_CHECK(tmp == s);
    TEST_CHECK(cfl_sds_len(s) == len);
    cfl_sds_destroy(s);
}


static int sds_twice(size_t sds_size, const char *input, const char* expect)
{
    int ret;
    cfl_sds_t s;
    cfl_sds_t tmp;

    s = cfl_sds_create_size(sds_size);
    ret = cfl_sds_snprintf(&s, cfl_sds_avail(s), "%s%s", input, input);
    if (!TEST_CHECK(ret >= 0)) {
        TEST_MSG("cfl_sds_snprintf failed. ret=%d", ret);
        return -1;
    }

    /* check if buffer size is enough */
    if (ret >= cfl_sds_avail(s)) {
        tmp = cfl_sds_increase(s, ret - cfl_sds_avail(s) + 1);
        if (!TEST_CHECK(tmp != NULL)) {
            TEST_MSG("cfl_sds_increase failed");
            cfl_sds_destroy(s);
            return -1;
        }
        s = tmp;
        ret = cfl_sds_snprintf(&s, cfl_sds_avail(s), "%s%s", input, input);
        if (!TEST_CHECK(ret >= 0)) {
            TEST_MSG("cfl_sds_snprintf failed. ret=%d", ret);
            cfl_sds_destroy(s);
            return -1;
        }
        else if (!TEST_CHECK(ret < cfl_sds_avail(s))) {
            TEST_MSG("buffer is short. avail=%d, ret=%d, sds=%s",cfl_sds_avail(s), ret, s);
            cfl_sds_destroy(s);
            return -1;
        }
    }

    if (!TEST_CHECK(strcmp(s, expect) == 0)) {
        TEST_MSG("strcmp failed. \ngot   =%s\nexpect=%s", s, expect);
    }

    cfl_sds_destroy(s);
    return 0;
}

struct sds_snprintf_case {
    size_t size;
    const char *input;
    const char *expect;
};

static void test_sds_snprintf_simple()
{
    int ret;
    cfl_sds_t buf;
    size_t buf_size = 256;

    buf = cfl_sds_create_size(buf_size);
    if (!TEST_CHECK(buf != NULL)) {
        TEST_MSG("cfl_sds_create_size failed");
        return;
    }

    ret = cfl_sds_snprintf(&buf, buf_size, "%d%s%zu", 10, "str", buf_size);
    if (!TEST_CHECK(ret >= 0)) {
        TEST_MSG("cfl_sds_snprintf failed. ret=%d", ret);
        cfl_sds_destroy(buf);
        return;
    }
    else if (!TEST_CHECK(ret < cfl_sds_avail(buf))) {
        TEST_MSG("buffer size is short");
        cfl_sds_destroy(buf);
        return;
    }

    if (!TEST_CHECK(strcmp(buf, "10str256") == 0)) {
        TEST_MSG("strcmp failed. \ngot   =%s\nexpect=10str256", buf);
    }

    cfl_sds_destroy(buf);
}

static void test_sds_snprintf_cases()
{
    int ret;
    int i;
    struct sds_snprintf_case cases[] = {
        {128, "a", "aa"},
        {10, "0123456789ABCDEF", "0123456789ABCDEF0123456789ABCDEF"},
        {0, NULL}
    };

    for (i=0; cases[i].input != NULL; i++) {
        ret = sds_twice(cases[i].size, cases[i].input, cases[i].expect);
        if (!TEST_CHECK(ret == 0)) {
            TEST_MSG("case %d failed", i);
        }
    }
}

TEST_LIST = {
    { "sds_usage" , test_sds_usage},
    { "sds_printf", test_sds_printf},
    { "sds_snprintf_simple", test_sds_snprintf_simple},
    { "sds_snprintf_cases", test_sds_snprintf_cases},
    { 0 }
};

/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#include <cfl/cfl.h>

static size_t record_count;
static size_t fail_at = SIZE_MAX;

static struct cfl_variant *create_record_root(struct cfl_arena *arena,
                                             struct cfl_kvlist *record)
{
    if (record_count++ == fail_at) {
        /* Simulate a failed record without leaking its completed contents. */
        cfl_kvlist_destroy(record);
        return NULL;
    }

    return cfl_variant_create_from_kvlist_in(arena, record);
}

#define cfl_variant_create_from_kvlist_in create_record_root
#define main benchmark_records_main
#include "../benchmarks/records.c"
#undef main
#undef cfl_variant_create_from_kvlist_in

#include "cfl_tests_internal.h"

static void test_heap_batch_failure(void)
{
    struct cfl_variant *roots[3] = {NULL, NULL, NULL};
    struct batch_options options;
    size_t failure_index;
    size_t index;
    int result;

    options.records = 3;
    options.referenced = CFL_FALSE;
    options.mutate = CFL_TRUE;

    for (failure_index = 0; failure_index < options.records; failure_index++) {
        record_count = 0;
        fail_at = SIZE_MAX;
        result = run_batch(NULL, roots, &options);
        TEST_CHECK(result == 0);
        if (result != 0) {
            return;
        }

        record_count = 0;
        fail_at = failure_index;
        result = run_batch(NULL, roots, &options);
        TEST_CHECK(result == -1);
        TEST_CHECK(record_count == failure_index + 1);
        for (index = 0; index < options.records; index++) {
            TEST_CHECK(roots[index] == NULL);
        }
    }
}

TEST_LIST = {
    {"heap_batch_failure", test_heap_batch_failure},
    {0}
};

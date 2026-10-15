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

struct test {
	struct cfl_list _head;
};

static void checks()
{
    struct test *t;
    struct cfl_list list;

    cfl_list_init(&list);
    TEST_CHECK(cfl_list_is_empty(&list));

    t = malloc(sizeof(struct test));
    cfl_list_add(&t->_head, &list);
    TEST_CHECK(!cfl_list_is_empty(&list));

    cfl_list_del(&t->_head);
    TEST_CHECK(cfl_list_is_empty(&list));

    free(t);
}

static void add()
{
    int i;
    int count = 0;
    struct cfl_list list;
    struct cfl_list *head;
    struct cfl_list *tmp;

    struct node {
        int value;
        struct cfl_list _head;
    };

    struct node **nodes;
    struct node *node;

    nodes = malloc(sizeof(struct node *) * 3);
    for (i = 0; i < 3; i++) {
        nodes[i] = malloc(sizeof(struct node));
        nodes[i]->value = i;
    }

    cfl_list_init(&list);
    cfl_list_add(&nodes[0]->_head, &list);
    cfl_list_add(&nodes[2]->_head, &list);


    node = nodes[2];
    cfl_list_add_before(&nodes[1]->_head, &node->_head, &list);

    /* print all nodes */
    printf("\n");
    cfl_list_foreach(head, &list) {
        node = cfl_list_entry(head, struct node, _head);
        printf("node value: %d\n", node->value);
        count++;
	}
    TEST_CHECK(count == 3);

    cfl_list_foreach_safe(head, tmp, &list) {
        node = cfl_list_entry(head, struct node, _head);
        cfl_list_del(&node->_head);
        free(node);
        count++;
	}

    free(nodes);
}

static void add_after()
{
    int i;
    int expected[] = {0, 1, 2, 3, 4};
    struct cfl_list list;
    struct cfl_list *head;
    struct cfl_list *tmp;
    struct node {
        int value;
        struct cfl_list _head;
    };
    struct node nodes[5];
    struct node *node;

    for (i = 0; i < 5; i++) {
        nodes[i].value = i;
    }

    cfl_list_init(&list);
    cfl_list_add(&nodes[0]._head, &list);
    cfl_list_add(&nodes[2]._head, &list);

    /* insert in the middle and after the tail */
    cfl_list_add_after(&nodes[1]._head, &nodes[0]._head, &list);
    cfl_list_add_after(&nodes[3]._head, &nodes[2]._head, &list);

    /* the next entry must point back to the inserted one */
    TEST_CHECK(nodes[2]._head.prev == &nodes[1]._head);
    TEST_CHECK(list.prev == &nodes[3]._head);

    /* a tail insert must keep the entry added after the old tail */
    cfl_list_add(&nodes[4]._head, &list);

    i = 0;
    cfl_list_foreach(head, &list) {
        node = cfl_list_entry(head, struct node, _head);
        TEST_CHECK(i < 5 && node->value == expected[i]);
        i++;
    }
    TEST_CHECK(i == 5);

    i = 4;
    for (head = list.prev; head != &list; head = head->prev) {
        node = cfl_list_entry(head, struct node, _head);
        TEST_CHECK(i >= 0 && node->value == expected[i]);
        i--;
    }
    TEST_CHECK(i == -1);

    cfl_list_foreach_safe(head, tmp, &list) {
        cfl_list_del(head);
    }
    TEST_CHECK(cfl_list_is_empty(&list));
}

TEST_LIST = {
    {"checks",  checks},
    {"add", 	add},
    {"add_after", add_after},
    { 0 }
};


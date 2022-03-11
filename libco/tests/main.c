#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "co-test.h"

int g_count = 0;

static void add_count() {
    g_count++;
}

static int get_count() {
    return g_count;
}

static void work_loop(void *arg) {
    const char *s = (const char*)arg;
    // printf("%s beg\n", s);
    for (int i = 0; i < 400 / 8; ++i) {
        printf("%s%d  ", s, get_count());
        add_count();
        co_yield();
    }
    // printf("%s fi\n", s);
}

static void work(void *arg) {
    work_loop(arg);
}

static void test_1() {

    struct co *thd1 = co_start("thread-1", work, "X");
    struct co *thd2 = co_start("thread-2", work, "Y");
    struct co *thd3 = co_start("thread-3", work, "Z");
    struct co *thd4 = co_start("thread-4", work, "a");
    struct co *thd5 = co_start("thread-5", work, "b");
    struct co *thd6 = co_start("thread-6", work, "c");
    struct co *thd7 = co_start("thread-7", work, "d");
    struct co *thd8 = co_start("thread-8", work, "e");

    co_yield();
    co_wait(thd1);
    co_yield();
    co_wait(thd2);
    co_wait(thd3);
    co_wait(thd4);
    co_wait(thd5);
    co_yield();
    co_wait(thd6);
    co_wait(thd7);
    co_wait(thd8);

//    printf("\n");
}

// -----------------------------------------------

static int g_running = 1;

static void do_produce(Queue *queue) {
    assert(!q_is_full(queue));
    Item *item = (Item*)malloc(sizeof(Item));
    if (!item) {
        fprintf(stderr, "New item failure\n");
        return;
    }
    item->data = (char*)malloc(10);
    if (!item->data) {
        fprintf(stderr, "New data failure\n");
        free(item);
        return;
    }
    memset(item->data, 0, 10);
    sprintf(item->data, "libco-%d", g_count++);
    q_push(queue, item);
}

static void producer(void *arg) {
    // printf("Produce something\n");
    Queue *queue = (Queue*)arg;
    for (int i = 0; i < 100; ) {
        // printf("queue sz = %d cap = %d\n", queue->sz, queue->cap);

        if (!q_is_full(queue)) {
            // co_yield();
            do_produce(queue);
            i += 1;
            // printf("Produce: g_running = %d\n", g_running);
        }
        co_yield();
        // printf("After yield\n");
    }
}

static void do_consume(Queue *queue) {
    assert(!q_is_empty(queue));

    Item *item = q_pop(queue);
    if (item) {
        printf("%s  ", (char *)item->data);
        free(item->data);
        free(item);
    }
}

static void consumer(void *arg) {
    Queue *queue = (Queue*)arg;
    while (g_running) {
        if (!q_is_empty(queue)) {
            do_consume(queue);
        }
        co_yield();
    }
}

static void test_2() {

    Queue *queue = q_new();
    printf("begin test2!!!\n");

    co_yield();
    printf("begin create pds 1\n");
    struct co *pd1 = co_start("producer-1", producer, queue);
    struct co *pd2 = co_start("producer-2", producer, queue);
    co_yield();
    printf("begin create pds 2\n");
    struct co *pd3 = co_start("producer-3", producer, queue);
    printf("begin create pds 3\n");
    co_yield();
    printf("begin create pds 4\n");
    struct co *pd4 = co_start("producer-4", producer, queue);
    printf("begin create pds 5\n");
    // struct co *pd5 = co_start("producer-5", producer, queue);
    // struct co *pd6 = co_start("producer-6", producer, queue);
    // struct co *pd7 = co_start("producer-7", producer, queue);

    struct co *cs1 = co_start("consumer-1", consumer, queue);
    printf("begin create pds 6\n");
    co_yield();
    printf("begin create pds 6\n");
    struct co *cs2 = co_start("consumer-2", consumer, queue);

    co_yield();
    co_wait(pd1);
    co_yield();
    co_wait(pd2);
    co_yield();
    co_wait(pd3);
    co_wait(pd4);
    // co_wait(pd5);
    // co_wait(pd6);
    // co_wait(pd7);

    g_running = 0;

    co_wait(cs1);
    co_yield();
    co_wait(cs2);

    while (!q_is_empty(queue)) {
        do_consume(queue);
    }

    q_free(queue);
    printf("end test2!!!\n");
}

int main() {
    setbuf(stdout, NULL);

    // printf("Test #1. Expect: (X|Y){0, 1, 2, ..., 199}\n");
    // test_1();

    // printf("\n\nTest #2. Expect: (libco-){200, 201, 202, ..., 399}\n");
    int T = 0;
    while(1) {
        test_2();
        printf("\n finished test_2 %d times\n", T++);
    }

    printf("\n\n");

    return 0;
}

#include "stack.h"

#include <assert.h>
#include <stdlib.h>

enum {
    INS_DEL_STEPS = 1000,
    INS_AMOUNT = 20,
    DEL_AMOUNT = 10,
    DEL_STEPS = INS_DEL_STEPS + 5,
};

void push_test(Stack* stk, size_t* const counter) {
    assert(stk);
    assert(counter);

    for (size_t i = 0; i < INS_AMOUNT; i++) {
        int push_val = rand();
        void const* push_ret = stack_push(stk, &push_val);
        assert(push_ret == &push_val);
        (*counter)++;
        assert(stack_size(stk) == *counter);

        int top_val = 0;
        void const* top_ret = stack_top(stk, &top_val);
        assert(top_ret == &top_val);

        assert(push_val == top_val);
    }
}

void pop_test(Stack* stk, size_t* const counter) {
    assert(stk);
    assert(counter);

    for (size_t i = 0; i < DEL_AMOUNT; i++) {
        int top_val = 0;
        void const* top_ret = stack_top(stk, &top_val);
        if (*counter) {
            assert(top_ret == &top_val);
        } else {
            assert(top_ret == NULL);
            assert(stack_get_error(stk) == STACK_OPERATION_ERROR);
        }

        int pop_val = 0;
        void const* pop_ret = stack_pop(stk, &pop_val);
        if (*counter) {
            assert(pop_ret == &pop_val);
        } else {
            assert(pop_ret == NULL);
            assert(stack_get_error(stk) == STACK_OPERATION_ERROR);
        }

        assert(pop_val == top_val);
        if (*counter) {
            (*counter)--;
            assert(stack_size(stk) == *counter);
        }
    }
}

void test_stack() {
    Stack* stk = stack_allocate(sizeof(int));
    assert(stk);

    printf("Start stack testing\n");

    size_t counter = 0;
    for (size_t i = 0; i < INS_DEL_STEPS; i++) {
        pop_test(stk, &counter);
        push_test(stk, &counter);
    }

    for (size_t i = 0; i < DEL_STEPS; i++) {
        pop_test(stk, &counter);
    }

    stack_free(stk);

    printf("Tests passed\n");
}

int main() {
    test_stack();
    return 0;
}

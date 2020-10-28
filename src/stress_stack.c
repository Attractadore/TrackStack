#define STACK_ELEM_TYPE int
#include "stack_generic.h"

#include <assert.h>
#include <stdlib.h>

enum {
    INS_DEL_STEPS = 1000,
    INS_AMOUNT = 20,
    DEL_AMOUNT = 10,
    DEL_STEPS = INS_DEL_STEPS + 5,
};

void push_test(Stack_int* stk, size_t* const counter) {
    assert(stk);
    assert(counter);

    for (size_t i = 0; i < INS_AMOUNT; i++) {
        int push_val = rand();
        StackPush_int(stk, push_val);
        (*counter)++;
        assert(StackSize_int(stk) == *counter);

        int top_val = StackTop_int(stk);
        assert(push_val == top_val);
    }
}

void pop_test(Stack_int* stk, size_t* const counter) {
    assert(stk);
    assert(counter);

    for (size_t i = 0; i < DEL_AMOUNT; i++) {
        int top_val = StackTop_int(stk);
        if (!*counter) {
            assert(StackGetError_int(stk) == STACK_OPERATION_ERROR);
        }

        int pop_val = StackPop_int(stk);
        if (!*counter) {
            assert(StackGetError_int(stk) == STACK_OPERATION_ERROR);
        }

        assert(pop_val == top_val);
        if (*counter) {
            (*counter)--;
            assert(StackSize_int(stk) == *counter);
        }
    }
}

void test_stack() {
    Stack_int* stk = StackAllocate_int();
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

    StackFree_int(stk);

    printf("Tests passed\n");
}

int main() {
    test_stack();
    return 0;
}

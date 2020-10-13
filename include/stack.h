#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum stack_error_e {
    STACK_OK,
    STACK_ALLOCATION_ERROR,
    STACK_CORRUPTION_ERROR,
} STACK_ERROR;

typedef struct stack_t Stack;

Stack* stack_allocate(size_t stk_elem_sz);

void const* stack_push(Stack* stk, void const* elem_p);

void* stack_pop(Stack* stk, void* elem_p);

void* stack_top(Stack* stk, void* elem_p);

size_t stack_size(Stack* stack);

bool stack_empty(Stack* stack);

void stack_free(Stack* stk);

STACK_ERROR stack_get_error(Stack* stk);

char const* stack_error_string(STACK_ERROR error);

#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum stack_error_e {
    STACK_OK,
    STACK_ALLOCATION_ERROR,
    STACK_OPERATION_ERROR,
    
#ifdef USE_CANARY
    STACK_METADATA_CANARY_OVERWRITE_ERROR,
#endif

#ifdef USE_HASH
    STACK_METADATA_HASH_ERROR,
#endif

#ifdef USE_DATA_CANARY
    STACK_DATA_CANARY_OVERWRITE_ERROR,
#endif

#ifdef USE_HASH_FULL
    STACK_DATA_HASH_ERROR,
#endif

#ifdef USE_POISON
    STACK_POISON_OVERWRITE_ERROR,
#endif

    STACK_CORRUPTION_ERROR,
} STACK_ERROR;

typedef struct stack_t Stack;

Stack* stack_allocate(size_t stk_elem_sz);

void const* stack_push(Stack* stk, void const* elem_p);

void* stack_pop(Stack* stk, void* elem_p);

void* stack_top(Stack* stk, void* elem_p);

size_t stack_size(Stack* stk);

bool stack_empty(Stack* stk);

void stack_free(Stack* stk);

STACK_ERROR stack_get_error(Stack* stk);

char const* stack_error_string(STACK_ERROR error);

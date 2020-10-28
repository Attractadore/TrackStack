#pragma once

#include "stack.h"

#ifndef STACK_ELEM_TYPE
#error STACK_ELEM_TYPE not defined
#endif

#define CAT_HELPER(name1, name2) name1 ## _ ## name2
#define CAT(name1, name2) CAT_HELPER(name1, name2)
#define OVERLOAD(name) CAT(name, STACK_ELEM_TYPE) 

struct CAT(OVERLOAD(stack), t);
typedef struct CAT(OVERLOAD(stack), t) OVERLOAD(Stack);

#define STACK_TYPE OVERLOAD(Stack)

#define STACK_ALLOCATE OVERLOAD(StackAllocate)
#define STACK_FREE OVERLOAD(StackFree)
#define STACK_PUSH OVERLOAD(StackPush)
#define STACK_POP OVERLOAD(StackPop)
#define STACK_TOP OVERLOAD(StackTop)
#define STACK_SIZE OVERLOAD(StackSize)
#define STACK_CAPACITY OVERLOAD(StackCapacity)
#define STACK_EMPTY OVERLOAD(StackEmpty)
#define STACK_RESERVE OVERLOAD(StackReserve)
#define STACK_DUMP OVERLOAD(StackDump)
#define STACK_GET_ERROR OVERLOAD(StackGetError)
#define STACK_ERROR_STRING OVERLOAD(StackErrorString)

static inline STACK_TYPE* STACK_ALLOCATE() {
    return (STACK_TYPE*) stack_allocate(sizeof(STACK_ELEM_TYPE));
}

static inline void STACK_PUSH(STACK_TYPE* stk, STACK_ELEM_TYPE elem) {
    stack_push((Stack*) stk, &elem);
}

static inline STACK_ELEM_TYPE STACK_POP(STACK_TYPE* stk) {
    STACK_ELEM_TYPE elem;
    stack_pop((Stack*) stk, &elem);
    return elem;
}

static inline STACK_ELEM_TYPE STACK_TOP(STACK_TYPE* stk) {
    STACK_ELEM_TYPE elem;
    stack_top((Stack*) stk, &elem);
    return elem;
}

static inline size_t STACK_SIZE(STACK_TYPE* stk) {
    return stack_size((Stack*) stk);
}

static inline size_t STACK_CAPACITY(STACK_TYPE* stk) {
    return stack_capacity((Stack*) stk);
}

static inline bool STACK_EMPTY(STACK_TYPE* stk) {
    return stack_empty((Stack*) stk);
}

static inline size_t STACK_RESERVE(STACK_TYPE* stk, size_t num) {
    return stack_reserve((Stack*) stk, num);
}

static inline void STACK_DUMP(STACK_TYPE* stk, FILE* file) {
    stack_dump((Stack*) stk, file);
}

static inline STACK_ERROR STACK_GET_ERROR(STACK_TYPE* stk) {
    return stack_get_error((Stack*) stk);
}

static inline char const* STACK_ERROR_STRING(STACK_ERROR err) {
    return stack_error_string(err);
}

static inline void STACK_FREE (STACK_TYPE* stk) {
    stack_free((Stack*) stk);
}

#undef STACK_ALLOLOCATE 
#undef STACK_FREE 
#undef STACK_PUSH 
#undef STACK_POP 
#undef STACK_TOP 
#undef STACK_SIZE 
#undef STACK_CAPACITY 
#undef STACK_EMPTY 
#undef STACK_RESERVER 
#undef STACK_DUMP 
#undef STACK_GET_ERROR 
#undef STACK_ERROR_STRING 

#undef STACK_TYPE
#undef STACK_ELEM_TYPE
#undef OVERLOAD
#undef CAT
#undef CAT_HELPER

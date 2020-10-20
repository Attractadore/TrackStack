#pragma once

#define USE_CANARY
#define USE_HASH_FULL
#define USE_LOG
#define USE_POISON
#define USE_DATA_CANARY

#if defined(USE_HASH_FAST) || defined(USE_HASH_FULL)
#define USE_HASH
#endif

#include "stack.h"

#include <stddef.h>

#if defined (USE_CANARY) || defined(USE_DATA_CANARY)
typedef unsigned long long canary_type;
#define CANARY_VALUE 0xF072E3546BAD189Cull
#endif

#ifdef USE_HASH
typedef unsigned long long hash_type;
#define HASH_INITIAL_VALUE 0x600D4A54ull
#endif

struct stack_t {
#ifdef USE_CANARY
    canary_type front_canary;
#endif

    void* data;
    size_t elem_sz;
    size_t size;
    size_t capacity;
    STACK_ERROR error;

#ifdef USE_HASH
    hash_type metadata_hash;
#ifdef USE_HASH_FULL
    hash_type data_hash;
#endif
#endif

#ifdef USE_CANARY
    canary_type back_canary;
#endif
};

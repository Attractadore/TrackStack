#include "stack.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

enum { STACK_DEFAULT_CAPACITY = 10 };

typedef unsigned long long canary_type;

struct stack_t {
    canary_type canary;
    void* data;
    size_t elem_sz;
    size_t size;
    size_t capacity;
    STACK_ERROR error;
};

canary_type stack_hash(Stack* stk) {
    assert(stk);
    return (canary_type) stk->data ^
           (canary_type) stk->elem_sz ^
           (canary_type) stk->size ^
           (canary_type) stk->capacity ^
           (canary_type) stk->error;
}

void stack_update_canary(Stack* stk) {
    assert(stk);

    stk->canary = stack_hash(stk);
}

void stack_dump_elem(FILE* file, void const* elem_p, size_t sz) {
    assert(file);
    assert(elem_p);
    assert(sz);

    fprintf(file, "0x");
    for (size_t i = 0; i < sz; i++) {
        fprintf(file, "%X", ((unsigned char const*) elem_p)[i]);
    }
    fprintf(file, "\n");
}

void stack_dump(Stack* stk, int line, char const* func_name, char const* file_name) {
    assert(stk);

    static FILE* log_file = NULL;
    if (!log_file) {
        log_file = fopen("stack_log", "ab");
        setvbuf(log_file, NULL, _IONBF, 0);
        if (!log_file) {
            return;
        }
    }

    time_t tm = time(NULL);
    fprintf(log_file, "%s\n", ctime(&tm));
    fprintf(log_file, "stack dump called from line %d from %s from file %s\n", line, func_name, file_name);
    fprintf(log_file, "stack status is: \"%s\"\n", stack_error_string(stk->error));
    fprintf(log_file, "stack address is: %p\n", (void*) stk);
    fprintf(log_file, "stack element size is %lu; stack size is %lu; stack capacity is %lu\n", stk->elem_sz, stk->size, stk->capacity);
    fprintf(log_file, "stack data address is: %p\n", stk->data);
    fprintf(log_file, "stack data is:\n");
    for (size_t i = 0; i < stk->capacity; i++) {
        if (i < stk->size) {
            fprintf(log_file, "[%lu]: ", i);
        } else {
            fprintf(log_file, "(%lu): ", i);
        }
        stack_dump_elem(log_file, (char const*) stk->data + i * stk->elem_sz, stk->elem_sz);
    }
    fprintf(log_file, "\n");
}

void stack_verify(Stack* stk, int line, char const* func_name, char const* file_name) {
    assert(stk);

    if (stk->size > stk->capacity) {
        stk->error = STACK_CORRUPTION_ERROR;
    }

    if (!stk->elem_sz) {
        stk->error = STACK_CORRUPTION_ERROR;
    }

    if (stk->error != STACK_OK && stk->error != STACK_ALLOCATION_ERROR && stk->error != STACK_CORRUPTION_ERROR) {
        stk->error = STACK_CORRUPTION_ERROR;
    }

    if (stk->canary != stack_hash(stk)) {
        stk->error = STACK_CORRUPTION_ERROR;
    }

    stack_dump(stk, line, func_name, file_name);

    if (stk->error == STACK_CORRUPTION_ERROR) {
        abort();
    }
}

#ifndef NDEBUG
#define stack_verify(stk) stack_verify(stk, __LINE__, __func__, __FILE__)
#else
#define stack_verify(stk)
#endif

void stack_resize(Stack* stk, size_t new_capacity) {
    assert(stk);
    assert(new_capacity >= stk->size);

    stack_verify(stk);

    void* new_data = NULL;
    while (new_capacity > stk->capacity && !(new_data = realloc(stk->data, new_capacity * stk->elem_sz))) {
        new_capacity = (stk->capacity + new_capacity) / 2;
    }
    if (!new_data) {
        stk->error = STACK_ALLOCATION_ERROR;
        stack_update_canary(stk);
        return;
    }
    stk->capacity = new_capacity;
    stk->data = new_data;
    stack_update_canary(stk);
}

size_t stack_recomended_capacity(Stack* stk) {
    assert(stk);

    stack_verify(stk);

    assert(stk->capacity >= STACK_DEFAULT_CAPACITY);

    if (stk->size < stk->capacity / 2 && stk->capacity / 2 >= STACK_DEFAULT_CAPACITY) {
        return stk->capacity / 2;
    }
    if (stk->size == stk->capacity) {
        return stk->capacity * 2;
    }
    return stk->capacity;
}

void stack_adjust(Stack* stk) {
    assert(stk);

    stack_verify(stk);

    size_t recomended_capacity = stack_recomended_capacity(stk);
    if (stk->capacity != recomended_capacity) {
        stack_resize(stk, recomended_capacity);
    }
}

Stack* stack_allocate(size_t stk_elem_sz) {
    assert(stk_elem_sz);

    Stack* stk = calloc(1, sizeof(*stk));
    if (!stk) {
        return NULL;
    }
    stk->error = STACK_OK;
    stk->elem_sz = stk_elem_sz;
    stk->size = 0;
    stk->capacity = STACK_DEFAULT_CAPACITY;
    stk->data = calloc(stk->capacity, stk->elem_sz);
    if (!stk->data) {
        free(stk);
        return NULL;
    }
    stack_update_canary(stk);
    return stk;
}

void const* stack_push(Stack* stk, void const* elem_p) {
    assert(stk);
    assert(elem_p);

    stack_verify(stk);

    stack_adjust(stk);
    if (stk->error == STACK_ALLOCATION_ERROR) {
        return NULL;
    }

    assert(stk->size < stk->capacity);

    memcpy((char*) stk->data + ((stk->size)++ * stk->elem_sz), elem_p, stk->elem_sz);
    stack_update_canary(stk);

    return elem_p;
}

void* stack_top(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    stack_verify(stk);

    if (!stk->size) {
        return NULL;
    }

    memcpy(elem_p, (char const*) stk->data + (stk->size - 1) * stk->elem_sz, stk->elem_sz);

    return elem_p;
}

void* stack_pop(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    stack_verify(stk);

    if (!stack_top(stk, elem_p)) {
        return NULL;
    }

    assert(stk->size);

    stk->size--;
    stack_update_canary(stk);
    stack_adjust(stk);

    return elem_p;
}

size_t stack_size(Stack* stk) {
    assert(stk);

    stack_verify(stk);

    return stk->size;
}

bool stack_empty(Stack* stk) {
    assert(stk);

    stack_verify(stk);

    return !stk->size;
}

void stack_free(Stack* stk) {
    assert(stk);

    free(stk->data);
    free(stk);
}

STACK_ERROR stack_get_error(Stack* stk) {
    assert(stk);

    stack_verify(stk);

    return stk->error;
}

char const* stack_error_string(STACK_ERROR error) {
    switch (error) {
        case STACK_OK:
            return "No error";
        case STACK_ALLOCATION_ERROR:
            return "Internal memory allocation error";
        case STACK_CORRUPTION_ERROR:
            return "Stack memory has been corrupted";
        default:
            return "Unknow error";
    }
}

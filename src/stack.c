#include "stack_check.c"

#include <stdlib.h>

#ifdef USE_HASH
void stack_update_hash(Stack* stk) {
    assert(stk);

    stk->metadata_hash = stack_metadata_hash(stk);
#ifdef USE_HASH_FULL
    stk->data_hash = stack_data_hash(stk);
#endif
}

void stack_update_metadata_hash(Stack* stk) {
    assert(stk);

    stk->metadata_hash = stack_metadata_hash(stk);
}

#ifdef USE_HASH_FULL
void stack_update_data_hash(Stack* stk) {
    assert(stk);

    stk->data_hash = stack_data_hash(stk);
}
#endif
#define STACK_REHASH_METADATA(stk) stack_update_metadata_hash(stk)
#define STACK_REHASH(stk) stack_update_hash(stk)
#else
#define STACK_REHASH_METADATA(stk)
#define STACK_REHASH(stk)
#endif

#ifdef USE_POISON
void stack_write_poison(Stack* stk, size_t first_elem, size_t num_elem) {
    assert(stk);
    assert(stk->data);
    assert(first_elem >= stk->size);
    assert(first_elem + num_elem <= stk->capacity);

    stk->error = STACK_OK;
    size_t start_i = first_elem * stk->elem_sz;
    size_t end_i = (first_elem + num_elem) * stk->elem_sz;
    write_poison((char*) stk->data + start_i, start_i, end_i - start_i);
    STACK_REHASH(stk);
}
#define WRITE_POISON(stk, first_elem, num_elem) stack_write_poison(stk, first_elem, num_elem)
#else
#define WRITE_POISON(stk, first_elem, num_elem)
#endif

#ifdef USE_DATA_CANARY
void stack_set_data_canaries(Stack* stk) {
    assert(stk);
    assert(stk->data);

    const canary_type temp_canary = CANARY_VALUE;
    memcpy((canary_type*) stk->data - 1, &temp_canary, sizeof(temp_canary));
    memcpy((char*) stk->data + stk->capacity * stk->elem_sz, &temp_canary, sizeof(temp_canary));
}
#define SET_DATA_CANARIES(stk) stack_set_data_canaries(stk)
#else
#define SET_DATA_CANARIES(stk)
#endif

void _stack_unsafe_resize(Stack* stk, size_t new_capacity) {
    assert(stk);

    stk->error = STACK_OK;
#ifdef USE_DATA_CANARY
    // Check for unallocated stack
    void* old_data = (stk->data) ? ((canary_type*) stk->data - 1) : NULL;
    size_t new_data_size = new_capacity * stk->elem_sz + 2 * sizeof(canary_type);
#else
    void* old_data = stk->data;
    size_t new_data_size = new_capacity * stk->elem_sz;
#endif

    void* new_data = realloc(old_data, new_data_size);
    if (!new_data) {
        stk->error = STACK_ALLOCATION_ERROR;
        STACK_REHASH_METADATA(stk);
        return;
    }
    stk->capacity = new_capacity;

#ifdef USE_DATA_CANARY
    stk->data = (canary_type*) new_data + 1;
#else
    stk->data = new_data;
#endif
}

void stack_resize(Stack* stk, size_t new_capacity) {
    assert(stk);
    assert(new_capacity && new_capacity >= stk->size);

    STACK_VERIFY_RETURN(stk,);

    stk->error = STACK_OK;
    _stack_unsafe_resize(stk, new_capacity);
    if (stk->error == STACK_ALLOCATION_ERROR && new_capacity > stk->capacity + 1) {
        _stack_unsafe_resize(stk, stk->capacity + 1);
    }
    if (stk->error == STACK_ALLOCATION_ERROR) {
        STACK_REHASH_METADATA(stk);
        return;
    }
    WRITE_POISON(stk, stk->size, stk->capacity - stk->size);
    SET_DATA_CANARIES(stk);
    STACK_REHASH(stk);
}

size_t stack_recomended_capacity(Stack* stk) {
    assert(stk);

    STACK_VERIFY_RETURN(stk, stk->capacity);

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

    STACK_VERIFY_RETURN(stk,);

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
    stk->capacity = 0;
    stk->data = NULL;
    _stack_unsafe_resize(stk, STACK_DEFAULT_CAPACITY);
    if (stk->error != STACK_OK) {
        free(stk);
        return NULL;
    }
    WRITE_POISON(stk, 0, stk->capacity);
    SET_DATA_CANARIES(stk);
#ifdef USE_CANARY
    stk->front_canary = CANARY_VALUE;
    stk->back_canary = CANARY_VALUE;
#endif
    STACK_REHASH(stk);
    return stk;
}

void const* stack_push(Stack* stk, void const* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_VERIFY_RETURN(stk, NULL);

    stk->error = STACK_OK;
    stack_adjust(stk);
    if (stk->error != STACK_OK) {
        return NULL;
    }

    assert(stk->size < stk->capacity);

    memcpy((char*) stk->data + ((stk->size)++ * stk->elem_sz), elem_p, stk->elem_sz);
    STACK_REHASH(stk);

    return elem_p;
}

void* stack_top(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_VERIFY_RETURN(stk, NULL);

    if (stk->size) {
        stk->error = STACK_OK;
        memcpy(elem_p, (char const*) stk->data + (stk->size - 1) * stk->elem_sz, stk->elem_sz);
    }
    else {
        stk->error = STACK_OPERATION_ERROR;
        elem_p = NULL;
    }
    
    STACK_REHASH_METADATA(stk);

    return elem_p;
}

void* stack_pop(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_VERIFY_RETURN(stk, NULL);

    if (!stack_top(stk, elem_p)) {
        return NULL;
    }

    assert(stk->size);

    stk->error = STACK_OK;
    stk->size--;
    WRITE_POISON(stk, stk->size, 1);
    stack_adjust(stk);
    STACK_REHASH(stk);

    return elem_p;
}

size_t stack_size(Stack* stk) {
    assert(stk);

    STACK_VERIFY_RETURN(stk, 0);

    return stk->size;
}

bool stack_empty(Stack* stk) {
    assert(stk);

    STACK_VERIFY_RETURN(stk, true);

    return !stk->size;
}

void stack_free(Stack* stk) {
    assert(stk);

#ifdef USE_DATA_CANARY
    free((canary_type*) stk->data - 1);
#else
    free(stk->data);
#endif
    free(stk);
}

STACK_ERROR stack_get_error(Stack* stk) {
    assert(stk);

    STACK_VERIFY(stk);

    return stk->error;
}

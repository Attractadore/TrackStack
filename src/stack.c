#include "stack_check.c"

#include <stdlib.h>

#define TO_STRING(name) #name

char const* get_stack_compilation_options() {
    return "This stack was compiled with "
#ifdef USE_CANARY
        TO_STRING(USE_CANARY) " "
#endif
#ifdef USE_DATA_CANARY
        TO_STRING(USE_DATA_CANARY) " "
#endif
#ifdef USE_HASH_FAST
        TO_STRING(USE_HASH_FAST) " "
#endif
#ifdef USE_HASH_FULL
        TO_STRING(USE_HASH_FULL) " "
#endif
#ifdef USE_POISON
        TO_STRING(USE_POISON) " "
#endif
#ifdef USE_LOG
        TO_STRING(USE_LOG) " with log file " STACK_LOG_FILENAME " "
#endif
#if !defined(USE_CANARY) && !defined(USE_DATA_CANARY) && !defined(USE_HASH) && !defined(USE_POISON) && !defined(USE_LOG)
                           "no protections"
#endif
        ;
}

#ifdef USE_HASH
void stack_update_metadata_hash(Stack* stk) {
    assert(stk);

    stk->metadata_hash = stack_metadata_hash(stk);
    STACK_LOG(stk, "Update metadata hash");
}
#define STACK_REHASH_METADATA(stk) stack_update_metadata_hash(stk)
#else
#define STACK_REHASH_METADATA(stk)
#endif

#ifdef USE_HASH_FULL
void stack_update_data_hash(Stack* stk) {
    assert(stk);

    stk->data_hash = stack_data_hash(stk);
    STACK_LOG(stk, "Update data hash");
}
#define STACK_REHASH_DATA(stk) stack_update_data_hash(stk)
#else
#define STACK_REHASH_DATA(stk)
#endif

#define STACK_REHASH(stk)       \
    STACK_REHASH_METADATA(stk); \
    STACK_REHASH_DATA(stk)

#ifdef USE_POISON
void stack_write_poison(Stack* stk, size_t first_elem, size_t num_elem) {
    assert(stk);
    assert(stk->data);
    assert(first_elem >= stk->size);
    assert(first_elem + num_elem <= stk->capacity);

    size_t start = first_elem * stk->elem_sz;
    size_t num = num_elem * stk->elem_sz;
    write_poison((char*) stk->data + start, num);
    STACK_LOG(stk, "Write %zu bytes of poison", num);
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
    STACK_LOG(stk, "Set data canaries");
}
#define SET_DATA_CANARIES(stk) stack_set_data_canaries(stk)
#else
#define SET_DATA_CANARIES(stk)
#endif

void stack_unsafe_resize(Stack* stk, size_t new_capacity) {
    assert(stk);

    STACK_LOG(stk, "Attempt resize to %zu from %zu", new_capacity, stk->capacity);
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

    stack_unsafe_resize(stk, new_capacity);
    // If resizing fails and new capacity if greater than current capacity + 1,
    // try to add only one extra element
    if (stk->error == STACK_ALLOCATION_ERROR && new_capacity > stk->capacity + 1) {
        new_capacity = stk->capacity + 1;
        stack_unsafe_resize(stk, new_capacity);
    }
    if (stk->error == STACK_ALLOCATION_ERROR) {
        return;
    }
    WRITE_POISON(stk, stk->size, stk->capacity - stk->size);
    SET_DATA_CANARIES(stk);
    STACK_LOG(stk, "Capacity is now %zu", stk->capacity);
}

static const double STACK_GROW_FACTOR = 2.0;
static const double STACK_GROW_THRESHOLD = 1.0;
static const double STACK_SHRINK_FACTOR = 2.0 / 3.0;
static const double STACK_SHRINK_THRESHOLD = 0.5;

size_t stack_recomended_capacity(Stack* stk) {
    assert(stk);

    if (stk->capacity < stk->min_capacity) {
        return stk->min_capacity;
    }

    size_t recomended_capacity = stk->capacity;
    // Shrink only if new capacity will be greater than or equal to the minimum capacity
    if (stk->size <= stk->capacity * STACK_SHRINK_THRESHOLD &&
        stk->capacity * STACK_SHRINK_FACTOR >= stk->min_capacity) {
        recomended_capacity = stk->capacity * STACK_SHRINK_FACTOR;
    } else if (stk->size >= stk->capacity * STACK_GROW_THRESHOLD) {
        recomended_capacity = stk->capacity * STACK_GROW_FACTOR;
    }

    return recomended_capacity;
}

void stack_adjust(Stack* stk) {
    assert(stk);

    size_t recomended_capacity = stack_recomended_capacity(stk);
    if (stk->capacity != recomended_capacity) {
        stack_resize(stk, recomended_capacity);
    }
}

void initialize_stack_log() {
    if (stack_global_log) {
        return;
    }
    stack_global_log = fopen(STACK_LOG_FILENAME, "ab");
    if (stack_global_log) {
        setvbuf(stack_global_log, NULL, _IONBF, 0);
    }
}

void finalize_stack_log() {
    if (!stack_global_count && stack_global_log) {
        fclose(stack_global_log);
    }
}

Stack* stack_allocate(size_t stk_elem_sz) {
    assert(stk_elem_sz);

    Stack* stk = calloc(1, sizeof(*stk));
    if (!stk) {
        return NULL;
    }
    initialize_stack_log();
    STACK_LOG(stk, "Start new Stack allocation; there are %zu allocated Stacks", stack_global_count);
    stack_global_count++;
#ifdef USE_CANARY
    stk->front_canary = CANARY_VALUE;
    stk->back_canary = CANARY_VALUE;
#endif
    stk->error = STACK_OK;
    stk->elem_sz = stk_elem_sz;
    stk->data = NULL;
    stk->size = 0;
    stk->capacity = 0;
    stk->min_capacity = STACK_DEFAULT_CAPACITY;
    stack_resize(stk, stk->min_capacity);
    if (stk->error != STACK_OK) {
        stack_free(stk);
        return NULL;
    }
    STACK_REHASH(stk);
    STACK_LOG(stk, "Allocated new Stack; there are %zu allocated Stacks", stack_global_count);
    return stk;
}

void stack_free(Stack* stk) {
    if (stk) {
        if (stk->data) {
#ifdef USE_DATA_CANARY
            free((canary_type*) stk->data - 1);
#else
            free(stk->data);
#endif
        }
        free(stk);

        stack_global_count--;
        STACK_LOG(stk, "Freed Stack; there are %zu allocated Stacks", stack_global_count);
        finalize_stack_log();
    }
}

void const* stack_push(Stack* stk, void const* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_LOG(stk, "Attempting to push element");
    STACK_VERIFY_RETURN(stk, NULL);

    stk->error = STACK_OK;
    stack_adjust(stk);
    if (stk->error != STACK_OK) {
        STACK_REHASH_METADATA(stk);
        return NULL;
    }

    assert(stk->size < stk->capacity);

    memcpy((char*) stk->data + ((stk->size)++ * stk->elem_sz), elem_p, stk->elem_sz);

    STACK_REHASH(stk);

    char elem_str[elem_str_size(stk->elem_sz) + 1];
    if (elem_to_str(elem_p, elem_str, stk->elem_sz, sizeof(elem_str))) {
        STACK_LOG(stk, "Pushed element %s", elem_str);
    } else {
        STACK_LOG(stk, "Pushed unknown element", elem_str);
    }

    return elem_p;
}

void* stack_top(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_LOG(stk, "Attempting to peek at top element");
    STACK_VERIFY_RETURN(stk, NULL);

    if (!stk->size) {
        stk->error = STACK_OPERATION_ERROR;
        STACK_LOG(stk, "Error: peeking at empty stack");
        STACK_REHASH_METADATA(stk);
        return NULL;
    }

    assert(stk->size);

    stk->error = STACK_OK;
    memcpy(elem_p, (char const*) stk->data + (stk->size - 1) * stk->elem_sz, stk->elem_sz);

    STACK_REHASH_METADATA(stk);

    char elem_str[elem_str_size(stk->elem_sz) + 1];
    if (elem_to_str(elem_p, elem_str, stk->elem_sz, sizeof(elem_str))) {
        STACK_LOG(stk, "Toped element %s", elem_str);
    } else {
        STACK_LOG(stk, "Toped unknown element", elem_str);
    }

    return elem_p;
}

void* stack_pop(Stack* stk, void* elem_p) {
    assert(stk);
    assert(elem_p);

    STACK_LOG(stk, "Attempting to pop element");
    STACK_VERIFY_RETURN(stk, NULL);

    if (!stk->size) {
        stk->error = STACK_OPERATION_ERROR;
        STACK_LOG(stk, "Error: poping empty stack");
        STACK_REHASH_METADATA(stk);
        return NULL;
    }

    assert(stk->size);

    stk->error = STACK_OK;
    memcpy(elem_p, (char const*) stk->data + (stk->size - 1) * stk->elem_sz, stk->elem_sz);

    stk->size--;
    WRITE_POISON(stk, stk->size, 1);
    stack_adjust(stk);
    STACK_REHASH(stk);

    char elem_str[elem_str_size(stk->elem_sz) + 1];
    if (elem_to_str(elem_p, elem_str, stk->elem_sz, sizeof(elem_str))) {
        STACK_LOG(stk, "Poped element %s", elem_str);
    } else {
        STACK_LOG(stk, "Poped unknown element", elem_str);
    }

    return elem_p;
}

size_t stack_size(Stack* stk) {
    assert(stk);

    STACK_LOG(stk, "Attempting to get size");
    STACK_VERIFY_RETURN(stk, 0);
    STACK_LOG(stk, "Return size");

    return stk->size;
}

size_t stack_capacity(Stack* stk) {
    assert(stk);

    STACK_LOG(stk, "Attempting to get capacity");
    STACK_VERIFY_RETURN(stk, 0);
    STACK_LOG(stk, "Return capacity");

    return stk->capacity;
}

bool stack_empty(Stack* stk) {
    assert(stk);

    STACK_LOG(stk, "Attempting to check if empty");
    STACK_VERIFY_RETURN(stk, true);
    STACK_LOG(stk, "Return empty status");

    return !stk->size;
}

size_t stack_reserve(Stack* stk, size_t new_capacity) {
    assert(stk);

    STACK_LOG(stk, "Attempting to reserve capacity");
    STACK_VERIFY_RETURN(stk, 0);

    if (new_capacity < STACK_DEFAULT_CAPACITY) {
        new_capacity = STACK_DEFAULT_CAPACITY;
    }

    stk->min_capacity = new_capacity;
    stack_adjust(stk);

    STACK_REHASH(stk);

    STACK_LOG(stk, "Reserved capacity for %zu elements", new_capacity);

    return stk->min_capacity;
}

STACK_ERROR stack_get_error(Stack* stk) {
    assert(stk);

    STACK_LOG(stk, "Attempting to get error state");
    STACK_VERIFY(stk);
    STACK_LOG(stk, "Return error state");

    return stk->error;
}

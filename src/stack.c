#include "stack.h"

#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(USE_HASH_FAST) || defined(USE_HASH_FULL)
#define USE_HASH
#endif

#if defined(USE_CANARY) || defined(USE_DATA_CANARY)
typedef unsigned long long canary_type;
#define CANARY_VALUE 0xF072E3546BAD189Cull
#endif

#ifdef USE_HASH
typedef unsigned long long hash_type;
#define HASH_INITIAL_VALUE 0x600D4A54ull
#endif

enum { STACK_DEFAULT_CAPACITY = 10 };

static size_t stack_global_count = 0;
struct stack_t {
#ifdef USE_CANARY
    canary_type front_canary;
#endif

    void* data;
    size_t elem_sz;
    size_t size;
    size_t capacity;
    size_t min_capacity;
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

unsigned char get_poison(void const* p) {
    return (unsigned char) p;
}

bool is_poison(void const* p) {
    return *(unsigned char*) p == get_poison(p);
}

void write_poison(void* arr, size_t num) {
    assert(arr);

    for (unsigned char* p = arr; p < (unsigned char*) arr + num; p++) {
        *p = get_poison(p);
    }
}

bool verify_poison(void const* arr, size_t num) {
    assert(arr);

    for (unsigned char const* p = arr; p < (unsigned char const*) arr + num; p++) {
        if (!is_poison(p)) {
            return false;
        }
    }

    return true;
}

#ifdef USE_HASH
hash_type rotate_left(hash_type value) {
    return value << 1 | value >> (sizeof(value) * CHAR_BIT - 1);
}

#define ARR_LENGTH(arr) (sizeof(arr) / sizeof(*arr))

hash_type stack_metadata_hash(Stack const* stk) {
    assert(stk);

    const hash_type hash_parts[] = {(hash_type) stk->data, (hash_type) stk->elem_sz, (hash_type) stk->size, (hash_type) stk->capacity};

    hash_type hash_value = HASH_INITIAL_VALUE;
    for (size_t i = 0; i < ARR_LENGTH(hash_parts); i++) {
        hash_value = rotate_left(hash_value) ^ hash_parts[i];
    }
    return hash_value;
}

hash_type stack_data_hash(Stack const* stk) {
    assert(stk);

    hash_type hash_value = HASH_INITIAL_VALUE;
    if (!stk->data) {
        return hash_value;
    }
    for (size_t i = 0; i < stk->capacity * stk->elem_sz; i++) {
        hash_value = rotate_left(hash_value) ^ ((unsigned char const*) stk->data)[i];
    }
    return hash_value;
}
#endif

size_t elem_str_size(size_t stk_elem_sz) {
    return CHAR_BIT / 4 * stk_elem_sz + 2;
}

char* elem_to_str(void const* p, char* str, size_t sz, size_t str_sz) {
    assert(p);
    assert(sz);

    if (str_sz < elem_str_size(sz) + 1) {
        return NULL;
    }

    str[0] = '0';
    str[1] = 'x';
    // Prints in correct order only on little endian systems
    unsigned char const* elem_p = p;
    for (size_t i = 0; i < sz; i++) {
        sprintf(str + 2 * (i + 1), "%.*X", CHAR_BIT / 4, elem_p[sz - i - 1]);
    }
    str[str_sz - 1] = '\0';

    return str;
}

void stack_dump(Stack* stk, FILE* dump_file) {
    assert(stk);
    assert(dump_file);

    time_t tm = time(NULL);
    fprintf(dump_file, "%s"
                       "Dump for Stack at %p\n"
                       "Stack status is: %d: %s\n",
                       ctime(&tm),
                       (void const*) stk,
                       stk->error, stack_error_string(stk->error));

#if defined(USE_CANARY) || defined(USE_DATA_CANARY)
    const int canary_field_width = sizeof(canary_type) * CHAR_BIT / 4;
#endif
#ifdef USE_HASH
    const int hash_field_width = sizeof(hash_type) * CHAR_BIT / 4;
#endif

#ifdef USE_HASH
    fprintf(dump_file, "Stack stored metadata hash is: %.*llX\n"
                       "Stack actual metadata hash is: %.*llX\n",
                       hash_field_width, stk->metadata_hash,
                       hash_field_width, stack_metadata_hash(stk));
#endif
#ifdef USE_CANARY
    fprintf(dump_file, "Stack default canary value is: %.*llX\n"
                       "Stack front canary is:         %.*llX\n"
                       "Stack back canary is:          %.*llX\n",
                       canary_field_width, CANARY_VALUE,
                       canary_field_width, stk->front_canary,
                       canary_field_width, stk->back_canary);
#endif
    fprintf(dump_file, "Stack element size is     %zu\n"
                       "Stack minimum capacity is %zu\n"
                       "Stack size is             %zu\n"
                       "Stack capacity is         %zu\n"
                       "Stack data is at: %p\n",
                       stk->elem_sz,
                       stk->min_capacity,
                       stk->size,
                       stk->capacity,
                       stk->data);
    if (!stk->data) {
        return;
    }

#ifdef USE_HASH_FULL
    fprintf(dump_file, "Stack stored data hash is:          %.*llX\n"
                       "Stack actual data hash is:          %.*llX\n",
                       hash_field_width, stk->data_hash,
                       hash_field_width, stack_data_hash(stk));
#endif
#ifdef USE_DATA_CANARY
    fprintf(dump_file, "Stack default data canary value is: %.*llX\n"
                       "Stack data front canary is:         %.*llX\n"
                       "Stack data back canary is:          %.*llX\n",
                       canary_field_width, CANARY_VALUE,
                       canary_field_width, *((canary_type const*) stk->data - 1),
                       canary_field_width, *((canary_type const*) ((char const*) stk->data + stk->capacity * stk->elem_sz)));
#endif
    fprintf(dump_file, "Stack data is:\n");
    char const* data = stk->data;
    char elem_str[elem_str_size(stk->elem_sz) + 1];
    for (size_t i = 0; i < stk->capacity; i++) {
        if (!elem_to_str(data, elem_str, stk->elem_sz, sizeof(elem_str))) {
            fprintf(dump_file, "Error dumping Stack data\n");
            break;
        }
        if (i < stk->size) {
            fprintf(dump_file, "[%lu]: ", i);
        } else {
            fprintf(dump_file, "(%lu): ", i);
        }
        fprintf(dump_file, "%s", elem_str);
#ifdef USE_POISON
        if (verify_poison(data, stk->elem_sz)) {
            fprintf(dump_file, " (poison)");
        }
#endif
        fprintf(dump_file, "\n");
        data += stk->elem_sz;
    }
}

static FILE* stack_global_log = NULL;
#ifndef STACK_LOG_FILENAME
#define STACK_LOG_FILENAME "stack_log"
#endif

#ifdef USE_LOG
#include <stdarg.h>
void stack_log(Stack* stk, char const* format, ...) {
    assert(stk);
    assert(format);

    if (!stack_global_log) {
        return;
    }
    time_t tm = time(NULL);
    char* time_str = ctime(&tm);
    *(strchr(time_str, '\n')) = '\0';
    fprintf(stack_global_log, "%s: Stack at %p: ", time_str, (void const*) stk);

    va_list args;
    va_start(args, format);
    vfprintf(stack_global_log, format, args);
    va_end(args);
    fprintf(stack_global_log, "\n");
}
// ## before __VA_ARGS__ is a GCC/Clang/ICC extension
#define STACK_LOG(stk, format, ...) stack_log(stk, format, ##__VA_ARGS__)
#else
#define STACK_LOG(stk, format, ...)
#endif

bool stack_error_recoverable(STACK_ERROR err) {
    return err == STACK_OK || err == STACK_ALLOCATION_ERROR || err == STACK_OPERATION_ERROR;
}

bool stack_error_valid(STACK_ERROR err) {
    return err >= STACK_OK && err <= STACK_CORRUPTION_ERROR;
}

void stack_verify(Stack* stk) {
    assert(stk);

    STACK_LOG(stk, "Begin verification");

    if (!stk->data && stk->size > 0) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto error;
    }

    if (stk->size > stk->capacity) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto error;
    }

    if (stk->capacity < STACK_DEFAULT_CAPACITY) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto error;
    }

    if (!stk->elem_sz) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto error;
    }

    if (!stack_error_valid(stk->error)) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto error;
    }

#ifdef USE_HASH
    if (stk->metadata_hash != stack_metadata_hash(stk)) {
        stk->error = STACK_METADATA_HASH_ERROR;
        goto error;
    }
#ifdef USE_HASH_FULL
    if (stk->data_hash != stack_data_hash(stk)) {
        stk->error = STACK_DATA_HASH_ERROR;
        goto error;
    }
#endif
#endif

#ifdef USE_CANARY
    if (stk->front_canary != CANARY_VALUE || stk->back_canary != CANARY_VALUE) {
        stk->error = STACK_METADATA_CANARY_OVERWRITE_ERROR;
        goto error;
    }
#endif

#ifdef USE_DATA_CANARY
    const canary_type front_canary = *((canary_type const*) stk->data - 1);
    char const* back_p = (char const*) stk->data + stk->capacity * stk->elem_sz;
    const canary_type back_canary = *((canary_type const*) back_p);
    if (front_canary != CANARY_VALUE || back_canary != CANARY_VALUE) {
        stk->error = STACK_DATA_CANARY_OVERWRITE_ERROR;
        goto error;
    }
#endif

#ifdef USE_POISON
    size_t start_i = stk->size * stk->elem_sz;
    size_t num = (stk->capacity - stk->size) * stk->elem_sz;
    if (!verify_poison((char const*) stk->data + start_i, num)) {
        stk->error = STACK_POISON_OVERWRITE_ERROR;
        goto error;
    }
#endif

    STACK_LOG(stk, "Verification successful");

    return;

error:
    STACK_LOG(stk, "Verification failed");
    if (stack_global_log) {
        stack_dump(stk, stack_global_log);
    }
}
#define STACK_VERIFY(stk) stack_verify(stk)
#define STACK_VERIFY_RETURN(stk, val)           \
    STACK_VERIFY(stk);                          \
    if (!stack_error_recoverable(stk->error)) { \
        return val;                             \
    }

char const* stack_error_string(STACK_ERROR error) {
    switch (error) {
        case STACK_OK:
            return "No error";
        case STACK_ALLOCATION_ERROR:
            return "Internal memory allocation error";
        case STACK_OPERATION_ERROR:
            return "Invalid stack operation was requested by user";

#ifdef USE_CANARY
        case STACK_METADATA_CANARY_OVERWRITE_ERROR:
            return "One of the stack's metadata canaries have been overwritten. "
                   "Metadata is probably corrupted";
#endif

#ifdef USE_HASH
        case STACK_METADATA_HASH_ERROR:
            return "Stack metadata hash differs from the one stored. "
                   "Metadata is probably corrupted";
#endif

#ifdef USE_DATA_CANARY
        case STACK_DATA_CANARY_OVERWRITE_ERROR:
            return "One of the stack's data canaries have been overwritten. "
                   "Data is possibly corrupted";
#endif

#ifdef USE_HASH_FULL
        case STACK_DATA_HASH_ERROR:
            return "Stack data hash differs from the one stored. "
                   "Data is possibly corrupted";
#endif

#ifdef USE_POISON
        case STACK_POISON_OVERWRITE_ERROR:
            return "Stack unused data memory has been overwritten. "
                   "Data is possibly corrupted";
#endif

        case STACK_CORRUPTION_ERROR:
            return "Stack memory has been corrupted";
        default:
            return "Unknown error";
    }
}

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

    *((canary_type*) stk->data - 1) = CANARY_VALUE;
    char* back_p = (char*) stk->data + stk->capacity * stk->elem_sz;
    *((canary_type*) back_p) = CANARY_VALUE;

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

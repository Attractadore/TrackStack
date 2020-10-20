#pragma once
#include "stack_def.c"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>

unsigned char get_poison(size_t i) {
    return i;
}

// \brief Fill array with poison
//
// \param[in, out] p Pointer to the array's first element
// \param[in] start_i The starting key to use for poison generation
// \param[in] num The number of bytes to fill with poison
void write_poison(void* p, size_t start_i, size_t num) {
    assert(p);

    unsigned char* start = p;
    for (size_t i = 0; i < num; i++) {
        start[i] = get_poison(start_i + i);
    }
}

bool is_poison(unsigned char b, size_t i) {
    return b == get_poison(i);
}

// \brief Check if an array is filled with poison
//
// \param[in] p Pointer to the array's first element
// \param[in] start_i The starting key used for poison generation
// \param[in] num The number of bytes to check for poison
bool verify_poison(void const* p, size_t start_i, size_t num) {
    assert(p);

    unsigned char const* start = p;
    for (size_t i = 0; i < num; i++) {
        if (!is_poison(start[i], start_i + i)) {
            return false;
        }
    }

    return true;
}

#ifdef USE_HASH
hash_type stack_metadata_hash(Stack const* stk) {
    assert(stk);

    hash_type hash_value = HASH_INITIAL_VALUE ^
                           (hash_type) stk->data ^
                           (hash_type) stk->elem_sz ^
                           (hash_type) stk->size ^
                           (hash_type) stk->capacity;
    return hash_value;
}

hash_type stack_data_hash(Stack const* stk) {
    assert(stk);
    assert(stk->data);

    hash_type hash_value = HASH_INITIAL_VALUE;
    for (size_t i = 0; i < stk->capacity * stk->elem_sz; i++) {
        hash_value ^= ((unsigned char const*) stk->data)[i];
    }
    return hash_value;
}
#endif

void dump_elem(FILE* file, void const* data_p, size_t offset, size_t sz) {
    assert(file);
    assert(data_p);
    assert(sz);

    unsigned char const* elem_p = (unsigned char const*) data_p + offset;

    // Prints in correct order only on little endian systems
    fprintf(file, "0x");
    for (size_t i = 0; i < sz; i++) {
        fprintf(file, "%.*X", CHAR_BIT / 4, elem_p[sz - i - 1]);
    }

#ifdef USE_POISON
    if (verify_poison(elem_p, offset, sz)) {
        fprintf(file, " (poison)");
    }
#endif

    fprintf(file, "\n");
}

void stack_dump(Stack* stk, int line, char const* func_name, char const* file_name, char const* log_name) {
    assert(stk);

    static FILE* log_file = NULL;
    if (!log_file) {
        log_file = fopen(log_name, "ab");
        setvbuf(log_file, NULL, _IONBF, 0);
        if (!log_file) {
            return;
        }
        fprintf(log_file, "Starting log\n\n");
    }

    time_t tm = time(NULL);
    fprintf(log_file, "%s", ctime(&tm));
    fprintf(log_file, "stack dump called from line %d from %s from file %s\n", line, func_name, file_name);
    fprintf(log_file, "stack status is: (%d) \"%s\"\n", stk->error, stack_error_string(stk->error));

#ifdef USE_HASH
    fprintf(log_file, "stack metadata hash is: %llX\n", stk->metadata_hash);

#ifdef USE_HASH_FULL
    fprintf(log_file, "stack data hash is: %llX\n", stk->data_hash);
#endif

#endif

#ifdef USE_CANARY
    fprintf(log_file, "stack default canary value is: %llX\n", CANARY_VALUE);
    fprintf(log_file, "stack front canary is: %llX\n", stk->front_canary);
    fprintf(log_file, "stack back canary is: %llX\n", stk->back_canary);
#endif

#ifdef USE_DATA_CANARY
    fprintf(log_file, "stack default data canary value is: %llX\n", CANARY_VALUE);
    fprintf(log_file, "stack data front canary is: %llX\n",
            *((canary_type const*) stk->data - 1));
    fprintf(log_file, "stack data back canary is: %llX\n",
            *((canary_type const*) ((char const*) stk->data + stk->capacity * stk->elem_sz)));
#endif

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
        dump_elem(log_file, stk->data, i * stk->elem_sz, stk->elem_sz);
    }
    fprintf(log_file, "\n");
}

#ifdef USE_LOG
#ifndef STACK_LOG_FILE
#define STACK_LOG_FILE "stack_log"
#endif
#define STACK_DUMP(stk, line, func_name, file_name) stack_dump(stk, line, func_name, file_name, STACK_LOG_FILE)
#else
#define STACK_DUMP(stk, line, func_name, file_name)
#endif

bool stack_error_recoverable(STACK_ERROR err) {
    return err == STACK_OK || err == STACK_ALLOCATION_ERROR;
}

bool stack_error_valid(STACK_ERROR err) {
    return err >= STACK_OK && err <= STACK_CORRUPTION_ERROR;
}

void stack_verify(Stack* stk, int line, char const* func_name, char const* file_name) {
    assert(stk);

    if (!stk->data) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto finish;
    }

    if (stk->size > stk->capacity) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto finish;
    }

    if (stk->capacity < STACK_DEFAULT_CAPACITY) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto finish;
    }

    if (!stk->elem_sz) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto finish;
    }

    if (!stack_error_valid(stk->error)) {
        stk->error = STACK_CORRUPTION_ERROR;
        goto finish;
    }

#ifdef USE_HASH
    if (stk->metadata_hash != stack_metadata_hash(stk)) {
        stk->error = STACK_METADATA_HASH_ERROR;
        goto finish;
    }
#ifdef USE_HASH_FULL
    if (stk->data_hash != stack_data_hash(stk)) {
        stk->error = STACK_DATA_HASH_ERROR;
        goto finish;
    }
#endif
#endif

#ifdef USE_CANARY
    if (stk->front_canary != CANARY_VALUE || stk->back_canary != CANARY_VALUE) {
        stk->error = STACK_METADATA_CANARY_OVERWRITE_ERROR;
        goto finish;
    }
#endif

#ifdef USE_DATA_CANARY
    const canary_type temp_canary = CANARY_VALUE;
    if (memcmp((char const*) stk->data - sizeof(temp_canary), &temp_canary, sizeof(temp_canary)) != 0 ||
        memcmp((char const*) stk->data + stk->capacity * stk->elem_sz, &temp_canary, sizeof(temp_canary)) != 0) {
        stk->error = STACK_DATA_CANARY_OVERWRITE_ERROR;
        goto finish;
    }
#endif

#ifdef USE_POISON
    size_t start_i = stk->size * stk->elem_sz;
    size_t num = (stk->capacity - stk->size) * stk->elem_sz;
    if (!verify_poison((char const*) stk->data + start_i, start_i, num)) {
        stk->error = STACK_POISON_OVERWRITE_ERROR;
        goto finish;
    }
#endif

finish:
    STACK_DUMP(stk, line, func_name, file_name);
}
#define STACK_VERIFY(stk) stack_verify(stk, __LINE__, __func__, __FILE__)
#define STACK_VERIFY_RETURN(stk, val) \
    STACK_VERIFY(stk); \
    if (!stack_error_recoverable(stk->error)) { \
        return val; \
    } \

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

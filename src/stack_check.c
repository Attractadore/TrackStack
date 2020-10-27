#pragma once
#include "stack_def.c"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

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

hash_type stack_metadata_hash(Stack const* stk) {
    assert(stk);

    hash_type hash_value = HASH_INITIAL_VALUE;
    hash_type hash_parts[] = {(hash_type) stk->data, (hash_type) stk->elem_sz, (hash_type) stk->size, (hash_type) stk->capacity};
    for (size_t i = 0; i < sizeof(hash_parts) / sizeof(*hash_parts); i++) {
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
    fprintf(dump_file, "%s", ctime(&tm));
    fprintf(dump_file, "Dump for Stack at %p\n", (void*) stk);
    fprintf(dump_file, "Stack status is: %d: %s\n", stk->error, stack_error_string(stk->error));

#if defined(USE_CANARY) || defined(USE_DATA_CANARY)
    const int canary_field_width = sizeof(canary_type) * CHAR_BIT / 4;
#endif
#ifdef USE_HASH
    const int hash_field_width = sizeof(hash_type) * CHAR_BIT / 4;
#endif

#ifdef USE_HASH
    fprintf(dump_file, "Stack stored metadata hash is: %.*llX\n", hash_field_width, stk->metadata_hash);
    fprintf(dump_file, "Stack actual metadata hash is: %.*llX\n", hash_field_width, stack_metadata_hash(stk));
#endif
#ifdef USE_CANARY
    fprintf(dump_file, "Stack default canary value is: %.*llX\n", canary_field_width, CANARY_VALUE);
    fprintf(dump_file, "Stack front canary is:         %.*llX\n", canary_field_width, stk->front_canary);
    fprintf(dump_file, "Stack back canary is:          %.*llX\n", canary_field_width, stk->back_canary);
#endif
    fprintf(dump_file, "Stack element size is     %zu\n"
                       "Stack minimum capacity is %zu\n"
                       "Stack size is             %zu\n"
                       "Stack capacity is         %zu\n",
            stk->elem_sz, stk->min_capacity, stk->size, stk->capacity);
    fprintf(dump_file, "Stack data is at: %p\n", stk->data);
    if (!stk->data) {
        return;
    }

#ifdef USE_HASH_FULL
    fprintf(dump_file, "Stack stored data hash is:          %.*llX\n", hash_field_width, stk->data_hash);
    fprintf(dump_file, "Stack actual data hash is:          %.*llX\n", hash_field_width, stack_data_hash(stk));
#endif
#ifdef USE_DATA_CANARY
    fprintf(dump_file, "Stack default data canary value is: %.*llX\n", canary_field_width, CANARY_VALUE);
    fprintf(dump_file, "Stack data front canary is:         %.*llX\n", canary_field_width,
            *((canary_type const*) stk->data - 1));
    fprintf(dump_file, "Stack data back canary is:          %.*llX\n", canary_field_width,
            *((canary_type const*) ((char const*) stk->data + stk->capacity * stk->elem_sz)));
#endif
    fprintf(dump_file, "Stack data is:\n");
    char const* data = stk->data;
    char elem_str[elem_str_size(stk->elem_sz) + 1];
    for (size_t i = 0; i < stk->capacity; i++) {
        if (i < stk->size) {
            fprintf(dump_file, "[%lu]: ", i);
        } else {
            fprintf(dump_file, "(%lu): ", i);
        }
        if (!elem_to_str(data, elem_str, stk->elem_sz, sizeof(elem_str))) {
            fprintf(dump_file, "Error dumping Stack data\n");
            break;
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
    fprintf(stack_global_log, "%s: ", time_str);

    fprintf(stack_global_log, "Stack at %p: ", (void const*) stk);
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
    const canary_type temp_canary = CANARY_VALUE;
    if (memcmp((char const*) stk->data - sizeof(temp_canary), &temp_canary, sizeof(temp_canary)) != 0 ||
        memcmp((char const*) stk->data + stk->capacity * stk->elem_sz, &temp_canary, sizeof(temp_canary)) != 0) {
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

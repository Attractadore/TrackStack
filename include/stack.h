/**
 * \file stack.h This header defines a generic stack data structure
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef enum stack_error_e {
    STACK_OK,
    STACK_ALLOCATION_ERROR,
    STACK_OPERATION_ERROR,
    STACK_METADATA_CANARY_OVERWRITE_ERROR,
    STACK_METADATA_HASH_ERROR,
    STACK_DATA_CANARY_OVERWRITE_ERROR,
    STACK_DATA_HASH_ERROR,
    STACK_POISON_OVERWRITE_ERROR,
    STACK_CORRUPTION_ERROR,
} STACK_ERROR;

typedef struct stack_t Stack;

/**
 * \brief Get list of protection features that this implementation was compiled with
 *
 * \return String describing protection features that this implementation was compiled with
 */
char const* get_stack_compilation_options();

/**
 * \brief Allocate a new Stack
 *
 * \param[in] stk_elem_sz The size of the type of element this Stack will store
 *
 * \return Pointer to new Stack, or NULL if an error occured
 *
 * \remark Free the returned pointer by calling #stack_free
 */
Stack* stack_allocate(size_t stk_elem_sz);

/**
 * \brief Push a new element to a Stack
 *
 * \param[in] stk The Stack to push to
 * \param[in] elem_p Pointer to the element to push
 *
 * \return elem_p if the element was succefully pushed to the stack, NULL otherwise
 */
void const* stack_push(Stack* stk, void const* elem_p);

/**
 * \brief Pop an element from a
 * Stack
 *
 * \param[in] stk The Stack to pop from
 * \param[in] elem_p Pointer to the element to store the result in
 *
 * \return elem_p if the element was succefully poped from the stack, NULL otherwise
 *
 * \remark Poping from an empty Stack will result in an error
 */
void* stack_pop(Stack* stk, void* elem_p);

/**
 * \brief Peek at a Stack's top element
 *
 * \param[in] stk The Stack to peek into
 * \param[in] elem_p Pointer to the element to store the result in
 *
 * \return elem_p if the element was succefully read from the stack, NULL otherwise
 *
 * \remark Peeking at the top element of an empty Stack will result in an error
 */
void* stack_top(Stack* stk, void* elem_p);

/**
 * \brief Get the number of elements currently stored in a Stack
 *
 * \param[in] stk The Stack whose size to query
 *
 * \return The Stack's size or 0 if an error occured.
 */
size_t stack_size(Stack* stk);

/**
 * \brief Get the number of elements in a Stack that memory is currently reserved for 
 *
 * \param[in] stk The Stack whose capacity to query
 *
 * \return The Stack's capacity or 0 if an error occured.
 */
size_t stack_capacity(Stack* stk);

/**
 * \brief Check if a Stack has no elements in it
 *
 * \param[in] stk The Stack to check
 *
 * \return false if this Stack's size is greater than 0, true is this Stack is empty or an error occured
 */
bool stack_empty(Stack* stk);

/**
 * \brief Set a Stack's internal minimum capacity
 *
 * \param[in] stk The Stack whose minimum capacity to set
 * \param[in] capacity The new minimum capacity
 *
 * \return The Stack's new minimum capacity, 0 if an error occured
 *
 * \remark The Stack's minimum capacity will never be less than an 
 *         implementation defined minimum
 */
size_t stack_reserve(Stack* stk, size_t capacity);

/**
 * \brief Dump all of a Stack's contents into a file in human-readable form
 *
 * \param[in] stk The Stack whose contents to dump
 * \param[in] dump_file The file to dump into
 */
void stack_dump(Stack* stk, FILE* dump_file);

/** 
 * \brief Free a Stack allocated by #allocate_stack
 *
 * \param[in] stk The Stack to free
 *
 * \remark This function accepts NULL
 */
void stack_free(Stack* stk);

/**
 * \brief Get a Stack's current error code
 *
 * \param[in] stk The Stack whose error code to query
 *
 * \return Error code describing the Stack's current state
 *
 * \remark Call #stack_error_string with the returned code to get more information
 */
STACK_ERROR stack_get_error(Stack* stk);

/**
 * \brief Get a human-readable representation of Stack error code
 *
 * \param[in] error The error code to get the message for
 *
 * \return String describing the error code
 */
char const* stack_error_string(STACK_ERROR error);

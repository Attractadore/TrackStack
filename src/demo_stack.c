
#define STACK_ELEM_TYPE int
#include "stack_generic.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define ARR_LENGTH(arr) (sizeof(arr) / sizeof(*arr))
enum { LINE_BUFFER_SIZE = 1u << 10 };

char const* const commands[] = {
    "help",
    "exit",
    "push",
    "pop",
    "top",
    "size",
    "empty",
    "capacity",
    "reserve",
    "dump",
    "error",
};

char const* const commands_args[] = {
    "",
    "",
    " (item)",
    "",
    "",
    "",
    "",
    "",
    " (capacity)",
    "",
    "",
};

char const* const commands_help[] = {
    "print this message",
    "quit this program",
    "push item to stack",
    "pop item from stack",
    "peek at top item",
    "get stack size",
    "check if stack is empty",
    "get stack capacity",
    "reserve stack capacity",
    "print stack information",
    "print stack error state",
};

void print_help() {
    for (size_t i = 0; i < ARR_LENGTH(commands); i++) {
        printf("%s%s: %s\n", commands[i], commands_args[i], commands_help[i]);
    }
}

int get_command_id(char const* command) {
    assert(command);

    for (size_t i = 0; i < ARR_LENGTH(commands); i++) {
        if (strcmp(command, commands[i]) == 0) {
            return i;
        }
    }
    return -1;
}

size_t get_command_buffer_size() {
    size_t max_command_length = 0;
    for (size_t i = 0; i < ARR_LENGTH(commands); i++) {
        size_t command_length = strlen(commands[i]);
        if (command_length > max_command_length) {
            max_command_length = command_length;
        }
    }
    // Reserve one extra char to prevent matching words that a command if a prefix of
    return max_command_length + 2;
}

size_t read_command(char const* line, char* command, size_t command_size) {
    assert(line);
    assert(command);

    memcpy(command, line, command_size);
    command[command_size - 1] = '\0';
    char* p = strchr(command, ' ');
    if (p) {
        *p = '\0';
    }
    return strlen(command);
}

void handle_input(char const* line, char const* command, Stack_int* stk, bool* bExit_p) {
    assert(line);
    assert(command);
    assert(stk);
    assert(bExit_p);

    int command_id = get_command_id(command);
    switch (command_id) {
        case 0:  // help
            print_help();
            break;
        case 1:  // exit
            *bExit_p = true;
            break;
        case 2:  // push
        {
            int v = 0;
            if (sscanf(line, "%d", &v) != 1) {
                printf("Sorry, %s takes an int as an argument\n", command);
                break;
            }
            StackPush_int(stk, v);
            if (StackGetError_int(stk) != STACK_OK) {
                printf("Failed to push %d to stack\n", v);
            }
            else {
                printf("Pushed %d to stack\n", v);
            }
        } break;
        case 3:  // pop
        {
            int v = StackPop_int(stk);
            if (StackGetError_int(stk) != STACK_OK) {
                printf("Failed to pop from stack\n");
            } else {
                printf("Popped %d from stack\n", v);
            }
        } break;
        case 4:  // top
        {
            int v = StackTop_int(stk);
            if (StackGetError_int(stk) != STACK_OK) {
                printf("Failed to peek at stack's top\n");
            } else {
                printf("Stack top is %d\n", v);
            }
        } break;
        case 5:  // size
            printf("Stack size is %zu\n", StackSize_int(stk));
            break;
        case 6:  // empty
            if (StackEmpty_int(stk)) {
                printf("Stack is empty\n");
            } else {
                printf("Stack is not empty\n");
            }
            break;
        case 7:  // capacity
            printf("Stack capacity is %zu\n", StackCapacity_int(stk));
            break;
        case 8:  // reserve
        {
            size_t amount = 0;
            if (sscanf(line, "%zu", &amount) != 1) {
                printf("Sorry, %s takes size_t as an argument\n", command);
                break;
            }
            if ((amount = StackReserve_int(stk, amount))) {
                printf("Reserved capacity for %zu elements\n", amount);
            } else {
                printf("Failed to reserve capacity for %zu elements\n", amount);
            }
        } break;
        case 9:  // dump
            StackDump_int(stk, stdout);
            break;
        case 10:  // error
            printf("%s\n", StackErrorString_int(StackGetError_int(stk)));
            break;
        default:
            printf("Sorry, unrecognised command\n");
    }
}

int main() {
    Stack_int* stk = StackAllocate_int();
    if (!stk) {
        return 1;
    }
    printf("%s\n", get_stack_compilation_options());
    print_help();

    char line[LINE_BUFFER_SIZE];
    size_t command_size = get_command_buffer_size();
    char command[command_size];

    bool bExit = false;
    char const* prompt = ">>> ";
    while (!bExit && printf("%s", prompt) && fgets(line, ARR_LENGTH(line), stdin)) {
        char* p = strchr(line, '\n');
        if (p) {
            *p = '\0';
        }
        size_t command_length = read_command(line, command, command_size);
        handle_input(line + command_length, command, stk, &bExit);
    }
    StackFree_int(stk);
    return 0;
}

#include "stack.h"

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
    memcpy(command, line, command_size);
    command[command_size - 1] = '\0';
    char* p = strchr(command, ' ');
    if (p) {
        *p = '\0';
    }
    return strlen(command);
}

void handle_input(char const* line, char const* command, Stack* stk, bool* bExit_p) {
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
            if (stack_push(stk, &v)) {
                printf("Pushed %d to stack\n", v);
            } else {
                printf("Failed to push %d to stack\n", v);
            }
        } break;
        case 3:  // pop
        {
            int v = 0;
            if (stack_pop(stk, &v)) {
                printf("Popped %d from stack\n", v);
            } else {
                printf("Failed to pop from stack\n");
            }
        } break;
        case 4:  // top
        {
            int v = 0;
            if (stack_top(stk, &v)) {
                printf("Stack top is %d\n", v);
            } else {
                printf("Failed to peek at stack's top\n");
            }
        } break;
        case 5:  // size
            printf("Stack size is %zu\n", stack_size(stk));
            break;
        case 6:  // empty
            if (stack_empty(stk)) {
                printf("Stack is empty\n");
            } else {
                printf("Stack is not empty\n");
            }
            break;
        case 7:  // capacity
            printf("Stack capacity is %zu\n", stack_capacity(stk));
            break;
        case 8:  // reserve
        {
            size_t amount = 0;
            if (sscanf(line, "%zu", &amount) != 1) {
                printf("Sorry, %s takes size_t as an argument\n", command);
                break;
            }
            if ((amount = stack_reserve(stk, amount))) {
                printf("Reserved capacity for %zu elements\n", amount);
            } else {
                printf("Failed to reserve capacity for %zu elements\n", amount);
            }
        } break;
        case 9:  // dump
            stack_dump(stk, stdout);
            break;
        case 10:  // error
            printf("%s\n", stack_error_string(stack_get_error(stk)));
            break;
        default:
            printf("Sorry, unrecognised command\n");
    }
}

int main() {
    Stack* int_stack = stack_allocate(sizeof(int));
    if (!int_stack) {
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
        handle_input(line + command_length, command, int_stack, &bExit);
    }
    stack_free(int_stack);
    return 0;
}

# About this reposity

This repository implements the stack data structure.
This stack does it's best to minimize the damage it can do after a buffer overflow or
other corruption.

# Building

[CMake](https://cmake.org/) is used as this projects build system. To build the project,
run the following from the root directory.

```
mkdir build
cd build
cmake ..
cmake --build .
```

You will now find two exectutables in the build directory (`StackDemo` and `StackStress`) and one shared
library (`StackLib`).

# Running the tests

`StackStress` tests `StackLib` by creating a `Stack` and then repeatedly poping from it and pushing to it.
If `StackStress` runs to completion with no assertion failures,
`StackLib` can be used for standard stack functionality.

# Running the demo

`StackDemo` allows you to play around with an interactive `Stack` that stores ints.

# Documentation

This project's documentation can be accessed [here](https://attractadore.github.io/TrackStackDocs).
If you want to generate the documentation yourself, you will need [doxygen](https://www.doxygen.nl).
Once you have doxygen run the following in the repository's root directory:

```
doxygen Doxyfile
```

The generated documentation can now be found in the docs/ folder.

# Stack protection features

## Enabling protection features

Each of the protection features described below can be enabled or disabled by adding or removing its symbolic parameter
from `StackLib's` `target_compile_definitions` in `CMakeLists.txt`.

## Metadata canaries

When this option is turned on, the `Stack's` a canary value will be inserted before and after a `Stack's` representation in memory.
If either of them gets altered, the `Stack` will refuse to perform any further operations.
To turn metadata canaries on, define `USE_CANARY`.

## Data canaries
When this option is turned on, a canary value will be inserted before and after a `Stack's` data array.
If either of them gets altered, the `Stack` will refuse to perform any further operation.
To turn data canaries on, define `USE_DATA`.

## Metadata hashing
When this option is turned on, a `Stack's` metadata will be hashed and the result stored.
If any of the `Stack's` fields are tampered with, the hash computed during the verification process
will not match the one stored,
and the `Stack` will refuse to perform any further operation.
To turn metadata hashing on, define `USE_HASH_FAST`.

## Data hashing
When this option is turned on, a `Stack's` data will be hashed and the result stored.
If the `Stack's` data is tampered with, the hash computed during the verification process
will not match the one stored,
and the `Stack` will refuse to perform any further operation.
To turn data hashing on, define `USE_HASH_FULL`.
Turning on data hashing also turns on metadata hashing.
Data hashing has a very high cost.

## Data poisoning
When this option is turned on, memory that has been reserved for a `Stack's` elements but not yet
used will be filled with certain values. If during the verification process
an unused byte's value doesn't match its corresponding poison value,
the `Stack` will refuse to perform any further operation.
To turn data poisoning on, define `USE_POISON`.
Data poisoning has a moderate cost.

## Event logging
When this option is turned on, the `Stack` will log what is happening to it.
To turn logging on, define `USE_LOG`. The default log file name is `stack_log`.
To change it, define `STACK_LOG_FILENAME=filename` where `filename` is the desired log file name.
Logging can slow down other `Stack` operations.
The generated log files can quickly start taking up a lot of disk space.

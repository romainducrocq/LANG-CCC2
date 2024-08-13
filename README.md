# wheelcc - the C compiler reinventing the wheel
> **THIS COMPILER IS A WORK IN PROGRESS! ANYTHING CAN CHANGE AT ANY MOMENT WITHOUT ANY NOTICE! USE THIS COMPILER AT YOUR OWN RISK!**

*__<ins>Reinventing the wheel</ins>__ (idiom): "Waste a great deal of time or effort in creating something that already exists."*
<!---->
A small, self-contained C compiler written from scratch in C++ for x86-64 GNU/Linux platforms. 

## Overview

The wheelcc compiler supports a large subset of C17 ([International Standard ISO/IEC 9899:2018](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n2310.pdf)), for which it has it's own built-in preprocessor, frontend, IR and backend. It emits x86-64 AT&T assembly for GNU/Linux, which is then linked with gcc/ld. The project is entirely written in C++17, and builds to a standalone executable plus a driver in bash. wheelcc is overall designed after Nora Sandler's [Writing a C Compiler](https://nostarch.com/writing-c-compiler), and was tested against it's [test suite](https://github.com/nlsandler/writing-a-c-compiler-tests).

## Usage

### Install

Get the repo, cd to the bin directory  
```
$ git clone --depth 1 --branch master https://github.com/romainducrocq/wheelcc.git
$ cd wheelcc/bin/
```
Configure the repo and install the build/runtime dependencies: `gcc g++ make cmake`  
```
$ ./configure.sh
```
Build the compiler in Release mode  
```
$ ./make.sh
```
Install the `wheelcc` command system-wide (creates a symlink to the driver in `/usr/local/bin/`). As an alternative, do not install and use `wheelcc/bin/driver.sh` instead  
```
$ ./install.sh
$ . ~/.bashrc
```

### Use

With file `main.c`  
```c
int puts(char* c);

int main(void) {
    puts("Hello, world!");
    return 0;
}
```

Compile and run  
```
$ wheelcc main.c
$ ./main
Hello, World!
```

Usage 
> **Note**: Except for one source file to compile, all other command-line arguments are optional.  
However, <ins>the order of arguments passed matters</ins>: they are parsed only in this order, any other order will fail!  
```
$ wheelcc --help
Usage: wheelcc [Help] [Debug] [Preprocess] [Include] [Link] [Linkdir] [Linklib] [Output] FILES

[Help]:
    --help          print help and exit

[Debug]:
    -v              enable verbose mode
    (Test/Debug build only):
    --lex           print  lexing    stage and exit
    --parse         print  parsing   stage and exit
    --validate      print  semantic  stage and exit
    --tacky         print  interm    stage and exit
    --codegen       print  assembly  stage and exit
    --codeemit      print  emission  stage and exit

[Preprocess]:
    -E              enable macro expansion with gcc

[Include]:
    -I<includedir>  add a list of paths to include path

[Link]:
    -S              compile, but do not assemble and link
    -c              compile and assemble, but do not link

[Linkdir]:
    -L<linkdir>     add a list of paths to link path

[Linklib]:
    -l<libname>     link with a list of library files

[Output]:
    -o <file>       write the output into <file>

FILES:              list of .c files to compile
```

### Test

cd to the test directory, install the testtime dependencies: `diffutils valgrind`
```
$ cd wheelcc/test/
$ ./get-dependencies.sh
```

Test the compiler
```
$ ./test-compiler.sh
```

Test the preprocessor
```
$ ./test-preprocessor.sh
```

Test memory leaks
```
$ ./test-memory.sh
```

wheelcc is frequently tested on these distributions (x86-64)
Debian GNU/Linux  | Ubuntu           | Rocky Linux      | Arch Linux
:---:             |:---:             |:---:             |:---:
:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:|:heavy_check_mark:

## Implementation details

### Preprocessor

wheelcc has a minimal built-in preprocessor that supports `include` header directives and comments (singleline and multiline). By default, included files are searched in the same directory as the source file currently being compiled, but other directories to search for can be added to the include path with the `-I` option. Other directives, like pragmas, are ignored and stripped out. The preprocessor does not natively support macros, but macro expansion can be enabled with the `-E` command-line option, which falls back on preprocessing with gcc.

### Compiler

TODO

### Optimization

(TBD)

### Linker

wheelcc has no built-in linker: gcc/ld is used to link the assembly outputed by the compiler. It complies with the System-V ABI, such that libraries already compiled with gcc or other compilers can be linked with the `-L` and `-l` command-line options and used at runtime in a program compiled by wheelcc. This also allows to link the C standard library APIs which signatures are compatible with the current implementation of wheelcc.  
(TBD, it is planned to support fasm as an alternative linker to produce very small executables.)

### Standard Library

(TBD, an experimental standard library is planned in the future, with at least support for the compiler tests.)

### Dependencies

wheelcc is mostly self-contained and aims to be as less bloated as possible. It depends just on the C and C++ standard libraries and header-only dependencies ([boost::regex](https://github.com/boostorg/regex),  [tinydir](https://github.com/cxong/tinydir)). The build/runtime only requires bash, gcc/g++ and cmake, which makes the compiler easy to build and use on any x86-64 GNU/Linux platform.

### Coding Style

wheelcc is implemented entirely with a restricted subset of C++17. The code follows a C-style procedural design where state context data is grouped into structures, and modified by local functions in a single corresponding translation unit. Each translation unit mainly represents a separate compilation stage. Thus, it can be thought of as mostly C plus some C\++ sugar for:  
- smart pointers with reference counting to manage the lifetime of AST datatypes.  
- single inheritance and polymorphism to emulate pattern matching over algebraic datatype.  
- standard containers, collections, string manipulation and move semantics.  

Very few other C++ features are used, and only when doing so provides a real advantage. This is to make the code cleaner without adding too much complexity.   

### Limitations

wheelcc supports a large subset of the C17 language, but many features of the language are not implemented. These include but are not limited to: increment and decrement operators, switch statements, union and enum data structures, variable-length arrays, const types, typedefs, function pointers, non-ascii characters, and float, short, auto, volatile, inline, register and restrict keywords. Any of these may or may not be implemented in the future. As such, wheelcc can not compile the C standard library and is not intended to be used as a production C compiler.

## C Language Reference

TODO

## Development

### Language features

- [x] 1\. Integer constants
- [x] 2\. Unary operators
- [x] 3\. Binary operators
- [x] 4\. Logical and relational operators
- [x] 5\. Local variables
- [x] 6\. Statements and conditional expressions
- [x] 7\. Compound statements
- [x] 8\. Loops
- [x] 9\. Functions
- [x] 10\. File-scope variables and storage-class specifiers

### Types

- [x] 11\. Long integers
- [x] 12\. Unsigned integers
- [x] 13\. Floating-point numbers
- [x] 14\. Pointers
- [x] 15\. Arrays and pointer arithmetic
- [x] 16\. Characters and strings
- [x] 17\. Supporting dynamic memory allocation
- [x] 18\. Structures

### Optimization

- [ ] 19\. Optimizing TAC programs
- [ ] 20\. Register allocation

****

@romainducrocq


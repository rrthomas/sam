# The SAM Virtual Machine

by Reuben Thomas


## Introduction

SAM is a simple virtual machine designed to teach some elements of low-level programming. It is programmed with an assembly language written as YAML.

SAM is self-contained, and performs I/O via the `TRAP` instruction, which provides access to I/O and possibly implementation-dependent facilities.


## Architecture

SAM is an array-based VM. Its memory is a single array of items, each of which is a word-sized VM instruction, or a reference to an array or other blob. It has a handful of registers. At any given time, computation is performed on a particular array, called the “stack”.


### Registers

The registers are as follows:

| Register  | Function  |
| --------- | --------- |
| `S0`      | The array which is the current `S`tack. |
| `IR`      | The `I`nstruction `R`egister holds the currently-executing instruction. |
| `P0 `     | The array from which code is being executed. |
| `PC`      | The `P`rogram `C`ounter points to the next instruction. |

All of the registers are word-sized.


### Arrays

Arrays are a series of VM instructions encoded as 8-byte words.

Arrays are addressed with signed integers.

A positive address `n` refers to the `n`th item, starting from zero.

A negative address –`n` refers to the `n`th item, counting from 1 at the end of the array.

A valid address is one that points to a valid instruction.


### Operation

Before SAM is started, the stack should be given suitable contents, and `PC` set to the address of some item in the stack.

```
begin
  load the stack item at `PC` into `IR`
  increment `PC` to point to the next stack item
  execute the instruction in `IR`
repeat
```

## Termination and errors

On error, SAM will return an error code to its environment.

The following table lists the errors and the conditions under which they are raised. Some further specific error conditions are documented with the instructions that raise them.

| Code  | Meaning  |
| ----- | -------- |
| `OK` | No error (returned by `HALT`). |
| `INVALID_OPCODE` | An attempt was made to execute an invalid opcode. |
| `INVALID_ADDRESS` | Invalid address. |
| `ARRAY_UNDERFLOW` | An attempt was made to access before the start of the given array. |
| `ARRAY_OVERFLOW` | An attempt was made to access after the end of the given array. |
| `WRONG_TYPE` | A array item’s type is incorrect for the current operation. |
| `INVALID_TRAP` | An invalid function number was given to `TRAP`. |


## Instruction set

The instruction set is listed below, with the instructions grouped according to function. The instructions are given in the following format:

> `NAME`  
> `before` → `after`
>
> Description.

**Stack comments** are written `before` → `after`, where `before` and `after` are stack pictures showing the items on top of the stack before and after the instruction is executed (the change is called the **stack effect**). An instruction only affects the items shown in its stack comments. **Stack pictures** are a representation of the top-most items on the stack, and are written `i₁` `i₂`…`iₙ₋₁` `iₙ` where the `iₖ` are stack items, with `iₙ` being on top of the stack. The symbols denoting different types of item are shown next.


### Types and their representations

| Symbol | Data type  |
| ------ | ---------- |
| `b`    | a boolean |
| `i`    | a signed integer |
| `f`    | a floating-point number |
| `n`    | a number (integer or floating point) |
| `r`    | a reference (pointer to a blob) |
| `a`    | a reference to an array |
| `c`    | a reference to a closure |
| `s`    | a reference to a string |
| `x`    | an unspecified item |

See the section “Machine code format” below for more details of binary representation.

Each type may be suffixed by a number in stack pictures; if the same combination of type and suffix appears more than once in a stack comment, it refers to identical stack items.

Integers are represented as twos-complement as part of an `INT` instruction.

Floats are 64-bit IEEE floats with the bottom bit cleared (this bit denotes the `FLOAT` instruction). No rounding is performed on the result of arithmetic operations.


### Do nothing


> `NOP`  
> →
>
> Do nothing.


### Literals

These instructions encode literal values.

> `INT`  
> → `i`
>
> Push the integer encoded in the `INT` instruction on to the stack. Increment `PC`.

> `FLOAT`  
> → `f`
>
> Push the float encoded in the `FLOAT` instruction on to the stack. Increment `PC`.

> `NULL`  
> → `NULL`
>
> Push the atom `NULL`.

> `FALSE`  
> → `FALSE`
>
> Push the boolean atom `FALSE`.

> `TRUE`  
> → `TRUE`
>
> Push the boolean atom `TRUE`.

> `STRING`  
>  → `s`
> Push a reference to the string `s`.


### Numeric type conversion

Numeric conversions:

> `I2F`  
> `i` → `f`
>
> Pop `i`, convert the integer to a float, and push the float.

> `F2I`  
> `f` → `i`
>
> Pop `f`, convert the float to an integer, and push the integer.


### Array manipulation

These instructions manage arrays.

> `S0`  
> → `a`
>
> Push a reference to `S0` to the stack.

> `SIZE`  
> `a` -> `i`
>
> Pop `a`, and push the number of items in that array to the stack.

> `NEW`  
> → `a`
>
> Create an empty array and push a reference to it to the stack.

> `DROP`  
> `x` →
>
> Discard `x` from the top of the stack.

> `SGET`  
> `i` → `x`
>
> Pop `i` from the stack. Push the `i`th item of the stack to the stack.

> `SET`  
> `x` `i` →
>
> Pop `i` from the stack. Set the `i`th item of the stack to `x`. Pop `x` from the stack.

> `DUP`  
> `x₁` → `x₁` `x₁`
>
> Push the top item of the stack to the stack.

> `SWAP`  
> `x₁` `x₂` → `x₂` `x₁`
>
> Swap the top two items on the stack.

> `OVER`  
> `x₁` `x₂` → `x₁` `x₂` `x₁`
>
> Push the second item of the stack to the stack.

> `GET`  
> `i` `a` → `x`
>
> Pop `i` and `a` from the stack. Push the `i`th item of the array pointed to by `a` to the stack.

> `SET`  
> `x` `i` `a` →
>
> Pop `i` and `a` from the stack. Set the `i`th item of the array pointed to by `a` to `x`. Pop `x` from the stack.

> `EXTRACT`  
> `i` `a` →
>
> Pop `i` and `a` from the stack. Move the `i`th item of the array pointed to by `a` to the top of the stack.

> `INSERT`  
> `i` `a` →
>
> Pop `i` and `a` from the stack. Insert the top item of the array pointed to by `a` at position `i`.

> `POP`  
> `a` → `x`
>
> Pop `x` from the array pointed to by `a`.

> `SHIFT`
> `a` → `x`
>
> Remove the first element from the array pointed to by `a` and push it to the stack.

> `APPEND`  
> `x` `a` →
>
> Append `x` to the array pointed to by `a`.

> `PREPEND`  
> `x` `a` →
>
> Prepend `x` to the array pointed to by `a`.

> `QUOTE`  
> → `x`
>
> Push the word at `PC` to the stack and increment `PC` to point to the next item.


### Control structures

These instructions implement branches, conditions and subroutine calls.

> `NEW_CLOSURE`  
> `x₁`…`xₙ` `i` `a` → `c`
>
> Pop `i` and `a`. Pop 2`i` stack items and create a new closure `c` with context `x₁`…`xₙ` and code `a`.

> `BLOB`  
> → `r`
>
> Push `IR` on to the stack.

> `JUMP`  
> `i` →
> 
> Pop `i`. Add `i` to `PC`.

> `JUMP_IF_FALSE`  
> `b` `i` →
> 
> Pop `b` and `i`. If `b` is false, add `i` to `PC`.

> `CALL`  
> `x₁`…`xₙ` `i₁` `c` `a` → `i₂`
>
> Pop `i₁`, `c` and `a`. Push `S0` and `P0` to the array given by `a`, then push `c`’s context array. Pop `i₁` stack items, and push them to the array given by `a`, in order from `x₁` to `xₙ`. Push `PC` to the current stack. Set `S0` to `a`, `P0` to `c`’s code array and `PC` to 0.

> `RET`  
> `x` `i` →
>
> Pop `x`. Set `P0` to item 1 of `S0` and `S0` to item 0 of `S0`. Pop `PC` from the stack, and push `x` to the stack.


### Logic and shifts

These instructions consist of bitwise logical operators and bitwise shifts. The result of performing the specified operation on the argument or arguments is left on the stack.

Logic functions:

> `NOT`  
> `x₁` → `x₂`
>
> If `x₁` is an integer, then invert all bits of `x₁`, giving its two’s complement `x₂`; if `x₁` is a boolean, pop it and push its logical negation `x₂`; otherwise, raise `WRONG_TYPE`.

> `AND`  
> `x₁` `x₂` → `x₃`
>
> If `x₁` and `x₂` are integers, `x₃` is the bit-by-bit logical “and” of `x₁` with `x₂`; if `x₁` and `x₂` are booleans, `x₃` is the logical “and” of `x₁` and `x₂`; otherwise, raise `WRONG_TYPE`.

> `OR`  
> `x₁` `x₂` → `x₃`
>
> If `x₁` and `x₂` are integers, `x₃` is the bit-by-bit inclusive-or of `x₁` with `x₂`; if `x₁` and `x₂` are booleans, `x₃` is the logical “or” of `x₁` and `x₂`; otherwise, raise `WRONG_TYPE`.

> `XOR`  
> `x₁` `x₂` → `x₃`
>
> If `x₁` and `x₂` are integers, `x₃` is the bit-by-bit exclusive-or of `x₁` with `x₂`; if `x₁` and `x₂` are booleans, `x₃` is the logical “exclusive-or” of `x₁` and `x₂`; otherwise, raise `WRONG_TYPE`.


Shifts:

> `LSH`  
> `x₁` `u` → `x₂`
>
> Perform a logical left shift of `u` bit-places on `x₁`, giving `x₂`. Put zero into the least significant bits vacated by the shift.

> `RSH`  
> `x₁` `u` → `x₂`
>
> Perform a logical right shift of `u` bit-places on `x₁`, giving `x₂`. Put zero into the most significant bits vacated by the shift.

> `ARSH`  
> `x₁` `u` → `x₂`
>
> Perform an arithmetic right shift of `u` bit-places on `x₁`, giving `x₂`. Copy the most significant bit into the most significant bits vacated by the shift.



### Comparison

> `EQ`  
> `x₁` `x₂` → `b`
>
> `i` is `TRUE` if `x₁` and `x₂` are equal, and `FALSE` otherwise.

> `LT`  
> `n₁` `n₂` → `b`
>
> `i` is `TRUE` if `n₁` is less than `n₂` and `FALSE` otherwise.



### Arithmetic

These instructions consist of monadic and dyadic operators. All calculations are made without bounds or overflow checking, except as detailed for certain instructions.

The result of dividing by zero is zero. Integer division rounds the quotient towards zero; signed division of –2³¹ by –1 gives a quotient of –2³¹ and a remainder of 0.

> `NEG`  
> `n₁` → `n₂`
>
> Negate `n₁`, giving its arithmetic inverse `n₂`.

> `ADD`  
> `n₁` `n₂` → `n₃`
>
> Add `n₂` to `n₁`, giving the sum `n₃`.

> `MUL`  
> `n₁` `n₂` → `n₃`
>
> Multiply `n₁` by `n₂`, giving the product `n₃`.

> `DIV`  
> `n₁` `n₂` → `n₃`
>
> Divide `n₁` by `n₂`, giving the quotient `n₃`.

> `REM`  
> `n₁` `n₂` → `n₃`
>
> Divide `n₁` by `n₂`, giving the remainder `n₃`.



### Errors

These instructions give access to SAM’s error mechanisms.

> `HALT`  
> →
>
> Stop SAM with error code `OK`.



### External access

These instructions allow access to I/O and other system facilities.

> `TRAP`  
> `n` →
>
> Execute trap `n`. Further stack items may also be consumed and returned, depending on `n`. If the trap is invalid, raise `INVALID_TRAP`.



## External interface

SAM’s external interface comes in three parts. The calling interface allows SAM to be controlled by other programs. The `TRAP` instruction allows implementations to provide access to system facilities, code written in other languages, and the speed of machine code in time-critical situations. The assembly format allows compiled code to be saved, reloaded and shared between systems.


### Machine code format

The encoding achieves the following aims:

+ Allow pointers to address the entire address space
+ As much precision as possible for floats & integers
+ Allow new atom and blob types to be defined
+ Compact instruction encoding

| Encoding | Meaning| Notes |
| --- | --- | --- |
| `x…x 0` | float |IEEE double-precision, low bit of mantissa forced to 0 |
| `x…x 01` | 62-bit integer |
| `x…x 011` | pointer |
| `x…x tttt 0111` | atom | 4-bit type, 7-byte payload |
| `x…x 01111`  | Trap | 59-bit function code |
| `iiiii…iiiii sss 011111`  | Instructions | 11 5-bit instructions, with 3-bit instruction set |


### Assembly format

TODO.


### Calling interface

See `libsam/sam.h`.


## Acknowledgements

Alistair Turnbull, as ever, has helped in many ways, from design suggestions to coding assistance, to code review and testing.

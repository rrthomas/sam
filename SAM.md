# The SAM Virtual Machine

by Reuben Thomas


## Introduction

SAM is a simple virtual machine designed playfully to teach some elements of low-level programming. It is programmed with an assembly language written as YAML.

SAM is self-contained, and performs I/O via the `TRAP` instruction, which provides access to I/O and possibly implementation-dependent facilities.


## Architecture

SAM is a stack-based VM. Its memory is a single stack of items, each of which is a word-sized VM instruction, or a reference to a stack or other blob. It has no other memory. It has a handful of registers.


### Registers

The registers are as follows:

| Register  | Function  |
| --------- | --------- |
| `S0`      | The current `S`tack. |
| `IR`      | The `I`nstruction `R`egister holds the currently-executing instruction. |
| `P0 `     | The current stack from which code is being executed. |
| `PC`      | The `P`rogram `C`ounter points to the next instruction. |

All of the registers are word-sized.


### The stack

The stack is represented as a series of VM instructions encoded as 4- or 8-byte words.

The stack is addressed with signed integers.

A positive address `n` refers to the `n`th item, starting from zero at the bottom of the stack.

A negative address –`n` refers to the `n`th item, counting from 1 at the top of the stack.

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
| `STACK_UNDERFLOW` | An attempt was made to access beyond the bottom of the given stack. |
| `STACK_OVERFLOW` | An attempt was made to access beyond the top of the given stack. |
| `WRONG_TYPE` | A stack item’s type is incorrect for the current operation. |
| `INVALID_TRAP` | An invalid function number was given to `TRAP`. |


## Instruction set

The instruction set is listed below, with the instructions grouped according to function. The instructions are given in the following format:

> `NAME`  
> `before` → `after`
>
> Description.

**Stack comments** are written `before` → `after`, where `before` and `after` are stack pictures showing the items on top of a stack before and after the instruction is executed (the change is called the **stack effect**). An instruction only affects the items shown in its stack comments. **Stack pictures** are a representation of the top-most items on the stack, and are written `i₁` `i₂`…`iₙ₋₁` `iₙ` where the `iₖ` are stack items, with `iₙ` being on top of the stack. The symbols denoting different types of stack item are shown next.


### Types and their representations

| Symbol | Data type  |
| ------ | ---------- |
| `i`    | a signed integer |
| `f`    | a floating-point number |
| `n`    | a number (integer or floating point) |
| `S`    | a reference to a string |
| `a`    | an atom |
| `r`    | a reference (pointer to a blob) |
| `s`    | a reference to a stack |
| `x`    | an unspecified item |

See the section “Machine code format” below for more details of binary representation.

Each type may be suffixed by a number in stack pictures; if the same combination of type and suffix appears more than once in a stack comment, it refers to identical stack items.

Integers are represented as twos-complement as part of an `INT` instruction.

Floats are IEEE floats (64-bit on an 8-byte word VM, or 32-bit on a 4-byte VM) with the bottom bit cleared (this bit denotes the `FLOAT` instruction). No rounding is performed on the result of arithmetic operations.


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
> → `a`
>
> Push the atom `NULL`.

> `STRING`  
>  → `S`
> Push a reference to the string `S`.


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


### Stack manipulation

These instructions manage stacks.

> `S0`  
> → `s`
>
> Push a reference to `S0` to the current stack.

> `SIZE`  
> `s` -> `i`
>
> Pop `s`, and push the number of items in that stack to the current stack.

> `NEW`  
> → `s`
>
> Create an empty stack and push a reference to it to the current stack.

> `DROP`  
> `x` →
>
> Discard `x` from the top of the stack.

> `GET`  
> `i` `s` → `x`
>
> Pop `i` and `s` from the stack. Push the `i`th item of the stack pointed to by `s` to the stack.

> `SET`  
> `x` `i` `s` →
>
> Pop `i` and `s` from the stack. Set the `i`th item of the stack pointed to by `s` to `x`. Pop `x` from the stack.

> `EXTRACT`  
> `i` `s` →
>
> Pop `i` and `s` from the stack. Move the `i`th stack item of the stack pointed to by `s` to the top of the stack.

> `INSERT`  
> `i` `s` →
>
> Pop `i` and `s` from the stack. Insert the top item of the stack pointed to by `s` at position `i`.

> `POP`  
> `s` → `x`
>
> Pop `x` from the stack pointed to by `s`.

> `SHIFT`
> `s` → `x`
>
> Remove the first element from the stack pointed to by `s` and push it to the current stack.

> `APPEND`  
> `x` `s` →
>
> Push `x` to the stack pointed to by `s`.

> `PREPEND`  
> `x` `s` →
>
> Prepend `x` to the array pointed to by `s`.

> `QUOTE`  
> → `x`
>
> Push the word at `PC` to the current stack and increment `PC` to point to the next stack item.


### Control structures

These instructions implement branches, conditions and subroutine calls.

> `BLOB`  
> → `r`
>
> Push `IR` on to the stack.

> `GO`  
> `s` →
> 
> Pop `s`. Set `PC` to the address of the first item of the stack pointed to by `s`.

> `DO`  
> `s₁` → `s₂` `i`
>
> Pop `s₁`. Push `P0` to the stack as a reference and `PC` as an integer, and set `P0` to `s₁` and `PC` to 0.

> `DONE`  
> `s` `i` →
>
> Pop `i` into `PC` and `s` into `P0`.

> `CALL`  
> `x₁`…`xₙ` `i` `s₁` `s₂` →
>
> Pop `i`, `s₁` and `s₂`. Pop `i` stack items, and push them to the stack given by `s₂`, in order from `x₁` to `xₙ`. Push `S0`, `P0`, and `PC` to the stack given by `s₂`. Set `S0` to `s₂`, `P0` to `s₁` and `PC` to 0.

> `RET`  
> `s₁` `s₂` `i` `x` →
>
> Pop `x`, `i`, `s₂` and `s₁`. Set `S0` to `s₁`, `P0` to `s₂`, and `PC` to `i`. Push `x` to the stack.

> `IF`  
> `i` `s₁` `s₂` → `p`
>
> Pop `s₁` and `s₂`. Pop `i`. If it is non-zero, perform the action of `DO` on `s₁`, otherwise on `s₂`.

> `WHILE`  
> `i` →
>
> Pop `i`. If it is the integer zero, perform the action of `DONE`.


### Logic and shifts

These instructions consist of bitwise logical operators and bitwise shifts. The result of performing the specified operation on the argument or arguments is left on the stack.

Logic functions:

> `NOT`  
> `x₁` → `x₂`
>
> Invert all bits of `x₁`, giving its logical inverse `x₂`.

> `AND`  
> `x₁` `x₂` → `x₃`
>
> `x₃` is the bit-by-bit logical “and” of `x₁` with `x₂`.

> `OR`  
> `x₁` `x₂` → `x₃`
>
> `x₃` is the bit-by-bit inclusive-or of `x₁` with `x₂`.

> `XOR`  
> `x₁` `x₂` → `x₃`
>
> `x₃` is the bit-by-bit exclusive-or of `x₁` with `x₂`.


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
> `x₁` `x₂` → `i`
>
> `i` is –1 if `x₁` and `x₂` are equal, and 0 otherwise.

> `LT`  
> `n₁` `n₂` → `i`
>
> `i` is –1 if `n₁` is less than `n₂` and 0 otherwise.



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

#### 64-bit encoding

| Encoding | Meaning| Notes |
| --- | --- | --- |
| `x…x 0` | float |IEEE double-precision, low bit of mantissa forced to 0 |
| `x…x 01` | 62-bit integer |
| `x…x 011` | pointer |
| `x…x tttt 0111` | atom | 4-bit type, 7-byte payload |
| `x…x 01111`  | Trap | 59-bit function code |
| `iiiii…iiiii sss 011111`  | Instructions | 11 5-bit instructions, with 3-bit instruction set |

#### 32-bit encoding

| Suffix | Meaning | Notes |
| --- | --- | --- |
| `x…x 0` | Float |IEEE single-precision, low bit of mantissa forced to 0 |
| `x…x 01` | pointer |
| `x…x 011` | 29-bit integer |
| `x…x tttt 0111` | atom  | 4-bit type, 3-byte payload |
| `x…x 01111` | Trap | 27-bit function code |
| `iiiii…iiiii s 011111` | Instructions | 5 5-bit instructions, with 1-bit instruction set |


### Assembly format

TODO.


### Calling interface

See `libsam/sam.h`.


## Acknowledgements

Alistair Turnbull, as ever, has helped in many ways, from design suggestions to coding assistance, to code review and testing.

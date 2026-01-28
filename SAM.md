# The SAM Virtual Machine

by Reuben Thomas


## Introduction

SAM is a simple virtual machine designed playfully to teach some elements of low-level programming. It is programmed with an assembly language written as YAML.

SAM is self-contained, and performs I/O via the `TRAP` instruction, which provides access to I/O and possibly implementation-dependent facilities.


## Architecture

SAM is a stack-based VM. Its memory is a single stack of items, each of which is a word-sized VM instruction. It has no other memory. It has a handful of registers.


### Registers

The registers are as follows:

| Register  | Function  |
| --------- | --------- |
| `SSIZE`   | The `S`tack `SIZE`. The number of items the stack can hold. |
| `S`      | The current `S`tack. |
| `SP`      | The `S`tack `P`ointer. The number of items currently on the stack. |
| `IR`      | The `I`nstruction `R`egister holds the currently-executing instruction. |
| `PC0`     | The current stack from which code is being executed. |
| `PC`      | The `P`rogram `C`ounter points to the next instruction. |

All of the registers are word-sized.


### The stack

The stack is represented as a series of VM instructions encoded as 4- or 8-byte words.

The stack is addressed with signed integers.

A positive address `n` refers to the `n`th item, starting from zero at the bottom of the stack.

A negative address –`n` refers to the `n`th item, counting from 1 at the top of the stack.

A valid address is one that points to a valid instruction.


### Operation

Before SAM is started, the stack should be given suitable contents, `SSIZE` and `SP` set appropriately, and `PC` to the address of some item in the stack.

```
begin
  load the stack item at `PC` into `IR`
  increment `PC` to point to the next stack item
  execute the instruction in `IR`
repeat
```

## Termination and errors

When SAM encounters a `HALT` instruction, it pops an integer from the top of the stack and returns it as its reason code. If an error occurs while popping the integer, that error code is returned instead.

Error codes are signed numbers. Negative codes are reserved for SAM; positive error codes may be used by user code.

The following table lists the errors and the conditions under which they are raised. Some further specific error conditions are documented with the instructions that raise them.

| Code  | Meaning  |
| ----- | -------- |
| `OK` | No error. |
| `INVALID_OPCODE` | An attempt was made to execute an invalid opcode. |
| `INVALID_ADDRESS` | Invalid address. |
| `STACK_UNDERFLOW` | The stack has underflowed, that is, an attempt was made to pop when it was empty. |
| `STACK_OVERFLOW` | The stack has overflowed, that is, an attempt was made to push to it when it already contained `SSIZE` items, or an attempt was made to access beyond the current top of the stack. |
| `WRONG_TYPE` | A stack item is not of the type expected. |
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
| `r`    | a reference (pointer to a bracket) |
| `x`    | an unspecified item |

See the section “Machine code format” below for more details of binary representation.

Each type may be suffixed by a number in stack pictures; if the same combination of type and suffix appears more than once in a stack comment, it refers to identical stack items.

Integers are represented as twos-complement as part of an `INT` instruction.

Floats are IEEE floats (64-bit on an 8-byte word VM, or 32-bit on a 4-byte VM) with the bottom bit cleared (this bit denotes the `FLOAT` instruction). No rounding is performed on the result of arithmetic operations.

A bracket is encoded as a `STACK` instruction pointing to a list of instructions, ending with a `RET` instruction.


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
> Push `OP` on to the stack.

> `FLOAT`  
> → `f`
>
> Push the float encoded in the `FLOAT` instruction on to the stack. Increment `PC`.


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

> `NEW`
> → `r`
>
> Create an empty stack and push a reference to it to the current stack.

> `POP`  
> `x` →
>
> Pop `x` from the top of the stack. If the stack is empty, raise the error `STACK_UNDERFLOW`.

> `GET`  
> `i` → `x`
>
> Pop `i` from the top of the stack. Push the `i`th stack item to the stack.

> `SET`  
> `x` `i` →
>
> Pop `i` and `x` from the stack. Set the `i`th stack item to `x`.

> `EXTRACT`  
> `x₁`…`xₙ` `i` → `x₂`…`xₙ` `x₁`
>
> Pop `i` from the top of the stack. Move the `i`th stack item to the top of the stack.

> `INSERT`  
> `x₁`…`xₙ` `i` →  `xₙ` `x₁`…`xₙ₋₁`
>
> Pop `i` from the top of the stack. Move the top stack item to position `i`.

> `IGET`  
> `i` `r` → `x`
>
> Push the `i`th item of the stack pointed to by `r` to the stack.

> `ISET`  
> `x` `i` `r` →
>
> Set the `i`th item of the stack pointed to by `r` to `x`.

> `IPOP`  
> `r` →
>
> Pop an item from the stack pointed to by `r`.

> `IPUSH`  
> `x` `r` →
>
> Push `x` the stack pointed to by `r`.


### Control structures

These instructions implement branches, conditions and subroutine calls.

> `CALL`  
> `x₁`…`xₙ` `i₁` `r₁` `r₂` `i₂` →
>
> Pop `i₁`, `r₁`, `r₂` and `i₂`. Pop `i₁` stack items, and push them to the stack given by `r₁`, in order from `x₁` to `xₙ`. Set `S0` to `r₁`. Push `PC0` and `PC` to the stack, and set `PC0` to `r₂` and `PC` to `i₂`.

> `RET`  
> `r` `i` →
>
> Pop `i` into `PC` and `r` into `PC0`.

> `STACK`  
> → `r`
>
> Push `IR` on to the stack.

> `GO`  
> `r` →
> 
> Pop `r`. Set `PC` to the address of the first item of the bracket pointed to by `r`.

> `DO`  
> `r₁` → `r₂` `i`
>
> Pop `r₁`. Push `PC0` to the stack as a reference and `PC` as an integer, and set `PC0` to `r₁` and `PC` to 0.

> `IF`  
> `i` `r₁` `r₂` → `p`
>
> Pop `r₁` and `r₂`. Pop `i`. If it is non-zero, perform the action of `DO` on `r₁`, otherwise on `r₂`.

> `WHILE`  
> `i` →
>
> Pop `i`. If it is zero, perform the action of `RET`.


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
> `n` →
>
> Stop SAM, returning reason code `n` to the calling program.



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
+ Some expansion ability for new atom & bracket types
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

Alistair Turnbull, as ever, has been a meticulous tester and reviewer.

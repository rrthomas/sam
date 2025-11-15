# The SAM Virtual Machine

by Reuben Thomas


## Introduction

SAM is a simple virtual machine designed playfully to teach some elements of low-level programming. It is programmed with an assembly language written as YAML.

SAM is self-contained, and performs I/O via the `TRAP` instruction, which provides access to I/O and possibly implementation-dependent facilities.


## Architecture

SAM is a stack-based VM. Its memory is a single stack of items, each of which is a VM instruction or a stack. It has no other memory. It has a handful of registers.


### Registers

The registers are as follows:

| Register  | Function  |
| --------- | --------- |
| `SSIZE`   | The `S`tack `SIZE`. The size of the stack in words. |
| `SP`      | The `S`tack `P`ointer. The number of words in the current stack. |
| `IR`      | The `I`nstruction `R`egister holds the currently-executing instruction word. |
| `OP`      | The `OP`erand is the operand encoded in the current instruction. |
| `I`       | The opcode of the currently executing `I`nstruction. |
| `PC`      | The `P`rogram `C`ounter points to the next instruction. |
| `PC0`     | Points to the start of the currently executing stack. |

All of the registers are word-sized.


### The stack

The stack is represented as a series of VM instructions encoded as 4-byte words.

The stack is addressed with signed integers.

A positive address `n` refers to the `n`th word, starting from zero at the bottom of the stack.

A negative address –`n` refers to the `n`th stack item, counting from 1 at the top of the stack.

A valid address is one that points to a valid instruction word whose opcode’s name does not start with an underscore.


### Operation

Before SAM is started, the stack should be given suitable contents, `SSIZE` and `SP` set appropriately, and `PC` and `PC0` set to point to some code in the stack.

```
begin
  load the stack item at `PC` into `IR`
  set `I` to the least significant byte of `IR`
  set `OP` to `IR` arithmetically shifted one byte to the right
  increment `PC`
  execute the instruction whose opcode is in `I`
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
| `STACK_OVERFLOW` | The stack has overflowed, that is, an attempt was made to push to it when it already contained `SSIZE` words, or an attempt was made to access beyond the current top of the stack. |
| `WRONG_TYPE` | A stack item is not of the type expected. |
| `BAD_BRACKET` | No matching `KET` found for a `BRA`, or vice versa. |
| `UNPAIRED_FLOAT` | A `FLOAT` instruction was not followed by `_FLOAT`. |
| `UNPAIRED_PUSH` | A `PUSH` instruction was not followed by `_PUSH`. |
| `INVALID_FUNCTION` | An invalid function number was given to  `TRAP`. |


## Instruction set

The instruction set is listed below, with the instructions grouped according to function. The instructions are given in the following format:

> `NAME`  
> `before` → `after`
>
> Description.

**Stack comments** are written `before` → `after`, where `before` and `after` are stack pictures showing the items on top of a stack before and after the instruction is executed (the change is called the **stack effect**). An instruction only affects the items shown in its stack comments. **Stack pictures** are a representation of the top-most items on the stack, and are written `i₁` `i₂`…`iₙ₋₁` `iₙ` where the `iₖ` are stack items, each of which occupies a whole number of words, with `iₙ` being on top of the stack. The symbols denoting different types of stack item are shown next.


### Types and their representations

| Symbol | Data type  |
| ------ | ---------- |
| `i`    | a signed integer |
| `f`    | a floating-point number |
| `n`    | a number (integer or floating point) |
| `l`    | a link (pointer to a stack) |
| `s`    | a scalar (anything other than a stack) |
| `c`    | a code item (stack or link) |
| `x`    | an unspecified item |

Each type may be suffixed by a number in stack pictures; if the same combination of type and suffix appears more than once in a stack comment, it refers to identical stack items.

Integers are stored as the top three bytes of an `INT` instruction, in twos-complement form.

Floats are 32-bit IEEE floats, whose top three bytes are stored in the top three bytes of a `FLOAT` instruction and bottom byte is in the second byte of the paired `_FLOAT` instruction.

A stack is encoded as a `BRA` instruction followed by the nested stack items and ending with a `KET` instruction. A link to a stack is encoded in a `LINK` instruction as the address of its `BRA` instruction.


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
> Push the float encoded in the `FLOAT` and following `_FLOAT` instruction on to the stack, or raise the error `UNPAIRED_FLOAT` if the following instruction is not a `_FLOAT` instruction. Increment `PC`.

> `_FLOAT`
>
> Raise the error `UNPAIRED_FLOAT`. This instruction should never be executed.


### Numeric type conversion

Numeric conversions:

> `I2F`  
> `i` → `f`
>
> Raise the error `WRONG_TYPE` if the top-most stack item is not an integer. Otherwise, pop it, convert the integer to a float, and push the float.

> `F2I`  
> `f` → `i`
>
> Raise the error `WRONG_TYPE` if the top-most stack item is not a float. Otherwise, pop it, convert the float to an integer, and push the integer.


### Stack manipulation

These instructions manage the stack.

> `PUSH`  
> → `x`
>
> Push the word encoded in the `PUSH` and following `_PUSH` instruction on to the stack, or raise the error `UNPAIRED_PUSH` if the following instruction is not a `_PUSH` instruction. Increment `PC`.

> `_PUSH`
>
> Raise the error `UNPAIRED_PUSH`. This instruction should never be executed.

> `POP`  
> `x` →
>
> If the stack is empty, raise the error `STACK_UNDERFLOW`. Pop `x` from the stack.

> `GET` `i`  
> → `x`
>
> Pop `i` from the top of the stack. Push the `i`th stack item to the stack. If that item is a stack, push a `LINK` instruction pointing to it.

> `SET`  
> `x` `i` →
>
> Pop `i` and `x` from the stack. Set the `i`th stack item to `x`.

> `IGET`  
> `i₁` `i₂` → `x`
>
> Push the `i₁`th item of the `i₂`th stack element, which must be a stack or link, to the stack.

> `ISET`  
> `x` `i₁` `i₂` →
>
> Set the `i₁`th item of the `i₂`th, which must be a stack or link, to `x`.


### Control structures

These instructions implement loops, conditions and subroutine calls.

> `BRA`  
> → `l`
>
> Push a link to `PC`–1 on to the stack, and add `OP`+1 to `PC`.

> `KET`  
> `l₁` `l₂` →
>
> Pop `l₂` into `PC` and `l₁` into `PC0`.

> `LINK`  
> → `l`
>
> Push `IR` on to the stack.

> `DO`  
> `i` → `l₁` `l₂`
>
> Pop `i`. If the stack item at that index is not a stack, raise `WRONG_TYPE`. Push `PC0` then `PC` to the stack, and set both `PC0` and `PC` to `i`.

> `IF`  
> `i` `c₁` `c₂` →
>
> Pop `c₁` and `c₂`. If either stack item is not code, raise `WRONG_TYPE`. Pop `i`. If it is non-zero, perform the action of `DO` on `c₁`, otherwise on `c₂`.

> `WHILE`  
> `i` →
>
> Pop `i`. If it is zero, perform the action of `KET`.

> `LOOP`
>
> Set `PC` to `PC0`.


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
> `s₁` `s₂` → `i`
>
> `i` is 1 if `s₁` and `s₂` are equal, and 0 otherwise. Integers, floats and links are compared.

> `LT`  
> `n₁` `n₂` → `i`
>
> `i` is 1 is `n₁` is less than `n₂` and 0 otherwise.



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

> `POW`  
> `n₁` `n₂` → `n₃`
>
> Raise `n₁` to the power `n₂`, giving the result `n₃`.



### Trigonometry

Trigonometric functions:

> `SIN`  
> `f₁` → `f₂`
>
> Calculate *sin* `f₁`, giving the result `f₂`.

> `COS`  
> `f₁` → `f₂`
>
> Calculate *cos* `f₁`, giving the result `f₂`.

> `DEG`  
> `f₁` → `f₂`
>
> Convert `f₁` radians to degrees, giving the result `f₂`.

> `RAD`  
> `f₁` → `f₂`
>
> Convert `f₁` degrees to radians, giving the result `f₂`.



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
> Execute trap `n`. Further stack items may also be consumed and returned, depending on `n`. If the trap is invalid, raise `INVALID_FUNCTION`.



## External interface

SAM’s external interface comes in three parts. The calling interface allows SAM to be controlled by other programs. The `TRAP` instruction allows implementations to provide access to system facilities, code written in other languages, and the speed of machine code in time-critical situations. The assembly format allows compiled code to be saved, reloaded and shared between systems.


### Assembly format

TODO.


### Calling interface

See `libsam/sam.h`.


## Acknowledgements

Alistair Turnbull, as ever, has been a meticulous tester and reviewer.

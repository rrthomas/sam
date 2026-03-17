// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#cgo pkg-config: sdl2 SDL2_gfx libgrapheme
//#include <stdlib.h>
//#include "sam.h"
//#include "sam_opcodes.h"
//#include "private.h"
//#include "traps_basic.h"
//#include "traps_math.h"
//#include "traps_graphics.h"
import "C"
import (
	"fmt"
	"unsafe"
)

type Word = C.sam_word_t
type Uword = C.sam_uword_t

var FLOAT_TAG = C.SAM_FLOAT_TAG
var FLOAT_TAG_MASK = C.SAM_FLOAT_TAG_MASK
var FLOAT_SHIFT = C.SAM_FLOAT_SHIFT
var INT_TAG = C.SAM_INT_TAG
var INT_TAG_MASK = C.SAM_INT_TAG_MASK
var INT_SHIFT = C.SAM_INT_SHIFT
var BLOB_TAG = C.SAM_BLOB_TAG
var BLOB_TAG_MASK = C.SAM_BLOB_TAG_MASK
var BLOB_SHIFT = C.SAM_BLOB_SHIFT
var ATOM_TAG = C.SAM_ATOM_TAG
var ATOM_TAG_MASK = C.SAM_ATOM_TAG_MASK
var ATOM_TYPE_MASK = C.SAM_ATOM_TYPE_MASK
var ATOM_TYPE_SHIFT = C.SAM_ATOM_TYPE_SHIFT
var ATOM_SHIFT = C.SAM_ATOM_SHIFT
var TRAP_TAG = C.SAM_TRAP_TAG
var TRAP_TAG_MASK = C.SAM_TRAP_TAG_MASK
var TRAP_FUNCTION_SHIFT = C.SAM_TRAP_FUNCTION_SHIFT
var INSTS_TAG = C.SAM_INSTS_TAG
var INSTS_TAG_MASK = C.SAM_INSTS_TAG_MASK
var INSTS_SHIFT = C.SAM_INSTS_SHIFT
var INST_SET_MASK = C.SAM_INST_SET_MASK
var INST_SET_SHIFT = C.SAM_INST_SET_SHIFT
var INST_MASK = C.SAM_INST_MASK
var ONE_INST_SHIFT = C.SAM_ONE_INST_SHIFT
var WORD_BIT = C.SAM_WORD_BIT
var WORD_MASK = Uword(((1 << C.SAM_WORD_BIT) - 1))

type Stack struct {
	stack *C.sam_blob_t
}

type State struct {
	state  *C.sam_state_t
	result Word
}

func (state *State) Stack() Stack {
	return Stack{state.state.s0}
}

func NewStack() Stack {
	stack := Stack{}
	C.sam_stack_new(&stack.stack)
	return stack
}

func NewString(str string) Stack {
	stack := Stack{}
	cstr := C.CString(str)
	C.sam_string_new(&stack.stack, cstr, C.size_t(len(str)))
	return stack
}
func NewState() State {
	state := C.sam_state_new()
	var stack *C.sam_blob_t
	C.sam_stack_new(&stack)
	state.s0 = stack
	return State{state: state}
}

func (s *Stack) Sp() Uword {
	var stack *C.sam_stack_t
	C.sam_stack_from_blob(s.stack, &stack)
	return stack.sp
}

func (s *Stack) StackPeek(addr Uword) (int, Uword) {
	var val Uword
	res := C.sam_stack_peek(s.stack, addr, &val)
	return int(res), val
}

func (s *Stack) StackPoke(addr Uword, val Uword) int {
	return int(C.sam_stack_poke(s.stack, addr, val))
}

func (s *Stack) PushStack(val Word) int {
	return int(C.sam_stack_push(s.stack, val))
}

func (s *Stack) PushArray(stack Stack) int {
	return int(C.sam_push_ref(s.stack, stack.stack))
}

func (s *Stack) PushInt(val Uword) int {
	return int(C.sam_push_int(s.stack, val))
}

func (s *Stack) PushFloat(f float64) int {
	return int(C.sam_push_float(s.stack, C.sam_float_t(f)))
}

func (s *Stack) PushAtom(atomType Uword, operand Uword) int {
	return int(C.sam_push_atom(s.stack, atomType, operand))
}

func (s *Stack) PushTrap(function Uword) int {
	return int(C.sam_push_trap(s.stack, function))
}

func (s *Stack) PushInsts(insts Uword) int {
	return int(C.sam_push_insts(s.stack, insts))
}

func Run(state *State, code *Stack) Word {
	state.state.p0 = code.stack
	res := C.sam_run(state.state)
	if res == ERROR_OK {
		blob := state.Stack().stack
		var stack *C.sam_stack_t
		C.sam_stack_from_blob(blob, &stack)
		var val Uword
		res := C.sam_stack_peek(blob, stack.sp-1, &val)
		state.result = Word(val)
		if res != ERROR_OK {
			panic("Run: error while getting HALT result")
		}
	}
	return res
}

func BasicInit() Word {
	return C.sam_basic_init()
}

func BasicFinish() {
	C.sam_basic_finish()
}

func GraphicsInit() Word {
	return C.sam_graphics_init()
}

func GraphicsFinish() {
	C.sam_graphics_finish()
}

func GraphicsWindowUsed() bool {
	return C.sam_graphics_window_used() != 0
}

func GraphicsWait() {
	C.sam_graphics_wait()
}

func SetDebug(flag bool) {
	if flag {
		C.do_debug = true
	} else {
		C.do_debug = false
	}
}

func (stack *Stack) PrintStack() {
	C.sam_print_stack(stack.stack)
}

func DumpScreen(file string) {
	C.sam_dump_screen(C.CString(file))
}

const (
	ERROR_OK                = C.SAM_ERROR_OK
	ERROR_INVALID_OPCODE    = C.SAM_ERROR_INVALID_OPCODE
	ERROR_INVALID_ADDRESS   = C.SAM_ERROR_INVALID_ADDRESS
	ERROR_STACK_UNDERFLOW   = C.SAM_ERROR_STACK_UNDERFLOW
	ERROR_STACK_OVERFLOW    = C.SAM_ERROR_STACK_OVERFLOW
	ERROR_WRONG_TYPE        = C.SAM_ERROR_WRONG_TYPE
	ERROR_INVALID_TRAP      = C.SAM_ERROR_INVALID_TRAP
	ERROR_TRAP_INIT         = C.SAM_ERROR_TRAP_INIT
	ERROR_NO_MEMORY         = C.SAM_ERROR_NO_MEMORY
	ERROR_INVALID_BLOB_TYPE = C.SAM_ERROR_INVALID_BLOB_TYPE
)

const (
	ATOM_NULL = C.SAM_ATOM_NULL
)

const (
	BLOB_STACK  = C.SAM_BLOB_STACK
	BLOB_STRING = C.SAM_BLOB_STRING
	BLOB_RAW    = C.SAM_BLOB_RAW
)

var errors = map[int]string{
	ERROR_OK:                "ERROR_OK",
	ERROR_INVALID_OPCODE:    "ERROR_INVALID_OPCODE",
	ERROR_INVALID_ADDRESS:   "ERROR_INVALID_ADDRESS",
	ERROR_STACK_UNDERFLOW:   "ERROR_STACK_UNDERFLOW",
	ERROR_STACK_OVERFLOW:    "ERROR_STACK_OVERFLOW",
	ERROR_WRONG_TYPE:        "ERROR_WRONG_TYPE",
	ERROR_INVALID_TRAP:      "ERROR_INVALID_TRAP",
	ERROR_TRAP_INIT:         "ERROR_TRAP_INIT",
	ERROR_NO_MEMORY:         "ERROR_NO_MEMORY",
	ERROR_INVALID_BLOB_TYPE: "ERROR_INVALID_BLOB_TYPE",
}

func (state *State) ErrorMessage(code Word) string {
	if code == ERROR_OK {
		res := C.disas(state.result)
		goRes := C.GoString(res)
		msg := "halt with result"
		if (state.result & C.SAM_BLOB_TAG_MASK) == C.SAM_BLOB_TAG {
			stack := state.result & ^C.SAM_BLOB_TAG_MASK
			if len(goRes) == 0 {
				goRes = "(empty stack)\n"
			}
			msg += fmt.Sprintf(" stack %#x:\n", stack) + goRes[0:max(len(goRes)-1, 0)]
		} else {
			msg += " " + goRes
		}
		C.free(unsafe.Pointer(res))
		return msg
	} else if code >= ERROR_OK && code <= ERROR_INVALID_BLOB_TYPE {
		return fmt.Sprintf(errors[int(code)])
	}
	return fmt.Sprintf("unknown error code %d (0x%x)", code, uint(Uword(code)&WORD_MASK))
}

type Instruction struct {
	Tag      Word
	Opcode   Uword
	Operands int // -1 means >=1 argument
	Terminal bool
}

var Instructions = map[string]Instruction{
	"int":   {C.SAM_INT_TAG, 0, 1, true},
	"float": {C.SAM_FLOAT_TAG, 0, 1, true},
	"trap":  {C.SAM_TRAP_TAG, 0, 1, true},

	// Blobs
	"stack":  {C.SAM_BLOB_TAG, C.SAM_BLOB_STACK, 1, true},
	"string": {C.SAM_BLOB_TAG, C.SAM_BLOB_STRING, -1, true},

	// Atoms
	"null": {C.SAM_ATOM_TAG, C.SAM_ATOM_NULL, 0, true},

	// Instructions
	"nop":     {C.SAM_INSTS_TAG, C.INST_NOP, 0, false},
	"new":     {C.SAM_INSTS_TAG, C.INST_NEW, 0, false},
	"s0":      {C.SAM_INSTS_TAG, C.INST_S0, 0, false},
	"drop":    {C.SAM_INSTS_TAG, C.INST_DROP, 0, false},
	"get":     {C.SAM_INSTS_TAG, C.INST_GET, 0, false},
	"set":     {C.SAM_INSTS_TAG, C.INST_SET, 0, false},
	"extract": {C.SAM_INSTS_TAG, C.INST_EXTRACT, 0, false},
	"insert":  {C.SAM_INSTS_TAG, C.INST_INSERT, 0, false},
	"pop":     {C.SAM_INSTS_TAG, C.INST_POP, 0, false},
	"shift":   {C.SAM_INSTS_TAG, C.INST_SHIFT, 0, false},
	"append":  {C.SAM_INSTS_TAG, C.INST_APPEND, 0, false},
	"prepend": {C.SAM_INSTS_TAG, C.INST_PREPEND, 0, false},
	"go":      {C.SAM_INSTS_TAG, C.INST_GO, 0, true},
	"do":      {C.SAM_INSTS_TAG, C.INST_DO, 0, true},
	"call":    {C.SAM_INSTS_TAG, C.INST_CALL, 0, true},
	"if":      {C.SAM_INSTS_TAG, C.INST_IF, 0, true},
	"while":   {C.SAM_INSTS_TAG, C.INST_WHILE, 0, false},
	"not":     {C.SAM_INSTS_TAG, C.INST_NOT, 0, false},
	"and":     {C.SAM_INSTS_TAG, C.INST_AND, 0, false},
	"or":      {C.SAM_INSTS_TAG, C.INST_OR, 0, false},
	"xor":     {C.SAM_INSTS_TAG, C.INST_XOR, 0, false},
	"eq":      {C.SAM_INSTS_TAG, C.INST_EQ, 0, false},
	"lt":      {C.SAM_INSTS_TAG, C.INST_LT, 0, false},
	"neg":     {C.SAM_INSTS_TAG, C.INST_NEG, 0, false},
	"add":     {C.SAM_INSTS_TAG, C.INST_ADD, 0, false},
	"mul":     {C.SAM_INSTS_TAG, C.INST_MUL, 0, false},
	"zero":    {C.SAM_INSTS_TAG, C.INST_0, 0, false},
	"false":   {C.SAM_INSTS_TAG, C.INST_0, 0, false},
	"one":     {C.SAM_INSTS_TAG, C.INST_1, 0, false},
	"_one":    {C.SAM_INSTS_TAG, C.INST_MINUS_1, 0, false},
	"true":    {C.SAM_INSTS_TAG, C.INST_MINUS_1, 0, false},
	"two":     {C.SAM_INSTS_TAG, C.INST_2, 0, false},
	"_two":    {C.SAM_INSTS_TAG, C.INST_MINUS_2, 0, false},
	"halt":    {C.SAM_INSTS_TAG, C.INST_HALT, 0, true},
}

var Traps = map[string]int{
	"SIZE":    C.TRAP_BASIC_SIZE,
	"QUOTE":   C.TRAP_BASIC_QUOTE,
	"COPY":    C.TRAP_BASIC_COPY,
	"RET":     C.TRAP_BASIC_RET,
	"LSH":     C.TRAP_BASIC_LSH,
	"RSH":     C.TRAP_BASIC_RSH,
	"ARSH":    C.TRAP_BASIC_ARSH,
	"DIV":     C.TRAP_BASIC_DIV,
	"REM":     C.TRAP_BASIC_REM,
	"ITER":    C.TRAP_BASIC_ITER,
	"NEXT":    C.TRAP_BASIC_NEXT,
	"NEW_MAP": C.TRAP_BASIC_NEW_MAP,
	"DEBUG":   C.TRAP_BASIC_DEBUG,
	"LOG":     C.TRAP_BASIC_LOG,

	"I2F": C.TRAP_MATH_I2F,
	"F2I": C.TRAP_MATH_F2I,
	"POW": C.TRAP_MATH_POW,
	"SIN": C.TRAP_MATH_SIN,
	"COS": C.TRAP_MATH_COS,
	"DEG": C.TRAP_MATH_DEG,
	"RAD": C.TRAP_MATH_RAD,

	"BLACK":           C.TRAP_GRAPHICS_BLACK,
	"WHITE":           C.TRAP_GRAPHICS_WHITE,
	"DISPLAY_WIDTH":   C.TRAP_GRAPHICS_DISPLAY_WIDTH,
	"DISPLAY_HEIGHT":  C.TRAP_GRAPHICS_DISPLAY_HEIGHT,
	"CLEARSCREEN":     C.TRAP_GRAPHICS_CLEARSCREEN,
	"SETDOT":          C.TRAP_GRAPHICS_SETDOT,
	"DRAWLINE":        C.TRAP_GRAPHICS_DRAWLINE,
	"DRAWRECT":        C.TRAP_GRAPHICS_DRAWRECT,
	"DRAWROUNDRECT":   C.TRAP_GRAPHICS_DRAWROUNDRECT,
	"FILLRECT":        C.TRAP_GRAPHICS_FILLRECT,
	"DRAWCIRCLE":      C.TRAP_GRAPHICS_DRAWCIRCLE,
	"FILLCIRCLE":      C.TRAP_GRAPHICS_FILLCIRCLE,
	"DRAWBITMAP":      C.TRAP_GRAPHICS_DRAWBITMAP,
	"FONT_REGULAR":    C.TRAP_GRAPHICS_FONT_REGULAR,
	"FONT_ITALIC":     C.TRAP_GRAPHICS_FONT_ITALIC,
	"FONT_BOLD":       C.TRAP_GRAPHICS_FONT_BOLD,
	"FONT_BOLDITALIC": C.TRAP_GRAPHICS_FONT_BOLDITALIC,
	"FONT_EMOJI":      C.TRAP_GRAPHICS_FONT_EMOJI,
	"TEXT":            C.TRAP_GRAPHICS_TEXT,
}

// The net change in `SP` caused by each instruction.
// If there is more than one possibility, then it's the change when execution
// continues at the next instruction.
var StackDifference = map[string]int{
	// Tag instructions
	"atom":  1,
	"int":   1,
	"float": 1,

	// Blobs
	"stack":  1,
	"string": 1,

	// Atoms
	"null": 1,

	// Instructions without immediate operand
	"nop":     0,
	"new":     1,
	"s0":      1,
	"drop":    -1,
	"get":     -1,
	"set":     -3,
	"extract": -2,
	"insert":  -2,
	"pop":     0,
	"shift":   0,
	"append":  -2,
	"prepend": -2,
	"go":      -1,
	"do":      -1,
	"call":    -3,
	"if":      -3,
	"while":   -1,
	"not":     0,
	"and":     -1,
	"or":      -1,
	"xor":     -1,
	"eq":      -1,
	"lt":      -1,
	"neg":     0,
	"add":     -1,
	"mul":     -1,
	"zero":    1,
	"false":   1,
	"one":     1,
	"_one":    1,
	"true":    1,
	"two":     1,
	"_two":    1,
	"halt":    -1,

	// Traps depend on the trap code.
}

type StackEffect struct {
	In  Uword
	Out Uword
}

var TrapStackEffect = map[string]StackEffect{
	// Basic traps
	"SIZE":    {1, 1},
	"QUOTE":   {0, 1},
	"COPY":    {1, 1},
	"RET":     {4, 0},
	"LSH":     {2, 1},
	"RSH":     {2, 1},
	"ARSH":    {2, 1},
	"DIV":     {2, 1},
	"REM":     {2, 1},
	"ITER":    {1, 1},
	"NEXT":    {1, 1},
	"NEW_MAP": {0, 1},
	"DEBUG":   {1, 0},
	"LOG":     {1, 0},

	// Math traps
	"I2F": {1, 1},
	"F2I": {1, 1},
	"POW": {2, 1},
	"SIN": {1, 1},
	"COS": {1, 1},
	"DEG": {1, 1},
	"RAD": {1, 1},

	// Graphics traps
	"BLACK":           {0, 1},
	"WHITE":           {0, 1},
	"DISPLAY_WIDTH":   {0, 1},
	"DISPLAY_HEIGHT":  {0, 1},
	"CLEARSCREEN":     {1, 0},
	"SETDOT":          {3, 0},
	"DRAWLINE":        {5, 0},
	"DRAWRECT":        {5, 0},
	"DRAWROUNDRECT":   {6, 0},
	"FILLRECT":        {5, 0},
	"DRAWCIRCLE":      {4, 0},
	"FILLCIRCLE":      {4, 0},
	"DRAWBITMAP":      {4, 0},
	"FONT_REGULAR":    {0, 1},
	"FONT_ITALIC":     {0, 1},
	"FONT_BOLD":       {0, 1},
	"FONT_BOLDITALIC": {0, 1},
	"FONT_EMOJI":      {0, 1},
	"TEXT":            {5, 0},
}

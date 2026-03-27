// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#cgo pkg-config: sdl2 libgrapheme
//#include <stdlib.h>
//#include "sam.h"
//#include "sam_sdl.h"
//#include "sam_opcodes.h"
//#include "private.h"
//#include "traps_basic.h"
//#include "traps_math.h"
//#include "traps_graphics.h"
//#include "traps_input.h"
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

func SdlInit() Word {
	return C.sam_sdl_init()
}

func SdlFinish() {
	C.sam_sdl_finish()
}

func SdlWindowUsed() bool {
	return C.sam_sdl_window_used() != 0
}

func SdlWait() {
	C.sam_sdl_wait()
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

	"KEYPRESSED":             C.TRAP_INPUT_KEYPRESSED,
	"KEY_A":                  C.TRAP_INPUT_KEY_A,
	"KEY_B":                  C.TRAP_INPUT_KEY_B,
	"KEY_C":                  C.TRAP_INPUT_KEY_C,
	"KEY_D":                  C.TRAP_INPUT_KEY_D,
	"KEY_E":                  C.TRAP_INPUT_KEY_E,
	"KEY_F":                  C.TRAP_INPUT_KEY_F,
	"KEY_G":                  C.TRAP_INPUT_KEY_G,
	"KEY_H":                  C.TRAP_INPUT_KEY_H,
	"KEY_I":                  C.TRAP_INPUT_KEY_I,
	"KEY_J":                  C.TRAP_INPUT_KEY_J,
	"KEY_K":                  C.TRAP_INPUT_KEY_K,
	"KEY_L":                  C.TRAP_INPUT_KEY_L,
	"KEY_M":                  C.TRAP_INPUT_KEY_M,
	"KEY_N":                  C.TRAP_INPUT_KEY_N,
	"KEY_O":                  C.TRAP_INPUT_KEY_O,
	"KEY_P":                  C.TRAP_INPUT_KEY_P,
	"KEY_Q":                  C.TRAP_INPUT_KEY_Q,
	"KEY_R":                  C.TRAP_INPUT_KEY_R,
	"KEY_S":                  C.TRAP_INPUT_KEY_S,
	"KEY_T":                  C.TRAP_INPUT_KEY_T,
	"KEY_U":                  C.TRAP_INPUT_KEY_U,
	"KEY_V":                  C.TRAP_INPUT_KEY_V,
	"KEY_W":                  C.TRAP_INPUT_KEY_W,
	"KEY_X":                  C.TRAP_INPUT_KEY_X,
	"KEY_Y":                  C.TRAP_INPUT_KEY_Y,
	"KEY_Z":                  C.TRAP_INPUT_KEY_Z,
	"KEY_1":                  C.TRAP_INPUT_KEY_1,
	"KEY_2":                  C.TRAP_INPUT_KEY_2,
	"KEY_3":                  C.TRAP_INPUT_KEY_3,
	"KEY_4":                  C.TRAP_INPUT_KEY_4,
	"KEY_5":                  C.TRAP_INPUT_KEY_5,
	"KEY_6":                  C.TRAP_INPUT_KEY_6,
	"KEY_7":                  C.TRAP_INPUT_KEY_7,
	"KEY_8":                  C.TRAP_INPUT_KEY_8,
	"KEY_9":                  C.TRAP_INPUT_KEY_9,
	"KEY_0":                  C.TRAP_INPUT_KEY_0,
	"KEY_RETURN":             C.TRAP_INPUT_KEY_RETURN,
	"KEY_ESCAPE":             C.TRAP_INPUT_KEY_ESCAPE,
	"KEY_BACKSPACE":          C.TRAP_INPUT_KEY_BACKSPACE,
	"KEY_TAB":                C.TRAP_INPUT_KEY_TAB,
	"KEY_SPACE":              C.TRAP_INPUT_KEY_SPACE,
	"KEY_MINUS":              C.TRAP_INPUT_KEY_MINUS,
	"KEY_EQUALS":             C.TRAP_INPUT_KEY_EQUALS,
	"KEY_LEFTBRACKET":        C.TRAP_INPUT_KEY_LEFTBRACKET,
	"KEY_RIGHTBRACKET":       C.TRAP_INPUT_KEY_RIGHTBRACKET,
	"KEY_BACKSLASH":          C.TRAP_INPUT_KEY_BACKSLASH,
	"KEY_SEMICOLON":          C.TRAP_INPUT_KEY_SEMICOLON,
	"KEY_APOSTROPHE":         C.TRAP_INPUT_KEY_APOSTROPHE,
	"KEY_GRAVE":              C.TRAP_INPUT_KEY_GRAVE,
	"KEY_COMMA":              C.TRAP_INPUT_KEY_COMMA,
	"KEY_PERIOD":             C.TRAP_INPUT_KEY_PERIOD,
	"KEY_SLASH":              C.TRAP_INPUT_KEY_SLASH,
	"KEY_CAPSLOCK":           C.TRAP_INPUT_KEY_CAPSLOCK,
	"KEY_F1":                 C.TRAP_INPUT_KEY_F1,
	"KEY_F2":                 C.TRAP_INPUT_KEY_F2,
	"KEY_F3":                 C.TRAP_INPUT_KEY_F3,
	"KEY_F4":                 C.TRAP_INPUT_KEY_F4,
	"KEY_F5":                 C.TRAP_INPUT_KEY_F5,
	"KEY_F6":                 C.TRAP_INPUT_KEY_F6,
	"KEY_F7":                 C.TRAP_INPUT_KEY_F7,
	"KEY_F8":                 C.TRAP_INPUT_KEY_F8,
	"KEY_F9":                 C.TRAP_INPUT_KEY_F9,
	"KEY_F10":                C.TRAP_INPUT_KEY_F10,
	"KEY_F11":                C.TRAP_INPUT_KEY_F11,
	"KEY_F12":                C.TRAP_INPUT_KEY_F12,
	"KEY_PRINTSCREEN":        C.TRAP_INPUT_KEY_PRINTSCREEN,
	"KEY_SCROLLLOCK":         C.TRAP_INPUT_KEY_SCROLLLOCK,
	"KEY_PAUSE":              C.TRAP_INPUT_KEY_PAUSE,
	"KEY_INSERT":             C.TRAP_INPUT_KEY_INSERT,
	"KEY_HOME":               C.TRAP_INPUT_KEY_HOME,
	"KEY_PAGEUP":             C.TRAP_INPUT_KEY_PAGEUP,
	"KEY_DELETE":             C.TRAP_INPUT_KEY_DELETE,
	"KEY_END":                C.TRAP_INPUT_KEY_END,
	"KEY_PAGEDOWN":           C.TRAP_INPUT_KEY_PAGEDOWN,
	"KEY_RIGHT":              C.TRAP_INPUT_KEY_RIGHT,
	"KEY_LEFT":               C.TRAP_INPUT_KEY_LEFT,
	"KEY_DOWN":               C.TRAP_INPUT_KEY_DOWN,
	"KEY_UP":                 C.TRAP_INPUT_KEY_UP,
	"KEY_NUMLOCKCLEAR":       C.TRAP_INPUT_KEY_NUMLOCKCLEAR,
	"KEY_KP_DIVIDE":          C.TRAP_INPUT_KEY_KP_DIVIDE,
	"KEY_KP_MULTIPLY":        C.TRAP_INPUT_KEY_KP_MULTIPLY,
	"KEY_KP_MINUS":           C.TRAP_INPUT_KEY_KP_MINUS,
	"KEY_KP_PLUS":            C.TRAP_INPUT_KEY_KP_PLUS,
	"KEY_KP_ENTER":           C.TRAP_INPUT_KEY_KP_ENTER,
	"KEY_KP_1":               C.TRAP_INPUT_KEY_KP_1,
	"KEY_KP_2":               C.TRAP_INPUT_KEY_KP_2,
	"KEY_KP_3":               C.TRAP_INPUT_KEY_KP_3,
	"KEY_KP_4":               C.TRAP_INPUT_KEY_KP_4,
	"KEY_KP_5":               C.TRAP_INPUT_KEY_KP_5,
	"KEY_KP_6":               C.TRAP_INPUT_KEY_KP_6,
	"KEY_KP_7":               C.TRAP_INPUT_KEY_KP_7,
	"KEY_KP_8":               C.TRAP_INPUT_KEY_KP_8,
	"KEY_KP_9":               C.TRAP_INPUT_KEY_KP_9,
	"KEY_KP_0":               C.TRAP_INPUT_KEY_KP_0,
	"KEY_KP_PERIOD":          C.TRAP_INPUT_KEY_KP_PERIOD,
	"KEY_NONUSBACKSLASH":     C.TRAP_INPUT_KEY_NONUSBACKSLASH,
	"KEY_APPLICATION":        C.TRAP_INPUT_KEY_APPLICATION,
	"KEY_POWER":              C.TRAP_INPUT_KEY_POWER,
	"KEY_KP_EQUALS":          C.TRAP_INPUT_KEY_KP_EQUALS,
	"KEY_F13":                C.TRAP_INPUT_KEY_F13,
	"KEY_F14":                C.TRAP_INPUT_KEY_F14,
	"KEY_F15":                C.TRAP_INPUT_KEY_F15,
	"KEY_F16":                C.TRAP_INPUT_KEY_F16,
	"KEY_F17":                C.TRAP_INPUT_KEY_F17,
	"KEY_F18":                C.TRAP_INPUT_KEY_F18,
	"KEY_F19":                C.TRAP_INPUT_KEY_F19,
	"KEY_F20":                C.TRAP_INPUT_KEY_F20,
	"KEY_F21":                C.TRAP_INPUT_KEY_F21,
	"KEY_F22":                C.TRAP_INPUT_KEY_F22,
	"KEY_F23":                C.TRAP_INPUT_KEY_F23,
	"KEY_F24":                C.TRAP_INPUT_KEY_F24,
	"KEY_EXECUTE":            C.TRAP_INPUT_KEY_EXECUTE,
	"KEY_HELP":               C.TRAP_INPUT_KEY_HELP,
	"KEY_MENU":               C.TRAP_INPUT_KEY_MENU,
	"KEY_SELECT":             C.TRAP_INPUT_KEY_SELECT,
	"KEY_STOP":               C.TRAP_INPUT_KEY_STOP,
	"KEY_AGAIN":              C.TRAP_INPUT_KEY_AGAIN,
	"KEY_UNDO":               C.TRAP_INPUT_KEY_UNDO,
	"KEY_CUT":                C.TRAP_INPUT_KEY_CUT,
	"KEY_COPY":               C.TRAP_INPUT_KEY_COPY,
	"KEY_PASTE":              C.TRAP_INPUT_KEY_PASTE,
	"KEY_FIND":               C.TRAP_INPUT_KEY_FIND,
	"KEY_MUTE":               C.TRAP_INPUT_KEY_MUTE,
	"KEY_VOLUMEUP":           C.TRAP_INPUT_KEY_VOLUMEUP,
	"KEY_VOLUMEDOWN":         C.TRAP_INPUT_KEY_VOLUMEDOWN,
	"KEY_KP_COMMA":           C.TRAP_INPUT_KEY_KP_COMMA,
	"KEY_KP_EQUALSAS400":     C.TRAP_INPUT_KEY_KP_EQUALSAS400,
	"KEY_INTERNATIONAL1":     C.TRAP_INPUT_KEY_INTERNATIONAL1,
	"KEY_INTERNATIONAL2":     C.TRAP_INPUT_KEY_INTERNATIONAL2,
	"KEY_INTERNATIONAL3":     C.TRAP_INPUT_KEY_INTERNATIONAL3,
	"KEY_INTERNATIONAL4":     C.TRAP_INPUT_KEY_INTERNATIONAL4,
	"KEY_INTERNATIONAL5":     C.TRAP_INPUT_KEY_INTERNATIONAL5,
	"KEY_INTERNATIONAL6":     C.TRAP_INPUT_KEY_INTERNATIONAL6,
	"KEY_INTERNATIONAL7":     C.TRAP_INPUT_KEY_INTERNATIONAL7,
	"KEY_INTERNATIONAL8":     C.TRAP_INPUT_KEY_INTERNATIONAL8,
	"KEY_INTERNATIONAL9":     C.TRAP_INPUT_KEY_INTERNATIONAL9,
	"KEY_LANG1":              C.TRAP_INPUT_KEY_LANG1,
	"KEY_LANG2":              C.TRAP_INPUT_KEY_LANG2,
	"KEY_LANG3":              C.TRAP_INPUT_KEY_LANG3,
	"KEY_LANG4":              C.TRAP_INPUT_KEY_LANG4,
	"KEY_LANG5":              C.TRAP_INPUT_KEY_LANG5,
	"KEY_LANG6":              C.TRAP_INPUT_KEY_LANG6,
	"KEY_LANG7":              C.TRAP_INPUT_KEY_LANG7,
	"KEY_LANG8":              C.TRAP_INPUT_KEY_LANG8,
	"KEY_LANG9":              C.TRAP_INPUT_KEY_LANG9,
	"KEY_ALTERASE":           C.TRAP_INPUT_KEY_ALTERASE,
	"KEY_SYSREQ":             C.TRAP_INPUT_KEY_SYSREQ,
	"KEY_CANCEL":             C.TRAP_INPUT_KEY_CANCEL,
	"KEY_CLEAR":              C.TRAP_INPUT_KEY_CLEAR,
	"KEY_PRIOR":              C.TRAP_INPUT_KEY_PRIOR,
	"KEY_RETURN2":            C.TRAP_INPUT_KEY_RETURN2,
	"KEY_SEPARATOR":          C.TRAP_INPUT_KEY_SEPARATOR,
	"KEY_OUT":                C.TRAP_INPUT_KEY_OUT,
	"KEY_OPER":               C.TRAP_INPUT_KEY_OPER,
	"KEY_CLEARAGAIN":         C.TRAP_INPUT_KEY_CLEARAGAIN,
	"KEY_CRSEL":              C.TRAP_INPUT_KEY_CRSEL,
	"KEY_EXSEL":              C.TRAP_INPUT_KEY_EXSEL,
	"KEY_KP_00":              C.TRAP_INPUT_KEY_KP_00,
	"KEY_KP_000":             C.TRAP_INPUT_KEY_KP_000,
	"KEY_THOUSANDSSEPARATOR": C.TRAP_INPUT_KEY_THOUSANDSSEPARATOR,
	"KEY_DECIMALSEPARATOR":   C.TRAP_INPUT_KEY_DECIMALSEPARATOR,
	"KEY_CURRENCYUNIT":       C.TRAP_INPUT_KEY_CURRENCYUNIT,
	"KEY_CURRENCYSUBUNIT":    C.TRAP_INPUT_KEY_CURRENCYSUBUNIT,
	"KEY_KP_LEFTPAREN":       C.TRAP_INPUT_KEY_KP_LEFTPAREN,
	"KEY_KP_RIGHTPAREN":      C.TRAP_INPUT_KEY_KP_RIGHTPAREN,
	"KEY_KP_LEFTBRACE":       C.TRAP_INPUT_KEY_KP_LEFTBRACE,
	"KEY_KP_RIGHTBRACE":      C.TRAP_INPUT_KEY_KP_RIGHTBRACE,
	"KEY_KP_TAB":             C.TRAP_INPUT_KEY_KP_TAB,
	"KEY_KP_BACKSPACE":       C.TRAP_INPUT_KEY_KP_BACKSPACE,
	"KEY_KP_A":               C.TRAP_INPUT_KEY_KP_A,
	"KEY_KP_B":               C.TRAP_INPUT_KEY_KP_B,
	"KEY_KP_C":               C.TRAP_INPUT_KEY_KP_C,
	"KEY_KP_D":               C.TRAP_INPUT_KEY_KP_D,
	"KEY_KP_E":               C.TRAP_INPUT_KEY_KP_E,
	"KEY_KP_F":               C.TRAP_INPUT_KEY_KP_F,
	"KEY_KP_XOR":             C.TRAP_INPUT_KEY_KP_XOR,
	"KEY_KP_POWER":           C.TRAP_INPUT_KEY_KP_POWER,
	"KEY_KP_PERCENT":         C.TRAP_INPUT_KEY_KP_PERCENT,
	"KEY_KP_LESS":            C.TRAP_INPUT_KEY_KP_LESS,
	"KEY_KP_GREATER":         C.TRAP_INPUT_KEY_KP_GREATER,
	"KEY_KP_AMPERSAND":       C.TRAP_INPUT_KEY_KP_AMPERSAND,
	"KEY_KP_DBLAMPERSAND":    C.TRAP_INPUT_KEY_KP_DBLAMPERSAND,
	"KEY_KP_VERTICALBAR":     C.TRAP_INPUT_KEY_KP_VERTICALBAR,
	"KEY_KP_DBLVERTICALBAR":  C.TRAP_INPUT_KEY_KP_DBLVERTICALBAR,
	"KEY_KP_COLON":           C.TRAP_INPUT_KEY_KP_COLON,
	"KEY_KP_HASH":            C.TRAP_INPUT_KEY_KP_HASH,
	"KEY_KP_SPACE":           C.TRAP_INPUT_KEY_KP_SPACE,
	"KEY_KP_AT":              C.TRAP_INPUT_KEY_KP_AT,
	"KEY_KP_EXCLAM":          C.TRAP_INPUT_KEY_KP_EXCLAM,
	"KEY_KP_MEMSTORE":        C.TRAP_INPUT_KEY_KP_MEMSTORE,
	"KEY_KP_MEMRECALL":       C.TRAP_INPUT_KEY_KP_MEMRECALL,
	"KEY_KP_MEMCLEAR":        C.TRAP_INPUT_KEY_KP_MEMCLEAR,
	"KEY_KP_MEMADD":          C.TRAP_INPUT_KEY_KP_MEMADD,
	"KEY_KP_MEMSUBTRACT":     C.TRAP_INPUT_KEY_KP_MEMSUBTRACT,
	"KEY_KP_MEMMULTIPLY":     C.TRAP_INPUT_KEY_KP_MEMMULTIPLY,
	"KEY_KP_MEMDIVIDE":       C.TRAP_INPUT_KEY_KP_MEMDIVIDE,
	"KEY_KP_PLUSMINUS":       C.TRAP_INPUT_KEY_KP_PLUSMINUS,
	"KEY_KP_CLEAR":           C.TRAP_INPUT_KEY_KP_CLEAR,
	"KEY_KP_CLEARENTRY":      C.TRAP_INPUT_KEY_KP_CLEARENTRY,
	"KEY_KP_BINARY":          C.TRAP_INPUT_KEY_KP_BINARY,
	"KEY_KP_OCTAL":           C.TRAP_INPUT_KEY_KP_OCTAL,
	"KEY_KP_DECIMAL":         C.TRAP_INPUT_KEY_KP_DECIMAL,
	"KEY_KP_HEXADECIMAL":     C.TRAP_INPUT_KEY_KP_HEXADECIMAL,
	"KEY_LCTRL":              C.TRAP_INPUT_KEY_LCTRL,
	"KEY_LSHIFT":             C.TRAP_INPUT_KEY_LSHIFT,
	"KEY_LALT":               C.TRAP_INPUT_KEY_LALT,
	"KEY_LGUI":               C.TRAP_INPUT_KEY_LGUI,
	"KEY_RCTRL":              C.TRAP_INPUT_KEY_RCTRL,
	"KEY_RSHIFT":             C.TRAP_INPUT_KEY_RSHIFT,
	"KEY_RALT":               C.TRAP_INPUT_KEY_RALT,
	"KEY_RGUI":               C.TRAP_INPUT_KEY_RGUI,
	"KEY_MODE":               C.TRAP_INPUT_KEY_MODE,
	"KEY_AUDIONEXT":          C.TRAP_INPUT_KEY_AUDIONEXT,
	"KEY_AUDIOPREV":          C.TRAP_INPUT_KEY_AUDIOPREV,
	"KEY_AUDIOSTOP":          C.TRAP_INPUT_KEY_AUDIOSTOP,
	"KEY_AUDIOPLAY":          C.TRAP_INPUT_KEY_AUDIOPLAY,
	"KEY_AUDIOMUTE":          C.TRAP_INPUT_KEY_AUDIOMUTE,
	"KEY_MEDIASELECT":        C.TRAP_INPUT_KEY_MEDIASELECT,
	"KEY_WWW":                C.TRAP_INPUT_KEY_WWW,
	"KEY_MAIL":               C.TRAP_INPUT_KEY_MAIL,
	"KEY_CALCULATOR":         C.TRAP_INPUT_KEY_CALCULATOR,
	"KEY_COMPUTER":           C.TRAP_INPUT_KEY_COMPUTER,
	"KEY_AC_SEARCH":          C.TRAP_INPUT_KEY_AC_SEARCH,
	"KEY_AC_HOME":            C.TRAP_INPUT_KEY_AC_HOME,
	"KEY_AC_BACK":            C.TRAP_INPUT_KEY_AC_BACK,
	"KEY_AC_FORWARD":         C.TRAP_INPUT_KEY_AC_FORWARD,
	"KEY_AC_STOP":            C.TRAP_INPUT_KEY_AC_STOP,
	"KEY_AC_REFRESH":         C.TRAP_INPUT_KEY_AC_REFRESH,
	"KEY_AC_BOOKMARKS":       C.TRAP_INPUT_KEY_AC_BOOKMARKS,
	"KEY_BRIGHTNESSDOWN":     C.TRAP_INPUT_KEY_BRIGHTNESSDOWN,
	"KEY_BRIGHTNESSUP":       C.TRAP_INPUT_KEY_BRIGHTNESSUP,
	"KEY_DISPLAYSWITCH":      C.TRAP_INPUT_KEY_DISPLAYSWITCH,
	"KEY_KBDILLUMTOGGLE":     C.TRAP_INPUT_KEY_KBDILLUMTOGGLE,
	"KEY_KBDILLUMDOWN":       C.TRAP_INPUT_KEY_KBDILLUMDOWN,
	"KEY_KBDILLUMUP":         C.TRAP_INPUT_KEY_KBDILLUMUP,
	"KEY_EJECT":              C.TRAP_INPUT_KEY_EJECT,
	"KEY_SLEEP":              C.TRAP_INPUT_KEY_SLEEP,
	"KEY_APP1":               C.TRAP_INPUT_KEY_APP1,
	"KEY_APP2":               C.TRAP_INPUT_KEY_APP2,
	"KEY_AUDIOREWIND":        C.TRAP_INPUT_KEY_AUDIOREWIND,
	"KEY_AUDIOFASTFORWARD":   C.TRAP_INPUT_KEY_AUDIOFASTFORWARD,
	"KEY_SOFTLEFT":           C.TRAP_INPUT_KEY_SOFTLEFT,
	"KEY_SOFTRIGHT":          C.TRAP_INPUT_KEY_SOFTRIGHT,
	"KEY_CALL":               C.TRAP_INPUT_KEY_CALL,
	"KEY_ENDCALL":            C.TRAP_INPUT_KEY_ENDCALL,
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

	// Input traps
	"KEYPRESSED":             {1, 1},
	"KEY_A":                  {0, 1},
	"KEY_B":                  {0, 1},
	"KEY_C":                  {0, 1},
	"KEY_D":                  {0, 1},
	"KEY_E":                  {0, 1},
	"KEY_F":                  {0, 1},
	"KEY_G":                  {0, 1},
	"KEY_H":                  {0, 1},
	"KEY_I":                  {0, 1},
	"KEY_J":                  {0, 1},
	"KEY_K":                  {0, 1},
	"KEY_L":                  {0, 1},
	"KEY_M":                  {0, 1},
	"KEY_N":                  {0, 1},
	"KEY_O":                  {0, 1},
	"KEY_P":                  {0, 1},
	"KEY_Q":                  {0, 1},
	"KEY_R":                  {0, 1},
	"KEY_S":                  {0, 1},
	"KEY_T":                  {0, 1},
	"KEY_U":                  {0, 1},
	"KEY_V":                  {0, 1},
	"KEY_W":                  {0, 1},
	"KEY_X":                  {0, 1},
	"KEY_Y":                  {0, 1},
	"KEY_Z":                  {0, 1},
	"KEY_1":                  {0, 1},
	"KEY_2":                  {0, 1},
	"KEY_3":                  {0, 1},
	"KEY_4":                  {0, 1},
	"KEY_5":                  {0, 1},
	"KEY_6":                  {0, 1},
	"KEY_7":                  {0, 1},
	"KEY_8":                  {0, 1},
	"KEY_9":                  {0, 1},
	"KEY_0":                  {0, 1},
	"KEY_RETURN":             {0, 1},
	"KEY_ESCAPE":             {0, 1},
	"KEY_BACKSPACE":          {0, 1},
	"KEY_TAB":                {0, 1},
	"KEY_SPACE":              {0, 1},
	"KEY_MINUS":              {0, 1},
	"KEY_EQUALS":             {0, 1},
	"KEY_LEFTBRACKET":        {0, 1},
	"KEY_RIGHTBRACKET":       {0, 1},
	"KEY_BACKSLASH":          {0, 1},
	"KEY_SEMICOLON":          {0, 1},
	"KEY_APOSTROPHE":         {0, 1},
	"KEY_GRAVE":              {0, 1},
	"KEY_COMMA":              {0, 1},
	"KEY_PERIOD":             {0, 1},
	"KEY_SLASH":              {0, 1},
	"KEY_CAPSLOCK":           {0, 1},
	"KEY_F1":                 {0, 1},
	"KEY_F2":                 {0, 1},
	"KEY_F3":                 {0, 1},
	"KEY_F4":                 {0, 1},
	"KEY_F5":                 {0, 1},
	"KEY_F6":                 {0, 1},
	"KEY_F7":                 {0, 1},
	"KEY_F8":                 {0, 1},
	"KEY_F9":                 {0, 1},
	"KEY_F10":                {0, 1},
	"KEY_F11":                {0, 1},
	"KEY_F12":                {0, 1},
	"KEY_PRINTSCREEN":        {0, 1},
	"KEY_SCROLLLOCK":         {0, 1},
	"KEY_PAUSE":              {0, 1},
	"KEY_INSERT":             {0, 1},
	"KEY_HOME":               {0, 1},
	"KEY_PAGEUP":             {0, 1},
	"KEY_DELETE":             {0, 1},
	"KEY_END":                {0, 1},
	"KEY_PAGEDOWN":           {0, 1},
	"KEY_RIGHT":              {0, 1},
	"KEY_LEFT":               {0, 1},
	"KEY_DOWN":               {0, 1},
	"KEY_UP":                 {0, 1},
	"KEY_NUMLOCKCLEAR":       {0, 1},
	"KEY_KP_DIVIDE":          {0, 1},
	"KEY_KP_MULTIPLY":        {0, 1},
	"KEY_KP_MINUS":           {0, 1},
	"KEY_KP_PLUS":            {0, 1},
	"KEY_KP_ENTER":           {0, 1},
	"KEY_KP_1":               {0, 1},
	"KEY_KP_2":               {0, 1},
	"KEY_KP_3":               {0, 1},
	"KEY_KP_4":               {0, 1},
	"KEY_KP_5":               {0, 1},
	"KEY_KP_6":               {0, 1},
	"KEY_KP_7":               {0, 1},
	"KEY_KP_8":               {0, 1},
	"KEY_KP_9":               {0, 1},
	"KEY_KP_0":               {0, 1},
	"KEY_KP_PERIOD":          {0, 1},
	"KEY_NONUSBACKSLASH":     {0, 1},
	"KEY_APPLICATION":        {0, 1},
	"KEY_POWER":              {0, 1},
	"KEY_KP_EQUALS":          {0, 1},
	"KEY_F13":                {0, 1},
	"KEY_F14":                {0, 1},
	"KEY_F15":                {0, 1},
	"KEY_F16":                {0, 1},
	"KEY_F17":                {0, 1},
	"KEY_F18":                {0, 1},
	"KEY_F19":                {0, 1},
	"KEY_F20":                {0, 1},
	"KEY_F21":                {0, 1},
	"KEY_F22":                {0, 1},
	"KEY_F23":                {0, 1},
	"KEY_F24":                {0, 1},
	"KEY_EXECUTE":            {0, 1},
	"KEY_HELP":               {0, 1},
	"KEY_MENU":               {0, 1},
	"KEY_SELECT":             {0, 1},
	"KEY_STOP":               {0, 1},
	"KEY_AGAIN":              {0, 1},
	"KEY_UNDO":               {0, 1},
	"KEY_CUT":                {0, 1},
	"KEY_COPY":               {0, 1},
	"KEY_PASTE":              {0, 1},
	"KEY_FIND":               {0, 1},
	"KEY_MUTE":               {0, 1},
	"KEY_VOLUMEUP":           {0, 1},
	"KEY_VOLUMEDOWN":         {0, 1},
	"KEY_KP_COMMA":           {0, 1},
	"KEY_KP_EQUALSAS400":     {0, 1},
	"KEY_INTERNATIONAL1":     {0, 1},
	"KEY_INTERNATIONAL2":     {0, 1},
	"KEY_INTERNATIONAL3":     {0, 1},
	"KEY_INTERNATIONAL4":     {0, 1},
	"KEY_INTERNATIONAL5":     {0, 1},
	"KEY_INTERNATIONAL6":     {0, 1},
	"KEY_INTERNATIONAL7":     {0, 1},
	"KEY_INTERNATIONAL8":     {0, 1},
	"KEY_INTERNATIONAL9":     {0, 1},
	"KEY_LANG1":              {0, 1},
	"KEY_LANG2":              {0, 1},
	"KEY_LANG3":              {0, 1},
	"KEY_LANG4":              {0, 1},
	"KEY_LANG5":              {0, 1},
	"KEY_LANG6":              {0, 1},
	"KEY_LANG7":              {0, 1},
	"KEY_LANG8":              {0, 1},
	"KEY_LANG9":              {0, 1},
	"KEY_ALTERASE":           {0, 1},
	"KEY_SYSREQ":             {0, 1},
	"KEY_CANCEL":             {0, 1},
	"KEY_CLEAR":              {0, 1},
	"KEY_PRIOR":              {0, 1},
	"KEY_RETURN2":            {0, 1},
	"KEY_SEPARATOR":          {0, 1},
	"KEY_OUT":                {0, 1},
	"KEY_OPER":               {0, 1},
	"KEY_CLEARAGAIN":         {0, 1},
	"KEY_CRSEL":              {0, 1},
	"KEY_EXSEL":              {0, 1},
	"KEY_KP_00":              {0, 1},
	"KEY_KP_000":             {0, 1},
	"KEY_THOUSANDSSEPARATOR": {0, 1},
	"KEY_DECIMALSEPARATOR":   {0, 1},
	"KEY_CURRENCYUNIT":       {0, 1},
	"KEY_CURRENCYSUBUNIT":    {0, 1},
	"KEY_KP_LEFTPAREN":       {0, 1},
	"KEY_KP_RIGHTPAREN":      {0, 1},
	"KEY_KP_LEFTBRACE":       {0, 1},
	"KEY_KP_RIGHTBRACE":      {0, 1},
	"KEY_KP_TAB":             {0, 1},
	"KEY_KP_BACKSPACE":       {0, 1},
	"KEY_KP_A":               {0, 1},
	"KEY_KP_B":               {0, 1},
	"KEY_KP_C":               {0, 1},
	"KEY_KP_D":               {0, 1},
	"KEY_KP_E":               {0, 1},
	"KEY_KP_F":               {0, 1},
	"KEY_KP_XOR":             {0, 1},
	"KEY_KP_POWER":           {0, 1},
	"KEY_KP_PERCENT":         {0, 1},
	"KEY_KP_LESS":            {0, 1},
	"KEY_KP_GREATER":         {0, 1},
	"KEY_KP_AMPERSAND":       {0, 1},
	"KEY_KP_DBLAMPERSAND":    {0, 1},
	"KEY_KP_VERTICALBAR":     {0, 1},
	"KEY_KP_DBLVERTICALBAR":  {0, 1},
	"KEY_KP_COLON":           {0, 1},
	"KEY_KP_HASH":            {0, 1},
	"KEY_KP_SPACE":           {0, 1},
	"KEY_KP_AT":              {0, 1},
	"KEY_KP_EXCLAM":          {0, 1},
	"KEY_KP_MEMSTORE":        {0, 1},
	"KEY_KP_MEMRECALL":       {0, 1},
	"KEY_KP_MEMCLEAR":        {0, 1},
	"KEY_KP_MEMADD":          {0, 1},
	"KEY_KP_MEMSUBTRACT":     {0, 1},
	"KEY_KP_MEMMULTIPLY":     {0, 1},
	"KEY_KP_MEMDIVIDE":       {0, 1},
	"KEY_KP_PLUSMINUS":       {0, 1},
	"KEY_KP_CLEAR":           {0, 1},
	"KEY_KP_CLEARENTRY":      {0, 1},
	"KEY_KP_BINARY":          {0, 1},
	"KEY_KP_OCTAL":           {0, 1},
	"KEY_KP_DECIMAL":         {0, 1},
	"KEY_KP_HEXADECIMAL":     {0, 1},
	"KEY_LCTRL":              {0, 1},
	"KEY_LSHIFT":             {0, 1},
	"KEY_LALT":               {0, 1},
	"KEY_LGUI":               {0, 1},
	"KEY_RCTRL":              {0, 1},
	"KEY_RSHIFT":             {0, 1},
	"KEY_RALT":               {0, 1},
	"KEY_RGUI":               {0, 1},
	"KEY_MODE":               {0, 1},
	"KEY_AUDIONEXT":          {0, 1},
	"KEY_AUDIOPREV":          {0, 1},
	"KEY_AUDIOSTOP":          {0, 1},
	"KEY_AUDIOPLAY":          {0, 1},
	"KEY_AUDIOMUTE":          {0, 1},
	"KEY_MEDIASELECT":        {0, 1},
	"KEY_WWW":                {0, 1},
	"KEY_MAIL":               {0, 1},
	"KEY_CALCULATOR":         {0, 1},
	"KEY_COMPUTER":           {0, 1},
	"KEY_AC_SEARCH":          {0, 1},
	"KEY_AC_HOME":            {0, 1},
	"KEY_AC_BACK":            {0, 1},
	"KEY_AC_FORWARD":         {0, 1},
	"KEY_AC_STOP":            {0, 1},
	"KEY_AC_REFRESH":         {0, 1},
	"KEY_AC_BOOKMARKS":       {0, 1},
	"KEY_BRIGHTNESSDOWN":     {0, 1},
	"KEY_BRIGHTNESSUP":       {0, 1},
	"KEY_DISPLAYSWITCH":      {0, 1},
	"KEY_KBDILLUMTOGGLE":     {0, 1},
	"KEY_KBDILLUMDOWN":       {0, 1},
	"KEY_KBDILLUMUP":         {0, 1},
	"KEY_EJECT":              {0, 1},
	"KEY_SLEEP":              {0, 1},
	"KEY_APP1":               {0, 1},
	"KEY_APP2":               {0, 1},
	"KEY_AUDIOREWIND":        {0, 1},
	"KEY_AUDIOFASTFORWARD":   {0, 1},
	"KEY_SOFTLEFT":           {0, 1},
	"KEY_SOFTRIGHT":          {0, 1},
	"KEY_CALL":               {0, 1},
	"KEY_ENDCALL":            {0, 1},
}

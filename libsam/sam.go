// Bind libsam into Go.
package libsam

//#cgo LDFLAGS: -lm
//#cgo CFLAGS: -DSAM_DEBUG
//#cgo pkg-config: sdl2 SDL2_gfx
//#include "sam.h"
//#include "sam_opcodes.h"
//#include "traps_math.h"
//#include "traps_graphics.h"
import "C"
import "fmt"

type Word = C.sam_word_t
type Uword = C.sam_uword_t

var FLOAT_TAG = C.SAM_FLOAT_TAG
var FLOAT_TAG_MASK = C.SAM_FLOAT_TAG_MASK
var FLOAT_SHIFT = C.SAM_FLOAT_SHIFT
var INT_TAG = C.SAM_INT_TAG
var INT_TAG_MASK = C.SAM_INT_TAG_MASK
var INT_SHIFT = C.SAM_INT_SHIFT
var REF_TAG = C.SAM_REF_TAG
var REF_TAG_MASK = C.SAM_REF_TAG_MASK
var REF_SHIFT = C.SAM_REF_SHIFT
var ATOM_TAG = C.SAM_ATOM_TAG
var ATOM_TAG_MASK = C.SAM_ATOM_TAG_MASK
var ATOM_TYPE_MASK = C.SAM_ATOM_TYPE_MASK
var ATOM_TYPE_SHIFT = C.SAM_ATOM_TYPE_SHIFT
var ATOM_SHIFT = C.SAM_ATOM_SHIFT
var ARRAY_TAG = C.SAM_ARRAY_TAG
var ARRAY_TAG_MASK = C.SAM_ARRAY_TAG_MASK
var ARRAY_TYPE_MASK = C.SAM_ARRAY_TYPE_MASK
var ARRAY_OFFSET_SHIFT = C.SAM_ARRAY_OFFSET_SHIFT
var TRAP_TAG = C.SAM_TRAP_TAG
var TRAP_TAG_MASK = C.SAM_TRAP_TAG_MASK
var TRAP_FUNCTION_SHIFT = C.SAM_TRAP_FUNCTION_SHIFT
var INSTS_TAG = C.SAM_INSTS_TAG
var INSTS_TAG_MASK = C.SAM_INSTS_TAG_MASK
var INSTS_SHIFT = C.SAM_INSTS_SHIFT
var INST_MASK = C.SAM_INST_MASK
var INST_SHIFT = C.SAM_INST_SHIFT
var WORD_BIT = C.SAM_WORD_BIT
var WORD_MASK = Uword(((1 << C.SAM_WORD_BIT) - 1))

type Stack struct {
	stack *C.sam_stack_t
}

type Frame struct {
	frame *C.sam_frame_t
	Pc    Uword
	Stack Stack
}

func NewStack() Stack {
	return Stack{stack: C.sam_stack_new()}
}

func NewFrame() Frame {
	return Frame{frame: C.sam_frame_new()}
}

func (s *Stack) Sp() Uword {
	return s.stack.sp
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
	return int(C.sam_push_stack(s.stack, val))
}

func (s *Stack) PushRef(addr Uword) int {
	return int(C.sam_push_ref(s.stack, addr))
}

func (s *Stack) PushInt(val Uword) int {
	return int(C.sam_push_int(s.stack, val))
}

func (s *Stack) PushFloat(f float32) int {
	return int(C.sam_push_float(s.stack, C.sam_float_t(f)))
}

func (s *Stack) PushAtom(atomType Uword, operand Uword) int {
	return int(C.sam_push_atom(s.stack, atomType, operand))
}

func (s *Stack) PushTrap(function Uword) int {
	return int(C.sam_push_trap(s.stack, function))
}

func (s *Stack) PushCode(stack Stack) int {
	return int(C.sam_push_code(s.stack, stack.stack.s0, stack.stack.sp))
}

func (s *Stack) PushInsts(insts Uword) int {
	return int(C.sam_push_insts(s.stack, insts))
}

func Run(frame Frame) Word {
	frame.frame.pc = frame.Pc
	frame.frame.stack = frame.Stack.stack
	res := C.sam_run(frame.frame)
	frame.Pc = frame.frame.pc
	return res
}

func Init() Stack {
	stack := NewStack()
	if stack.stack == nil {
		panic("sam_stack_new returned NULL")
	}
	return stack
}

func DebugInit(stack Stack) int {
	return int(C.sam_debug_init(stack.stack))
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

func PrintStack() {
	C.sam_print_stack()
}

func DumpScreen(file string) {
	C.sam_dump_screen(C.CString(file))
}

const (
	ERROR_OK                 = C.SAM_ERROR_OK
	ERROR_HALT               = C.SAM_ERROR_HALT
	ERROR_INVALID_OPCODE     = C.SAM_ERROR_INVALID_OPCODE
	ERROR_INVALID_ADDRESS    = C.SAM_ERROR_INVALID_ADDRESS
	ERROR_STACK_UNDERFLOW    = C.SAM_ERROR_STACK_UNDERFLOW
	ERROR_STACK_OVERFLOW     = C.SAM_ERROR_STACK_OVERFLOW
	ERROR_WRONG_TYPE         = C.SAM_ERROR_WRONG_TYPE
	ERROR_BAD_BRACKET        = C.SAM_ERROR_BAD_BRACKET
	ERROR_INVALID_TRAP       = C.SAM_ERROR_INVALID_TRAP
	ERROR_TRAP_INIT          = C.SAM_ERROR_TRAP_INIT
	ERROR_NO_MEMORY          = C.SAM_ERROR_NO_MEMORY
	ERROR_INVALID_ARRAY_TYPE = C.SAM_ERROR_INVALID_ARRAY_TYPE
)

const (
	ARRAY_STACK = C.SAM_ARRAY_STACK
	ARRAY_RAW   = C.SAM_ARRAY_RAW
)

var errors = map[int]string{
	ERROR_OK:                 "ERROR_OK",
	ERROR_HALT:               "ERROR_HALT",
	ERROR_INVALID_OPCODE:     "ERROR_INVALID_OPCODE",
	ERROR_INVALID_ADDRESS:    "ERROR_INVALID_ADDRESS",
	ERROR_STACK_UNDERFLOW:    "ERROR_STACK_UNDERFLOW",
	ERROR_STACK_OVERFLOW:     "ERROR_STACK_OVERFLOW",
	ERROR_WRONG_TYPE:         "ERROR_WRONG_TYPE",
	ERROR_BAD_BRACKET:        "ERROR_BAD_BRACKET",
	ERROR_INVALID_TRAP:       "ERROR_INVALID_TRAP",
	ERROR_TRAP_INIT:          "ERROR_TRAP_INIT",
	ERROR_NO_MEMORY:          "ERROR_NO_MEMORY",
	ERROR_INVALID_ARRAY_TYPE: "ERROR_INVALID_ARRAY_TYPE",
}

func ErrorMessage(code Word) string {
	baseCode := code & C.SAM_RET_MASK
	if baseCode >= ERROR_OK && baseCode <= ERROR_TRAP_INIT {
		reasonCode := code >> C.SAM_RET_SHIFT
		if baseCode == ERROR_HALT {
			return fmt.Sprintf("halt with reason %d (0x%x)", reasonCode, uint(reasonCode))
		}
		return fmt.Sprintf(errors[int(baseCode)])
	}
	return fmt.Sprintf("unknown code %d (0x%x)", code, uint(Uword(code)&WORD_MASK))
}

type InstOpcode struct {
	Tag      Word
	Opcode   Uword
	Terminal bool
}

var Instructions = map[string]InstOpcode{
	"ref":   {C.SAM_REF_TAG, 0, true},
	"int":   {C.SAM_INT_TAG, 0, true},
	"float": {C.SAM_FLOAT_TAG, 0, true},
	"trap":  {C.SAM_TRAP_TAG, 0, true},

	// Instructions
	"nop":     {C.SAM_INSTS_TAG, C.INST_NOP, false},
	"pop":     {C.SAM_INSTS_TAG, C.INST_POP, false},
	"get":     {C.SAM_INSTS_TAG, C.INST_GET, false},
	"set":     {C.SAM_INSTS_TAG, C.INST_SET, false},
	"extract": {C.SAM_INSTS_TAG, C.INST_EXTRACT, false},
	"insert":  {C.SAM_INSTS_TAG, C.INST_INSERT, false},
	"do":      {C.SAM_INSTS_TAG, C.INST_DO, true},
	"if":      {C.SAM_INSTS_TAG, C.INST_IF, true},
	"while":   {C.SAM_INSTS_TAG, C.INST_WHILE, false},
	"go":      {C.SAM_INSTS_TAG, C.INST_GO, true},
	"not":     {C.SAM_INSTS_TAG, C.INST_NOT, false},
	"and":     {C.SAM_INSTS_TAG, C.INST_AND, false},
	"or":      {C.SAM_INSTS_TAG, C.INST_OR, false},
	"xor":     {C.SAM_INSTS_TAG, C.INST_XOR, false},
	"lsh":     {C.SAM_INSTS_TAG, C.INST_LSH, false},
	"rsh":     {C.SAM_INSTS_TAG, C.INST_RSH, false},
	"arsh":    {C.SAM_INSTS_TAG, C.INST_ARSH, false},
	"eq":      {C.SAM_INSTS_TAG, C.INST_EQ, false},
	"lt":      {C.SAM_INSTS_TAG, C.INST_LT, false},
	"neg":     {C.SAM_INSTS_TAG, C.INST_NEG, false},
	"add":     {C.SAM_INSTS_TAG, C.INST_ADD, false},
	"mul":     {C.SAM_INSTS_TAG, C.INST_MUL, false},
	"div":     {C.SAM_INSTS_TAG, C.INST_DIV, false},
	"rem":     {C.SAM_INSTS_TAG, C.INST_REM, false},
	"i2f":     {C.SAM_INSTS_TAG, C.INST_I2F, false},
	"f2i":     {C.SAM_INSTS_TAG, C.INST_F2I, false},
	"zero":    {C.SAM_INSTS_TAG, C.INST_0, false},
	"false":   {C.SAM_INSTS_TAG, C.INST_0, false},
	"one":     {C.SAM_INSTS_TAG, C.INST_1, false},
	"_one":    {C.SAM_INSTS_TAG, C.INST_MINUS_1, false},
	"true":    {C.SAM_INSTS_TAG, C.INST_MINUS_1, false},
	"two":     {C.SAM_INSTS_TAG, C.INST_2, false},
	"_two":    {C.SAM_INSTS_TAG, C.INST_MINUS_2, false},
	"halt":    {C.SAM_INSTS_TAG, C.INST_HALT, true},
}

var Traps = map[string]int{
	"POW": C.TRAP_MATH_POW,
	"SIN": C.TRAP_MATH_SIN,
	"COS": C.TRAP_MATH_COS,
	"DEG": C.TRAP_MATH_DEG,
	"RAD": C.TRAP_MATH_RAD,

	"BLACK":          C.TRAP_GRAPHICS_BLACK,
	"WHITE":          C.TRAP_GRAPHICS_WHITE,
	"DISPLAY_WIDTH":  C.TRAP_GRAPHICS_DISPLAY_WIDTH,
	"DISPLAY_HEIGHT": C.TRAP_GRAPHICS_DISPLAY_HEIGHT,
	"CLEARSCREEN":    C.TRAP_GRAPHICS_CLEARSCREEN,
	"SETDOT":         C.TRAP_GRAPHICS_SETDOT,
	"DRAWLINE":       C.TRAP_GRAPHICS_DRAWLINE,
	"DRAWRECT":       C.TRAP_GRAPHICS_DRAWRECT,
	"DRAWROUNDRECT":  C.TRAP_GRAPHICS_DRAWROUNDRECT,
	"FILLRECT":       C.TRAP_GRAPHICS_FILLRECT,
	"DRAWCIRCLE":     C.TRAP_GRAPHICS_DRAWCIRCLE,
	"FILLCIRCLE":     C.TRAP_GRAPHICS_FILLCIRCLE,
	"DRAWBITMAP":     C.TRAP_GRAPHICS_DRAWBITMAP,
}

// The net change in `SP` caused by each instruction.
// If there is more than one possibility, then it's the change when execution
// continues at the next instruction.
var StackDifference = map[string]int{
	// Tag instructions
	"ref": 1,

	// Atom instructions
	"int":   1,
	"float": 1,

	// Instructions without immediate operand
	"nop":     0,
	"pop":     -1,
	"get":     0,
	"set":     -2,
	"extract": -1,
	"insert":  -1,
	"do":      -1,
	"if":      -3,
	"while":   -1,
	"go":      -1,
	"not":     0,
	"and":     -1,
	"or":      -1,
	"xor":     -1,
	"lsh":     -1,
	"rsh":     -1,
	"arsh":    -1,
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

	"halt": -1,

	"i2f": 0,
	"f2i": 0,
	"div": -1,
	"rem": -1,
	"pow": -1,
	"sin": 0,
	"cos": 0,
	"deg": 0,
	"rad": 0,
	// Trap depends on the trap code.
}

type StackEffect struct {
	In  Uword
	Out Uword
}

var TrapStackEffect = map[string]StackEffect{
	"BLACK":          {0, 1},
	"WHITE":          {0, 1},
	"DISPLAY_WIDTH":  {0, 1},
	"DISPLAY_HEIGHT": {0, 1},
	"CLEARSCREEN":    {1, 0},
	"SETDOT":         {3, 0},
	"DRAWLINE":       {5, 0},
	"DRAWRECT":       {5, 0},
	"DRAWROUNDRECT":  {6, 0},
	"FILLRECT":       {5, 0},
	"DRAWCIRCLE":     {4, 0},
	"FILLCIRCLE":     {4, 0},
	"DRAWBITMAP":     {4, 0},
}

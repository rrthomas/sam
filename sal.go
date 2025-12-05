//nolint:govet
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"strings"

	"github.com/alecthomas/participle/v2"
	"github.com/alecthomas/participle/v2/lexer"
	"github.com/goccy/go-yaml"
)

var cli struct {
	AST        bool               `help:"Print AST for expression."`
	Set        map[string]float64 `short:"s" help:"Set variables."`
	Expression []string           `arg required help:"Expression to evaluate."`
}

type Operator int

const (
	OpMul Operator = iota
	OpDiv
	OpAdd
	OpSub
)

var operatorMap = map[string]Operator{"+": OpAdd, "-": OpSub, "*": OpMul, "/": OpDiv}

func (o *Operator) Capture(s []string) error {
	*o = operatorMap[s[0]]
	return nil
}

// E --> T {( "+" | "-" ) T}
// T --> F {( "*" | "/" ) F}
// F --> P ["^" F]
// P --> v | "(" E ")" | "-" T

type Value struct {
	Pos           lexer.Position
	Number        *float64    `  @(Float|Int)`
	Variable      *string     `| @Ident`
	Subexpression *Expression `| "(" @@ ")"`
}

type Factor struct {
	Pos      lexer.Position
	Base     *Value `@@`
	Exponent *Value `( "^" @@ )?`
}

type OpFactor struct {
	Pos      lexer.Position
	Operator Operator `@("*" | "/")`
	Factor   *Factor  `@@`
}

type Term struct {
	Pos   lexer.Position
	Left  *Factor     `@@`
	Right []*OpFactor `@@*`
}

type OpTerm struct {
	Pos      lexer.Position
	Operator Operator `@("+" | "-")`
	Term     *Term    `@@`
}

type Expression struct {
	Pos   lexer.Position
	Left  *Term     `@@`
	Right []*OpTerm `@@*`
}

// Display

func (o Operator) String() string {
	switch o {
	case OpMul:
		return "*"
	case OpDiv:
		return "/"
	case OpSub:
		return "-"
	case OpAdd:
		return "+"
	}
	panic("unsupported operator")
}

func (v *Value) String() string {
	if v.Number != nil {
		return fmt.Sprintf("float %g", *v.Number)
	}
	if v.Variable != nil {
		return *v.Variable
	}
	return "(" + v.Subexpression.String() + ")"
}

func (f *Factor) String() string {
	out := f.Base.String()
	if f.Exponent != nil {
		out += " ^ " + f.Exponent.String()
	}
	return out
}

func (o *OpFactor) String() string {
	return fmt.Sprintf("%s %s", o.Operator, o.Factor)
}

func (t *Term) String() string {
	out := []string{t.Left.String()}
	for _, r := range t.Right {
		out = append(out, r.String())
	}
	return strings.Join(out, " ")
}

func (o *OpTerm) String() string {
	return fmt.Sprintf("%s %s", o.Operator, o.Term)
}

func (e *Expression) String() string {
	out := []string{e.Left.String()}
	for _, r := range e.Right {
		out = append(out, r.String())
	}
	return strings.Join(out, " ")
}

// Compilation

func (o Operator) Compile(ctx *Context) {
	switch o {
	case OpMul:
		ctx.asm = append(ctx.asm, "mul")
	case OpDiv:
		ctx.asm = append(ctx.asm, "div")
	case OpSub:
		ctx.asm = append(ctx.asm, "neg, add")
	case OpAdd:
		ctx.asm = append(ctx.asm, "add")
	default:
		panic("unsupported operator")
	}
}

func (v *Value) Compile(ctx *Context) {
	if v.Number != nil {
		ctx.asm = append(ctx.asm, fmt.Sprintf("float %g", *v.Number))
	}
	if v.Variable != nil {
		ctx.asm = append(ctx.asm, *v.Variable)
	}
	v.Subexpression.Compile(ctx)
}

func (f *Factor) Compile(ctx *Context) {
	out := f.Base.String()
	if f.Exponent != nil {
		out += " ^ " + f.Exponent.String()
	}
	ctx.asm = append(ctx.asm, out)
}

func (o *OpFactor) Compile(ctx *Context) {
	o.Factor.Compile(ctx)
	o.Operator.Compile(ctx)
}

func (t *Term) Compile(ctx *Context) {
	t.Left.Compile(ctx)
	for _, r := range t.Right {
		r.Compile(ctx)
	}
}

func (o *OpTerm) Compile(ctx *Context) {
	o.Term.Compile(ctx)
	o.Operator.Compile(ctx)
}

func (e *Expression) Compile(ctx *Context) {
	e.Left.Compile(ctx)
	for _, r := range e.Right {
		r.Compile(ctx)
	}
}

type Context struct {
	labels map[string]float64
	asm    []string
}

var parser = participle.MustBuild[Expression]()

func Sal(src string, ast bool) []byte {
	expr, err := parser.ParseString("", src)
	if err != nil {
		panic(fmt.Errorf("error in source %v", err))
	}

	if ast {
		if err := json.NewEncoder(os.Stdout).Encode(expr); err != nil {
			panic("error encoding JSON for AST")
		}
	}
	ctx := Context{labels: cli.Set, asm: make([]string, 0)}
	expr.Compile(&ctx)
	// fmt.Println(ctx.asm)
	ctx.asm = append(ctx.asm, "int 0", "halt")
	yaml, err := yaml.Marshal(ctx.asm)
	if err != nil {
		panic("error encoding YAML for compilation output")
	}
	return yaml
}

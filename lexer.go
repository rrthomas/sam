package main

import (
	"io"

	"github.com/alecthomas/participle/v2"
	"github.com/alecthomas/participle/v2/lexer"
)

var (
	lex = lexer.MustStateful(lexer.Rules{
		"Root": {
			{"comment", `#.*`, nil},
			{"backslash", `\\`, nil},
			{"whitespace", `[\r\t ]+`, nil},

			{"Keyword", `\b(if|fn|loop|then|else|break|continue|return)\b`, nil},
			{"Ident", `\b([[:alpha:]_]\w*)\b`, nil},
			{"Int", `\b(\d+)\b`, nil},
			{"Float", `\b(\d+\.\d+?)\b`, nil},
			{"String", `"`, lexer.Push("String")},
			{"LiteralString", `` + "`.*?`|'.*?'" + ``, nil},
			{"Newline", `\n`, nil},
			{"Operator", `->|%=|>=|<=|&&|\|\||==|!=`, nil},
			{"Assignment", `(\^=|\+=|-=|\*=|/=|\|=|&=|%=|=)`, nil},
			{"SingleOperator", `[-+*/<>%^!|&]`, nil},
			{"Punct", `[]` + "`" + `~[()@#${}:;?.,]`, nil},
		},
		"String": {
			{"Escaped", `\\.`, nil},
			{"StringEnd", `"`, lexer.Pop()},
			{"Expr", `{`, lexer.Push("StringExpr")},
			{"Chars", `[^{"\\]+`, nil},
		},
		"StringExpr": {
			{"ExprEnd", `}`, lexer.Pop()},
			lexer.Include("Root"),
		},
	})
	parser = participle.MustBuild[Body](
		participle.Lexer(&fixupLexerDefinition{}),
		participle.UseLookahead(1),
		unquoteLiteral(),
	)

	identToken     = lex.Symbols()["Ident"]
	stringEndToken = lex.Symbols()["StringEnd"]
	intToken       = lex.Symbols()["Int"]
	floatToken     = lex.Symbols()["Float"]
)

func unquoteLiteral() participle.Option {
	return participle.Map(func(token lexer.Token) (lexer.Token, error) {
		token.Value = token.Value[1 : len(token.Value)-1]
		return token, nil
	}, "LiteralString")
}

// A Lexer that inserts semi-colons and collapses \-separated lines.
type fixupLexerDefinition struct{}

func (l *fixupLexerDefinition) Lex(path string, r io.Reader) (lexer.Lexer, error) { // nolint: golint
	ll, err := lex.Lex(path, r)
	if err != nil {
		return nil, err
	}
	return &fixupLexer{lexer: ll}, nil
}

func (l *fixupLexerDefinition) Symbols() map[string]lexer.TokenType { // nolint: golint
	return lex.Symbols()
}

type fixupLexer struct {
	lexer lexer.Lexer
	last  lexer.Token
	extra *lexer.Token
}

func (l *fixupLexer) Next() (lexer.Token, error) {
next:
	for {
		if l.extra != nil {
			token := *l.extra
			l.extra = nil
			l.last = token
			return token, nil
		}

		token, err := l.lexer.Next()
		if err != nil {
			return token, err
		}
		if token.Value == "\n" {
			// Do we need to replace the newline with a semi-colon?
			switch l.last.Value {
			case "\\":
				l.last = token
				continue next

			case ")", "}", "]":
				token.Value = ";"
				token.Type = ';'

			default:
				switch l.last.Type {
				case intToken, floatToken, stringEndToken, identToken:
					token.Value = ";"
					token.Type = ';'

				default:
					l.last = token
					continue next
				}
			}
		} else if token.Value == "}" {
			// Do we need to insert a semi-colon?
			if l.last.Value != ";" {
				l.extra = &lexer.Token{Value: token.Value, Type: token.Type}
				token.Value = ";"
				token.Type = ';'
			}
		}
		l.last = token
		return token, nil
	}
}

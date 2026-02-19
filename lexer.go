/*
SAL lexer

Copyright © 2025-2026 Reuben Thomas <rrt@sc3d.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
package main

import (
	"io"

	"github.com/alecthomas/participle/v2"
	"github.com/alecthomas/participle/v2/lexer"
)

var (
	lex = lexer.MustStateful(lexer.Rules{
		"Root": {
			{Name: "comment", Pattern: `//.*`, Action: nil},
			{Name: "backslash", Pattern: `\\`, Action: nil},
			{Name: "whitespace", Pattern: `[\r\t ]+`, Action: nil},

			{Name: "Keyword", Pattern: `\b(if|fn|loop|then|else|break|continue|return)\b`, Action: nil},
			{Name: "Ident", Pattern: `\b([[:alpha:]_]\w*)\b`, Action: nil},
			{Name: "Int", Pattern: `\b(\d+)\b`, Action: nil},
			{Name: "Float", Pattern: `\b(\d+\.\d+?)\b`, Action: nil},
			{Name: "String", Pattern: `"`, Action: lexer.Push("String")},
			{Name: "LiteralString", Pattern: `` + "`.*?`|'.*?'" + ``, Action: nil},
			{Name: "Newline", Pattern: `\n`, Action: nil},
			{Name: "Operator", Pattern: `->|%=|>=|<=|&&|\|\||==|!=`, Action: nil},
			{Name: "Assignment", Pattern: `=|:=`, Action: nil},
			{Name: "SingleOperator", Pattern: `[-+*/<>%^!|&]`, Action: nil},
			{Name: "Punct", Pattern: `[]` + "`" + `~[()@#${}:;?.,]`, Action: nil},
		},
		"String": {
			{Name: "Escaped", Pattern: `\\.`, Action: nil},
			{Name: "StringEnd", Pattern: `"`, Action: lexer.Pop()},
			{Name: "Expr", Pattern: `{`, Action: lexer.Push("StringExpr")},
			{Name: "Chars", Pattern: `[^{"\\]+`, Action: nil},
		},
		"StringExpr": {
			{Name: "ExprEnd", Pattern: `}`, Action: lexer.Pop()},
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
			case ";":
				continue next

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

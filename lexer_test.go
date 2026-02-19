/*
Tests for SAL lexer

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
	"strings"
	"testing"

	"github.com/alecthomas/assert/v2"
)

func TestLexer(t *testing.T) {
	tokens, err := parser.Lex("", strings.NewReader(`
	// Comment
	fn foo() {
		if true {
			print("hello")
		}
		a := 1 + \
			 2
		b := `+"`literal string`"+`
	}
	`))
	assert.NoError(t, err)
	actual := []string{}
	for _, token := range tokens {
		if token.Type == parser.Lexer().Symbols()["Whitespace"] {
			continue
		}
		actual = append(actual, token.Value)
	}
	expected := []string{
		"fn", "foo", "(", ")", "{", "if", "true", "{", "print", "(", "\"", "hello", "\"", ")", ";", "}", ";", "a", ":=",
		"1", "+", "2", ";", "b", ":=", "literal string", ";", "}", ";", "",
	}
	assert.Equal(t, expected, actual)
}

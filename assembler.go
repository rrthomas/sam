/*
Copyright © 2025 Reuben Thomas <rrt@sc3d.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
package main

import (
	"errors"
	"fmt"
	"io"
	"os"

	"github.com/goccy/go-yaml"
	"github.com/goccy/go-yaml/ast"
)

func check(e error) {
	if e != nil {
		panic(e)
	}
}

// Read the program from a YAML file
func readProg(progFile string) ast.Node {
	r, err := os.Open(progFile)
	check(err)
	dec := yaml.NewDecoder(r)

	var doc ast.Node
	if err := dec.Decode(&doc); err == io.EOF {
		panic(errors.New("input was empty"))
	}

	// Check that there was only one document in the file
	var eofDoc ast.Node
	if err := dec.Decode(&eofDoc); err != io.EOF {
		panic(errors.New("only one program allowed at a time"))
	}

	return doc
}

// Assemble a program
type assembler struct {
}

func (a assembler) Visit(n ast.Node) ast.Visitor {
	print(fmt.Sprintf("visiting %+v of type %+v\n", n, n.Type()))
	return a
}

func Assemble(progFile string) {
	prog := readProg(progFile)
	ast.Walk(assembler{}, prog)
}

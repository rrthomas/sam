/*
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
	// "log"

	"fmt"
	"os"
	"path/filepath"

	"github.com/rrthomas/sam/libsam"
	"github.com/spf13/cobra"
)

func main() {
	Execute()
}

const STACK_WORDS = 4096

// rootCmd represents the base command when called without any subcommands
var rootCmd = &cobra.Command{
	Use:     "sam",
	Version: "0.1",
	Short:   "SAM, the Super-Awesome Machine",
	Long:    "A simple virtual machine and run time for playful low-level programming.",
	Args:    cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		state := libsam.NewState()
		libsam.SetDebug(debug)
		progFile := args[0]

		var yaml []byte
		{
			ext := filepath.Ext(progFile)
			var err error
			switch ext {
			case ".yaml":
				yaml, err = os.ReadFile(progFile)

			case ".sal":
				var source []byte
				if source, err = os.ReadFile(progFile); err == nil {
					yaml = Sal(string(source), printAst, printAsm)
				}

			default:
				return fmt.Errorf("unknown program file type %v", ext)
			}
			if err != nil {
				return fmt.Errorf("error reading program %v", progFile)
			}
		}

		// Assemble and run the program
		Assemble(state, yaml)
		err := libsam.GraphicsInit()
		if err != libsam.ERROR_OK {
			os.Exit(int(err))
		}
		res := libsam.DebugInit(state)
		if res != libsam.ERROR_OK {
			libsam.GraphicsFinish()
			os.Exit(int(res))
		}
		code := state.Code()
		code.PrintStack()
		res2 := libsam.Run(state)

		if debug {
			stack := state.Stack()
			stack.PrintStack()
			fmt.Printf("sam_run returns: %s\n", libsam.ErrorMessage(res2))
		}

		if libsam.GraphicsWindowUsed() {
			if pbmFile != "" {
				libsam.DumpScreen(pbmFile)
			}
		}

		if wait && libsam.GraphicsWindowUsed() {
			libsam.GraphicsWait()
		}

		libsam.GraphicsFinish()
		os.Exit(int(err))

		return nil
	},
}

var (
	debug    bool
	wait     bool
	printAst bool
	printAsm bool
	pbmFile  string
)

// Execute adds all child commands to the root command and sets flags appropriately.
// This is called by main.main(). It only needs to happen once to the rootCmd.
func Execute() {
	err := rootCmd.Execute()
	if err != nil {
		os.Exit(1)
	}
}

func init() {
	rootCmd.SetUsageTemplate(`Usage: {{.CommandPath}} [OPTION...] PROGRAM
	
{{.Flags.FlagUsages}}`)
	rootCmd.Flags().BoolVar(&debug, "debug", false, "output debug information to standard error")
	rootCmd.Flags().BoolVar(&wait, "wait", false, "wait for user to close window on termination")
	rootCmd.Flags().BoolVar(&printAst, "ast", false, "print SAL abstract syntax tree")
	rootCmd.Flags().BoolVar(&printAsm, "asm", false, "print SAM assembler for SAL program")
	rootCmd.Flags().StringVar(&pbmFile, "dump-screen", "", "output screen to PBM file `FILE`")
	rootCmd.SetVersionTemplate(`{{.DisplayName}} {{.Version}}

Copyright (C) 2025-2026 Reuben Thomas <rrt@sc3d.org>

Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.`)
}

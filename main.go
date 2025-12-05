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
	// "log"

	"fmt"
	"os"

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
	Run: func(cmd *cobra.Command, args []string) {
		libsam.Init()
		libsam.SetDebug(debug)
		prog_file := args[0]
		Assemble(prog_file)
		err := libsam.TrapsInit()
		if err != libsam.ERROR_OK {
			os.Exit(int(err))
		}
		res := libsam.DebugInit()
		if res != libsam.ERROR_OK {
			libsam.TrapsFinish()
			os.Exit(int(res))
		}
		libsam.PrintStack()
		res2 := libsam.Run(1)

		if debug {
			libsam.PrintStack()
			fmt.Printf("sam_run returns: %s\n", libsam.ErrorMessage(res2))
		}

		if libsam.TrapsWindowUsed() {
			if pbmFile != "" {
				libsam.DumpScreen(pbmFile)
			}
		}

		if wait && libsam.TrapsWindowUsed() {
			libsam.TrapsWait()
		}

		libsam.TrapsFinish()
		os.Exit(int(err))
	},
}

var (
	debug   bool
	wait    bool
	pbmFile string
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
	rootCmd.SetUsageTemplate(`Usage: {{.CommandPath}} [OPTION...] PROGRAM.yaml
	
{{.Flags.FlagUsages}}`)
	rootCmd.Flags().BoolVar(&debug, "debug", false, "output debug information to standard error")
	rootCmd.Flags().BoolVar(&wait, "wait", false, "wait for user to close window on termination")
	rootCmd.Flags().StringVar(&pbmFile, "dump-screen", "", "output screen to PBM file `FILE`")
	rootCmd.SetVersionTemplate(`{{.DisplayName}} {{.Version}}

Copyright (C) 2025 Reuben Thomas <rrt@sc3d.org>

Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.`)
}

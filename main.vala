#! /usr/bin/env -S vala --vapidir=. --vapidir=./vala-extra-vapis --pkg config --pkg sdl2 --pkg yaml-0.1 --pkg sam
// SAM front end.
//
// (c) Reuben Thomas 2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

using Config;

using Yaml;

using Sam;

// yaml.add_constructor("!include", yaml_include.Constructor(base_dir="."))

// def instruction(inst, op=None):
//	s = f"SAM_INSN_{inst.upper()}"
//	if op is not None:
//		s = f"LSHIFT({op}, SAM_OP_SHIFT) | {s}"
//	return s


// def compile_inst(inst, level, pc):
//	labels = {}

//	if isinstance(inst, dict):  # label
//		assert len(inst) == 1
//		(label, inst), *_ = inst.items()
//		labels[label] = pc

//	if isinstance(inst, list):  # sub-routine
// words, sub_labels, nwords = compile_list(inst, level + 1, pc + 1)
//		words.insert(0, indented(instruction("bra", nwords), level))
//		words.append(indented(instruction("ket", nwords), level))
//		labels.update(sub_labels)
//		return words, labels, nwords + 2

//	toks = inst.split(" ")
//	inst = toks[0]
//	operand = None

//	if inst in ("int", "link", "float", "trap", "push"):
//		if len(toks) != 2:
//			raise ValueError(f"{inst} needs an operand")
//		operand = toks[1]

//		if inst in ("int", "link"):
//			return [indented(instruction(inst, operand), level)], labels, 1
//		elif inst == "float":
//			f = struct.pack("!f", float(operand)).hex()
//			return (
//				[
//					indented(instruction("float", f"0x{f} >> SAM_OP_SHIFT"), level),
//					indented(instruction("_float", f"0x{f} & SAM_OP_MASK"), level),
//				],
//				labels,
//				2,
//			)
//		elif inst == "trap":
//			return [indented(instruction("trap", f"TRAP_{operand}"), level)], labels, 1
//		else:  # inst == "push"
//			return (
//				[
//					indented(instruction("push", f"{operand} >> SAM_OP_SHIFT"), level),
//					indented(instruction("_push", f"{operand} & SAM_OP_MASK"), level),
//				],
//				labels,
//				2,
//			)
//	else:
//		if len(toks) != 1:
//			raise ValueError(f"unexpected operand for {inst}")
//		return [indented(instruction(inst), level)], labels, 1


// def compile_list(l, level=0, pc0=0):
//	labels, words = {}, []
//	pc = pc0
//	for inst in l:
//		sub_words, sub_labels, n_sub_words = compile_inst(inst, level, pc)
//		words.extend(sub_words)
//		labels.update(sub_labels)
//		pc += n_sub_words
//	return words, labels, pc - pc0


// def compile(program_file: str) -> tuple[list[str], int]:
//	prog = yaml.full_load(open(program_file).read())

//	c_source = []
//	words, labels, n_words = compile_list(prog)

//	for label, n in labels.items():
//		c_source.append(f"#define {label} {n}")
//	for word in words:
//		c_source.append(word)

//	c_source.append(indented(instruction("link", 1)))
//	c_source.append(indented(instruction("link", 1)))

//	return c_source, n_words + 2


// PROGRAM = sys.argv[1]

// c_source, program_words = compile(PROGRAM)

// with open(PROGRAM_SOURCE, "w") as fh:
//	fh.write("""\
// #include "sam.h"
// #include "sam_opcodes.h"
// #include "sam_traps.h"
// #include "sam_private.h"

// sam_word_t SAM_PROGRAM[SAM_STACK_WORDS] = {
// """)
//	fh.write("\n".join(c_source))
//	fh.write("};")

// subprocess.check_call(
//	[
//		"gcc",
//		"-Wall",
//		"-Wextra",
//		"-g",
//		"-D_GNU_SOURCE",
//		"-DSAM_DEBUG",
//		"-o",
//		TEST_EXE,
//		"sam_main.c",
//		PROGRAM_SOURCE,
//		f"-DSAM_PROGRAM_LEN={program_words}",
//		"-L",
//		".libs",
//		"-lsam",
//		"-lm",
//		"-lSDL2",
//		"-lSDL2_gfx",
//	]
// )

public class Main : Object {
	private static bool version = false;
	private static bool debug_flag = false;
	private static bool wait_flag = false;
	private static string? pbm_file = null;

	private const GLib.OptionEntry[] options = {
		{ "version", '\0', OptionFlags.NONE, OptionArg.NONE, ref version, "Print version and exit", null },

		{ "debug", '\0', OptionFlags.NONE, OptionArg.NONE, ref debug_flag, "output debug information to standard error", null },
		{ "wait", '\0', OptionFlags.NONE, OptionArg.NONE, ref wait_flag, "wait for user to close window on termination", null },
		{ "dump-screen", '\0', OptionFlags.NONE, OptionArg.FILENAME, ref pbm_file, "display version information and exit", "FILE" },
		// list terminator
		{ null }
	};

	public static int main (string[] args) {
		try {
			var opt_context = new OptionContext ("- Run SAM, the Super-Awesome Machine.");
			opt_context.set_help_enabled (true);
			opt_context.add_main_entries (options, null);
			opt_context.parse (ref args);
		} catch (OptionError e) {
			printerr ("error: %s\n", e.message);
			printerr ("Run '%s --help' to see a full list of available command line options.\n", args[0]);
			return 1;
		}

		if (version) {
			print (@"$PACKAGE_STRING\n");
			return 0;
		}

		word_t error = traps_init ();
		if (error != Sam.Error.OK) {
			return error;
		}
		int res = init (PROGRAM, STACK_WORDS, PROGRAM_LEN);
		if (res != Sam.Error.OK) {
			traps_finish ();
			return res;
		}
		print_stack ();
		res = sam_run ();

		if (debug_flag) {
			Sam.debug ("sam_run returns %d\n", res);
			if (traps_window_used ())
				if (pbm_file != null)
					dump_screen (pbm_file);
		}

		if (wait_flag && traps_window_used ()) {
			while (traps_process_events () == 0) {
				SDL.Timer.delay (update_interval);
			}
		}

		traps_finish ();
		return error;
	}
}

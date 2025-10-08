/* Table of command-line options  */

/*
 * O: Option
 *
 * O(long name, argument, argument docstring, docstring)
 */

O ("debug", no_argument, "", "output debug information to standard error")
O ("wait", no_argument, "", "wait for user to close window on termination")
O ("dump-screen", required_argument, "FILE", "output screen to PBM file FILE")
O ("help", optional_argument, "", "display this help message and exit")
O ("version", optional_argument, "", "display version information and exit")

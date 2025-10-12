// SAM main routine.
//
// (c) Reuben Thomas 1994-2025
//
// The package is distributed under the GNU Public License version 3, or,
// at your option, any later version.
//
// THIS PROGRAM IS PROVIDED AS IS, WITH NO WARRANTY. USE IS AT THE USER’S
// RISK.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <SDL2/SDL_timer.h>

#include "sam.h"
#include "sam_opcodes.h"
#include "sam_traps.h"
#include "sam_private.h"

#define PACKAGE_NAME "SAM"
#define VERSION "0.1"
#define PACKAGE_BUGREPORT "rrt@sc3d.org"

#define SAM_COPYRIGHT_STRING                    \
    "Copyright (C) 2020-2025 Reuben Thomas."
#define SAM_VERSION_STRING                      \
    PACKAGE_NAME " " VERSION

/* Options table */
struct option longopts[] = {
#define O(longname, arg, argstring, docstring)  \
    {longname, arg, NULL, '\0'},
#include "tbl_opts.h"
#undef O
    {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
    char *screen_pbm_file = NULL;
    bool do_wait = false;

    for (;;) {
        int this_optind = optind ? optind : 1, longindex = -1, c;

        /* Leading `:' so as to return ':' for a missing arg, not '?' */
        c = getopt_long (argc, argv, ":", longopts, &longindex);

        if (c == -1)
            break;
        else if (c == '?') { /* Unknown option */
            fprintf(stderr, "Unknown option `%s'", argv[this_optind]);
            exit(EXIT_FAILURE);
        } else if (c == ':') { /* Missing argument */
            fprintf(stderr, "%s: Option `%s' requires an argument\n",
                    argv[0], argv[this_optind]);
            exit(EXIT_FAILURE);
        }

        switch (longindex) {
        case 0:
            do_debug = true;
            break;
        case 1:
            do_wait = true;
            break;
        case 2:
            screen_pbm_file = optarg;
            break;
        case 3:
            {
#define OPT_COLUMN_WIDTH 24
                char buf[OPT_COLUMN_WIDTH + 1];
                printf("Usage: %s [OPTION]...\n"
                       "\n"
                       "Run SAM, the Super-Awesome Machine.\n"
                       "\n",
                       argv[0]);
#define O(longname, arg, argstring, docstring)                  \
                sprintf(buf, "--%s %s", longname, argstring);   \
                printf("%-24s%s\n", buf, docstring);
#include "tbl_opts.h"
#undef O
                printf ("\n"
                        "Report bugs to " PACKAGE_BUGREPORT ".\n");
                exit (EXIT_SUCCESS);
            }
        case 4:
            printf (SAM_VERSION_STRING "\n"
                    SAM_COPYRIGHT_STRING "\n"
                    PACKAGE_NAME " comes with ABSOLUTELY NO WARRANTY.\n"
                    "You may redistribute copies of " PACKAGE_NAME "\n"
                    "under the terms of the GNU General Public License.\n"
                    "For more information about these matters, see the file named COPYING.\n");
            exit (EXIT_SUCCESS);
        default:
            break;
        }
    }

    sam_word_t error = SAM_ERROR_OK;
    HALT_IF_ERROR(sam_traps_init());
    int res = sam_init(SAM_PROGRAM, SAM_STACK_WORDS, SAM_PROGRAM_LEN);
    HALT_IF_ERROR(res);
    sam_print_stack();
    res = sam_run();

 error:
#ifdef SAM_DEBUG
    if (do_debug) {
        debug("sam_run returns %d\n", res);
        if (sam_traps_window_used())
            if (screen_pbm_file != NULL)
                sam_dump_screen(screen_pbm_file);
    }
#endif

    if (do_wait && sam_traps_window_used ()) {
        while (!sam_traps_process_events()) {
            SDL_Delay(sam_update_interval);
        }
    }

    sam_traps_finish();
    return error;
}

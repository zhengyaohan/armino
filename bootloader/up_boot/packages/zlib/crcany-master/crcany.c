/*
  crcany version 2.0, 29 December 2020

  Copyright (C) 2014, 2016, 2017, 2020 Mark Adler

  This software is provided 'as-is', without any express or implied warranty.
  In no event will the authors be held liable for any damages arising from the
  use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim
     that you wrote the original software. If you use this software in a
     product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu
*/

/* Version history:
   1.0  22 Dec 2014  First version
   1.1  15 Jul 2016  Allow negative numbers
                     Move common code to model.[ch]
   1.2  17 Jul 2016  Move generic CRC code to crc.[ch] and crcdbl.[ch]
   1.3  23 Jul 2016  Build xorout into the tables
   1.4  30 Jul 2016  Fix a bug in word-wise table generation
                     Reduce verbosity of testing
   1.5  23 Oct 2016  Improve use of data types and C99 compatibility
                     Add verifications summary message
   1.6  11 Feb 2017  Add creation of remaining bits function (_rem)
                     Add and check the residue value of a model
                     Improve the generated comments and prototypes
   1.7  23 Dec 2017  Update to the latest CRC catalog
                     Minor improvements to code generation
   2.0  29 Dec 2020  Use fixed-width integers in generated code
                     Normalize generated-code loop constructs
                     Optimize the generated bit-reversal code
                     Refactor to split tests from code generation
                     Add crcadd to generate CRC code without testing
                     Bring code into compliance with C99 standard
                     Replace Ruby script with Python for portability
                     Add copy of Greg Cook's all-CRCs page for safekeeping
                     Update to the latest CRC catalog
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define local static

// Include all CRC functions referenced by a table of functions all[], and
// define a CRC function type crc_f. src/allcrcs.c is generated by crcgen. This
// needs to be linked with compilations of src/crc*.c, also generated by
// crcgen.
#include "src/allcrcs.c"

// Normalize str in place to contain only lower case letters and digits.
local void normalize(char *str) {
    char *p = str;
    while (islower(*p) || isdigit(*p))
        p++;
    while (isalnum(*p)) {
        if (isupper(*p))
            *p += 'a' - 'A';
        p++;
    }
    if (*p) {
        char *q = p;
        while (*++p)
            if (isalnum(*p))
                *q++ = tolower(*p);
        *q = 0;
    }
}

// Return the number of leading digits in str.
local size_t digs(char const *str) {
    char const *p = str;
    while (isdigit(*p))
        p++;
    return p - str;
}

// Number of CRCs defined in all (last entry is zeros).
#define NUMALL (sizeof(all) / sizeof(all[0]) - 1)

// Pick a CRC from the list based on id, return its index in all[], or -1 if
// not found. Return -2 if help was requested. If id is NULL, then CRC-32 is
// used.
local int pick(char *id) {
    // default CRC = standard PKZip/Ethernet CRC-32
    char def[] = "CRC-32";          // mutable string
    if (id == NULL)
        id = def;

    // normalize the id to lower case letters and digits
    normalize(id);
    if (*id == 0) {
        fputs("CRC not found\n", stderr);
        return -1;
    }

    // if asking for help or a list, list all of the CRCs -- return -2 to
    // return 0 from the command
    if (strcmp(id, "help") == 0 || strcmp(id, "list") == 0) {
        int k = 0;
        while (all[k].func != NULL) {
            printf("%s (%s)\n", all[k].name, all[k].match);
            k++;
        }
        return -2;
    }

    // drop the leading "crc", if present
    if (strncmp(id, "crc", 3) == 0)
        id += 3;

    // search for a matching CRC
    char hit[NUMALL];
    int index = -1, count = 0;
    for (size_t k = 0; k < NUMALL; k++) {
        char const *match = all[k].match;
        size_t di = digs(id), dm = digs(match);
        if (di && strncmp(id, match, di > dm ? di : dm))
            continue;       // if id has a width, it must match
        if (di && strcmp(id + di, match + dm) == 0)
            return k;       // if identical after width, then found
        hit[k] = strstr(match + dm, id + di) != NULL;
        if (hit[k]) {       // if contained, then match
            index = k;
            count++;
        }
    }

    // if there was exactly one match, return it
    if (count == 1)
        return index;

    // report multiple or no matches and return not found
    if (count) {
        fprintf(stderr, "%s matched multiple CRCs:\n", id);
        for (size_t k = 0; k < NUMALL; k++)
            if (hit[k])
                fprintf(stderr, "    %s\n", all[k].name);
    }
    else
        fprintf(stderr, "CRC %s not found\n", id);
    return -1;
}

// Return the CRC of the file in, using the CRC function func.
local uintmax_t crc_file(crc_f func, FILE *in) {
    unsigned char buf[16384];
    size_t got;
    uintmax_t crc = func(0, NULL, 0);
    while ((got = fread(buf, 1, sizeof(buf), in)) != 0)
        crc = func(crc, buf, got);
    return crc;
}

// Print the specified CRC computed on the provided files or on stdin.
int main(int argc, char **argv) {
    // set the CRC to apply
    int n = 1;
    int x = pick(n < argc && argv[n][0] == '-' ? argv[n++] + 1 : NULL);
    if (x < 0)
        return x + 2;
    crc_f func = all[x].func;
    unsigned width = all[x].width;
    printf("%s\n", all[x].name);

    // compute the CRC of the paths in the remaining arguments, or of stdin if
    // there are no more arguments -- include the paths in the output if there
    // are two or more
    int ret = 0;                    // set to 1 if any file errors
    int name = argc - n > 1;        // true to print the path name with the CRC
    do {
        FILE *in = n == argc ? stdin : fopen(argv[n], "rb");
        if (in == NULL) {           // open error
            perror(argv[n]);
            ret = 1;
            continue;
        }
        uintmax_t crc = crc_file(func, in);
        if (ferror(in)) {           // read error
            perror(n == argc ? NULL : argv[n]);
            ret = 1;
        }
        else {
            printf("0x%0*jx", (width + 3) >> 2, crc);
            if (name)
                printf(" %s", argv[n]);
            putchar('\n');
        }
        if (in != stdin)
            fclose(in);
    } while (++n < argc);
    return ret;
}
/* zipknit.c -- merge zip files

  Copyright (C) 2013 Mark Adler
  Version 1.5  11 November 2013

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */

/* Merge the zip files provided on the command line to a single zip file on
   stdout.  Example:

       zipknit archive1.zip archive2.zip archive3.zip > totalarchive.zip

   If names are duplicated across the input zip files, the output zip file will
   be generated nevertheless with all of the input entries, but the user will
   be warned about the duplicate names.  In that case, the resulting zip file
   should be considered corrupt, the duplications should be remedied, and the
   zipknit should be tried again.

   If zipknit is given no arguments, an empty zip file is produced.

   This version does not work self-extracting zip files, split/spanned zip
   files, or zip files with an encrypted and/or compressed central directory.
   zipknit will work on zip files with encrpyted entries, including strong
   encryption, simply copying those entries with the encryption unaffected.
   zipknit works on Zip64 files, and will produce Zip64 output from non-Zip64
   input if required by the total size or total number of entries in the
   output.  All of the original information from the local and central headers
   is retained, including extra blocks and comments.  zipknit is agnostic to
   the compression methods used -- nothing is decompressed or recompressed. Any
   zip file comments, Zip64 extensible data sectors, or version data from the
   Zip64 end record in the input zip files are discarded.  If there is a Zip64
   end record in the output zip file, the version needed to extract is set to
   the maximum of the versions needed to extract in the entries, and 45 (4.5),
   where 4.5 is needed for Zip64 itself.

   Compile and link zipknit.c and try.c, with try.h available for inclusion.
 */

/*
 * Change history:
 *
 * 1.0  23 Oct 2013     - First version
 * 1.1  27 Oct 2013     - Fix bug when missing needed extra field
 *                      - Allow Zip64 even when end record doesn't ask for it
 *                      - Add comment above on zip file discarded data
 *                      - Add warning that duplicates means output is corrupted
 *                      - Make Zip64 end records if any Zip64 in local entries
 *                      - Use size_t for number of entries -- check it fits
 * 1.2   1 Nov 2013     - Check that size_t is big enough at compile time
 *                      - Use Zip64 when the number of entries or the offset of
 *                        the central directory is exactly the maximum value
 *                        for regular headers
 *                      - Use uint64_t instead of unsigned long long
 *                      - Always put file name at start of input error messages
 *                      - Check for exact length of Zip64 extra field
 * 1.3   2 Nov 2013     - Update try.c and try.h to version 1.1
 * 1.4   9 Nov 2013     - Check for encrypted or compressed central directory
 *                      - Warn about junk at the end of an input zip file
 *                      - Warn about empty zip files
 *                      - Correct comments on encrypted zip files
 *                      - Fix bug when knitting a non-span signature
 * 1.5  11 Nov 2013     - Fix copyright year
 *                      - Add more information on out-of-memory errors
 *                      - Fix bug when output offsets overflow 32 bits
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include "try.h"

#if SIZE_MAX < 4294967295U
#  error zipknit.c requires that size_t be at least 32 bits
#endif

#define local static

/* How to print size_t and off_t with printf().  You may need to update these
   for your compiler.  (Do not include the % sign.) */
#define PSIZET "lu"
#define POFFT "lld"

/* Zip file structure for use by the zip_* functions. */
typedef volatile struct {
        /* output zip file */
    FILE *out;              /* output file */
    unsigned char *dir;     /* central directory headers */
    size_t size;            /* allocated size of dir (for resizing) */
    size_t len;             /* current length of dir */
    size_t tot;             /* total number of entries */
    size_t ver;             /* maximum version of zip needed to extract */
    off_t pos;              /* current offset in output file */
    int zip64;              /* true to write zip64 end records */
        /* current input zip file being processed */
    FILE *in;               /* zip file */
    char *path;             /* path of zip file (for error messages) */
    off_t zlen;             /* total number of bytes in zip file */
    size_t num;             /* number of entries in the zip file */
    off_t beg;              /* offset of first local or end (0 or 4) */
    off_t cen;              /* offset of central directory */
    off_t end;              /* offset of the end of central directory record */
    off_t off;              /* current offset in the input file */
    off_t *loc;             /* local header offsets */
} zip_t;

/* Zip file format header signatures. */
#define LOC "PK\x03\x04"            /* local header sig */
#define CEN "PK\x01\x02"            /* central header sig */
#define END "PK\x05\x06"            /* end record sig */
#define END64 "PK\x06\x06"          /* zip64 end record sig */
#define LOC64 "PK\x06\x07"          /* zip64 end locator sig */
#define SPAN "PK00"                 /* spanning signature for single segment */
#define SPLIT "PK\x07\x08"          /* starting signature for a split zip */

/* Zip file format header lengths. */
#define LOC_LEN 30                  /* base local header */
#define CEN_LEN 46                  /* base central header */
#define END_LEN 22                  /* base end record */
#define END_MAX (END_LEN + 65535)   /* max end record with comment */
#define END64_LEN 56                /* base zip64 end record */
#define LOC64_LEN 20                /* zip64 end locator */
#define BIG_LEN END64_LEN           /* longest base header length */

/* Get or put a two, four, or eight-byte unsigned little-endian value from or
   to a buffer of unsigned chars.  GET2() and GET4() returns a size_t,
   whereas GET8() returns a uint64_t. */
#define GET2(a) ((a)[0] + ((size_t)((a)[1]) << 8))
#define GET4(a) (GET2(a) + (GET2((a) + 2) << 16))
#define GET8(a) (GET4(a) + ((uint64_t)GET4((a) + 4) << 32))
#define PUT2(a, n) ((a)[0] = n, (a)[1] = (n) >> 8)
#define PUT4(a, n) (PUT2(a, n), PUT2((a) + 2, (n) >> 16))
#define PUT8(a, n) (PUT4(a, n), PUT4((a) + 4, (n) >> 32))

/* Get a four-byte unsigned little endian value from a buffer of unsigned
   chars, and check that it will fit in an off_t value for file access (in case
   off_t is only 32 bits). */
local inline off_t get4o(unsigned char *a)
{
    off_t ret;

    ret = (off_t)GET4(a);                                       if (ret < 0) throw(3, "file offset is too large for off_t");
    return ret;
}

/* Get an eight-byte unsigned little endian value from a buffer of unsigned
   chars, and check that it will fit in an off_t value for file access.  This
   is not called unless check_64() has already established that off_t is at
   least 64 bits. */
local inline off_t get8o(unsigned char *a)
{
    off_t ret;

    ret = (off_t)GET8(a);                                       if (ret < 0) throw(3, "file offset is too large for off_t");
    return ret;
}

/* Check that the off_t type is at least 64 bits.  This is used when Zip64
   headers are encountered, or when the output requires the Zip64 format. */
local void check_64(zip_t *zip)
{
                                                                if ((uint64_t)((off_t)0 - 1) < 0xffffffffffffffff) throw(3, "%s: cannot process zip64 file (off_t too small)", zip->path);
}

/* Read len bytes to buf from offset off in zip file zip.  zip->off is updated
   to after the read data.  If this returns, the data was read successfully. */
local inline void zip_read(zip_t *zip, off_t off, void *buf, size_t len)
{
    int ret;
    size_t got;

    ret = fseeko(zip->in, off, SEEK_SET);                       if (ret) throw(1, "%s: could not seek: %s", zip->path, strerror(errno));
    got = fread(buf, 1, len, zip->in);                          if (got != len) throw(1, "%s: could not read: %s", zip->path, strerror(errno));
    zip->off = off + len;
}

/* Return the length of the zip file zip. */
local inline off_t zip_len(zip_t *zip)
{
    int ret;

    ret = fseeko(zip->in, 0, SEEK_END);                         if (ret) throw(1, "%s: could not seek: %s", zip->path, strerror(errno));
    zip->off = ftello(zip->in);
    return zip->off;
}

/* If end at offset off contains the proper end of central directory record,
   set zip->cen to the offset of the central directory, zip->num to the number
   of entries, and zip->end to the offset of the end of central directory
   record or the Zip64 end of central directory record.  Otherwise, leave those
   unchanged.  This cannot be spoofed by a zip file within the zip file
   comment.  (Doesn't mean it can't be spoofed -- just not by that.)  If Zip64
   is indicated, then those headers are looked for, validated, and used. */
local inline void zip_end_test(zip_t *zip, unsigned char *end, off_t off)
{
    size_t disk, start, crypt = 0;
    uint64_t here, total;
    off_t len, cen, beg, size;
    unsigned char head[BIG_LEN];

    /* check and get data from end of central directory record */
    if (memcmp(end, END, 4))
        return;
    disk = GET2(end + 4);
    start = GET2(end + 6);
    here = GET2(end + 8);
    total = GET2(end + 10);
    len = get4o(end + 12);
    cen = get4o(end + 16);
    beg = off;

    /* if zip64 is indicated, check and get data from zip64 end record */
    if (disk == 0xffff || start == 0xffff ||
        here == 0xffff || total == 0xffff ||
        len == 0xffffffff || cen == 0xffffffff || cen + len != beg) {
        check_64(zip);
        if (beg < END64_LEN + LOC64_LEN)
            return;

        /* read zip64 locator record */
        len = beg - LOC64_LEN;
        zip_read(zip, len, head, LOC64_LEN);
        if (memcmp(head, LOC64, 4))
            return;
        disk = GET4(head + 4);
        beg = get8o(head + 8);
        disk |= GET4(head + 16) - 1;
        if (beg > len - END64_LEN)
            return;

        /* read zip64 end record */
        zip_read(zip, beg, head, END64_LEN);
        size = get8o(head + 4);
        if (memcmp(head, END64, 4) || beg + 12 + size != len)
            return;
        disk |= GET4(head + 16);
        disk |= GET4(head + 20);
        here = GET8(head + 24);
        total = GET8(head + 32);
        len = get8o(head + 40);
        cen = get8o(head + 48);

        /* check for encrypted and/or compressed central directory */
        if (GET2(head + 14) >= 62 && size > END64_LEN) {
            memset(head, 0, 20);
            zip_read(zip, zip->off, head, size - END64_LEN > 20 ? 20 : size - END64_LEN);
            crypt = GET2(head) || GET2(head + 18);
        }
    }
    else
        disk |= start;

    /* verify that the central directory ends at the end record */
    if (cen + len != beg)
        return;
                                                                if (crypt) throw(3, "%s: cannot process encrpyted central directory", zip->path);
                                                                if (disk != 0 || here != total) throw(3, "%s: cannot process split or spanned zip file", zip->path);

    /* found a good end record */
                                                                if (total > (uint64_t)((size_t)0 - 1)) throw(3, "%s: too many entries to process in memory", zip->path);
    zip->num = (size_t)total;
    zip->cen = cen;
    zip->end = beg;
    len = zip->zlen - off - END_LEN - (off_t)GET2(end + 20);
    if (len)
        fprintf(stderr, "%s: has %" POFFT " junk bytes at the end\n", zip->path, len);
    return;
}

/* Find the start of the end of central directory record, the number of entries
   in the zip file, and the start of the central directory.  This searches the
   end of the zip file for the end of central directory record if necessary. */
local void zip_central_find(zip_t *zip)
{
    size_t get, n;
    unsigned char head[END_MAX];

    /* initialize for an empty zip file */
    zip->num = 0;
    zip->beg = 0;
    zip->cen = 0;
    zip->end = 0;

    /* verify that it is a zip file based on the first four bytes, and if
       empty, return */
    zip->zlen = zip_len(zip);                                   if (zip->zlen < END_LEN) throw(3, "%s: not a zip file", zip->path);
    zip_read(zip, 0, head, 4);                                  if (memcmp(head, SPLIT, 4) == 0) throw(3, "%s: cannot process split or spanned zip file", zip->path);
    if (memcmp(head, SPAN, 4) == 0) {
        zip->beg = 4;
        zip_read(zip, 4, head, 4);
    }
    if (memcmp(head, LOC, 4)) {
                                                                if (memcmp(head, END, 4) && memcmp(head, END64, 4)) throw(3, "%s: not a zip file", zip->path);
        return;
    }

    /* try the most common case first, where there is no zip file comment
       (reads much less data) */
    zip_read(zip, zip->zlen - END_LEN, head, END_LEN);
    zip_end_test(zip, head, zip->zlen - END_LEN);
    if (zip->cen)
        return;

    /* there must be a zip file comment or junk at the end of the zip file --
       search backwards for the end of central directory record */
    get = zip->zlen < END_MAX ? (size_t)zip->zlen : END_MAX;
    zip_read(zip, zip->zlen - get, head, get);
    for (n = get - END_LEN; n; n--) {
        zip_end_test(zip, head + n - 1, zip->zlen - get + n - 1);
        if (zip->cen)
            return;
    }
                                                                throw(3, "%s: no end record found", zip->path);
}

/* Grow dir by size.  Return a pointer to the start of the grown space. */
local inline unsigned char *zip_grow(zip_t *zip, size_t size)
{
    zip->len += size;                                           if (zip->len < size) throw(3, "%s: central directory too large for size_t", zip->path);
    if (zip->size < zip->len) {
        void *temp;

        if (zip->size == 0)
            zip->size = 32768;
        while (zip->size < zip->len)
            zip->size <<= 1;
        temp = realloc(zip->dir, zip->size);                    if (temp == NULL) throw(5, "out of memory on a %" PSIZET " request", zip->size);
        zip->dir = temp;
    }
    return zip->dir + zip->len - size;
}

/* Append the zip file's central directory to zip->dir, updating the offsets of
   the local header in each entry for the output zip file locations.  Save the
   offsets of the local headers in the zip file for use when copying. */
local void zip_central_load(zip_t *zip)
{
    size_t n, name, extra, comment, var, skip, more;
    unsigned char *next;
    off_t loc, mov;

    free(zip->loc);
    zip->loc = malloc((zip->num + 1) * sizeof(off_t));          if (zip->loc == NULL) throw(5, "out of memory on a %" PSIZET " request", (zip->num + 1) * sizeof(off_t));
    zip->off = zip->cen;
    for (n = 0; n < zip->num; n++) {
        /* append central directory header, growing dir as needed */
                                                                if (zip->off + CEN_LEN > zip->end) throw(3, "%s: central directory overlaps end record", zip->path);
        next = zip_grow(zip, CEN_LEN);
        zip_read(zip, zip->off, next, CEN_LEN);                 if (memcmp(next, CEN, 4)) throw(3, "%s: missing central directory signature", zip->path);
        name = GET2(next + 28);
        extra = GET2(next + 30);
        comment = GET2(next + 32);
        var = name + extra + comment;                           if (zip->off + (off_t)var > zip->end) throw(3, "%s: central directory overlaps end record", zip->path);
        next = zip_grow(zip, var);
        zip_read(zip, zip->off, next, var);
        next -= CEN_LEN;

        /* update maximum version to extract */
        var = GET2(next + 6);
        if (var > zip->ver)
            zip->ver = var;

        /* update offset of local header to location in output file */
        skip = GET4(next + 20) == 0xffffffff ? 8 : 0;
        if (GET4(next + 24) == 0xffffffff)
            skip += 8;
        more = GET2(next + 34) == 0xffff ? 8 : 0;
        if (skip || more)
            zip->zip64 = 1;
        if (GET4(next + 42) == 0xffffffff) {
            /* get offset from Zip64 extended information extra field */
            check_64(zip);
            zip->zip64 = 1;
            next += CEN_LEN + name;
            while (extra >= 4) {
                var = 4 + GET2(next + 2);
                if (var > extra) {
                    extra = 0;
                    break;
                }
                if (GET2(next) == 1)
                    break;
                extra -= var;
                next += var;
            }
                                                                if (extra < 4) throw(3, "%s: missing zip64 extra field", zip->path);
                                                                if (var != 4 + skip + 8 + more) throw(3, "%s: invalid zip64 extra field", zip->path);
            /* update offset in extra field */
            next += 4 + skip;
            loc = get8o(next);
            mov = loc - zip->beg + zip->pos;
            PUT8(next, mov);
        }
        else {
            /* update offset in central directory header */
            loc = get4o(next + 42);
            mov = loc - zip->beg + zip->pos;                    if (mov < 0) throw(3, "%s: output zip file grew beyond capacity of off_t", zip->path);
            if (mov < 0xffffffff)
                PUT4(next + 42, mov);
            else {
                /* new offset doesn't fit in four bytes -- insert a new Zip64
                   extended information extra field to hold the large offset */
                                                                if (extra + 12 > 0xffff) throw(3, "%s: extra field too large to add Zip64 field", zip->path);
                check_64(zip);
                zip->zip64 = 1;
                PUT2(next + 30, extra + 12);
                var = 0xffffffff;
                PUT4(next + 42, var);
                next = zip_grow(zip, 12) - extra - comment;
                memmove(next + 12, next, extra + comment);
                PUT2(next, 1);
                PUT2(next + 2, 8);
                PUT8(next + 4, mov);
            }
        }

        /* save input file offset */
        zip->loc[n] = loc;
    }
                                                                if (zip->off != zip->end) throw(3, "%s: central directory does not reach end record", zip->path);
    next = realloc(zip->dir, zip->len);
    if (next != NULL) {
        zip->dir = next;
        zip->size = zip->len;
    }
}

/* Create and initialize a new zip_t structure. */
local zip_t *zip_new(FILE *out)
{
    zip_t *zip;

    zip = malloc(sizeof(zip_t));                                if (zip == NULL) throw(5, "out of memory on a %" PSIZET " request", sizeof(zip_t));
    zip->out = out;
    zip->dir = NULL;
    zip->size = 0;
    zip->len = 0;
    zip->tot = 0;
    zip->ver = 0;
    zip->pos = 0;
    zip->zip64 = 0;
    zip->in = NULL;
    zip->loc = NULL;
    return zip;
}

/* Open a zip file for reading, find and load the central directory. */
local void zip_open(zip_t *zip, char *path)
{
    zip->in = fopen(path, "rb");                                if (zip->in == NULL) throw(1, "%s: could not open: %s", path, strerror(errno));
    zip->path = path;
    zip->off = 0;
    zip_central_find(zip);
    zip_central_load(zip);
    if (zip->num == 0)
        fprintf(stderr, "zipknit warning: %s is empty\n", zip->path);

}

/* off_t comparison function for use with mergesort(). */
local int cmp_off(const void *p, const void *q)
{
    off_t a, b;

    a = *(off_t *)p;
    b = *(off_t *)q;
    return a > b ? 1 : a < b ? -1 : 0;
}

/* All writes to the output zip file go through here. */
local void zip_write(zip_t *zip, unsigned char *buf, size_t len)
{
    size_t writ;

    writ = fwrite(buf, 1, len, zip->out);                       if (writ != len) throw(2, "error writing output: %s", strerror(errno));
}

/* Buffer size for copying.  Must be >= LOC_LEN. */
#define BUFSIZE 16384

/* Copy the local entries to the output zip file, as is.  Examine the local
   headers to see if any require Zip64 and update zip->zip64 accordingly. */
local void zip_local_copy(zip_t *zip)
{
    int ret;
    off_t left;
    size_t n, get;
    unsigned char buf[BUFSIZE];

    if (zip->num == 0)
        return;
    ret = mergesort(zip->loc, zip->num, sizeof(off_t),
                    cmp_off);                                   if (ret) throw(5, "out of memory on a %" PSIZET " request", zip->num * sizeof(off_t));
                                                                if (zip->loc[0] != zip->beg) throw(3, "%s: first entry not at start of file", zip->path);
    zip->off = zip->beg;
    zip->loc[zip->num] = zip->cen;
    for (n = 0; n < zip->num; n++) {
        left = zip->loc[n + 1] - zip->loc[n];                   if (left < LOC_LEN) throw(3, "%s: invalid central offsets", zip->path);
        get = LOC_LEN;
        zip_read(zip, zip->off, buf, get);                      if (memcmp(buf, LOC, 4)) throw(3, "%s: invalid local header", zip->path);
        if (GET4(buf + 18) == 0xffffffff || GET4(buf + 22) == 0xffffffff)
            zip->zip64 = 1;
        for (;;) {
            left -= get;
            zip_write(zip, buf, get);
            zip->pos += get;                                    if (zip->pos < 0) throw(2, "output too large for off_t");
            get = left > BUFSIZE ? BUFSIZE : (size_t)left;
            if (get == 0)
                break;
            zip_read(zip, zip->off, buf, get);
        }
    }
    zip->tot += zip->num;                                       if (zip->tot < zip->num) throw(2, "total number of entries too large for size_t");
}

/* Close a zip file that was opened for reading. */
local void zip_close(zip_t *zip)
{
    if (zip == NULL || zip->in == NULL)
        return;
    fclose(zip->in);
    zip->in = NULL;
    free(zip->loc);
    zip->loc = NULL;
}

/* Finish off the output zip file by writing the central directory and end of
   central directory record, including Zip64 records if warranted. */
local void zip_end(zip_t *zip)
{
    size_t put;
    unsigned char end[BIG_LEN];

    zip_write(zip, zip->dir, zip->len);
    if (zip->zip64 || zip->tot >= 0xffff || zip->pos >= 0xffffffff) {
        check_64(zip);
        memcpy(end, END64, 4);
        PUT8(end + 4, (off_t)(END64_LEN - 12));
        if (zip->ver < 45)
            zip->ver = 45;
        PUT2(end + 12, zip->ver);
        PUT2(end + 14, zip->ver);
        PUT4(end + 16, 0);
        PUT4(end + 20, 0);
        PUT8(end + 24, zip->tot);
        PUT8(end + 32, zip->tot);
        PUT8(end + 40, zip->len);
        PUT8(end + 48, zip->pos);
        zip_write(zip, end, END64_LEN);
        memcpy(end, LOC64, 4);
        PUT4(end + 4, 0);
        PUT8(end + 8, zip->pos + zip->len);
        PUT4(end + 16, 1);
        zip_write(zip, end, LOC64_LEN);
    }
    memcpy(end, END, 4);
    PUT4(end + 4, 0);
    put = zip->tot > 0xffff ? 0xffff : (size_t)zip->tot;
    PUT2(end + 8, put);
    PUT2(end + 10, put);
    put = zip->len > 0xffffffff ? 0xffffffff : (size_t)zip->len;
    PUT4(end + 12, put);
    put = zip->pos > 0xffffffff ? 0xffffffff : (size_t)zip->pos;
    PUT4(end + 16, put);
    PUT2(end + 20, 0);
    zip_write(zip, end, END_LEN);
}

/* Block of data for hash table. */
typedef struct {
    void *dat;
    size_t len;
} block_t;

/* Compare two blocks of data for ordering.  If one block is a prefix of the
   other longer block, then the longer block is considered greater. */
local inline int blkcmp(void *a, size_t m, void *b, size_t n)
{
    int ret;

    ret = memcmp(a, b, m > n ? n : m);
    return ret || m == n ? ret : (m > n ? 1 : -1);
}

/* Check for duplicate names in the central directory. */
local void zip_names_check(zip_t *zip)
{
    int dup = 0;
    size_t size, n, len, k;
    uint64_t hash;
    unsigned char *next, *name;
    block_t *table;

    if (zip->tot < 2)
        return;
    size = zip->tot << 3;
    if ((size ^ (size - 1)) == 0)
        size++;
    table = calloc(size, sizeof(block_t));                      if (table == NULL) throw(5, "out of memory on a %" PSIZET " request", size * sizeof(block_t));
    next = zip->dir;
    for (n = zip->tot; n; n--) {
        name = next + CEN_LEN;
        len = GET2(next + 28);
        hash = 1;
        for (k = 0; k < len; k++)
            hash = hash * 0x9ae16a3b2f90404fULL + name[k];
        hash %= size;
        while (table[hash].dat != NULL) {
            if (blkcmp(table[hash].dat, table[hash].len, name, len) == 0) {
                dup = 1;
                fputs("zipknit warning: duplicate name: ", stderr);
                if (len)
                    fwrite(name, 1, len, stderr);
                else
                    fputs("<stdin>", stderr);
                putc('\n', stderr);
                break;
            }
            hash++;
            if (hash == size)
                hash = 0;
        }
        table[hash].dat = name;
        table[hash].len = len;
        next = name + len + GET2(next + 30) + GET2(next + 32);
    }
    free(table);
    if (dup)
        fputs("zipknit warning: zip output is corrupted -- remedy the duplicates and retry\n", stderr);
}

/* Free the allocations in the zip_t instance and the instance itself. */
local void zip_free(zip_t *zip)
{
    if (zip == NULL)
        return;
    free(zip->loc);
    free(zip->dir);
    free((void *)zip);
}

/* Merge the zip files listed on the command line, writing the resulting zip
   file to stdout. */
int main(int argc, char **argv)
{
    ball_t err;
    zip_t *zip = NULL;
    FILE *out = stdout;

    try {
        zip = zip_new(out);

        /* copy the local entries from all of the zip files to out, while
           maintaining in memory the central directory information */
        while (++argv, --argc) {
            zip_open(zip, *argv);
            zip_local_copy(zip);
            zip_close(zip);
        }

        /* check for duplicate names in the output zip file and warn the user
           if there are any */
        zip_names_check(zip);

        /* write out the central directory and end of central directory to
           complete the zip file on out */
        zip_end(zip);
    }
    always {
        fclose(out);
        zip_close(zip);
        zip_free(zip);
    }
    catch (err) {
        fprintf(stderr, "zipknit error: %s -- aborting\n", err.why);
        drop(err);
        return err.code;
    }
    return 0;
}

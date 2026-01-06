/*
 * Copyright (c) 2007-2018, GDash Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:

 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "fileops/binaryimport.hpp"
#include "fileops/c64import.hpp"

#include <cstring>
#include <glib.h>
#include <stdexcept>
#include "misc/logger.hpp"
#include "misc/printf.hpp"


class ResizingByteArray : public std::vector<unsigned char> {
public:
    using std::vector<unsigned char>::size;
    using std::vector<unsigned char>::empty;
    using std::vector<unsigned char>::clear;
    unsigned char &operator[](size_t s) {
        if (s >= size())
            this->resize(s + 1);
        // call op[] of parent class
        return this->std::vector<unsigned char>::operator[](s);
    }
};


static ResizingByteArray out;
static int outpos;


/* default effect table, taken from afl boulder dash. used to detect if
   game-wide diego effects are present. */
static unsigned char default_effect[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x08, 0x09, 0x0a, 0x0b,
    0x10, 0x10, 0x12, 0x12, 0x14, 0x14, 0x16, 0x16, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2a, 0x2c, 0x2d, 0x2e, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x30, 0x31, 0x32, 0x33, 0x38, 0x38, 0x3a, 0x3a, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x60, 0x46, 0x4e, 0x22, 0x2e, 0x62, 0x2e, 0x4a, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
    0x64, 0x44, 0x44, 0x44, 0x44, 0x48, 0x48, 0x48, 0x48, 0x00, 0x00, 0x00, 0x66, 0x68, 0x6a, 0x68,
    0x66, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x62, 0x66, 0x68, 0x6a, 0x00, 0x4e, 0x4e, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x4c, 0x4c, 0x40, 0x40, 0x42, 0x2e, 0x40,
};


static void startwith(C64Import::GdCavefileFormat format) {
    out.clear();
    for (size_t i = 0; C64Import::gd_format_strings[format][i] != 0; ++i) {
        out[i] = C64Import::gd_format_strings[format][i];
    }
    outpos = 12;
}


/* save bd1 caves from memory map */
/* atari: a boolean value, true if try atari map. */
static bool try_bd1(std::vector<unsigned char> const &memory, bool atari) {
    /* there are cave pointers at 0x5806. two byte entries pointing to caves. */
    /* their value is relative to 0x582e. */
    /* atari values are 3500 and 3528. */
    int cavepointers = atari ? 0x3500 : 0x5806;
    int cavestart = atari ? 0x3528 : 0x582e;
    int positions[20];
    const char *type = atari ? "atari" : "c64";
    SetLoggerContextForFunction context(Printf("BD1 import %s", type));

    startwith(atari ? C64Import::GD_FORMAT_BD1_ATARI : C64Import::GD_FORMAT_BD1);

    /* try to autodetect */

    /* fixme what is this? */
    if (!atari) {
        if (memory[0x5f3a] != 0x44 || memory[0x5f3b] != 0x44 || memory[0x5f3c] != 0x48 || memory[0x5f3d] != 0x48) {
            gd_debug("Assumptions failed for BD1(e).");
            return false;
        }
    }

    /* 20 (4 groups * 5 caves (4cave+1intermission)) */
    for (int i = 0; i < 20; i++) {
        int lo = memory[cavepointers + i * 2];
        int hi = memory[cavepointers + i * 2 + 1];
        int start = hi * 256 + lo + cavestart, pos = start;
        positions[i] = start;
        /* check if position is not the same as any previous cave */
        for (int j = 0; j < i; ++j)
            if (positions[j] == positions[i]) {
                gd_debug("Invalid cave position %x at index %d - same as for cave at index %d", pos, i, j);
                return false;
            }

        if (pos > (0xffff - 0x0400)) {
            gd_debug("Cannot interpret memory contents as BD1(e) -- invalid cave pointer.");
            return false;
        }
        if (i < 16)
            gd_debug("Cave %c, addr: %04x", char(i + 'A'), pos);
        else
            gd_debug("Intermission %d, addr: %04x", int(i - 15), pos);

        /* copy */
        /* first 32 bytes - cave options */
        for (int j = 0; j < 32; j++)
            out[outpos++] = memory[pos++]; /* 5levels: 5random, 5diamond, 5time */

        /* now cave objects */
        int j = memory[pos++];
        while (j != 0xFF) { /* végét jelenti; de kiírtuk ezt is */
            if (pos - start > 0x400) {
                /* bd1 caves cannot be this long */
                gd_debug("Cannot interpret memory contents as BD1(e) -- cave data too long.");
                return false;
            }
            out[outpos++] = j;
            if (j == 0x0F) {
                /* crazy cream 3 extension: escape byte 0x0f means raster */
                out[outpos++] = memory[pos++];  /* param byte 1: object to draw */
                out[outpos++] = memory[pos++];  /* param byte 2: begin column */
                out[outpos++] = memory[pos++];  /* param byte 3: begin row */
                out[outpos++] = memory[pos++];  /* param byte 4: amount of rows */
                out[outpos++] = memory[pos++];  /* param byte 5: amount of columns */
                out[outpos++] = memory[pos++];  /* param byte 6: distance between rows */
                out[outpos++] = memory[pos++];  /* param byte 7: distance between columns */
            } else {
                switch (j >> 6) {
                    case 0: /* point */
                        out[outpos++] = memory[pos++];  /* x */
                        out[outpos++] = memory[pos++];  /* y */
                        break;
                    case 1: /* line */
                        out[outpos++] = memory[pos++];  /* x */
                        out[outpos++] = memory[pos++];  /* y */
                        out[outpos++] = memory[pos++];  /* len */
                        out[outpos++] = memory[pos++];  /* dir */
                        break;
                    case 2: /* fillrect */
                        out[outpos++] = memory[pos++];  /* x */
                        out[outpos++] = memory[pos++];  /* y */
                        out[outpos++] = memory[pos++];  /* w */
                        out[outpos++] = memory[pos++];  /* h */
                        out[outpos++] = memory[pos++];  /* fill */
                        break;
                    case 3: /* outline */
                        out[outpos++] = memory[pos++];  /* x */
                        out[outpos++] = memory[pos++];  /* y */
                        out[outpos++] = memory[pos++];  /* w */
                        out[outpos++] = memory[pos++];  /* h */
                        break;
                }
            }
            j = memory[pos++];
        }
        out[outpos++] = j; /* lezáró 0xff kiírása */
    }
    gd_debug("Found 20 BD1 caves!");
    return true;
}


/* save plck caves from c64 memory map */
static bool try_plck(std::vector<unsigned char> const &memory) {
    SetLoggerContextForFunction context("PLCK import");
    int x, i;
    int has_names = 0;
    int has_diego = 0;
    int valid;
    int ok;

    startwith(C64Import::GD_FORMAT_PLC);
    /* try plck */
    valid = 0;

    /* try to detect plck cave selection table. assume there are at least 5 caves. */
    /* selection table without names. */
    ok = 0;
    for (x = 0; x < 5; x++)
        if (memory[0x5e8b + x] == 0x0e || memory[0x5e8b + x] == 0x19)
            ok++;
    if (ok == 5) {
        valid = 1;
        has_names = 0;
    }

    /* selection table with names. */
    ok = 0;
    for (x = 0; x < 5; x++)
        if (memory[0x5e8b + 13 * x + 12] == 0x0e || memory[0x5e8b + 13 * x + 12] == 0x19)
            ok++;
    if (ok == 5) {
        valid = 1;
        has_names = 1;
    }
    if (!valid) {
        gd_debug("Assumptions failed for PLCK - could not find cave selection table.");
        return false;
    }

    gd_debug(has_names ? "PLCK caves have names." : "PLCK caves have no names.");

    i = 0;
    //~ while (i<17) {   // For Bert Bogger Pro 2
    /* while present and (selectable or nonselectable)   <- find any valid byte in cave selection table. */
    while ((!has_names && (memory[0x5e8b + i] != 0 && (memory[0x5e8b + i] == 0x0e || memory[0x5e8b + i] == 0x19)))
            || (has_names && (memory[0x5e8b + i * 13 + 12] != 0 && (memory[0x5e8b + i * 13 + 12] == 0x0e || memory[0x5e8b + i * 13 + 12] == 0x19)))) {
        int j, pos, zero;

        pos = 0x7000 + 0x200 * i;

        /* check if memory is not filled with zeroes. */
        zero = 1;
        for (j = 0; j < 512 && zero; j++)
            if (memory[pos + j] != 0)
                zero = 0;
        if (zero)
            break;

        /* check for error */
        if (i > 71) {
            /* caves cannot be this long */
            gd_debug("Data corrupt or detection failed for plck caves -- too many caves.");
            return false;
        }

        if (memory[pos + 0x1e5] == 0x20 && memory[pos + 0x1e6] == 0x90 && memory[pos + 0x1e7] == 0x46)
            has_diego = 1;
        gd_debug("Cave %d, addr: %04x%s", int(i + 1), pos, (has_diego ? " - has diego effects." : ""));

        /* copy 1f0 bytes for cave */
        for (j = 0; j < 0x1f0; ++j)
            out[outpos++] = memory[pos++];

        /* and fill the rest with our own data. this way we stay compatible, as cave was always 1f0 bytes */
        if (has_names) {    /* we detected that it has names */
            if (memory[0x5e8b + i * 13 + 12] != 0x0e && memory[0x5e8b + i * 13 + 12] != 0x19) {
                gd_debug("ERROR: cave selection table corrupt or autodetection failed");
                out[outpos++] = 0x19;
                out[outpos++] = 0x19 + 1;
            } else {
                out[outpos++] = memory[0x5e8b + i * 13 + 12];
                out[outpos++] = memory[0x5e8b + i * 13 + 12] + 1; /* save twice, add +1 for second for detection in gdash */
            }
            for (j = 0; j < 12; j++)
                out[outpos++] = memory[0x5e8b + i * 13 + j];
            out[outpos++] = 0;  /* fill the rest with zero */
            out[outpos++] = 0;
        } else { /* no names */
            if (memory[0x5e8b + i] != 0x0e && memory[0x5e8b + i] != 0x19) {
                gd_debug("ERROR: cave selection table corrupt or autodetection failed");
                out[outpos++] = 0x19;
                out[outpos++] = 0x19 + 1;
            } else {
                out[outpos++] = memory[0x5e8b + i];
                out[outpos++] = memory[0x5e8b + i] + 1; /* save twice for detection, add 1 to second one */
            }
            for (j = 2; j < 16; j++)
                out[outpos++] = 0;
        }

        i++;
    }
    gd_debug("Found %d PLCK caves in %d bytes!", i, outpos);

    /* now try to do something about diego effects. */
    /* if we found at least one cave with diego effects, this check can be skipped. */
    if (!has_diego) {
        int j;
        const int numbers = sizeof(default_effect) / sizeof(default_effect[0]);
        char diffs[numbers];
        int n;

        /* check effect table from memory at 3b00. */
        n = 0;
        for (j = 0; j < numbers; j++)
            if (default_effect[j] != memory[0x3b00 + j]) {
                n++;
                diffs[j] = 1;
            } else
                diffs[j] = 0;
        if (n != 0) {
            /* found an unstandard effect table */
            int b_stone_to = memory[0x3b00 + 0x11];
            int f_diamond_to = memory[0x3b00 + 0x17];
            int explosion_to = memory[0x3b00 + 0x1e];
            int dirt_pointer = memory[0x3b00 + 0x42];
            int growing_pointer = memory[0x3b00 + 0x6b];

            /* report effect table changes that we are able to convert to the per cave description. */
            gd_debug("Found global diego effects!");
            if (b_stone_to != 0x10)
                gd_debug("Effect for bouncing stone.");
            if (f_diamond_to != 0x16)
                gd_debug("Effect for falling diamond.");
            if (explosion_to != 0x1e)
                gd_debug("Effect for explosion.");
            if (dirt_pointer != 0x46)
                gd_debug("Dirt looks like effect.");
            if (growing_pointer != 0x4e)
                gd_debug("Growing wall looks like effect.");

            /* go through all caves and add no1v5.3e compatible flags */
            for (j = 0; j < i; j++) {
                /* flags used previously on c64; gdash also knows them */
                out[12 + 0x0200 * j + 0x1e5] = 0x20;
                out[12 + 0x0200 * j + 0x1e6] = 0x90;
                out[12 + 0x0200 * j + 0x1e7] = 0x46;

                /* set detected stuff for cave */
                out[12 + 0x0200 * j + 0x1ea] = b_stone_to;
                out[12 + 0x0200 * j + 0x1eb] = f_diamond_to;
                out[12 + 0x0200 * j + 0x1ec] = explosion_to;
                out[12 + 0x0200 * j + 0x1ed] = dirt_pointer;
                out[12 + 0x0200 * j + 0x1ee] = growing_pointer;
                out[12 + 0x0200 * j + 0x1ef] = 200; /* FIXME AMOEBA THRESHOLD */
            }
        }
        /* null out effects we could handle */
        diffs[0x11] = 0;
        diffs[0x17] = 0;
        diffs[0x1e] = 0;
        diffs[0x42] = 0;
        diffs[0x6b] = 0;
        /* check if growing wall (delayed)==growing wall in terms of graphics, if it is, then it is no diff to original */
        if (memory[0x3b00 + 0x6b] == memory[0x3b00 + 0x6c])
            diffs[0x6c] = 0;

        /* and check if there are others we cannot fit into a "standard" cawe */
        for (j = 0; j < numbers; j++)
            if (diffs[j]) {
                gd_debug("Don't know how to handle effect for element number %x, default %x, this one %x", j, default_effect[j], memory[0x3b00 + j]);
            }
    }
    return true;
}



/* save plck caves from atari memory map */
static bool try_atari_plck(std::vector<unsigned char> const &memory) {
    SetLoggerContextForFunction context("PLCK import Atari");
    int x, i;
    int ok;
    int has_selection;

    startwith(C64Import::GD_FORMAT_PLC_ATARI);

    /* try to detect the cave selection table. assume there are at least 5 caves. */
    /* example:
        6040: 66 60 4A 41 49 4C 20 20 20 20 2E 43 41 56 59 53  f.JAIL    .CAVYS
        6050: 41 46 45 20 20 20 20 2E 43 41 56 59 52 41 49 4E  AFE    .CAVYRAIN
        6060: 20 20 20 20 2E 43 41 56 59 45 53 43 41 50 41 44      .CAVYESCAPAD
        6070: 45 2E 43 41 56 59 43 48 41 53 45 20 20 20 2E 49  E.CAVYCHASE   .I
        6080: 4E 54 4E 52 45 53 43 55 45 20 20 2E 43 41 56 59  NTNRESCUE  .CAVY
        we detect the Y's and N's after CAV or INT
    */

    /* caves are preceded with $ab $ab $ab $ab CAVENAME */
    /*
        6FF0: AB AB AB AB 4A 41 49 4C 2E 43 41 56 9B 20 20 20  ....JAIL.CAV.
        7000: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
        7010: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
    */
    /* try to detect this; assume at least 5 caves. */
    ok = 0;
    for (x = 0; x < 5; x++) {
        int pos = 0x6ff0 + x * 0x200;
        if (memory[pos] == 0xAB && memory[pos + 1] == 0xAB && memory[pos + 2] == 0xAB && memory[pos + 3] == 0xAB)
            ok++;
    }
    if (ok != 5) {
        gd_debug("Assumptions failed for Atari PLCK - could not find $ab $ab $ab $ab before caves.");
        return false;
    }

    /* check if it has a selection table */
    ok = 0;
    for (x = 0; x < 5; x++) {
        if (memory[0x604e + x * 13] == 'Y' || memory[0x604e + x * 13] == 'N')
            ok++;
    }
    has_selection = (ok == 5);
    if (has_selection)
        gd_debug("Found selection table.");

    i = 0;
    /* detect caves by $ab bytes before them */
    while (memory[0x6ff0 + i * 0x200] == 0xAB && memory[0x6ff1 + i * 0x200] == 0xAB && memory[0x6ff2 + i * 0x200] == 0xAB && memory[0x6ff3 + i * 0x200] == 0xAB) {
        int j, pos;
        int selectable;

        pos = 0x7000 + 0x200 * i;
        if (has_selection)
            selectable = memory[0x604e + i * 13] == 'Y';
        else
            /* has no selection table... we make intermissions unselectable. */
            selectable = memory[pos + 0x1da] == 0;

        /* check for error */
        if (i > 71) {
            /* caves cannot be this long */
            gd_debug("Data corrupt or detection failed for plck caves -- too many caves.");
            return false;
        }

        bool has_diego = false;
        if (memory[pos + 0x1e5] == 0x20 && memory[pos + 0x1e6] == 0x90 && memory[pos + 0x1e7] == 0x46)
            has_diego = true;
        gd_debug("Cave %d, addr: %04x, sel: %d%s", i + 1, pos, selectable, has_diego ? " - has diego effects." : "");

        /* copy 1f0 bytes for cave */
        for (j = 0; j < 0x1f0; ++j)
            out[outpos++] = memory[pos++];

        /* and fill the rest with our own data. this way we stay compatible, as cave was always 1f0 bytes */
        /* we have to stay compatible with c64 plck import routine above. */
        /* so we use 0x19 for selectable caves, 0x0e for nonselectable. */
        out[outpos++] = selectable ? 0x19 : 0x0e;
        out[outpos++] = (selectable ? 0x19 : 0x0e) + 1; /* save twice, add +1 for second for detection in gdash */
        /* copy cave name, which is:
            6FE0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
            6FF0: AB AB AB AB 4A 41 49 4C 2E 43 41 56 9B 20 20 20  ....JAIL.CAV.        ... cave name
            7000: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD     ... cave data
            7010: 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44 44  DDDDDDDDDDDDDDDD
            7020: 44 44 44 44 44 44 44 44 4E 77 77 77 77 77 77 77  DDDDDDDDNwwwwwww
        */

        pos = 0x7000 + 0x200 * i;
        for (j = 0; j < 12; j++) {
            unsigned char c = memory[pos - 0x10 + 4 + j];

            /* somehow this character set the end-of-string in atari version */
            if (c == 0x9b)
                c = 0x20;
            out[outpos++] = c;
        }
        out[outpos++] = 0;  /* fill the rest with zero */
        out[outpos++] = 0;

        i++;
    }
    gd_debug("Found %d PLCK caves in %d bytes!", i, outpos);

    return true;
}



/* save crazy light caves from memory map */
static bool try_crli(std::vector<unsigned char> const &memory) {
    SetLoggerContextForFunction context("CrLi import");

    startwith(C64Import::GD_FORMAT_CRLI);

    /* try autodetect */
    bool b = true;
    if (memory[0x6ffc] != 'V' || memory[0x6ffd] != '3' || memory[0x6ffe] != '.' || memory[0x6fff] != '0') /* version string */
        b = false;
    for (int x = 0x7060; x < 0x7090; x++) /* cave selection table */
        if (memory[x] != 0 && memory[x] != 1 && memory[x] != 255)
            b = false;
    if (!b) {
        gd_debug("Assumptions failed for crazy light.");
        return false;
    }

    int caves = 0;
    for (int i = 0; i < 48; i++) {
        if (memory[0x7060 + i] != 0xFF) {
            caves++;

            int startpos = memory[0x7000 + i] + memory[0x7030 + i] * 256L;
            int pos = startpos;
            int cavepos = 0;
            /* 'decompress' data, to see how many bytes there are */
            while (cavepos < 0x3b0) {   /* <- loop until the uncompressed reaches its size */
                if (memory[pos] == 0xbf) {
                    /* escaped byte */
                    cavepos += memory[pos + 2]; /* number of bytes */
                    pos += 3;
                } else {
                    /* plain data */
                    cavepos++;
                    pos++;
                }
            }
            pos += 14;  /* add 14 bytes for name */
            int n = pos - startpos; /* bytes to copy */
            startpos -= 14; /* name is BEFORE the cave data */
            gd_debug("Cave %d, addr: %04x (%d), length %d bytes", caves, pos, pos % n);

            out[outpos++] = memory[0x7060 + i]; /* is_selectable */
            for (int j = 0; j < n; j++)
                out[outpos++] = memory[startpos++];
        }
    }
    gd_debug("Found %d crazy light caves in %d bytes!", caves, outpos);
    return true;
}

static bool try_crdr(std::vector<unsigned char> const &memory) {
    SetLoggerContextForFunction context("CrDr import");
    int i, caves;

    startwith(C64Import::GD_FORMAT_CRDR_7);

    /* try autodetect */
//  if (memory[0x6ffc]!='V' || memory[0x6ffd]!='3' || memory[0x6ffe]!='.' || memory[0x6fff]!='0')   /* version string */
//      b=0;

    caves = 0;
    for (i = 0; i < 20; i++) {
        caves++;

        int startpos = memory[0x7500 + i] + memory[0x7580 + i] * 256L;
        int pos = startpos;

        /* name */
        for (int j = 0; j < 14; j++)
            out[outpos++] = memory[0x8c00 + i * 16 + j];

        /* selectable */
        /* XXX seems to be broken; rather set unselectable intermissions */
        out[outpos++] =/*memory[0x7410+caves]*/(caves % 5) != 0;

        /* cave data */
        for (int j = 0; j < 0x49; j++)
            out[outpos++] = memory[pos++];
        /* cave objects */
        while (memory[pos] != 0xff) {
            switch (memory[pos]) {
                case 1: /* point */
                    for (int j = 0; j < 4; j++)
                        out[outpos++] = memory[pos++];
                    break;
                case 2: /* rectangle */
                    for (int j = 0; j < 6; j++)
                        out[outpos++] = memory[pos++];
                    break;
                case 3: /* fillrect */
                    for (int j = 0; j < 6; j++)
                        out[outpos++] = memory[pos++];
                    break;
                case 4: /* line */
                    for (int j = 0; j < 6; j++)
                        out[outpos++] = memory[pos++];
                    break;
                case 6: /* ??? */
                    for (int j = 0; j < 5; j++) {
                        out[outpos++] = memory[pos++];
                    }
                    break;
                case 7: /* ??? */
                    for (int j = 0; j < 3; j++) {
                        out[outpos++] = memory[pos++];
                    }
                    break;
                case 11: /* raster */
                    for (int j = 0; j < 8; j++)
                        out[outpos++] = memory[pos++];
                    break;
                default:
                    gd_debug("Unknown crdr object code %x at %d. Aborting.", memory[pos], pos);
                    return false;
                    break;
            }
        }
        out[outpos++] = memory[pos++];  /* copy $ff */
        int n = pos - startpos; /* bytes to copied */
        gd_debug("Cave %d, addr: %04x (%d), length %d bytes", caves, pos, pos, n);
    }
    gd_debug("Found %d crazy dream caves in %d bytes!", caves, outpos);
    return true;
}



/* save bd2 caves from memory map */
static bool try_bd2(std::vector<unsigned char> const &memory, bool atari) {
    const int cavepointers = atari ? 0x86b0 : 0x89b0;
    const int cavecolors = atari ? 0x86d8 : 0x89d8;
    int i;
    int unsupported = 0, uns[256];
    const char *type = atari ? "atari" : "c64";
    SetLoggerContextForFunction context(Printf("BD2 import %s", type));

    /* 256 counters for the 256 possible bytes (8bit). mark each unsupported extension found. */
    /* if more than 5 types found, bail out with error */
    for (i = 0; i < 256; ++i)
        uns[i] = 0;

    startwith(C64Import::GD_FORMAT_BD2);

    /* 4*5 (4 groups * 5 caves (4cave+1intermission)) */
    for (i = 0; i < 4 * 5; i++) {
        int lo, hi, j, pos, start;
        int amount, n, mappos;

        lo = cavepointers + i * 2;
        hi = cavepointers + i * 2 + 1;
        start = pos = memory[hi] * 256L + memory[lo];
        if (pos > (0xffff - 0x0400)) {
            gd_debug("Cannot interpret memory contents as BD2 -- invalid cave pointer.");
            return false;
        }
        if (i < 16)
            gd_debug("Cave %c, addr: %04x", char(i + 'A'), pos);
        else
            gd_debug("Intermission %d, addr: %04x", int(i - 15), pos);

        /* copy */
        /* first bytes: cave options */
        for (j = 0; j <= 0x19; j++)
            out[outpos++] = memory[pos++];          /* cave options */

        j = memory[pos++];
        while (j != 0xFF) { /* végét jelenti; de kiírtuk ezt is */
            if (pos - start > 0x400) {
                /* bd1 caves cannot be this long */
                gd_debug("Data corrupt or detection failed for bd2 caves -- cave data too long.");
                return false;
            }
            out[outpos++] = j;
            switch (j) {
                case 0: /* line */
                    out[outpos++] = memory[pos++];  /* obj */
                    out[outpos++] = memory[pos++];  /* y */
                    out[outpos++] = memory[pos++];  /* x */
                    out[outpos++] = memory[pos++];  /* dir */
                    out[outpos++] = memory[pos++];  /* len */
                    break;
                case 1: /* rectangle (outline) */
                    out[outpos++] = memory[pos++];  /* obj */
                    out[outpos++] = memory[pos++];  /* y */
                    out[outpos++] = memory[pos++];  /* x */
                    out[outpos++] = memory[pos++];  /* h */
                    out[outpos++] = memory[pos++];  /* w */
                    break;
                case 2: /* fillrect */
                    out[outpos++] = memory[pos++];  /* obj */
                    out[outpos++] = memory[pos++];  /* y */
                    out[outpos++] = memory[pos++];  /* x */
                    out[outpos++] = memory[pos++];  /* h */
                    out[outpos++] = memory[pos++];  /* w */
                    out[outpos++] = memory[pos++];  /* fillobj */
                    break;
                case 3: /* point */
                    out[outpos++] = memory[pos++];  /* obj */
                    out[outpos++] = memory[pos++];  /* y */
                    out[outpos++] = memory[pos++];  /* x */
                    break;
                case 4: /* raster */
                    out[outpos++] = memory[pos++];  /* obj */
                    out[outpos++] = memory[pos++];  /* y */
                    out[outpos++] = memory[pos++];  /* x */
                    out[outpos++] = memory[pos++];  /* h */
                    out[outpos++] = memory[pos++];  /* w */
                    out[outpos++] = memory[pos++];  /* dy */
                    out[outpos++] = memory[pos++];  /* dx */
                    break;
                case 5: /* profi boulder extension: bitmap */
                    out[outpos++] = memory[pos++];  /* obj */
                    amount = memory[pos++];
                    out[outpos++] = amount; /* amount */
                    out[outpos++] = memory[pos++];  /* target msb */
                    out[outpos++] = memory[pos++];  /* target lsb */
                    for (n = 0; n < amount; ++n)
                        out[outpos++] = memory[pos++];  /* data */
                    break;
                case 6: /* join */
                    out[outpos++] = memory[pos++];  /* add to this */
                    out[outpos++] = memory[pos++];  /* add this */
                    out[outpos++] = memory[pos++];  /* dy*40+dx */
                    break;
                case 7: /* slime permeabilty */
                    out[outpos++] = memory[pos++];  /* perm */
                    break;
                case 9: /* profi boulder extension: plck map */
                    lo = memory[pos++];
                    hi = memory[pos++];
                    mappos = hi * 256 + lo;
                    out[outpos++] = memory[pos++];  /* inbox y */
                    out[outpos++] = memory[pos++];  /* inbox x */
                    for (n = 0; n < 40 * (22 - 2) / 2; n++) /* 40*20 caves, upper and lower row not contained, 1byte/2 elements */
                        out[outpos++] = memory[mappos + n];
                    break;
                default:
                    if (uns[j] == 0) {
                        /* not seen this extension previously */
                        gd_debug("Found unsupported bd2 extension n.%d", j);
                        unsupported++;
                        uns[j] = 1; /* mark the newly found unknown extension */
                    }
                    if (unsupported > 5) {
                        /* found to many unsupported extensions - this can't be bd2 */
                        gd_debug("Data corrupt or detection failed for bd2 caves -- too many unknown extensions.");
                        return false;
                    }
                    break;
            }
            j = memory[pos++];  /* read next */
        }
        out[outpos++] = j; /* closing 0xff */
        out[outpos++] = memory[pos++];  /* animation */

        /* color table */
        lo = cavecolors + i * 2;
        hi = cavecolors + i * 2 + 1;
        pos = memory[hi] * 256L + memory[lo]; /* pointer to the three colors */
        out[outpos++] = memory[pos++];
        out[outpos++] = memory[pos++];
        out[outpos++] = memory[pos++];
    }

    return true;
}


/* save plck caves from memory map */
static bool try_1stb(std::vector<unsigned char> const &memory) {
    SetLoggerContextForFunction context("1stB import");
    startwith(C64Import::GD_FORMAT_FIRSTB);

    /* there must be exactly 20 caves, according to bd inside faq. */
    int i;
    for (i = 0; i < 20; i++) {
        int pos = 0x7010 + 0x400 * i;

        /* 1stb caves have cave time, diamonds and the like encoded in bcd numbers,
           one digit in each bytes. check that. */
        for (int j = 0x370; j < 0x379 + 3; j++)
            if (memory[pos + j] > 9) {
                gd_debug("Data corrupt or detection failed for 1stb caves.");
                return false;
            }
        gd_debug("Cave %d, addr: %04x", i + 1, pos);

        for (int j = 0; j < 0x400; ++j)
            out[outpos++] = memory[pos++];
    }
    gd_debug("Found %d 1stb caves!", i);
    return true;
}


/** load memory dump, and fill memory map. returns the memory dump, or throws an exception describing the problem. */
std::vector<unsigned char> load_memory_dump(unsigned char const *file, size_t length) {
    const unsigned char vicemagic[] = {
        0x56, 0x49, 0x43, 0x45, 0x20, 0x53, 0x6E, 0x61, 0x70, 0x73, 0x68, 0x6F,
        0x74, 0x20, 0x46, 0x69, 0x6C, 0x65, 0x1A, 0x01, 0x00, 0x43, 0x36, 0x34
    };
    std::vector<unsigned char> memory(65536);

    if (memcmp(vicemagic, file, sizeof(vicemagic)) == 0) {
        /* FOUND a vice snapshot file. */
        gd_debug("File is a VICE snapshot.");
        memcpy(&memory[0], file + 0x80, 65536);
        return memory;
    }

    /* 65538 bytes: we hope that this is a full-memory map saved by vice. check it. */
    if (length == 65538) {
        /* check start address */
        if (file[0] != 0 || file[1] != 0)
            throw std::runtime_error(
                "Memory map should begin from address 0000. "
                "Use save \"filename\" 0 0000 ffff in vice monitor.");
        memcpy(&memory[0], file + 2, 65536);
        gd_debug("%s looks like a proper VICE memory map.");
        return memory;
    }

    /* or maybe a 64k map saved by atari800. read it. */
    if (length == 65536) {
        memcpy(&memory[0], file, 65536);
        gd_debug("%s is maybe an atari800 memory map.");
        return memory;
    }

    /* or maybe a 64k map saved by emu64. read it. */
    if (length == 65535) {
        memcpy(&memory[0], file, 65535);
        memory[65535] = 0;
        gd_debug("%s is maybe an emu64 memory map.");
        return memory;
    }

    throw std::runtime_error(
        "Memory map file should be 65536+2 bytes long or 65536 bytes long. "
        "Use save \"filename\" 0 0000 ffff in vice monitor. "
        "Use write 0000 ffff filename in atari800 monitor.");
}


std::vector<unsigned char> gdash_binary_import(std::vector<unsigned char> const &memory) {
    if (try_plck(memory) || try_atari_plck(memory) || try_bd1(memory, false) || try_bd1(memory, true)
            || try_bd2(memory, false) || try_bd2(memory, true) || try_crli(memory) || try_1stb(memory) || try_crdr(memory)) {
        /* write data length in little endian */
        out[8] = ((outpos - 12)) & 0xff;
        out[9] = ((outpos - 12) >> 8) & 0xff;
        out[10] = ((outpos - 12) >> 16) & 0xff;
        out[11] = ((outpos - 12) >> 24) & 0xff;

        return out;
    }

    throw std::runtime_error("Could not import cave data -- unknown cave format!");
}

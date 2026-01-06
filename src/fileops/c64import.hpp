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
#ifndef C64IMPORT_HPP_INCLUDED
#define C64IMPORT_HPP_INCLUDED

#include "config.h"

#include <vector>
#include <glib.h>
#include "cave/cavetypes.hpp"

// forward declaration
class CaveStored;


/// A pack of routines to import C64 data files created by any2gdash.
class C64Import {
public:
    // import tables for elements and characters in texts
    static const char bd_internal_character_encoding[];
    static GdElementEnum const import_table_crli[];
    static GdElementEnum const import_table_bd1[0x40];
    static GdElementEnum const import_table_plck[0x10];
    static GdElementEnum const import_table_1stb[0x80];
    static GdElementEnum const import_table_crdr[0x100];

    /// File formats known to the c64 importer.
    enum GdCavefileFormat {
        GD_FORMAT_BD1,          ///<  boulder dash 1
        GD_FORMAT_BD1_ATARI,    ///<  boulder dash 1 atari version
        GD_FORMAT_BD2,          ///<  boulder dash 2 with rockford's extensions
        GD_FORMAT_BD2_ATARI,    ///<  boulder dash 2, atari version
        GD_FORMAT_PLC,          ///<  construction kit
        GD_FORMAT_PLC_ATARI,    ///<  construction kit, atari version
        GD_FORMAT_DLB,          ///<  no one's delight boulder dash
        GD_FORMAT_CRLI,         ///<  crazy light construction kit
        GD_FORMAT_CRDR_7,       ///<  crazy dream 7
        GD_FORMAT_FIRSTB,       ///<  first boulder
        GD_FORMAT_BD1_PLUS,     ///<  BoulderDash+
        GD_FORMAT_BD2_CADUTA,   ///<  Caduta Massi

        GD_FORMAT_UNKNOWN,      ///<  unknown format
    };

    static char const *gd_format_strings[GD_FORMAT_UNKNOWN + 1];

    enum ImportHack {
        None,
        Crazy_Dream_1,
        Crazy_Dream_7,
        Crazy_Dream_9,
        Deluxe_Caves_1,
        Deluxe_Caves_3,
        Masters_Boulder,
        BoulderDashPlus,
        BoulderDashCaduta
    };

    // set some cave parameters for original c64 engines
    static void cave_set_bd1_defaults(CaveStored &cave);
    static void cave_set_bd2_defaults(CaveStored &cave);
    static void cave_set_plck_defaults(CaveStored &cave);
    static void cave_set_1stb_defaults(CaveStored &cave);
    static void cave_set_crdr_7_defaults(CaveStored &cave);
    static void cave_set_crli_defaults(CaveStored &cave);

    static void cave_set_engine_defaults(CaveStored &cave, GdEngineEnum engine);

    // import helper routines
    typedef GdElementEnum(*ImportFuncArray)(unsigned char const data[], unsigned i);
    typedef GdElementEnum(*ImportFuncByte)(unsigned char const c, unsigned i);

    static GdElementEnum bd1_import(unsigned char const data[], unsigned i);
    static GdElementEnum bd1_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum deluxecaves_1_import(unsigned char const data[], unsigned i);
    static GdElementEnum deluxecaves_1_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum deluxecaves_3_import(unsigned char const data[], unsigned i);
    static GdElementEnum deluxecaves_3_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum firstboulder_import(unsigned char const data[], unsigned i);
    static GdElementEnum firstboulder_import_byte(unsigned char const c, unsigned i);
    static GdElementEnum crazylight_import(unsigned char const data[], unsigned i);
    static GdElementEnum crazylight_import_byte(unsigned char const c, unsigned i);

    static int slime_plck(unsigned c64_data);
    static GdString name_from_c64_bin(const unsigned char *data);

    // import routines
    static int cave_copy_from_bd1(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format, ImportHack hack);
    static int cave_copy_from_bd2(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format, ImportHack hack);
    static int cave_copy_from_plck(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format);
    static int cave_copy_from_dlb(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_1stb(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_crdr_7(CaveStored &cave, const guint8 *data, int remaining_bytes);
    static int cave_copy_from_crli(CaveStored &cave, const guint8 *data, int remaining_bytes);

    static GdCavefileFormat imported_get_format(const guint8 *buf);
    static std::vector<CaveStored> caves_import_from_buffer(const guint8 *buf, int length);
};
#endif


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

#include "fileops/loadfile.hpp"

#include <glib/gi18n.h>
#include <stdexcept>
#include <fstream>
#include "cave/caveset.hpp"
#include "fileops/binaryimport.hpp"
#include "fileops/brcimport.hpp"
#include "fileops/c64import.hpp"
#include "fileops/bdcffload.hpp"
#include "misc/logger.hpp"
#include "misc/util.hpp"
#include "misc/autogfreeptr.hpp"


/** load some caveset from the binary data in the buffer.
 * the length may be -1, if the caller is pretty sure of what he's doing. */
CaveSet create_from_buffer(const unsigned char *buffer, int length, char const *filename) {
    /* set logging context to filename */
    SetLoggerContextForFunction finally(gd_tostring_free(g_filename_display_basename(filename)));

    /* try to load as a .GDS file */
    if ((length >= 12 || length == -1) && C64Import::imported_get_format(buffer) != C64Import::GD_FORMAT_UNKNOWN) {
        std::vector<CaveStored> cavelist = C64Import::caves_import_from_buffer(buffer, length);
        /* if unable to load, exit here. error was reported by import_from_buffer() */
        if (cavelist.empty())
            throw std::runtime_error(_("Error loading GDS file."));
        /* no serious error :) */
        CaveSet newcaves;
        newcaves.caves = std::move(cavelist);
        newcaves.last_selected_cave = newcaves.first_selectable_cave_index();
        newcaves.set_name_from_filename(filename);
        return newcaves;
    }

    /* try to load as a BRC file (boulder remake) */
    if (g_str_has_suffix(filename, ".brc") || g_str_has_suffix(filename, "*.BRC")) {
        if (length != 96000) {
            throw std::runtime_error(_("BRC files must be 96000 bytes long."));
        }
        CaveSet newcaves;
        brc_import(newcaves, (guint8 *) buffer);
        newcaves.last_selected_cave = newcaves.first_selectable_cave_index();
        newcaves.set_name_from_filename(filename);
        return newcaves;
    }

    /* try to load as BDCFF */
    if (g_str_has_suffix(filename, ".bd") || g_str_has_suffix(filename, ".BD")) {
        CaveSet newcaves = load_from_bdcff((char const *) buffer);
        newcaves.last_selected_cave = newcaves.first_selectable_cave_index();
        /* remember filename, as the input is a bdcff file */
        if (g_path_is_absolute(filename)) {
            newcaves.filename = filename;
        } else {
            /* make an absolute filename if needed */
            AutoGFreePtr<char> currentdir(g_get_current_dir());
            AutoGFreePtr<char> absolute(g_build_path(G_DIR_SEPARATOR_S, (char*) currentdir, filename, NULL));
            newcaves.filename = absolute;
        }
        return newcaves;
    }

    /* if could not determine file format so far, try to load as a snapshot file */
    if (g_str_has_suffix(filename, ".vsf") || g_str_has_suffix(filename, ".VSF")
            || length == 65535 || length == 65536 || length == 65538) {
        std::vector<unsigned char> memory = load_memory_dump(buffer, length);
        std::vector<unsigned char> imported = gdash_binary_import(memory);
        return create_from_buffer(&imported[0], imported.size(), filename);
    }

    throw std::runtime_error(_("Cannot determine file format."));
}


/**
 * Create a caveset by loading it from a file.
 * @param filename The name of the file, which can be BDCFF or other binary formats.
 * @return The caveset loaded. If impossible to load, throws an exception.
 */
CaveSet load_caveset_from_file(const char *filename) {
    std::vector<unsigned char> contents = load_file_to_vector(filename);
    /* -1 because the loader adds a terminating zero */
    return create_from_buffer(&contents[0], contents.size() - 1, filename);
}


/**
 * Load a file to an array of bytes.
 * @param filename The name of the file.
 * @return The file loaded. If impossible to load, throws an exception.
 */
std::vector<unsigned char> load_file_to_vector(char const *filename) {
    /* open file */
    std::ifstream is;
    is.open(filename, std::ios::in | std::ios::binary);
    if (!is)
        throw std::runtime_error(_("Unable to open file."));
    /* check size */
    is.seekg(0, is.end);
    int filesize = is.tellg();
    is.seekg(0, is.beg);
    if (filesize > (2 * 1 << 20))
        throw std::runtime_error(_("File bigger than 2MiB, refusing to load."));
    /* read file. the vector will be one bytes bigger, so it can be added a terminating zero char. */
    std::vector<unsigned char> contents(filesize + 1);
    if (!is.read((char *) &contents[0], filesize))
        throw std::runtime_error(_("Unable to read file."));
    is.close();
    contents[filesize] = '\0';

    return contents;
}

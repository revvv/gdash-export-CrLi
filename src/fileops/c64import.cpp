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

#include "config.h"

#include "fileops/c64import.hpp"

#include <glib/gi18n.h>
#include "settings.hpp"
#include "cave/cavestored.hpp"
#include "misc/logger.hpp"
#include "misc/printf.hpp"
#include "misc/util.hpp"
#include "cave/elementproperties.hpp"
#include "cave/object/caveobjectcopypaste.hpp"
#include "cave/object/caveobjectfillrect.hpp"
#include "cave/object/caveobjectjoin.hpp"
#include "cave/object/caveobjectline.hpp"
#include "cave/object/caveobjectmaze.hpp"
#include "cave/object/caveobjectpoint.hpp"
#include "cave/object/caveobjectrandomfill.hpp"
#include "cave/object/caveobjectraster.hpp"
#include "cave/object/caveobjectrectangle.hpp"

/* deluxe caves 3
 * - diagonal movements
 * 18 = glued dirt
 * 19 = glued diamond
 * 1a = glued stone
 *
 * deluxe caves 1
 * 6=steel explodable
 * 2a=O_BRICK_NON_SLOPED;
 *
 * no one's boulder 31, cave b, cave d
 * 2f = glued dirt (non eatable)
 */

char const *C64Import::gd_format_strings[GD_FORMAT_UNKNOWN + 1] = {
    /* these strings should come in the same order as the enum */
    "GDashBD1",     // GD_FORMAT_BD1
    "GDashB1A",     // GD_FORMAT_BD1_ATARI
    "GDashBD2",     // GD_FORMAT_BD2
    "GDashB2A",     // GD_FORMAT_BD2_ATARI
    "GDashPLC",     // GD_FORMAT_PLC
    "GDashPCA",     // GD_FORMAT_PLC_ATARI
    "GDashDLB",     // GD_FORMAT_DLB
    "GDashCRL",     // GD_FORMAT_CRLI
    "GDashCD7",     // GD_FORMAT_CRDR_7
    "GDash1ST",     // GD_FORMAT_FIRSTB
    "GDashB1+",     // GD_FORMAT_BD1_PLUS
    "GDashB20",     // GD_FORMAT_BD2_CADUTA
    NULL
};

/// conversion table for imported bd1 caves.
GdElementEnum const C64Import::import_table_bd1[0x40] = {
    /*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
    /*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_STEEL_EXPLODABLE, O_STEEL,
    /*  8 */ O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    /*  c */ O_FIREFLY_1_scanned, O_FIREFLY_2_scanned, O_FIREFLY_3_scanned, O_FIREFLY_4_scanned,
    /* 10 */ O_STONE, O_STONE_scanned, O_STONE_F, O_STONE_F_scanned,
    /* 14 */ O_DIAMOND, O_DIAMOND_scanned, O_DIAMOND_F, O_DIAMOND_F_scanned,
    /* 18 */ O_ACID, O_ACID_scanned, O_UNKNOWN, O_EXPLODE_0,  /* ACID: marek roth extension in crazy dream 3 */
    /* 1c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
    /* 20 */ O_PRE_DIA_0, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
    /* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
    /* 28 */ O_PRE_PL_3, O_UNKNOWN, O_H_EXPANDING_WALL, O_H_EXPANDING_WALL_scanned,
    /* 2c */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_DIRT_GLUED,
    /* 30 */ O_BUTTER_4, O_BUTTER_1, O_BUTTER_2, O_BUTTER_3,
    /* 34 */ O_BUTTER_4_scanned, O_BUTTER_1_scanned, O_BUTTER_2_scanned, O_BUTTER_3_scanned,
    /* 38 */ O_PLAYER, O_PLAYER_scanned, O_AMOEBA, O_AMOEBA_scanned,
    /* 3c */ O_VOODOO, O_INVIS_OUTBOX, O_SLIME, O_UNKNOWN
};

/// conversion table for imported plck caves.
GdElementEnum const C64Import::import_table_plck[0x10] = {
    /*  0 */ O_STONE, O_DIAMOND, O_MAGIC_WALL, O_BRICK,
    /*  4 */ O_STEEL, O_H_EXPANDING_WALL, O_VOODOO, O_DIRT,
    /*  8 */ O_FIREFLY_1, O_BUTTER_4, O_AMOEBA, O_SLIME,
    /* 12 */ O_PRE_INVIS_OUTBOX, O_PRE_OUTBOX, O_INBOX, O_SPACE
};

/// conversion table for imported 1stb caves.
GdElementEnum const C64Import::import_table_1stb[0x80] = {
    /*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
    /*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
    /*  8 */ O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    /*  c */ O_FIREFLY_1_scanned, O_FIREFLY_2_scanned, O_FIREFLY_3_scanned, O_FIREFLY_4_scanned,
    /* 10 */ O_STONE, O_STONE_scanned, O_STONE_F, O_STONE_F_scanned,
    /* 14 */ O_DIAMOND, O_DIAMOND_scanned, O_DIAMOND_F, O_DIAMOND_F_scanned,
    /* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
    /* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_0,
    /* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
    /* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
    /* 28 */ O_PRE_PL_3, O_CLOCK, O_H_EXPANDING_WALL, O_H_EXPANDING_WALL_scanned,   /* CLOCK: not mentioned in marek's bd inside faq */
    /* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_EXPANDING_WALL_SWITCH, O_EXPANDING_WALL_SWITCH,
    /* 30 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
    /* 34 */ O_BUTTER_3_scanned, O_BUTTER_4_scanned, O_BUTTER_1_scanned, O_BUTTER_2_scanned,
    /* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
    /* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
    /* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
    /* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
    /* 48 */ O_BLADDER_8, O_BLADDER_8, O_EXPLODE_0, O_EXPLODE_1,
    /* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
    /* 50 */ O_PLAYER, O_PLAYER_scanned, O_PLAYER_BOMB, O_PLAYER_BOMB_scanned,
    /* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED, O_VOODOO, O_AMOEBA,
    /* 58 */ O_AMOEBA_scanned, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
    /* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
    /* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
    /* 64 */ O_GHOST, O_GHOST_scanned, O_GHOST_EXPL_1, O_GHOST_EXPL_2,
    /* 68 */ O_GHOST_EXPL_3, O_GHOST_EXPL_4, O_GRAVESTONE, O_STONE_GLUED,
    /* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_TIME_PENALTY,
    /* 70 */ O_WAITING_STONE, O_WAITING_STONE_scanned, O_CHASING_STONE, O_CHASING_STONE_scanned,
    /* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
    /* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
    /* 7c */ O_BITER_1_scanned, O_BITER_2_scanned, O_BITER_3_scanned, O_BITER_4_scanned,
};

/// conversion table for imported crazy dream caves.
GdElementEnum const C64Import::import_table_crdr[0x100] = {
    /*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
    /*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
    /*  8 */ O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    /*  c */ O_FIREFLY_1_scanned, O_FIREFLY_2_scanned, O_FIREFLY_3_scanned, O_FIREFLY_4_scanned,
    /* 10 */ O_STONE, O_STONE_scanned, O_STONE_F, O_STONE_F_scanned,
    /* 14 */ O_DIAMOND, O_DIAMOND_scanned, O_DIAMOND_F, O_DIAMOND_F_scanned,
    /* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
    /* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_0,    /* 6 different stages */
    /* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
    /* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
    /* 28 */ O_PRE_PL_3, O_CLOCK, O_H_EXPANDING_WALL, O_H_EXPANDING_WALL_scanned,   /* CLOCK: not mentioned in marek's bd inside faq */
    /* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_EXPANDING_WALL_SWITCH, O_EXPANDING_WALL_SWITCH,
    /* 30 */ O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
    /* 34 */ O_BUTTER_3_scanned, O_BUTTER_4_scanned, O_BUTTER_1_scanned, O_BUTTER_2_scanned,
    /* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
    /* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
    /* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
    /* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
    /* 48 */ O_BLADDER_8, O_BLADDER_8, O_EXPLODE_0, O_EXPLODE_1,
    /* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
    /* 50 */ O_PLAYER, O_PLAYER_scanned, O_PLAYER_BOMB, O_PLAYER_BOMB_scanned,
    /* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED, O_VOODOO, O_AMOEBA,
    /* 58 */ O_AMOEBA_scanned, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
    /* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
    /* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
    /* 64 */ O_GHOST, O_GHOST_scanned, O_GHOST_EXPL_1, O_GHOST_EXPL_2,
    /* 68 */ O_GHOST_EXPL_3, O_GHOST_EXPL_4, O_GRAVESTONE, O_STONE_GLUED,
    /* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_TIME_PENALTY,
    /* 70 */ O_WAITING_STONE, O_WAITING_STONE_scanned, O_CHASING_STONE, O_CHASING_STONE_scanned,
    /* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
    /* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
    /* 7c */ O_BITER_1_scanned, O_BITER_2_scanned, O_BITER_3_scanned, O_BITER_4_scanned,

    /* 80 */ O_POT, O_PLAYER_STIRRING, O_GRAVITY_SWITCH, O_GRAVITY_SWITCH,
    /* 84 */ O_PNEUMATIC_HAMMER, O_PNEUMATIC_HAMMER, O_BOX, O_BOX,
    /* 88 */ O_UNKNOWN, O_UNKNOWN, O_ACID, O_ACID_scanned,
    /* 8c */ O_KEY_1, O_KEY_2, O_KEY_3, O_UNKNOWN,
    /* 90 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* 94 */ O_UNKNOWN, O_TELEPORTER, O_UNKNOWN, O_SKELETON,
    /* 98 */ O_WATER, O_WATER_16, O_WATER_15, O_WATER_14,
    /* 9c */ O_WATER_13, O_WATER_12, O_WATER_11, O_WATER_10,
    /* a0 */ O_WATER_9, O_WATER_8, O_WATER_7, O_WATER_6,
    /* a4 */ O_WATER_5, O_WATER_4, O_WATER_3, O_WATER_2,
    /* a8 */ O_WATER_1, O_COW_ENCLOSED_1, O_COW_ENCLOSED_2, O_COW_ENCLOSED_3,
    /* ac */ O_COW_ENCLOSED_4, O_COW_ENCLOSED_5, O_COW_ENCLOSED_6, O_COW_ENCLOSED_7,
    /* b0 */ O_COW_1, O_COW_2, O_COW_3, O_COW_4,
    /* b4 */ O_COW_1_scanned, O_COW_2_scanned, O_COW_3_scanned, O_COW_4_scanned,
    /* b8 */ O_DIRT_GLUED, O_STEEL_EXPLODABLE, O_DOOR_1, O_DOOR_2,
    /* bc */ O_DOOR_3, O_FALLING_WALL, O_FALLING_WALL_F, O_FALLING_WALL_F_scanned,
    /* c0 */ O_WALLED_DIAMOND, O_UNKNOWN, O_WALLED_KEY_1, O_WALLED_KEY_2,
    /* c4 */ O_WALLED_KEY_3, O_BRICK, O_UNKNOWN, O_DIRT,    /* c5=brick?! (vital key), c7=dirt?! (think twice -  it has a code which will change it to dirt.) */
    /* c8 */ O_DIRT2, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* cc */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* d0 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* d4 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* d8 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* dc */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
    /* e0 */ O_ALT_FIREFLY_1, O_ALT_FIREFLY_2, O_ALT_FIREFLY_3, O_ALT_FIREFLY_4,
    /* e4 */ O_ALT_FIREFLY_1_scanned, O_ALT_FIREFLY_2_scanned, O_ALT_FIREFLY_3_scanned, O_ALT_FIREFLY_4_scanned,
    /* e8 */ O_ALT_BUTTER_3, O_ALT_BUTTER_4, O_ALT_BUTTER_1, O_ALT_BUTTER_2,
    /* ec */ O_ALT_BUTTER_3_scanned, O_ALT_BUTTER_4_scanned, O_ALT_BUTTER_1_scanned, O_ALT_BUTTER_2_scanned,
    /* f0 */ O_WATER, O_WATER, O_WATER, O_WATER,
    /* f4 */ O_WATER, O_WATER, O_WATER, O_WATER,
    /* f8 */ O_WATER, O_WATER, O_WATER, O_WATER,
    /* fc */ O_WATER, O_WATER, O_WATER, O_WATER,
};

/// conversion table for imported 1stb caves.
/// @todo check O_PRE_DIA_0 and O_EXPLODE_0
GdElementEnum const C64Import::import_table_crli[0x80] = {
    /*  0 */ O_SPACE, O_DIRT, O_BRICK, O_MAGIC_WALL,
    /*  4 */ O_PRE_OUTBOX, O_OUTBOX, O_PRE_INVIS_OUTBOX, O_INVIS_OUTBOX,
    /*  8 */ O_FIREFLY_1, O_FIREFLY_2, O_FIREFLY_3, O_FIREFLY_4,
    /*  c */ O_FIREFLY_1_scanned, O_FIREFLY_2_scanned, O_FIREFLY_3_scanned, O_FIREFLY_4_scanned,
    /* 10 */ O_STONE, O_STONE_scanned, O_STONE_F, O_STONE_F_scanned,
    /* 14 */ O_DIAMOND, O_DIAMOND_scanned, O_DIAMOND_F, O_DIAMOND_F_scanned,
    /* 18 */ O_PRE_CLOCK_1, O_PRE_CLOCK_2, O_PRE_CLOCK_3, O_PRE_CLOCK_4,
    /* 1c */ O_BITER_SWITCH, O_BITER_SWITCH, O_BLADDER_SPENDER, O_PRE_DIA_0,    /* 6 different stages, the first is the pre_dia_0 */
    /* 20 */ O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4,
    /* 24 */ O_PRE_DIA_5, O_INBOX, O_PRE_PL_1, O_PRE_PL_2,
    /* 28 */ O_PRE_PL_3, O_CLOCK, O_H_EXPANDING_WALL, O_H_EXPANDING_WALL_scanned,   /* CLOCK: not mentioned in marek's bd inside faq */
    /* 2c */ O_CREATURE_SWITCH, O_CREATURE_SWITCH, O_EXPANDING_WALL_SWITCH, O_EXPANDING_WALL_SWITCH,
    /*-- version1 -->*/
    // 30 // O_BUTTER_3, O_BUTTER_4, O_BUTTER_1, O_BUTTER_2,
    // 34 // O_BUTTER_3_scanned, O_BUTTER_4_scanned, O_BUTTER_1_scanned, O_BUTTER_2_scanned,
    /*<-- version1 --*/
    /*-- version2 -->*/
    // 30 // O_BUTTER_4, O_BUTTER_1, O_BUTTER_2, O_BUTTER_3, // fix order
    // 34 // O_BUTTER_4_scanned, O_BUTTER_1_scanned, O_BUTTER_2_scanned, O_BUTTER_3_scanned, // fix order
    /*<-- version2 --*/
    /*-- version3 -->*/// (right "B")      // (up "b")         // (left "C")       // (down "c")
    /* 30 */              O_BUTTER_3        , O_BUTTER_2        , O_BUTTER_1        , O_BUTTER_4,          // fix order again
    /* 34 */              O_BUTTER_3_scanned, O_BUTTER_2_scanned, O_BUTTER_1_scanned, O_BUTTER_4_scanned,  // fix order again
    /*<-- version3 --*/
    /* 38 */ O_STEEL, O_SLIME, O_BOMB, O_SWEET,
    /* 3c */ O_PRE_STONE_1, O_PRE_STONE_2, O_PRE_STONE_3, O_PRE_STONE_4,
    /* 40 */ O_BLADDER, O_BLADDER_1, O_BLADDER_2, O_BLADDER_3,
    /* 44 */ O_BLADDER_4, O_BLADDER_5, O_BLADDER_6, O_BLADDER_7,
    /* 48 */ O_BLADDER_8, O_BLADDER_8, O_EXPLODE_0, O_EXPLODE_1,
    /* 4c */ O_EXPLODE_2, O_EXPLODE_3, O_EXPLODE_4, O_EXPLODE_5,
    /* 50 */ O_PLAYER, O_PLAYER_scanned, O_PLAYER_BOMB, O_PLAYER_BOMB_scanned,
    /* 54 */ O_PLAYER_GLUED, O_PLAYER_GLUED, O_VOODOO, O_AMOEBA,
    /* 58 */ O_AMOEBA_scanned, O_BOMB_TICK_1, O_BOMB_TICK_2, O_BOMB_TICK_3,
    /* 5c */ O_BOMB_TICK_4, O_BOMB_TICK_5, O_BOMB_TICK_6, O_BOMB_TICK_7,
    /* 60 */ O_BOMB_EXPL_1, O_BOMB_EXPL_2, O_BOMB_EXPL_3, O_BOMB_EXPL_4,
    /* 64 */ O_ACID, O_ACID, O_FALLING_WALL, O_FALLING_WALL_F,
    /* 68 */ O_FALLING_WALL_F_scanned, O_BOX, O_GRAVESTONE, O_STONE_GLUED,
    /* 6c */ O_DIAMOND_GLUED, O_DIAMOND_KEY, O_TRAPPED_DIAMOND, O_GRAVESTONE,
    /* 70 */ O_WAITING_STONE, O_WAITING_STONE_scanned, O_CHASING_STONE, O_CHASING_STONE_scanned,
    /* 74 */ O_PRE_STEEL_1, O_PRE_STEEL_2, O_PRE_STEEL_3, O_PRE_STEEL_4,
    /* 78 */ O_BITER_1, O_BITER_2, O_BITER_3, O_BITER_4,
    /* 7c */ O_BITER_1_scanned, O_BITER_2_scanned, O_BITER_3_scanned, O_BITER_4_scanned,
};

/// Internal character (letter) codes in c64 games.
/// Used for converting names of caves imported from crli and other types of binary data.
/// Missing: "triple line" after >, diamond between ()s, player's head after ).
const char C64Import::bd_internal_character_encoding[] = "            ,!./0123456789:*<=>  ABCDEFGHIJKLMNOPQRSTUVWXYZ( ) _";

/// Set engine parameters to BD1 defaults in cave.
void C64Import::cave_set_bd1_defaults(CaveStored &cave) {
    for (unsigned i = 0; i < 5; ++i)
        cave.level_amoeba_threshold[i] = 200;
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = true;
    cave.amoeba_timer_wait_for_hatching = false;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.diagonal_movements = false;
    cave.voodoo_collects_diamonds = false;
    cave.voodoo_dies_by_stone = false;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_backwards = false;
    cave.creatures_direction_auto_change_on_start = false;
    cave.creatures_direction_auto_change_time = 0;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = true;
    cave.intermission_rewardlife = false;
    cave.magic_wall_stops_amoeba = false;
    cave.magic_wall_breakscan = true;
    cave.magic_timer_zero_is_infinite = true;
    cave.magic_timer_wait_for_hatching = false;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = false;
    cave.short_explosions = true;
    cave.slime_predictable = true;
    cave.snap_element = O_SPACE;
    cave.max_time = 999;

    cave.scheduling = GD_SCHEDULING_BD1;
    cave.pal_timing = true;
    cave.level_ckdelay[0] = 12;
    cave.level_ckdelay[1] = 6;
    cave.level_ckdelay[2] = 3;
    cave.level_ckdelay[3] = 1;
    cave.level_ckdelay[4] = 0;
};

/// Set engine parameters to BD2 defaults in cave.
void C64Import::cave_set_bd2_defaults(CaveStored &cave) {
    for (unsigned i = 0; i < 5; ++i)
        cave.level_amoeba_threshold[i] = 200;
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = false;
    cave.amoeba_timer_wait_for_hatching = false;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.diagonal_movements = false;
    cave.voodoo_collects_diamonds = false;
    cave.voodoo_dies_by_stone = false;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_backwards = false;
    cave.creatures_direction_auto_change_on_start = false;
    cave.creatures_direction_auto_change_time = 0;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = true;
    cave.intermission_rewardlife = false;
    cave.magic_wall_stops_amoeba = false; /* marek roth bd inside faq 3.0 */
    cave.magic_timer_zero_is_infinite = true;
    cave.magic_timer_wait_for_hatching = false;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = false;
    cave.short_explosions = true;
    cave.slime_predictable = true;
    cave.snap_element = O_SPACE;
    cave.max_time = 999;

    cave.pal_timing = true;
    cave.scheduling = GD_SCHEDULING_BD2;
    cave.level_ckdelay[0] = 9;  /* 180ms */
    cave.level_ckdelay[1] = 8;  /* 160ms */
    cave.level_ckdelay[2] = 7;  /* 140ms */
    cave.level_ckdelay[3] = 6;  /* 120ms */
    cave.level_ckdelay[4] = 6;  /* 120ms (!) same as level4 */
};

/// Set engine parameters to Construction Kit defaults in cave.
void C64Import::cave_set_plck_defaults(CaveStored &cave) {
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = false;
    cave.amoeba_timer_wait_for_hatching = false;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.border_scan_first_and_last = false;
    cave.diagonal_movements = false;
    cave.voodoo_collects_diamonds = false;
    cave.voodoo_dies_by_stone = false;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_backwards = false;
    cave.creatures_direction_auto_change_on_start = false;
    cave.creatures_direction_auto_change_time = 0;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = true;
    cave.intermission_rewardlife = false;
    cave.magic_wall_stops_amoeba = false;
    cave.magic_timer_zero_is_infinite = true;
    cave.magic_timer_wait_for_hatching = false;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = false;
    cave.short_explosions = true;
    cave.snap_element = O_SPACE;
    cave.max_time = 999;

    cave.pal_timing = true;
    cave.scheduling = GD_SCHEDULING_PLCK;
};

/// Set engine parameters to 1stB defaults in cave.
void C64Import::cave_set_1stb_defaults(CaveStored &cave) {
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = false;
    cave.amoeba_timer_wait_for_hatching = true;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.voodoo_collects_diamonds = true;
    cave.voodoo_dies_by_stone = true;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_direction_auto_change_on_start = true;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = false;
    cave.intermission_rewardlife = true;
    cave.magic_timer_zero_is_infinite = true;
    cave.magic_timer_wait_for_hatching = true;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = true;
    cave.short_explosions = false;
    cave.slime_predictable = true;
    cave.snap_element = O_SPACE;
    cave.max_time = 999;

    cave.pal_timing = true;
    cave.scheduling = GD_SCHEDULING_PLCK;
    cave.amoeba_enclosed_effect = O_PRE_DIA_1;  /* not immediately to diamond=but with animation */
    cave.dirt_looks_like = O_DIRT2;
};

/// Set engine parameters to Crazy Dream 7 defaults in cave.
void C64Import::cave_set_crdr_7_defaults(CaveStored &cave) {
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = false;
    cave.amoeba_timer_wait_for_hatching = true;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.voodoo_collects_diamonds = true;
    cave.voodoo_dies_by_stone = true;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_direction_auto_change_on_start = false;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = false;
    cave.intermission_rewardlife = true;
    cave.magic_timer_zero_is_infinite = false;
    cave.magic_timer_wait_for_hatching = true;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = true;
    cave.short_explosions = false;
    cave.slime_predictable = true;
    cave.snap_element = O_SPACE;
    cave.max_time = 999;

    cave.pal_timing = true;
    cave.scheduling = GD_SCHEDULING_CRDR;
    cave.amoeba_enclosed_effect = O_PRE_DIA_1;  /* not immediately to diamond=but with animation */
    cave.water_does_not_flow_down = true;
    cave.skeletons_worth_diamonds = 1;  /* in crdr=skeletons can also be used to open the gate */
    cave.gravity_affects_all = false; /* the intermission "survive" needs this flag */
};

/// Set engine parameters to Crazy Light defaults in cave.
void C64Import::cave_set_crli_defaults(CaveStored &cave) {
    cave.amoeba_growth_prob = 31250;
    cave.amoeba_fast_growth_prob = 250000;
    cave.amoeba_timer_started_immediately = false;
    cave.amoeba_timer_wait_for_hatching = true;
    cave.lineshift = true;
    cave.wraparound_objects = true;
    cave.voodoo_collects_diamonds = true;
    cave.voodoo_dies_by_stone = true;
    cave.voodoo_disappear_in_explosion = true;
    cave.voodoo_any_hurt_kills_player = false;
    cave.creatures_direction_auto_change_on_start = false;
    for (unsigned i = 0; i < 5; ++i)
        cave.level_hatching_delay_time[i] = 2;
    cave.intermission_instantlife = false;
    cave.intermission_rewardlife = true;
    cave.magic_timer_zero_is_infinite = false;
    cave.magic_timer_wait_for_hatching = true;
    cave.pushing_stone_prob = 250000;
    cave.pushing_stone_prob_sweet = 1000000;
    cave.active_is_first_found = true;
    cave.short_explosions = false;
    cave.slime_predictable = true;
    cave.max_time = 999;

    cave.pal_timing = true;
    cave.scheduling = GD_SCHEDULING_PLCK;
    cave.amoeba_enclosed_effect = O_PRE_DIA_1;  /* not immediately to diamond=but with animation */
};

/// Set engine parameters to some defaults in cave.
void C64Import::cave_set_engine_defaults(CaveStored &cave, GdEngineEnum engine) {
    switch (engine) {
        case GD_ENGINE_BD1:
            cave_set_bd1_defaults(cave);
            break;
        case GD_ENGINE_BD2:
            cave_set_bd2_defaults(cave);
            break;
        case GD_ENGINE_PLCK:
            cave_set_plck_defaults(cave);
            break;
        case GD_ENGINE_1STB:
            cave_set_1stb_defaults(cave);
            break;
        case GD_ENGINE_CRDR7:
            cave_set_crdr_7_defaults(cave);
            break;
        case GD_ENGINE_CRLI:
            cave_set_crli_defaults(cave);
            break;

        case GD_ENGINE_MAX:
            g_assert_not_reached();
            break;
    }
}

/// Import a BD1 element.
/// @param c The code byte used in BD1
/// @param i Index in the array, from which the byte was read. Only to be able to report the error correctly
/// @return The GDash element.
GdElementEnum C64Import::bd1_import_byte(unsigned char const c, unsigned i) {
    GdElementEnum e = O_UNKNOWN;
    if (c < G_N_ELEMENTS(import_table_bd1))
        e = import_table_bd1[c];
    if (e == O_UNKNOWN)
        gd_warning("Invalid BD1 element in imported file at cave data[%d]: %02x", i, unsigned(c));
    return nonscanned_pair(e);
}

GdElementEnum C64Import::bd1_import(unsigned char const data[], unsigned i) {
    return bd1_import_byte(data[i], i);
}

/// Import a deluxe caves 1 element.
/// Deluxe caves 1 contained a special element, non-sloped brick. No import table only for that.
/// @param c The code byte used in the imported cave data.
/// @param i Index in the array, from which the byte was read. Only to be able to report the error correctly
/// @return The GDash element.
GdElementEnum C64Import::deluxecaves_1_import_byte(unsigned char const c, unsigned i) {
    if (c == 0x2a)
        return O_BRICK_NON_SLOPED;
    return bd1_import_byte(c, i);
}

GdElementEnum C64Import::deluxecaves_1_import(unsigned char const data[], unsigned i) {
    return deluxecaves_1_import_byte(data[i], i);
}

/// Import a deluxe caves 1 element.
/// Deluxe caves 1 contained a special element, non-sloped brick. No import table only for that.
/// @param c The code byte used in the imported cave data.
/// @param i Index in the array, from which the byte was read. Only to be able to report the error correctly
/// @return The GDash element.
GdElementEnum C64Import::deluxecaves_3_import_byte(unsigned char const c, unsigned i) {
    switch (c) {
        case 0x2a:
            return O_BRICK_NON_SLOPED;
        case 0x18:
            return O_DIRT_GLUED;
        case 0x19:
            return O_DIAMOND_GLUED;
        case 0x1a:
            return O_STONE_GLUED;
    }
    return bd1_import_byte(c, i);
}

GdElementEnum C64Import::deluxecaves_3_import(unsigned char const data[], unsigned i) {
    return deluxecaves_3_import_byte(data[i], i);
}

/// Import a 1stB element.
/// @param c The code byte used in the original data file
/// @param i Index in the array, from which the byte was read. Only to be able to report the error correctly
/// @return The GDash element.
GdElementEnum C64Import::firstboulder_import_byte(unsigned char const c, unsigned i) {
    GdElementEnum e = O_UNKNOWN;
    if (c < G_N_ELEMENTS(import_table_1stb))
        e = import_table_1stb[c];
    if (e == O_UNKNOWN)
        gd_warning("Invalid 1stB element in imported file at cave data[%d]: %02x", i, unsigned(c));
    return nonscanned_pair(e);
}

GdElementEnum C64Import::firstboulder_import(unsigned char const data[], unsigned i) {
    return firstboulder_import_byte(data[i], i);
}

/// Import a Crazy Light element.
/// @param c The code byte used in the original data file
/// @param i Index in the array, from which the byte was read. Only to be able to report the error correctly
/// @return The GDash element.
GdElementEnum C64Import::crazylight_import_byte(unsigned char const c, unsigned i) {
    GdElementEnum e = O_UNKNOWN;
    if (c < G_N_ELEMENTS(import_table_crli))
        return import_table_crli[c];
    if (e == O_UNKNOWN)
        gd_warning("Invalid CrLi element in imported file at cave data[%d]: %02x", i, unsigned(c));
    return nonscanned_pair(e);
}

GdElementEnum C64Import::crazylight_import(unsigned char const data[], unsigned i) {
    return crazylight_import_byte(data[i], i);
}


/*
 *
 * cave import routines.
 * take a cave, data, and maybe remaining bytes.
 * return the number of bytes read, -1 if error.
 *
 */

/**
 * Checksum function for cave import routines.
 * Used to recognize caves which need some hacks added,
 * besides normal importing.
 * @param data The input array of bytes
 * @param length The size
 * @return 16-bit checksum
 */
static guint checksum(unsigned char const data[], size_t length) {
    guint a = 1, b = 0;
    for (size_t i = 0; i < length; i++) {
        a = (a + data[i]) % 251; // the prime closest to (and less than) 256
        b = (b + a) % 251;
    }
    return b * 256 + a;
}

/*
  take care of required diamonds values==0 or >100.
  in original bd, the counter was only two-digit. so bd3 cave f
  says 150 diamonds required, but you only had to collect 50.
  also, gate opening is triggered by incrementing diamond
  count and THEN checking if more required; so if required was
  0, you had to collect 100. (also check crazy light 8 cave "1000")

  also do this with the time values.

  http://www.boulder-dash.nl/forum/viewtopic.php?t=88
*/

/// Import bd1 cave data into our format.
int C64Import::cave_copy_from_bd1(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format, ImportHack hack) {
    guint8 code;

    SetLoggerContextForFunction scf(cave.name);

    /* some checks */
    g_assert(format == GD_FORMAT_BD1 || format == GD_FORMAT_BD1_ATARI);
    /* cant be shorted than this: header + no objects + delimiter */
    if (remaining_bytes < 33) {
        gd_critical("truncated BD1 cave data, %d bytes", remaining_bytes);
        return -1;
    }

    cave_set_engine_defaults(cave, GD_ENGINE_BD1);
    if (format == GD_FORMAT_BD1_ATARI)
        cave.scheduling = GD_SCHEDULING_BD1_ATARI;

    /* set import func */
    ImportFuncArray import_func = bd1_import;
    ImportFuncByte import_func_byte = bd1_import_byte;
    switch (hack) {
        case Deluxe_Caves_1:
            import_func = deluxecaves_1_import;
            import_func_byte = deluxecaves_1_import_byte;
            break;
        case Deluxe_Caves_3:
            import_func = deluxecaves_3_import;
            import_func_byte = deluxecaves_3_import_byte;
            break;
        case Masters_Boulder:
        case Crazy_Dream_1:
        case BoulderDashPlus:
        case BoulderDashCaduta:
        case None:                  /* avoid warning */
            /* original bd1 */
            break;
        case Crazy_Dream_7:
        case Crazy_Dream_9:
            g_assert_not_reached();
            break;
    }

    /* set visible size for intermission */
    if (cave.intermission) {
        cave.x2 = 19;
        cave.y2 = 11;
    }

    /* cave number data[0] */
    cave.diamond_value = data[2];
    cave.extra_diamond_value = data[3];

    for (int level = 0; level < 5; level++) {
        cave.level_amoeba_time[level] = data[1];
        if (cave.level_amoeba_time[level] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[level] = 999;
        cave.level_magic_wall_time[level] = data[1];
        cave.level_rand[level] = data[4 + level];
        cave.level_diamonds[level] = data[9 + level] % 100; /* check comment above */
        if (cave.level_diamonds[level] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here means 100 to collect */
            cave.level_diamonds[level] = 100;
        cave.level_time[level] = data[14 + level];
        if (cave.level_time[level] == 0)        /* 0 immediately underflowed to 999. */
            cave.level_time[level] = 999;
    }

    /* LogicDeLuxe extension: acid
        $16 Acid speed (unused in the original BD1)
        $17 Bit 2: if set, Acid's original position converts to explosion puff during spreading. Otherwise, Acid remains intact, ie. it's just growing. (unused in the original BD1)
        $1C Acid eats this element. (also Probability of element 1)

        there is no problem importing these; as other bd1 caves did not contain acid at all, so it does not matter
        how we set the values.
    */
    if (data[0x1c] < 0x40) {
        // commented next line, because this will delete SetLoggerContextForFunction(): game/cave name
        //Logger l;   /* ignore error here, as it is usually not an error */
        GdElementEnum acideat = import_func(data, 0x1c);
        if (acideat != O_UNKNOWN)
            cave.acid_eats_this = acideat;
        //l.clear();
    }
    cave.acid_spread_ratio = data[0x16] * 1000000.0 / 255.0; /* acid speed */
    cave.acid_turns_to = (data[0x17] & (1 << 2)) ? O_EXPLODE_3 : O_ACID;

    if (format == GD_FORMAT_BD1_ATARI) {
        /* atari colors */
        cave.color1 = GdColor::from_atari(data[0x13]);
        cave.color2 = GdColor::from_atari(data[0x14]);
        cave.color3 = GdColor::from_atari(data[0x15]);
        cave.color4 = GdColor::from_atari(data[0x16]);  /* in atari, amoeba was green */
        cave.color5 = GdColor::from_atari(data[0x16]);  /* in atari, slime was green */
        cave.colorb = GdColor::from_atari(data[0x17]);  /* border = background */
        cave.color0 = GdColor::from_atari(data[0x17]);  /* background */
    } else {
        /* c64 colors */
        cave.colorb = GdColor::from_c64(0); /* border = background, fixed color */
        cave.color0 = GdColor::from_c64(0); /* background, fixed color */
        cave.color1 = GdColor::from_c64(data[0x13] & 0xf);
        cave.color2 = GdColor::from_c64(data[0x14] & 0xf);
        cave.color3 = GdColor::from_c64(data[0x15] & 0x7); /* lower 3 bits only (vic-ii worked this way) */
        cave.color4 = cave.color3;  /* in bd1, amoeba was color3 */
        cave.color5 = cave.color3;  /* no slime, but let it be color 3 */
    }

    /* random fill */
    cave.random_fill_1 = import_func(data, 24 + 0);
    cave.random_fill_2 = import_func(data, 24 + 1);
    cave.random_fill_3 = import_func(data, 24 + 2);
    cave.random_fill_4 = import_func(data, 24 + 3);
    cave.random_fill_probability_1 = data[28 + 0];
    cave.random_fill_probability_2 = data[28 + 1];
    cave.random_fill_probability_3 = data[28 + 2];
    cave.random_fill_probability_4 = data[28 + 3];

    /*
     * Decode the explicit cave data
     */
    int index = 32;
    while (index < remaining_bytes && data[index] != 0xFF && index < 255) {
        code = data[index];

        /* crazy dream 3 extension: */
        if (code == 0x0f) {
            /* as this one uses nonstandard dx dy values, create points instead */
            int x1 = data[index + 2];
            int y1 = data[index + 3] - 2;
            int nx = data[index + 4];
            int ny = data[index + 5];
            int dx = data[index + 6];
            int dy = data[index + 7] + 1;
            GdElementEnum element = import_func(data, index + 1);

            for (int y = 0; y < ny; y++)
                for (int x = 0; x < nx; x++) {
                    int pos = x1 + y1 * 40 + y * dy * 40 + x * dx;
                    cave.objects.push_back(CavePoint(Coordinate(pos % 40, pos / 40), element));
                }
            index += 8;
        } else if (hack == BoulderDashPlus && code == 0xFE) {  /* 0xFE - raster, used in BoulderDash+ */
            /* like above but dont modiy dy, create points instead */
            int el = data[index + 1];   el=el;   // avoid warning
            int x1 = data[index + 2];
            int y1 = data[index + 3] - 2;
            int nx = data[index + 4];
            int ny = data[index + 5];
            int dx = data[index + 6];
            int dy = data[index + 7];
            GdElementEnum element = import_func(data, index + 1);

            for (int y = 0; y < ny; y++)
                for (int x = 0; x < nx; x++) {
                    int pos = x1 + y1 * 40 + y * dy * 40 + x * dx;
                    Coordinate p(pos % 40, pos / 40);
                    cave.objects.push_back(CavePoint(p, element));
                }
            index += 8;
        } else {
            /* object is code&3f, object type is upper 2 bits */
            GdElementEnum element = import_func_byte(code & 0x3F, index);

            /* 2bits MSB determine object type */
            switch ((code >> 6) & 3) {
                case 0: {
                    /* 00: POINT */
                    Coordinate p(data[index + 1], data[index + 2] - 2);
                    cave.objects.push_back(CavePoint(p, element));
                    // if (object.x1>=cave.w || object.y1>=cave.h)
                    // gd_warning("invalid point coordinates %d,%d at byte %d", object.x1, object.y1, index);
                    index += 3;
                }
                break;
                case 1: {
                    /* 01: LINE */
                    Coordinate p1(data[index + 1], data[index + 2] - 2);
                    int length = (gint8) data[index + 3] - 1;
                    int direction = data[index + 4];
                    if (length >= 0) {
                        if (direction > MV_UP_LEFT) {
                            gd_warning("invalid line direction %d at byte %d", direction, index);
                            direction = MV_STILL;
                        }
                        Coordinate p2(p1.x + length * gd_dx[direction + 1], p1.y + length * gd_dy[direction + 1]);
                        cave.objects.push_back(CaveLine(p1, p2, element));
                        // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2>=cave.h)
                        //  gd_warning("invalid line coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
                    } else {
                        gd_warning("line length negative, not displaying line at all, at byte %d", index);
                    }
                    index += 5;
                }
                break;
                case 2: {
                    /* 10: FILLED RECTANGLE */
                    Coordinate p1(data[index + 1], data[index + 2] - 2);
                    Coordinate p2(p1.x + data[index + 3] - 1, p1.y + data[index + 4] - 1); /* stored width and height, not coordinates */

                    cave.objects.push_back(CaveFillRect(p1, p2, element, import_func(data, index + 5)));
                    // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2>=cave.h)
                    //  gd_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);

                    index += 6;
                }
                break;
                case 3: {
                    /* 11: OPEN RECTANGLE (OUTLINE) */
                    Coordinate p1(data[index + 1], data[index + 2] - 2);
                    Coordinate p2(p1.x + data[index + 3] - 1, p1.y + data[index + 4] - 1); /* stored width and height, not coordinates */

                    cave.objects.push_back(CaveRectangle(p1, p2, element));
                    // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2>=cave.h)
                    //  gd_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
                    index += 5;
                }
                break;
            }
        }
    }
    if (data[index] != 0xFF) {
        gd_critical("import error, cave not delimited with 0xFF");
        return -1;
    }
    index++;    // the delimiter

    return index;
}

/// Import bd2 cave data into our format.
/// Very similar to the bd1 importer, only the encoding was different.
int C64Import::cave_copy_from_bd2(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format, ImportHack hack) {
    g_assert(format == GD_FORMAT_BD2 || format == GD_FORMAT_BD2_ATARI);

    SetLoggerContextForFunction scf(cave.name);
    if (remaining_bytes < 0x1A + 5) {
        gd_critical("truncated BD2 cave data, %d bytes", remaining_bytes);
        return -1;
    }
    cave_set_engine_defaults(cave, GD_ENGINE_BD2);
    if (format == GD_FORMAT_BD2_ATARI)
        cave.scheduling = GD_SCHEDULING_BD2_PLCK_ATARI;

    /* set visible size for intermission */
    if (cave.intermission) {
        cave.x2 = 19;
        cave.y2 = 11;
    }

    cave.diamond_value = data[1];
    cave.extra_diamond_value = data[2];
    for (int i = 0; i < 5; i++) {
        cave.level_amoeba_time[i] = data[0];
        if (cave.level_amoeba_time[i] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_rand[i] = data[13 + i];
        cave.level_diamonds[i] = data[8 + i];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
            cave.level_diamonds[i] = 1000;
        cave.level_time[i] = data[3 + i];
        cave.level_magic_wall_time[i] = data[0];
    }
    cave.random_fill_1 = bd1_import(data, 0x16 + 0);
    cave.random_fill_2 = bd1_import(data, 0x16 + 1);
    cave.random_fill_3 = bd1_import(data, 0x16 + 2);
    cave.random_fill_4 = bd1_import(data, 0x16 + 3);
    cave.random_fill_probability_1 = data[0x12 + 0];
    cave.random_fill_probability_2 = data[0x12 + 1];
    cave.random_fill_probability_3 = data[0x12 + 2];
    cave.random_fill_probability_4 = data[0x12 + 3];

    /*
     * Decode the explicit cave data
     */
    int index = 0x1A;
    while (index < remaining_bytes && data[index] != 0xFF) {
        /* EXTREME CARE HERE! coordinates are in "wrong" order, first y then x. */

        switch (data[index]) {
            case 0: {
                /* LINE */
                GdElementEnum element = bd1_import(data, index + 1);
                Coordinate p1(data[index + 3], data[index + 2]);
                int direction = data[index + 4] / 2; /* they are multiplied by two - 0 is up, 2 is upright, 4 is right... */
                int length = data[index + 5] - 1;
                if (direction > MV_UP_LEFT) {
                    gd_warning("invalid line direction %d at byte %d", direction, index);
                    direction = MV_STILL;
                }
                Coordinate p2(p1.x + length * gd_dx[direction + 1], p1.y + length * gd_dy[direction + 1]);
                cave.objects.push_back(CaveLine(p1, p2, element));
                // if (x1>=cave.w || y1>=cave.h || x2>=cave.w || y2 >=cave.h)
                //  gd_warning("invalid line coordinates %d,%d %d,%d at byte %d", x1, y1, x2, y2, index);
                index += 6;
            }
            break;
            case 1: {
                /* OPEN RECTANGLE */
                GdElementEnum element = bd1_import(data, index + 1);
                Coordinate p1(data[index + 3], data[index + 2]);
                Coordinate p2(p1.x + data[index + 5] - 1, p1.y + data[index + 4] - 1); /* were stored as width&height */
                cave.objects.push_back(CaveRectangle(p1, p2, element));
                // if (x1>=cave.w || y1>=cave.h || x2>=cave.w || y2 >=cave.h)
                //  gd_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", x1, y1, x2, y2, index);
                index += 6;
            }
            break;
            case 2: {
                /* FILLED RECTANGLE */
                GdElementEnum element = bd1_import(data, index + 1);
                Coordinate p1(data[index + 3], data[index + 2]);
                Coordinate p2(p1.x + data[index + 5] - 1, p1.y + data[index + 4] - 1); /* were stored as width&height */
                GdElementEnum fill_element = bd1_import(data, index + 6);
                cave.objects.push_back(CaveFillRect(p1, p2, element, fill_element));
                // if (x1>=cave.w || y1>=cave.h || x2>=cave.w || y2 >=cave.h)
                //  gd_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", x1, y1, x2, y2, index);
                index += 7;
            }
            break;
            case 3: {
                /* POINT */
                GdElementEnum element = bd1_import(data, index + 1);
                Coordinate p(data[index + 3], data[index + 2]);
                cave.objects.push_back(CavePoint(p, element));
                // if (x1>=cave.w || y1>=cave.h)
                //  gd_warning("invalid point coordinates %d,%d at byte %d", x1, y1, index);
                index += 4;
            }
            break;
            case 4: {
                /* RASTER */
                GdElementEnum element = bd1_import(data, index + 1);
                Coordinate p1(data[index + 3], data[index + 2]);
                int ny = data[index + 4] - 1;
                int nx = data[index + 5] - 1;
                Coordinate dist(data[index + 7], data[index + 6]);
                Coordinate p2(p1.x + dist.x * nx, p1.y + dist.y * ny); /* calculate p2, we use that */
                cave.objects.push_back(CaveRaster(p1, p2, dist, element));
                // if (x1>=cave.w || y1>=cave.h || x2>=cave.w || y2 >=cave.h)
                //  gd_warning("invalid raster coordinates %d,%d %d,%d at byte %d", x1, y1, x2, y2, index);
                index += 8;
            }
            break;
            case 5: {
                /* profi boulder extension: bitmap */
                GdElementEnum element = bd1_import(data, index + 1);
                int bytes = data[index + 2]; /* number of bytes in bitmap */
                if (bytes >= cave.w * cave.h / 8)
                    gd_warning("invalid bitmap length at byte %d", index - 4);
                int addr = 0;
                addr += data[index + 3]; /*msb */
                addr += data[index + 4] << 8; /*lsb */
                addr -= 0x0850; /* this was a pointer to the cave work memory (used during game). */
                if (addr >= cave.w * cave.h)
                    gd_warning("invalid bitmap start address at byte %d", index - 4);
                int x1 = addr % 40;
                int y1 = addr / 40;
                for (int i = 0; i < bytes; i++) { /* for ("bytes" number of bytes) */
                    int val = data[index + 5 + i];
                    for (int n = 0; n < 8; n++) { /* for (8 bits in a byte) */
                        if (val & 1)
                            /* convert to single points... */
                            cave.objects.push_back(CavePoint(Coordinate(x1, y1), element));
                        val = val >> 1;
                        x1++;
                        if (x1 >= cave.w) {
                            x1 = 0;
                            y1++;
                        }
                    }
                }
                index += 5 + bytes; /* 5 description bytes and "bytes" data bytes */
            }
            break;
            case 6: {
                /* JOIN */
                GdElementEnum search_element = bd1_import(data, index + 1);
                GdElementEnum put_element = bd1_import(data, index + 2);
                Coordinate dist(data[index + 3] % 40, data[index + 3] / 40); /* they are stored in the same byte! */
                cave.objects.push_back(CaveJoin(dist, search_element, put_element));
                index += 4;
            }
            break;
            case 7:
                /* interesting this is set here, and not in the cave header */
                for (int i = 0; i < 5; i++)
                    cave.level_slime_permeability_c64[i] = data[index + 1];
                index += 2;
                break;
            case 9: {
                /* profi boulder extension by player: plck-like cave map. the import routine inserts it here. */
                if (!cave.map.empty()) {
                    gd_warning("contains more than one PLCK map");
                    cave.map.remove();
                }
                /* create map */
                cave.map.set_size(cave.w, cave.h, O_STEEL);
                int n = 0;  /* number of bytes read from map */
                for (int y = 1; y < cave.h - 1; y++) /* the first and the last rows are not stored. */
                    for (int x = 0; x < cave.w; x += 2) {
                        cave.map(x, y) = import_table_plck[data[index + 3 + n] >> 4]; /* msb 4 bits */
                        cave.map(x + 1, y) = import_table_plck[data[index + 3 + n] % 16]; /* lsb 4 bits */
                        n++;
                    }
                /* the position of inbox is stored. this is to check the cave */
                int ry = data[index + 1] - 2;
                int rx = data[index + 2];
                /* at the start of the cave, bd scrolled to the last player placed during the drawing (setup) of the cave.
                   i think this is why a map also stored the coordinates of the player - we can use this to check its integrity */
                if (rx >= cave.w || ry < 0 || ry >= cave.h || cave.map(rx, ry) != O_INBOX)
                    gd_warning("embedded PLCK map may be corrupted, player coordinates %d,%d", rx, rx);
                index += 3 + n;
            }
            break;
            case 0x08 :
                if(hack == BoulderDashCaduta) {
                    int amount = data[index + 1];
                    int x   = data[index + 3];
                    int y   = data[index + 2];
                    // int lsb = data[index + 4];  // only for completeness
                    // int msb = data[index + 5];  // only for completeness
                    // data start after 
                    int didx = index + 6;
                    int ppos = y * 40 + x;
                    int x1   = 0;
                    int y1   = 0;
                    GdElementEnum element = O_UNKNOWN;

                    for(int n=0; n<amount; n++) {
                        guint8 obj = data[didx + n];

                        {
                            element = bd1_import_byte(obj, didx + n);   // obj, index only for logging
                            x1 = ppos % 40;
                            y1 = ppos / 40;
                            Coordinate p(x1, y1);
                            cave.objects.push_back(CavePoint(p, element));
                            ppos++;
                        }

                        obj += 0x34;   // see code $AC0B .. $AC2C in dump // what the heck do this?

                        {
                            element = bd1_import_byte(obj, didx + n);   // index only for logging
                            x1 = ppos % 40;
                            y1 = ppos / 40;
                            Coordinate p(x1, y1);
                            cave.objects.push_back(CavePoint(p, element));
                            ppos++;
                        }
                    }
                    break;
                } else {
                    // fallthrough
                }
            case 0x14 :
                if(hack == BoulderDashCaduta) {
                    /* POINT */
                    GdElementEnum element = bd1_import(data, index + 1);
                    Coordinate p(data[index + 3], data[index + 2]);
                    cave.objects.push_back(CavePoint(p, element));
                    // if (x1>=cave.w || y1>=cave.h)
                    //  gd_warning("invalid point coordinates %d,%d at byte %d", x1, y1, index);
                    index += 4;
                    break;
                } else {
                    // fallthrough
                }
            default:
                gd_warning("unknown bd2 extension no. %02x at byte %d", unsigned(data[index]), index);
                index += 1; /* skip that byte */
        }
    }
    if (data[index] != 0xFF) {
        gd_critical("import error, cave not delimited with 0xFF");
        return -1;
    }
    index++;    /* skip delimiter */
    index++;    /* animation byte - told the engine which objects to animate - to make game faster */

    /* the colors from the memory dump are appended here */
    if (format == GD_FORMAT_BD2) {  // hack == BoulderDashCaduta 
        /* c64 colors */
        cave.colorb = GdColor::from_c64(0); /* always black */
        cave.color0 = GdColor::from_c64(0); /* always black */
        cave.color1 = GdColor::from_c64(data[index + 0] & 0xf);
        cave.color2 = GdColor::from_c64(data[index + 1] & 0xf);
        cave.color3 = GdColor::from_c64(data[index + 2] & 0x7); /* lower 3 bits only! */
        cave.color4 = cave.color1;  /* in bd2, amoeba was color1 */
        cave.color5 = cave.color1;  /* slime too */
        index += 3;
    } else {
        /* atari colors */
        cave.color1 = GdColor::from_atari(data[index + 0]);
        cave.color2 = GdColor::from_atari(data[index + 1]);
        cave.color3 = GdColor::from_atari(data[index + 2]);
        cave.color4 = GdColor::from_atari(data[index + 3]); /* amoeba and slime */
        cave.color5 = GdColor::from_atari(data[index + 3]);
        cave.colorb = GdColor::from_atari(data[index + 4]); /* background and border */
        cave.color0 = GdColor::from_atari(data[index + 4]);
        index += 5;
    }

    return index;
}

/// Take a slime predictability byte, and convert it to a bitmask.
/// Used for caves created with the original construction kit.
int C64Import::slime_plck(unsigned c64_data) {
    const int values[] = {0x00, 0x10, 0x18, 0x38, 0x3c, 0x7c, 0x7e, 0xfe, 0xff};
    if (c64_data > G_N_ELEMENTS(values)) {
        gd_warning("Invalid PLCK slime permeability value %x", c64_data);
        return 0xff;
    }

    return values[c64_data];
}

/// Import plck cave data into our format.
/// Length is always 512 bytes, and contains if it is an intermission cave.
int C64Import::cave_copy_from_plck(CaveStored &cave, const guint8 *data, int remaining_bytes, GdCavefileFormat format) {
    /* i don't really think that all this table is needed, but included to be complete. */
    /* this is for the dirt and expanding wall looks like effect. */
    /* it also contains the individual frames */
    static GdElementEnum plck_graphic_table[] = {
        /* 3000 */ O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN, O_UNKNOWN,
        /* 3100 */ O_BUTTER_1, O_MAGIC_WALL, O_PRE_DIA_1, O_PRE_DIA_2, O_PRE_DIA_3, O_PRE_DIA_4, O_PRE_DIA_5, O_OUTBOX_CLOSED,
        /* 3200 */ O_AMOEBA, O_VOODOO, O_STONE, O_DIRT, O_DIAMOND, O_STEEL, O_PLAYER, O_BRICK,
        /* 3300 */ O_SPACE, O_OUTBOX_OPEN, O_FIREFLY_1, O_EXPLODE_1, O_EXPLODE_2, O_EXPLODE_3, O_MAGIC_WALL, O_MAGIC_WALL,
        /* 3400 */ O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK, O_PLAYER_TAP_BLINK,
        /* 3500 */ O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT, O_PLAYER_LEFT,
        /* 3600 */ O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT, O_PLAYER_RIGHT,
        /* 3700 */ O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1, O_BUTTER_1,
        /* 3800 */ O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA, O_AMOEBA,
    };

    g_assert(format == GD_FORMAT_PLC || format == GD_FORMAT_PLC_ATARI);

    if (remaining_bytes < 512) {
        gd_critical("truncated plck cave data!");
        return -1;
    }

    cave_set_engine_defaults(cave, GD_ENGINE_PLCK);
    if (format == GD_FORMAT_PLC_ATARI)
        cave.scheduling = GD_SCHEDULING_BD2_PLCK_ATARI;
    cave.intermission = data[0x1da] != 0;
    if (cave.intermission) {    /* set visible size for intermission */
        cave.x2 = 19;
        cave.y2 = 11;
    }

    /* cave selection table, was not part of cave data, rather given in game packers.
     * if a new enough version of any2gdash is used, it will put information after the cave.
     * detect this here and act accordingly */
    if ((data[0x1f0] == data[0x1f1] - 1) && (data[0x1f0] == 0x19 || data[0x1f0] == 0x0e)) {
        /* found selection table */
        cave.selectable = data[0x1f0] == 0x19;
        cave.name = "";
        for (int j = 0; j < 12; j++)
            if (data[0x1f2 + j] >= 32 && data[0x1f2 + j] < 128)
                cave.name += data[0x1f2 + j];
        gd_strchomp(cave.name); /* remove spaces */
    } else
        /* no selection info found, let intermissions be unselectable */
        cave.selectable = !cave.intermission;

    cave.diamond_value = data[0x1be];
    cave.extra_diamond_value = data[0x1c0];
    int slime_perm_c64 = slime_plck(data[0x1c2]); /* convert here so error gets reported only once */
    for (int i = 0; i < 5; i++) {
        /* plck does not really have levels, so just duplicate data five times */
        cave.level_amoeba_time[i] = data[0x1c4];
        if (cave.level_amoeba_time[i] == 0) /* immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_time[i] = data[0x1ba];
        if (cave.level_time[i] == 0)       /* immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_time[i] = 999;
        cave.level_diamonds[i] = data[0x1bc];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
            cave.level_diamonds[i] = 1000;
        cave.level_ckdelay[i] = data[0x1b8];
        cave.level_magic_wall_time[i] = data[0x1c6];
        cave.level_slime_permeability_c64[i] = slime_perm_c64;
        /* set these to the same value on each level, so the engine does
         * not think that levels are different */
        cave.level_timevalue[i] = 1;
        cave.level_rand[i] = 0;
        cave.level_speed[i] = cave.level_ckdelay[i]*20;
        if (cave.level_speed[i] < 80)
            cave.level_speed[i] = 80;
    }

    if (format == GD_FORMAT_PLC_ATARI) {
        /* use atari colors */
        cave.colorb = GdColor::from_atari(0); /* border */
        /* indexes in data are not the same order as on c64!!! */
        cave.color0 = GdColor::from_atari(data[0x1e3]); /* background */
        cave.color1 = GdColor::from_atari(data[0x1db]);
        cave.color2 = GdColor::from_atari(data[0x1dd]);
        cave.color3 = GdColor::from_atari(data[0x1df]);
        /* in atari plck, slime and amoeba could not coexist in the same cave. */
        /* if amoeba was used, the graphics turned to green, and data at 0x1e1 was set to 0xd4. */
        /* if slime was used, graphics to blue, and data at 0x1e1 was set to 0x72. */
        /* these two colors could not be changed in the editor at all. */
        /* (maybe they could have been changed in a hex editor) */
        cave.color4 = GdColor::from_atari(data[0x1e1]);
        cave.color5 = GdColor::from_atari(data[0x1e1]);
    } else {
        /* use c64 colors */
        cave.colorb = GdColor::from_c64(data[0x1db] & 0xf); /* border */
        cave.color0 = GdColor::from_c64(data[0x1dd] & 0xf);
        cave.color1 = GdColor::from_c64(data[0x1df] & 0xf);
        cave.color2 = GdColor::from_c64(data[0x1e1] & 0xf);
        cave.color3 = GdColor::from_c64(data[0x1e3] & 0x7); /* lower 3 bits only! */
        cave.color4 = cave.color3;  /* in plck, amoeba was color3 */
        cave.color5 = cave.color3;  /* same for slime */
    }

    /* ... the cave is stored like a map. */
    cave.map.set_size(cave.w, cave.h, O_STEEL);
    /* cave map looked like this. */
    /* two rows of steel wall ($44's), then cave description, 20 bytes (40 nybbles) for each line. */
    /* the bottom and top lines were not stored... originally. */
    /* some games write to the top line; so we import that, too. */
    /* also dlp 155 allowed writing to the bottom line; the first 20 $44-s now store the bottom line. */
    /* so the cave is essentially shifted one row down in the file: cave.map[y][x]=data[... y+1 mod height ][x] */
    /* FOR NOW, WE DO NOT IMPORT THE BOTTOM BORDER */
    for (int y = 0; y < cave.h - 1; y++)
        for (int x = 0; x < cave.w; x += 2) {
            cave.map(x, y) = import_table_plck[data[((y + 1) % cave.h) * 20 + x / 2] >> 4]; /* msb 4 bits: we do not check index ranges, as >>4 and %16 will result in 0..15 */
            cave.map(x + 1, y) = import_table_plck[data[((y + 1) % cave.h) * 20 + x / 2] % 16]; /* lsb 4 bits */
        }

    /* check for diego-effects */
    /* c64 magic values (byte sequences)  0x20 0x90 0x46, also 0xa9 0x1c 0x85 */
    if ((data[0x1e5] == 0x20 && data[0x1e6] == 0x90 && data[0x1e7] == 0x46) || (data[0x1e5] == 0xa9 && data[0x1e6] == 0x1c && data[0x1e7] == 0x85)) {
        /* diego effects enabled. */
        cave.stone_bouncing_effect = bd1_import(data, 0x1ea);
        cave.diamond_falling_effect = bd1_import(data, 0x1eb);
        /* explosions: 0x1e was explosion 4. */
        cave.explosion_3_effect = bd1_import(data, 0x1ec);
        /* pointer to element graphic.
           two bytes/column (one element), that is data[xxx]%16/2.
           also there are 16bytes/row.
           that is, 0x44=stone, upper left character. 0x45=upper right, 0x54=lower right, 0x55=lower right.
           so high nybble must be shifted right twice -> data[xxx]/16*4. */
        cave.dirt_looks_like = plck_graphic_table[(data[0x1ed] / 16) * 4 + (data[0x1ed] % 16) / 2];
        cave.expanding_wall_looks_like = plck_graphic_table[(data[0x1ee] / 16) * 4 + (data[0x1ee] % 16) / 2];
        for (int i = 0; i < 5; i++)
            cave.level_amoeba_threshold[i] = data[0x1ef];
    }

    return 512;
}

/// No one's delight boulder dash importer.
/// essentially rle compressed plck maps.
int C64Import::cave_copy_from_dlb(CaveStored &cave, const guint8 *data, int remaining_bytes) {
    SetLoggerContextForFunction scf(cave.name);
    cave_set_engine_defaults(cave, GD_ENGINE_PLCK); /* essentially the plck engine */

    int slime_perm_c64 = slime_plck(data[5]); /* convert here so error gets reported only once */
    for (int i = 0; i < 5; i++) {
        /* does not really have levels, so just duplicate data five times */
        cave.level_time[i] = data[1];
        cave.level_diamonds[i] = data[2];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 needed */
            cave.level_diamonds[i] = 1000;
        cave.level_ckdelay[i] = data[0];
        cave.level_amoeba_time[i] = data[6];
        if (cave.level_amoeba_time[i] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_magic_wall_time[i] = data[7];
        cave.level_slime_permeability_c64[i] = slime_perm_c64;
        /* set these to the same value on each level, so the engine does
         * not think that levels are different */
        cave.level_timevalue[i] = 1;
        cave.level_rand[i] = 0;
        cave.level_speed[i] = cave.level_ckdelay[i]*20;
        if (cave.level_speed[i] < 80)
            cave.level_speed[i] = 80;
    }
    cave.diamond_value = data[3];
    cave.extra_diamond_value = data[4];

    /* then 5 color bytes follow */
    cave.colorb = GdColor::from_c64(data[8] & 0xf); /* border */
    cave.color0 = GdColor::from_c64(data[9] & 0xf);
    cave.color1 = GdColor::from_c64(data[10] & 0xf);
    cave.color2 = GdColor::from_c64(data[11] & 0xf);
    cave.color3 = GdColor::from_c64(data[12] & 0x7); /* lower 3 bits only! */
    cave.color4 = cave.color3;  /* in plck, amoeba was color3 */
    cave.color5 = cave.color3;  /* same for slime */

    /* cave map is compressed. */
    /* employ a state machine to decompress data. */
    int pos = 13; /* those 13 bytes were the cave values above */
    int cavepos = 0;
    guint8 byte = 0;
    guint8 separator = 0;
    guint8 decomp[512];
    enum {
        START,      /* initial state */
        SEPARATOR,  /* got a separator */
        RLE,        /* after a separator, got the byte to duplicate */
        NORMAL      /* normal, copy bytes till separator */
    } state;
    state = START;
    while (cavepos < 400 && pos < remaining_bytes) {
        switch (state) {
            case START:
                /* first byte is a separator. remember it */
                separator = data[pos];
                /* after the first separator, no rle data, just copy. */
                state = NORMAL;
                break;
            case SEPARATOR:
                /* we had a separator. remember this byte, as this will be duplicated (or more) */
                byte = data[pos];
                state = RLE;
                break;
            case RLE:
                /* we had the first byte, duplicate this n times. */
                if (data[pos] == 0xff) {
                    /* if it is a 0xff, we will have another byte, which is also a length specifier. */
                    /* and for this one, duplicate only 254 times */
                    if (cavepos + 254 > 400) {
                        gd_critical("DLB import error: RLE data overflows buffer");
                        return -1;
                    }
                    for (int i = 0; i < 254; i++)
                        decomp[cavepos++] = byte;
                } else {
                    /* if not 0xff, duplicate n times and back to copy mode */
                    if (cavepos + data[pos] > 400) {
                        gd_critical("DLB import error: RLE data overflows buffer");
                        return -1;
                    }
                    for (int i = 0; i < data[pos]; i++)
                        decomp[cavepos++] = byte;
                    state = NORMAL;
                }
                break;
            case NORMAL:
                /* bytes duplicated; now only copy the remaining, till the next separator. */
                if (data[pos] == separator)
                    state = SEPARATOR;
                else
                    decomp[cavepos++] = data[pos];  /* copy this byte and state is still NORMAL */
                break;
        }
        pos++;
    }
    if (cavepos != 400) {
        gd_critical("DLB import error: RLE processing, cave length %d, should be 400", cavepos);
        return -1;
    }

    cave.map.set_size(cave.w, cave.h);
    /* first and last row was not stored - it is always steel wall */
    for (int x = 0; x < cave.w; x++) {
        cave.map(x, 0) = O_STEEL;
        cave.map(x, cave.h - 1) = O_STEEL;
    }
    /* process uncompressed map */
    for (int y = 1; y < cave.h - 1; y++)
        for (int x = 0; x < cave.w; x += 2) {
            cave.map(x, y) = import_table_plck[decomp[((y - 1) * cave.w + x) / 2] >> 4]; /* msb 4 bits */
            cave.map(x + 1, y) = import_table_plck[decomp[((y - 1) * cave.w + x) / 2] % 16]; /* lsb 4 bits */
        }

    /* return number of bytes read from buffer */
    return pos;
}

GdString C64Import::name_from_c64_bin(const unsigned char *data) {
    GdString name;

    for (unsigned i = 0; i < 14; i++) {
        int c = data[i];

        /* import cave name; a conversion table is used for each character */
        if (c < 0x40)
            c = bd_internal_character_encoding[c];
        else if (c == 0x74)
            c = ' ';
        else if (c == 0x76)
            c = '?';
        else
            c = ' ';
        if (i > 0)  /* from the second one, change to lowercase */
            c = g_ascii_tolower(c);

        name += c;
    }
    gd_strchomp(name); /* remove trailing and leading spaces */

    return name;
}

/**
 * This function processes an amoeba probability pattern.
 * @param input The byte read from the file
 * @return The GDash probability
 */
static GdProbability amoeba_probability(unsigned char input) {
    // drop lower 2 bits
    input = input >> 2;

    // count the number of '1' bits
    int bits = 0;
    // while has any more bits
    while (input != 0) {
        // if the lowest bit is one, increment
        if (input & 1)
            bits++;
        // and drop the lowest one.
        input = input >> 1;
    }

    // here we have the number of 1 bits in the upper six bits.
    // The order does not count. The probability of all being
    // zero (thus the amoeba growing) is 1/2^bits.
    return 1000000 / (1 << bits);
}

/// Import a 1stB encoded cave.
int C64Import::cave_copy_from_1stb(CaveStored &cave, const guint8 *data, int remaining_bytes) {
    if (remaining_bytes < 1024) {
        gd_critical("truncated 1stb cave data!");
        return -1;
    }

    cave_set_engine_defaults(cave, GD_ENGINE_1STB);

    /* copy name */
    cave.name = name_from_c64_bin(data + 0x3a0);
    SetLoggerContextForFunction scf(cave.name);

    cave.intermission = data[0x389] != 0;
    /* if it is intermission but not scrollable */
    if (cave.intermission && !data[0x38c]) {
        cave.x2 = 19;
        cave.y2 = 11;
    }

    cave.diamond_value = 100 * data[0x379] + 10 * data[0x379 + 1] + data[0x379 + 2];
    cave.extra_diamond_value = 100 * data[0x376] + 10 * data[0x376 + 1] + data[0x376 + 2];
    for (int i = 0; i < 5; i++) {
        /* does not really have levels, so just duplicate data five times */
        cave.level_time[i] = 100 * data[0x370] + 10 * data[0x370 + 1] + data[0x370 + 2];
        if (cave.level_time[i] == 0)    /* same as gate opening after 0 diamonds */
            cave.level_time[i] = 1000;
        cave.level_diamonds[i] = 100 * data[0x373] + 10 * data[0x373 + 1] + data[0x373 + 2];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
            cave.level_diamonds[i] = 1000;
        cave.level_ckdelay[i] = data[0x38a];
        cave.level_amoeba_time[i] = 256 * (int)data[0x37c] + data[0x37d];
        if (cave.level_amoeba_time[i] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_magic_wall_time[i] = 256 * (int)data[0x37e] + data[0x37f];
        cave.level_slime_permeability_c64[i] = data[0x38b];
        cave.level_bonus_time[i] = data[0x392];
        cave.level_penalty_time[i] = data[0x393];
        cave.level_amoeba_threshold[i] = 256 * (int)data[0x390] + data[0x390 + 1];
        /* set these to the same value on each level, so the engine does
         * not think that levels are different */
        cave.level_timevalue[i] = 1;
        cave.level_rand[i] = 0;
        cave.level_speed[i] = cave.level_ckdelay[i]*20;
        if (cave.level_speed[i] < 80)
            cave.level_speed[i] = 80;
    }
    /* also has no random data... */

    cave.colorb = GdColor::from_c64(data[0x384] & 0xf); /* border */
    cave.color0 = GdColor::from_c64(data[0x385] & 0xf);
    cave.color1 = GdColor::from_c64(data[0x386] & 0xf);
    cave.color2 = GdColor::from_c64(data[0x387] & 0xf);
    cave.color3 = GdColor::from_c64(data[0x388] & 0x7); /* lower 3 bits only! */
    cave.color4 = cave.color1;
    cave.color5 = cave.color1;

    cave.amoeba_growth_prob = amoeba_probability(data[0x382]);
    cave.amoeba_fast_growth_prob = amoeba_probability(data[0x383]);

    if (data[0x380] != 0)
        cave.creatures_direction_auto_change_time = data[0x381];
    else
        cave.diagonal_movements = data[0x381] != 0;

    /* ... the cave is stored like a map. */
    cave.map.set_size(cave.w, cave.h);
    for (int y = 0; y < cave.h; y++)
        for (int x = 0; x < cave.w; x++)
            cave.map(x, y) = firstboulder_import(data, y * 40 + x);

    cave.magic_wall_sound = data[0x38d] == 0xf1;
    /* 2d was a normal switch, 2e a changed one. */
    cave.creatures_backwards = data[0x38f] == 0x2d;
    /* 2e horizontal, 2f vertical. */
    cave.expanding_wall_changed = data[0x38e] == 0x2f;

    cave.biter_delay_frame = data[0x394];
    cave.magic_wall_stops_amoeba = data[0x395] == 0; /* negated!! */
    cave.bomb_explosion_effect = firstboulder_import(data, 0x396);
    cave.explosion_effect = firstboulder_import(data, 0x397);
    cave.stone_bouncing_effect = firstboulder_import(data, 0x398);
    cave.diamond_birth_effect = firstboulder_import(data, 0x399);
    cave.magic_diamond_to = firstboulder_import(data, 0x39a);

    cave.bladder_converts_by = firstboulder_import(data, 0x39b);
    cave.diamond_falling_effect = firstboulder_import(data, 0x39c);
    cave.biter_eat = firstboulder_import(data, 0x39d);

    // slime does this: eats element 1, and adds 3 to it.
    // for example: element1=stone, and codes were: stone, stone (scanned), stone falling, stone falling (scanned)
    // so by eating a stone it created a stone falling scanned.
    cave.slime_eats_1 = firstboulder_import(data, 0x39e);
    cave.slime_converts_1 = firstboulder_import_byte(data[0x39e] + 3, 0x39e);
    cave.slime_eats_2 = firstboulder_import(data, 0x39f);
    cave.slime_converts_2 = firstboulder_import_byte(data[0x39f] + 3, 0x39e);

    cave.magic_diamond_to = firstboulder_import(data, 0x39a);

    /* length is always 1024 bytes */
    return 1024;
}

/// Import a crazy dream 7 cave.
int C64Import::cave_copy_from_crdr_7(CaveStored &cave, const guint8 *data, int remaining_bytes) {
    /* if we have name, convert */
    cave.name = name_from_c64_bin(data + 0);
    SetLoggerContextForFunction scf(cave.name);

    cave.selectable = data[14] != 0;

    /* jump 15 bytes, 14 was the name and 15 selectability */
    data += 15;
    if (memcmp((char *)data + 0x30, "V4\0020", 4) != 0)
        gd_warning("unknown crdr version %c%c%c%c", data[0x30], data[0x31], data[0x32], data[0x33]);

    cave_set_engine_defaults(cave, GD_ENGINE_CRDR7);

    for (int i = 0; i < 5; i++) {
        cave.level_time[i] = (int)data[0x0] * 100 + data[0x1] * 10 + data[0x2];
        if (cave.level_time[i] == 0)    /* same as gate opening after 0 diamonds */
            cave.level_time[i] = 1000;
        cave.level_diamonds[i] = (int)data[0x3] * 100 + data[0x4] * 10 + data[0x5];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
            cave.level_diamonds[i] = 1000;
        cave.level_ckdelay[i] = data[0x1A];
        cave.level_rand[i] = data[0x40];
        cave.level_amoeba_time[i] = (int)data[0xC] * 256 + data[0xD];
        if (cave.level_amoeba_time[i] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_magic_wall_time[i] = (int)data[0xE] * 256 + data[0xF];
        cave.level_slime_permeability_c64[i] = data[0x1B];
        cave.level_bonus_time[i] = data[0x22];
        cave.level_penalty_time[i] = data[0x23];
        cave.level_amoeba_threshold[i] = 256 * (int)data[0x20] + data[0x21];
        /* set these to the same value on each level, so the engine does
         * not think that levels are different */
        cave.level_timevalue[i] = 1;
        cave.level_speed[i] = cave.level_ckdelay[i]*20;
        if (cave.level_speed[i] < 80)
            cave.level_speed[i] = 80;
    }
    cave.extra_diamond_value = (int)data[0x6] * 100 + data[0x7] * 10 + data[0x8];
    cave.diamond_value = (int)data[0x9] * 100 + data[0xA] * 10 + data[0xB];
    if (data[0x10])
        cave.creatures_direction_auto_change_time = data[0x11];
    cave.colorb = GdColor::from_c64(data[0x14] & 0xf); /* border */
    cave.color0 = GdColor::from_c64(data[0x15] & 0xf);
    cave.color1 = GdColor::from_c64(data[0x16] & 0xf);
    cave.color2 = GdColor::from_c64(data[0x17] & 0xf);
    cave.color3 = GdColor::from_c64(data[0x18] & 0x7); /* lower 3 bits only! */
    cave.color4 = cave.color3;
    cave.color5 = cave.color1;
    cave.intermission = data[0x19] != 0;
    /* if it is intermission but not scrollable */
    if (cave.intermission && !data[0x1c]) {
        cave.x2 = 19;
        cave.y2 = 11;
    }

    cave.amoeba_growth_prob = amoeba_probability(data[0x12]);
    cave.amoeba_fast_growth_prob = amoeba_probability(data[0x13]);
    /* expanding wall direction change - 2e horizontal, 2f vertical */
    cave.expanding_wall_changed = data[0x1e] == 0x2f;
    /* 2c was a normal switch, 2d a changed one. */
    cave.creatures_backwards = data[0x1f] == 0x2d;
    cave.biter_delay_frame = data[0x24];
    cave.magic_wall_stops_amoeba = data[0x25] == 0; /* negated!! */
    cave.bomb_explosion_effect = import_table_crdr[data[0x26]];
    cave.explosion_effect = import_table_crdr[data[0x27]];
    cave.stone_bouncing_effect = import_table_crdr[data[0x28]];
    cave.diamond_birth_effect = import_table_crdr[data[0x29]];
    cave.magic_diamond_to = import_table_crdr[data[0x2a]];

    cave.bladder_converts_by = import_table_crdr[data[0x2b]];
    cave.diamond_falling_effect = import_table_crdr[data[0x2c]];
    cave.biter_eat = import_table_crdr[data[0x2d]];
    cave.slime_eats_1 = import_table_crdr[data[0x2e]];
    cave.slime_converts_1 = import_table_crdr[data[0x2e] + 3];
    cave.slime_eats_2 = import_table_crdr[data[0x2f]];
    cave.slime_converts_2 = import_table_crdr[data[0x2f] + 3];

    cave.diagonal_movements = (data[0x34] & 1) != 0;
    cave.gravity_change_time = data[0x35];
    cave.pneumatic_hammer_frame = data[0x36];
    cave.hammered_wall_reappear_frame = data[0x37];
    cave.hammered_walls_reappear = data[0x3f] != 0;
    /*
        acid in crazy dream 8:
        jsr $2500   ; true random
        cmp $03a8   ; compare to ratio
        bcs out     ; if it was smaller, forget it for now.

        ie. random<=ratio, then acid grows.
    */
    cave.acid_spread_ratio = data[0x38] * 1000000.0 / 255.0;
    cave.acid_eats_this = import_table_crdr[data[0x39]];
    switch (data[0x3a] & 3) {
        case 0:
            cave.gravity = MV_UP;
            break;
        case 1:
            cave.gravity = MV_DOWN;
            break;
        case 2:
            cave.gravity = MV_LEFT;
            break;
        case 3:
            cave.gravity = MV_RIGHT;
            break;
    }
    cave.snap_element = ((data[0x3a] & 4) != 0) ? O_EXPLODE_1 : O_SPACE;
    /* we do not know the values for these, so do not import */
    /// @todo do we know?
    //  cave.dirt_looks_like... data[0x3c]
    //  cave.expanding_wall_looks_like... data[0x3b]
    cave.random_fill_1 = import_table_crdr[data[0x41 + 0]];
    cave.random_fill_2 = import_table_crdr[data[0x41 + 1]];
    cave.random_fill_3 = import_table_crdr[data[0x41 + 2]];
    cave.random_fill_4 = import_table_crdr[data[0x41 + 3]];
    cave.random_fill_probability_1 = data[0x45 + 0];
    cave.random_fill_probability_2 = data[0x45 + 1];
    cave.random_fill_probability_3 = data[0x45 + 2];
    cave.random_fill_probability_4 = data[0x45 + 3];

    data += 0x49;
    int index = 0;
    Coordinate copy_p1, copy_p2;
    while (data[index] != 0xff) {
        switch (data[index]) {
            case 1: { /* point */
                GdElementEnum element = import_table_crdr[data[index + 1]];
                Coordinate p(data[index + 2], data[index + 3]);
                cave.objects.push_back(CavePoint(p, element));
                // if (object.x1>=cave.w || object.y1>=cave.h)
                //  gd_warning("invalid point coordinates %d,%d at byte %d", object.x1, object.y1, index);
                index += 4;
            }
            break;
            case 2: { /* rectangle */
                GdElementEnum element = import_table_crdr[data[index + 1]];
                Coordinate p1(data[index + 2], data[index + 3]);
                Coordinate p2(p1.x + data[index + 4] - 1, p1.y + data[index + 5] - 1); /* stored as width and height, so calculate p2 */
                cave.objects.push_back(CaveRectangle(p1, p2, element));
                // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2 >=cave.h)
                //  gd_warning("invalid rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
                index += 6;
            }
            break;
            case 3: { /* fillrect */
                GdElementEnum element = import_table_crdr[data[index + 1]];
                Coordinate p1(data[index + 2], data[index + 3]);
                Coordinate p2(p1.x + data[index + 4] - 1, p1.y + data[index + 5] - 1); /* stored as width and height, so calculate p2 */
                cave.objects.push_back(CaveFillRect(p1, p2, element, element));
                // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2 >=cave.h)
                //  gd_warning("invalid filled rectangle coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
                index += 6;
            }
            break;

            case 4: { /* line */
                GdElementEnum element = import_table_crdr[data[index + 1]];
                Coordinate p1(data[index + 2], data[index + 3]);
                int length = data[index + 4];
                int direction = data[index + 5];
                int nx = (direction - 128) % 40;
                int ny = (direction - 128) / 40;

                /* if either is bigger than one, we cannot treat this as a line. create points instead */
                if (abs(nx) > 1 || abs(ny) > 1) {
                    for (int i = 0; i < length; i++) {
                        cave.objects.push_back(CavePoint(Coordinate(p1), element));
                        p1.x += nx;
                        p1.y += ny;
                    }
                } else {
                    Coordinate p2(p1.x + (length - 1)*nx, p1.y + (length - 1)*ny);

                    cave.objects.push_back(CaveLine(p1, p2, element));
                    /* this is a normal line, and will be appended. only do the checking here */
                    // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2>=cave.h)
                    //  gd_warning("invalid line coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index-5);
                }
                index += 6;
            }
            break;
            case 6: /* copy */
                copy_p1 = Coordinate(data[index + 1], data[index + 2]);
                copy_p2 = Coordinate(copy_p1.x + data[index + 3] - 1, copy_p1.y + data[index + 4] - 1);
                // if (cx1>=cave.w || cy1>=cave.h || cx1+cw>cave.w || cy1+ch>cave.h)
                //  gd_warning("invalid copy coordinates %d,%d or size %d,%d at byte %d", cx1, cy1, cw, ch, index);
                index += 5;
                break;
            case 7: /* paste */
                cave.objects.push_back(CaveCopyPaste(copy_p1, copy_p2, Coordinate(data[index + 1], data[index + 2])));
                // if (data[index+1]>=cave.w || data[index+2]>=cave.h || data[index+1]+cw>cave.w || data[index+2]+ch>cave.h)
                //  gd_warning("invalid paste coordinates %d,%d at byte %d", data[index+1], data[index+2], index);
                index += 3;
                break;
            case 11: { /* raster */
                GdElementEnum element = import_table_crdr[data[index + 1]];
                Coordinate p1(data[index + 2], data[index + 3]);
                Coordinate dist(data[index + 4], data[index + 5]);
                int nx = data[index + 6] - 1;
                int ny = data[index + 7] - 1;
                Coordinate p2(p1.x + dist.x * nx, p1.y + dist.y * ny); /* we use endpoints rather than count */
                cave.objects.push_back(CaveRaster(p1, p2, dist, element));
                // if (object.x1>=cave.w || object.y1>=cave.h || object.x2>=cave.w || object.y2 >=cave.h)
                //  gd_warning("invalid raster coordinates %d,%d %d,%d at byte %d", object.x1, object.y1, object.x2, object.y2, index);
                index += 8;
            }
            break;
            default:
                gd_warning("unknown crdr extension no. %02x at byte %d", data[index], index);
                index += 1; /* skip that byte */
                break;
        }
    }
    index++;    /* skip $ff */

    return 15 + 0x49 + index;
}


/// Import a Crazy Light cave.
int C64Import::cave_copy_from_crli(CaveStored &cave, const guint8 *data, int remaining_bytes) {
    guint8 uncompressed[1024];
    int datapos, cavepos;
    bool cavefile;
    const char *versions[] = {"V2.2", "V2.6", "V3.0"};
    enum _versions {
        none,
        V2_2,   /*XXX whats the difference between 2.2 and 2.6?*/
        V2_6,
        V3_0
    } version = none;

    cave_set_engine_defaults(cave, GD_ENGINE_CRLI);

    /* detect if this is a cavefile */
    if (data[0] == 0 && data[1] == 0xc4 && data[2] == 'D' && data[3] == 'L' && data[4] == 'P') {
        datapos = 5; /* cavefile, skipping 0x00 0xc4 D L P */
        cavefile = true;
    } else {
        datapos = 15; /* converted from snapshot, skip "selectable" and 14byte name */
        cavefile = false;
    }

    /* if we have name, convert */
    if (!cavefile)
        cave.name = name_from_c64_bin(data + 1);
    SetLoggerContextForFunction scf(cave.name);

    /* uncompress rle data */
    cavepos = 0;
    while (cavepos < 0x3b0) { /* <- loop until the uncompressed reaches its size */
        if (datapos >= remaining_bytes) {
            gd_critical("truncated crli cave data");
            return -1;
        }
        if (data[datapos] == 0xbf) {    /* magic value 0xbf is the escape byte */
            if (datapos + 2 >= remaining_bytes) {
                gd_critical("truncated crli cave data");
                return -1;
            }
            if (data[datapos + 2] + datapos >= (int) sizeof(uncompressed)) {
                /* we would run out of buffer, this must be some error */
                gd_critical("invalid crli cave data - RLE length value is too big");
                return -1;
            }
            /* 0xbf, number, byte to dup */
            for (unsigned i = 0; i < data[datapos + 2]; i++)
                uncompressed[cavepos++] = data[datapos + 1];

            datapos += 3;
        } else
            uncompressed[cavepos++] = data[datapos++];
    }

    /* check crli version */
    for (unsigned i = 0; i < G_N_ELEMENTS(versions); i++)
        if (memcmp((char *)uncompressed + 0x3a0, versions[i], 4) == 0)
            version = _versions(i + 1);
    /* v3.0 has falling wall and box, and no ghost. */
    ImportFuncArray import;
    ImportFuncByte import_byte;
    import = (version >= V3_0) ? crazylight_import : firstboulder_import;
    import_byte = (version >= V3_0) ? crazylight_import_byte : firstboulder_import_byte;

    if (version == none) {
        gd_warning("unknown crli version %c%c%c%c", uncompressed[0x3a0], uncompressed[0x3a1], uncompressed[0x3a2], uncompressed[0x3a3]);
        import = crazylight_import;
    }

    /* process map */
    cave.map.set_size(cave.w, cave.h);
    for (int y = 0; y < cave.h; y++)
        for (int x = 0; x < cave.w; x++) {
            int index = y * cave.w + x;

            cave.map(x, y) = import(uncompressed, index);
        }

    /* crli has no levels */
    for (unsigned i = 0; i < 5; i++) {
        cave.level_time[i] = (int)uncompressed[0x370] * 100 + uncompressed[0x371] * 10 + uncompressed[0x372];
        if (cave.level_time[i] == 0)    /* same as gate opening after 0 diamonds */
            cave.level_time[i] = 1000;
        cave.level_diamonds[i] = (int)uncompressed[0x373] * 100 + uncompressed[0x374] * 10 + uncompressed[0x375];
        if (cave.level_diamonds[i] == 0)    /* gate opening is checked AFTER adding to diamonds collected, so 0 here is 1000 (!) needed */
            cave.level_diamonds[i] = 1000;
        cave.level_ckdelay[i] = uncompressed[0x38A];
        cave.level_amoeba_time[i] = (int)uncompressed[0x37C] * 256 + uncompressed[0x37D];
        if (cave.level_amoeba_time[i] == 0) /* 0 immediately underflowed to 999, so we use 999. example: sendydash 3, cave 02. */
            cave.level_amoeba_time[i] = 999;
        cave.level_magic_wall_time[i] = (int)uncompressed[0x37E] * 256 + uncompressed[0x37F];
        cave.level_slime_permeability_c64[i] = uncompressed[0x38B];
        cave.level_bonus_time[i] = uncompressed[0x392];
        cave.level_penalty_time[i] = uncompressed[0x393];
        cave.level_amoeba_threshold[i] = 256 * (int)uncompressed[0x390] + uncompressed[0x390 + 1];
        /* set these to the same value on each level, so the engine does
         * not think that levels are different */
        cave.level_timevalue[i] = 1;
        cave.level_rand[i] = 0;
        cave.level_speed[i] = cave.level_ckdelay[i]*20;
        if (cave.level_speed[i] < 80)
            cave.level_speed[i] = 80;
    }
    cave.extra_diamond_value = (int)uncompressed[0x376] * 100 + uncompressed[0x377] * 10 + uncompressed[0x378];
    cave.diamond_value = (int)uncompressed[0x379] * 100 + uncompressed[0x37A] * 10 + uncompressed[0x37B];
    if (uncompressed[0x380])
        cave.creatures_direction_auto_change_time = uncompressed[0x381];
    cave.colorb = GdColor::from_c64(uncompressed[0x384] & 0xf); /* border */
    cave.color0 = GdColor::from_c64(uncompressed[0x385] & 0xf);
    cave.color1 = GdColor::from_c64(uncompressed[0x386] & 0xf);
    cave.color2 = GdColor::from_c64(uncompressed[0x387] & 0xf);
    cave.color3 = GdColor::from_c64(uncompressed[0x388] & 0x7); /* lower 3 bits only! */
    cave.color4 = cave.color3;
    cave.color5 = cave.color1;
    cave.intermission = uncompressed[0x389] != 0;
    /* if it is intermission but not scrollable */
    if (cave.intermission && !uncompressed[0x38c]) {
        cave.x2 = 19;
        cave.y2 = 11;
    }

    cave.amoeba_growth_prob = amoeba_probability(data[0x382]);
    cave.amoeba_fast_growth_prob = amoeba_probability(data[0x383]);
    /* 2c was a normal switch, 2d a changed one. */
    cave.creatures_backwards = uncompressed[0x38f] == 0x2d;
    cave.magic_wall_sound = uncompressed[0x38d] == 0xf1;
    /* 2e horizontal, 2f vertical. we implement this by changing them */
    if (uncompressed[0x38e] == 0x2f)
        for (int y = 0; y < cave.h; y++)
            for (int x = 0; x < cave.w; x++) {
                if (cave.map(x, y) == O_H_EXPANDING_WALL)
                    cave.map(x, y) = O_V_EXPANDING_WALL;
            }
    cave.biter_delay_frame = uncompressed[0x394];
    cave.magic_wall_stops_amoeba = uncompressed[0x395] == 0; /* negated!! */
    cave.bomb_explosion_effect = import(uncompressed, 0x396);
    cave.explosion_effect = import(uncompressed, 0x397);
    cave.stone_bouncing_effect = import(uncompressed, 0x398);
    cave.diamond_birth_effect = import(uncompressed, 0x399);
    cave.magic_diamond_to = import(uncompressed, 0x39a);

    cave.bladder_converts_by = import(uncompressed, 0x39b);
    cave.diamond_falling_effect = import(uncompressed, 0x39c);
    cave.biter_eat = import(uncompressed, 0x39d);
    cave.slime_eats_1 = import(uncompressed, 0x39e);
    cave.slime_converts_1 = import_byte(uncompressed[0x39e] + 3, 0x39e);
    cave.slime_eats_2 = import(uncompressed, 0x39f);
    cave.slime_converts_2 = import_byte(uncompressed[0x39f] + 3, 0x39f);

    /* v3.0 has some new properties. */
    if (version >= V3_0) {
        cave.diagonal_movements = uncompressed[0x3a4] != 0;
        cave.amoeba_too_big_effect = import(uncompressed, 0x3a6);
        cave.amoeba_enclosed_effect = import(uncompressed, 0x3a7);
        /*
            acid in crazy dream 8:
            jsr $2500   ; true random
            cmp $03a8   ; compare to ratio
            bcs out     ; if it was smaller, forget it for now.

            ie. random<=ratio, then acid grows.
        */
        cave.acid_spread_ratio = uncompressed[0x3a8] * 1000000.0 / 255.0;
        cave.acid_eats_this = import(uncompressed, 0x3a9);
        cave.expanding_wall_looks_like = import(uncompressed, 0x3ab);
        cave.dirt_looks_like = import(uncompressed, 0x3ac);
    } else {
        /* version is <= 3.0, so this is a 1stb cave. */
        /* the only parameters, for which this matters, are these: */
        if (uncompressed[0x380] != 0)
            cave.creatures_direction_auto_change_time = uncompressed[0x381];
        else
            cave.diagonal_movements = uncompressed[0x381] != 0;
    }

    if (cavefile)
        cave.selectable = !cave.intermission; /* best we can do */
    else
        cave.selectable = !data[0]; /* given by converter */

    return datapos;
}


/** deluxe caves 3 effect */
static void deluxe_caves_3_add_specials(std::vector<CaveStored> & caveset) {
    for (size_t i = 0; i < caveset.size(); ++i) {
        CaveStored &cave = caveset[i];
        SetLoggerContextForFunction scf(cave.name);

        cave.snap_element = O_EXPLODE_1;
        cave.diagonal_movements = true;

        switch (i) {
            case 6:  /* cave f */
                cave.stone_bouncing_effect = O_BUTTER_1;
                cave.diamond_falling_effect = O_EXPLODE_3;
                break;
            case 7: /* cave g */
                gd_warning("effects not supported");
                break;
            case 13: /* cave l */
                gd_warning("effects not working perfectly");
                cave.stone_bouncing_effect = O_FIREFLY_1;
                break;
            case 18:
                cave.diamond_bouncing_effect = O_STONE;
                break;
        }
    }
}

/// Crazy Dream 7 hacks
static void crazy_dream_7_add_specials(std::vector<CaveStored> & caveset) {
    for (size_t i = 0; i < caveset.size(); ++i) {
        CaveStored &cave = caveset[i];

        if (cave.name == "Crazy maze")
            cave.skeletons_needed_for_pot = 0;
    }
}


/// This function adds some hardcoded elements to a Crazy Dream 9 cave.
/// Crazy Dream 9 had some caves and random fills, which were not encoded in the
/// cave data.
static void crazy_dream_9_add_specials(std::vector<CaveStored> & caveset) {
    for (size_t i = 0; i < caveset.size(); ++i) {
        CaveStored &cave = caveset[i];

        /* check cave name and the checksum. both are hardcoded here */
        if (cave.name == "Rockfall") {
            CaveRandomFill o(Coordinate(0, 0), Coordinate(39, 21));
            o.set_replace_only(O_BLADDER_SPENDER);  /* replace spenders drawn */
            o.set_random_fill(O_DIRT, O_DIAMOND, O_STONE, O_ACID, O_DIRT);
            o.set_random_prob(37, 32, 2, 0);
            cave.objects.push_back(std::move(o));
        }

        if (cave.name == "Roll dice now!") {
            CaveRandomFill o(Coordinate(0, 0), Coordinate(39, 21));
            o.set_replace_only(O_BLADDER_SPENDER);  /* replace spenders drawn */
            o.set_random_fill(O_DIRT, O_STONE, O_BUTTER_3, O_DIRT, O_DIRT);
            o.set_random_prob(0x18, 0x08, 0, 0);
            cave.objects.push_back(std::move(o));
        }

        if (cave.name == "Random maze") {
            cave.objects.push_back(CaveMaze(Coordinate(1, 4), Coordinate(35, 20), O_NONE, O_DIRT, CaveMaze::Perfect));
        }

        if (cave.name == "Metamorphosis") {
            CaveMaze m(Coordinate(4, 1), Coordinate(38, 19), O_NONE, O_BLADDER_SPENDER, CaveMaze::Perfect);  /* paths=spender */
            m.set_widths(1, 3);
            cave.objects.push_back(std::move(m));

            CaveRandomFill r(Coordinate(4, 1), Coordinate(38, 19));
            r.set_replace_only(O_BLADDER_SPENDER);  /* replace spenders drawn */
            r.set_random_fill(O_DIRT, O_STONE, O_DIRT, O_DIRT, O_DIRT);
            r.set_random_prob(0x18, 0, 0, 0);
            cave.objects.push_back(std::move(r));
            cave.creatures_backwards = true; /* XXX why? where is it stored? */
        }

        if (cave.name == "All the way") {
            cave.objects.push_back(CaveMaze(Coordinate(1, 1), Coordinate(35, 19), O_BRICK, O_PRE_DIA_1, CaveMaze::Unicursal));
            /* a point which "breaks" the unicursal maze */
            cave.objects.push_back(CavePoint(Coordinate(35, 18), O_BRICK));
        }
    }
}


static void masters_boulder_add_hack(std::vector<CaveStored> & caveset) {
    for (size_t i = 0; i < caveset.size(); ++i) {
        CaveStored &cave = caveset[i];
        SetLoggerContextForFunction scf(cave.name);

        switch (i) {
            case 1:  /* cave b */
                for (int j = 0; j < 5; ++j)
                    cave.level_hatching_delay_time[j] = 3;  /* secs */
                break;
        }
    }
}


/// Detect the file format of a GDash data file created by any2gdash.
C64Import::GdCavefileFormat C64Import::imported_get_format(const guint8 *buf) {
    /* compare first 8 bytes of data - no strcmp! and case sensitive. */
    for (int i = 0; gd_format_strings[i] != NULL; ++i)
        if (memcmp((char *)buf, gd_format_strings[i], strlen(gd_format_strings[i])) == 0)
            return GdCavefileFormat(i);

    return GD_FORMAT_UNKNOWN;
}


/// Load caveset from memory buffer.
/// Loads the caveset from a memory buffer.
/// @return a vector of newly created caves.
std::vector<CaveStored> C64Import::caves_import_from_buffer(const guint8 *buf, int length) {
    gboolean numbering;
    int intermissionnum, num;
    int cavelength;
    std::vector<CaveStored> caveset;

    if (length != -1 && length < 12) {
        gd_critical("buffer too short to be a GDash datafile");
        return caveset; /* empty */
    }
    int encodedlength = GINT32_FROM_LE(*((guint32 *)(buf + 8)));
    if (length != -1 && encodedlength != length - 12) {
        gd_critical("file length and data size mismatch in GDash datafile");
        return caveset; /* empty */
    }
    GdCavefileFormat format = imported_get_format(buf);
    if (format == GD_FORMAT_UNKNOWN) {
        gd_critical("buffer does not contain a GDash datafile");
        return caveset; /* empty */
    }

    buf += 12;      /* first 12 bytes are cave type. skip that */
    length = encodedlength;

    // check for hacks.
    ImportHack hack = None;
    guint cs = checksum(buf, length);
    //~ g_debug("%d %x", length, cs);
    if (format == GD_FORMAT_PLC && length == 10240 && cs == 0xbdec)
        hack = Crazy_Dream_1;
    if (format == GD_FORMAT_CRLI && length == 9895 && cs == 0xbc4e)
        hack = Crazy_Dream_9;
    if (format == GD_FORMAT_BD1 && length == 1634 && cs == 0xf725)
        hack = Deluxe_Caves_1;
    if (format == GD_FORMAT_BD1 && length == 1452 && cs == 0xb4a6)
        hack = Deluxe_Caves_3;
    if (format == GD_FORMAT_CRDR_7 && length == 3759 && cs == 0x16b5)
        hack = Crazy_Dream_7;
    if (format == GD_FORMAT_BD1 && length == 1241 && cs == 0x926f)
        hack = Masters_Boulder;
    if (format == GD_FORMAT_BD1_PLUS) {
        format = GD_FORMAT_BD1;
        hack = BoulderDashPlus;
    }
    if (format == GD_FORMAT_BD2_CADUTA) {
        format = GD_FORMAT_BD2;
        hack = BoulderDashCaduta;
    }

    int bufp = 0;
    int cavenum = 0;
    while (bufp < length) {
        int insertpos = -1; /* default is to append cave to caveset */

        CaveStored newcave;

        cavelength = 0; /* to avoid compiler warning */

        switch (format) {
            case GD_FORMAT_BD1_PLUS:        /* boulder dash plus */
            case GD_FORMAT_BD1:             /* boulder dash 1 */
            case GD_FORMAT_BD1_ATARI:       /* boulder dash 1, atari version */
            case GD_FORMAT_BD2:             /* boulder dash 2 */
            case GD_FORMAT_BD2_ATARI:       /* boulder dash 2, atari version */
            case GD_FORMAT_BD2_CADUTA:      /* boulder dash 2, caduta massi  */
                /* these are not in the data so we guess */
                newcave.selectable = (cavenum < 16) && (cavenum % 4 == 0);
                newcave.intermission = cavenum > 15;
                /* no name, so we make up one */
                if (newcave.intermission)
                    newcave.name = Printf(_("Intermission %d"), cavenum - 15);
                else
                    newcave.name = Printf(_("Cave %c"), char('A' + cavenum));

                switch (format) {
                    case GD_FORMAT_BD1:
                    case GD_FORMAT_BD1_ATARI:
                        cavelength = cave_copy_from_bd1(newcave, buf + bufp, length - bufp, format, hack);
                        break;
                    case GD_FORMAT_BD2:
                    case GD_FORMAT_BD2_ATARI:
                    case GD_FORMAT_BD2_CADUTA:
                        cavelength = cave_copy_from_bd2(newcave, buf + bufp, length - bufp, format, hack);
                        break;
                    default:
                        g_assert_not_reached();
                };

                /* original bd1 had level order ABCDEFGH... and then the last four were the intermissions.
                 * those should be inserted between D-E, H-I... caves. */
                if (cavenum > 15)
                    insertpos = (cavenum - 15) * 5 - 1;
                break;

            case GD_FORMAT_FIRSTB:
                cavelength = cave_copy_from_1stb(newcave, buf + bufp, length - bufp);
                /* every fifth cave (4+1 intermission) is selectable. */
                newcave.selectable = cavenum % 5 == 0;
                break;

            case GD_FORMAT_PLC:             /* peter liepa construction kit */
            case GD_FORMAT_PLC_ATARI: {             /* peter liepa construction kit, atari version */
                SetLoggerContextForFunction scf(Printf("Cave %02d", cavenum));
                cavelength = cave_copy_from_plck(newcave, buf + bufp, length - bufp, format);
            }
            if (cavelength != -1 && hack == Crazy_Dream_1)
                newcave.diagonal_movements = true;
            break;

            case GD_FORMAT_DLB: /* no one's delight boulder dash, something like rle compressed plck caves */
                /* but there are 20 of them, as if it was a bd1 or bd2 game. also num%5=4 is intermission. */
                /* we have to set intermission flag on our own, as the file did not contain the info explicitly */
                newcave.intermission = (cavenum % 5) == 4;
                if (newcave.intermission) { /* also set visible size */
                    newcave.x2 = 19;
                    newcave.y2 = 11;
                }
                newcave.selectable = cavenum % 5 == 0;  /* original selection scheme */
                if (newcave.intermission)
                    newcave.name = Printf(_("Intermission %d"), cavenum / 5 + 1);
                else
                    newcave.name = Printf(_("Cave %c"), char('A' + (cavenum % 5 + cavenum / 5 * 4)));

                cavelength = cave_copy_from_dlb(newcave, buf + bufp, length - bufp);
                break;

            case GD_FORMAT_CRLI:
                cavelength = cave_copy_from_crli(newcave, buf + bufp, length - bufp);
                break;

            case GD_FORMAT_CRDR_7:
                cavelength = cave_copy_from_crdr_7(newcave, buf + bufp, length - bufp);
                break;

            case GD_FORMAT_UNKNOWN:
                g_assert_not_reached();
                break;
        }

        if (cavelength == -1) {
            gd_critical("Aborting cave import.");
            break;
        }
        if (insertpos < 0)
            caveset.push_back(std::move(newcave));
        else
            caveset.insert(caveset.begin() + insertpos, std::move(newcave));
        cavenum++;
        bufp += cavelength;

        /* hack: some dlb files contain junk data after 20 caves. */
        if (format == GD_FORMAT_DLB && cavenum == 20) {
            if (bufp < length)
                gd_warning("excess data in dlb file, %d bytes", int(length - bufp));
            break;
        }
    }

    /* try to detect if plc caves are in standard layout. */
    /* that is, caveset looks like an original, (4 cave,1 intermission)+ */
    if (format == GD_FORMAT_PLC)
        /* if no selection table stored by any2gdash */
        if ((buf[2 + 0x1f0] != buf[2 + 0x1f1] - 1) || (buf[2 + 0x1f0] != 0x19 && buf[2 + 0x1f0] != 0x0e)) {
            bool standard = (caveset.size() % 5) == 0; /* cave count % 5 != 0 -> nonstandard */
            for (unsigned n = 0; standard && n < caveset.size(); ++n)
                if ((n % 5 == 4 && !caveset[n].intermission) || (n % 5 != 4 && caveset[n].intermission))
                    standard = false; /* 4 cave, 1 intermission */

            /* if test passed, update selectability */
            if (standard)
                for (unsigned n = 0; n < caveset.size(); ++n)
                    caveset[n].selectable = (n % 5) == 0;
        }

    /* try to give some names for the caves */
    cavenum = 1;
    intermissionnum = 1;
    num = 1;
    /* use numbering instead of letters, if following formats or too many caves (as we would run out of letters) */
    numbering = format == GD_FORMAT_PLC || format == GD_FORMAT_CRLI || caveset.size() > 26;
    for (unsigned n = 0; n < caveset.size(); ++n) {
        CaveStored &cave = caveset[n];
        if (cave.name != "") /* if it already has a name, skip */
            continue;

        if (cave.intermission) {
            if (numbering)
                cave.name = Printf(_("Intermission %02d"), num);
            else
                cave.name = Printf(_("Intermission %d"), intermissionnum);
        } else {
            if (numbering)
                cave.name = Printf(_("Cave %02d"), num);
            else
                cave.name = Printf(_("Cave %c"), char('A' - 1 + cavenum));
        }

        num++;
        if (cave.intermission)
            intermissionnum++;
        else
            cavenum++;
    }

    /* if the uses requests, we make all caves selectable. intermissions not. */
    if (gd_import_as_all_caves_selectable)
        for (unsigned n = 0; n < caveset.size(); ++n)
            /* make selectable if not an intermission. */
            /* also selectable, if it was selectable originally, for some reason. */
            caveset[n].selectable = caveset[n].selectable || !caveset[n].intermission;

    /* add hacks */
    if (hack == Deluxe_Caves_3)
        deluxe_caves_3_add_specials(caveset);
    if (hack == Crazy_Dream_7)
        crazy_dream_7_add_specials(caveset);
    if (hack == Crazy_Dream_9)
        crazy_dream_9_add_specials(caveset);
    if (hack == Masters_Boulder)
        masters_boulder_add_hack(caveset);

    return caveset;
}


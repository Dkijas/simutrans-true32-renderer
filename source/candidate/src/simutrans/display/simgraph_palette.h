/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DISPLAY_SIMGRAPH_PALETTE_H
#define DISPLAY_SIMGRAPH_PALETTE_H


#include "../simcolor.h"


/*
 * Base colour tables shared by every software renderer.
 *
 * These are byte-for-byte the tables that simgraph16.cc defines privately.
 * They are duplicated here rather than extracted out of simgraph16.cc because
 * that file is frozen for the duration of the 32 bit migration: the first 32
 * bit cut has to be purely additive. The duplication is deliberate, is tracked,
 * and disappears when the shared core is extracted (roadmap phase P11) -- at
 * which point simgraph16.cc includes this header instead of carrying its own
 * copies.
 *
 * Only simgraph32.cc includes this today, so the 16 bit build is untouched.
 */


static rgb888_t display_day_lights[LIGHT_COUNT] = {
	{ 0x57, 0x65, 0x6F }, // Dark windows, lit yellowish at night
	{ 0x7F, 0x9B, 0xF1 }, // Lighter windows, lit blueish at night
	{ 0xFF, 0xFF, 0x53 }, // Yellow light
	{ 0xFF, 0x21, 0x1D }, // Red light
	{ 0x01, 0xDD, 0x01 }, // Green light
	{ 0x6B, 0x6B, 0x6B }, // Non-darkening grey 1 (menus)
	{ 0x9B, 0x9B, 0x9B }, // Non-darkening grey 2 (menus)
	{ 0xB3, 0xB3, 0xB3 }, // non-darkening grey 3 (menus)
	{ 0xC9, 0xC9, 0xC9 }, // Non-darkening grey 4 (menus)
	{ 0xDF, 0xDF, 0xDF }, // Non-darkening grey 5 (menus)
	{ 0xE3, 0xE3, 0xFF }, // Nearly white light at day, yellowish light at night
	{ 0xC1, 0xB1, 0xD1 }, // Windows, lit yellow
	{ 0x4D, 0x4D, 0x4D }, // Windows, lit yellow
	{ 0xE1, 0x00, 0xE1 }, // purple light for signals
	{ 0x01, 0x01, 0xFF }  // blue light
};


/*
 * special colors during nighttime
 */
static rgb888_t display_night_lights[LIGHT_COUNT] = {
	{ 0xD3, 0xC3, 0x80 }, // Dark windows, lit yellowish at night
	{ 0x80, 0xC3, 0xD3 }, // Lighter windows, lit blueish at night
	{ 0xFF, 0xFF, 0x53 }, // Yellow light
	{ 0xFF, 0x21, 0x1D }, // Red light
	{ 0x01, 0xDD, 0x01 }, // Green light
	{ 0x6B, 0x6B, 0x6B }, // Non-darkening grey 1 (menus)
	{ 0x9B, 0x9B, 0x9B }, // Non-darkening grey 2 (menus)
	{ 0xB3, 0xB3, 0xB3 }, // non-darkening grey 3 (menus)
	{ 0xC9, 0xC9, 0xC9 }, // Non-darkening grey 4 (menus)
	{ 0xDF, 0xDF, 0xDF }, // Non-darkening grey 5 (menus)
	{ 0xFF, 0xFF, 0xE3 }, // Nearly white light at day, yellowish light at night
	{ 0xD3, 0xC3, 0x80 }, // Windows, lit yellow
	{ 0xD3, 0xC3, 0x80 }, // Windows, lit yellow
	{ 0xE1, 0x00, 0xE1 }, // purple light for signals
	{ 0x01, 0x01, 0xFF }  // blue light
};


// the players colors and colors for simple drawing operations
// each 8 colors are a player color
static const rgb888_t special_pal[SPECIAL_COLOR_COUNT] =
{
	{  36,  75, 103 }, {  57,  94, 124 }, {  76, 113, 145 }, {  96, 132, 167 }, { 116, 151, 189 }, { 136, 171, 211 }, { 156, 190, 233 }, { 176, 210, 255 },
	{  88,  88,  88 }, { 107, 107, 107 }, { 125, 125, 125 }, { 144, 144, 144 }, { 162, 162, 162 }, { 181, 181, 181 }, { 200, 200, 200 }, { 219, 219, 219 },
	{  17,  55, 133 }, {  27,  71, 150 }, {  37,  86, 167 }, {  48, 102, 185 }, {  58, 117, 202 }, {  69, 133, 220 }, {  79, 149, 237 }, {  90, 165, 255 },
	{ 123,  88,   3 }, { 142, 111,   4 }, { 161, 134,   5 }, { 180, 157,   7 }, { 198, 180,   8 }, { 217, 203,  10 }, { 236, 226,  11 }, { 255, 249,  13 },
	{  86,  32,  14 }, { 110,  40,  16 }, { 134,  48,  18 }, { 158,  57,  20 }, { 182,  65,  22 }, { 206,  74,  24 }, { 230,  82,  26 }, { 255,  91,  28 },
	{  34,  59,  10 }, {  44,  80,  14 }, {  53, 101,  18 }, {  63, 122,  22 }, {  77, 143,  29 }, {  92, 164,  37 }, { 106, 185,  44 }, { 121, 207,  52 },
	{   0,  86,  78 }, {   0, 108,  98 }, {   0, 130, 118 }, {   0, 152, 138 }, {   0, 174, 158 }, {   0, 196, 178 }, {   0, 218, 198 }, {   0, 241, 219 },
	{  74,   7, 122 }, {  95,  21, 139 }, { 116,  37, 156 }, { 138,  53, 173 }, { 160,  69, 191 }, { 181,  85, 208 }, { 203, 101, 225 }, { 225, 117, 243 },
	{  59,  41,   0 }, {  83,  55,   0 }, { 107,  69,   0 }, { 131,  84,   0 }, { 155,  98,   0 }, { 179, 113,   0 }, { 203, 128,   0 }, { 227, 143,   0 },
	{  87,   0,  43 }, { 111,  11,  69 }, { 135,  28,  92 }, { 159,  45, 115 }, { 183,  62, 138 }, { 230,  74, 174 }, { 245, 121, 194 }, { 255, 156, 209 },
	{  20,  48,  10 }, {  44,  74,  28 }, {  68,  99,  45 }, {  93, 124,  62 }, { 118, 149,  79 }, { 143, 174,  96 }, { 168, 199, 113 }, { 193, 225, 130 },
	{  54,  19,  29 }, {  82,  44,  44 }, { 110,  69,  58 }, { 139,  95,  72 }, { 168, 121,  86 }, { 197, 147, 101 }, { 226, 173, 115 }, { 255, 199, 130 },
	{   8,  11, 100 }, {  14,  22, 116 }, {  20,  33, 139 }, {  26,  44, 162 }, {  41,  74, 185 }, {  57, 104, 208 }, {  76, 132, 231 }, {  96, 160, 255 },
	{  43,  30,  46 }, {  68,  50,  85 }, {  93,  70, 110 }, { 118,  91, 130 }, { 143, 111, 170 }, { 168, 132, 190 }, { 193, 153, 210 }, { 219, 174, 230 },
	{  63,  18,  12 }, {  90,  38,  30 }, { 117,  58,  42 }, { 145,  78,  55 }, { 172,  98,  67 }, { 200, 118,  80 }, { 227, 138,  92 }, { 255, 159, 105 },
	{  11,  68,  30 }, {  33,  94,  56 }, {  54, 120,  81 }, {  76, 147, 106 }, {  98, 174, 131 }, { 120, 201, 156 }, { 142, 228, 181 }, { 164, 255, 207 },
	{  64,   0,   0 }, {  96,   0,   0 }, { 128,   0,   0 }, { 192,   0,   0 }, { 255,   0,   0 }, { 255,  64,  64 }, { 255,  96,  96 }, { 255, 128, 128 },
	{   0, 128,   0 }, {   0, 196,   0 }, {   0, 225,   0 }, {   0, 240,   0 }, {   0, 255,   0 }, {  64, 255,  64 }, {  94, 255,  94 }, { 128, 255, 128 },
	{   0,   0, 128 }, {   0,   0, 192 }, {   0,   0, 224 }, {   0,   0, 255 }, {   0,  64, 255 }, {   0,  94, 255 }, {   0, 106, 255 }, {   0, 128, 255 },
	{ 128,  64,   0 }, { 193,  97,   0 }, { 215, 107,   0 }, { 235, 118,   0 }, { 255, 128,   0 }, { 255, 149,  43 }, { 255, 170,  85 }, { 255, 193, 132 },
	{   8,  52,   0 }, {  16,  64,   0 }, {  32,  80,   4 }, {  48,  96,   4 }, {  64, 112,  12 }, {  84, 132,  20 }, { 104, 148,  28 }, { 128, 168,  44 },
	{ 164, 164,   0 }, { 180, 180,   0 }, { 193, 193,   0 }, { 215, 215,   0 }, { 235, 235,   0 }, { 255, 255,   0 }, { 255, 255,  64 }, { 255, 255, 128 },
	{  32,   4,   0 }, {  64,  20,   8 }, {  84,  28,  16 }, { 108,  44,  28 }, { 128,  56,  40 }, { 148,  72,  56 }, { 168,  92,  76 }, { 184, 108,  88 },
	{  64,   0,   0 }, {  96,   8,   0 }, { 112,  16,   0 }, { 120,  32,   8 }, { 138,  64,  16 }, { 156,  72,  32 }, { 174,  96,  48 }, { 192, 128,  64 },
	{  32,  32,   0 }, {  64,  64,   0 }, {  96,  96,   0 }, { 128, 128,   0 }, { 144, 144,   0 }, { 172, 172,   0 }, { 192, 192,   0 }, { 224, 224,   0 },
	{  64,  96,   8 }, {  80, 108,  32 }, {  96, 120,  48 }, { 112, 144,  56 }, { 128, 172,  64 }, { 150, 210,  68 }, { 172, 238,  80 }, { 192, 255,  96 },
	{  32,  32,  32 }, {  48,  48,  48 }, {  64,  64,  64 }, {  80,  80,  80 }, {  96,  96,  96 }, { 172, 172, 172 }, { 236, 236, 236 }, { 255, 255, 255 },
	{  41,  41,  54 }, {  60,  45,  70 }, {  75,  62, 108 }, {  95,  77, 136 }, { 113, 105, 150 }, { 135, 120, 176 }, { 165, 145, 218 }, { 198, 191, 232 }
};


#endif

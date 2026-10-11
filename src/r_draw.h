// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw.h
/// \brief Low-level span/column drawer functions

#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_defs.h"

// -------------------------------
// COMMON STUFF FOR 8bpp AND 16bpp
// -------------------------------
extern UINT8 *topleft;

// -------------------------
// COLUMN DRAWING CODE STUFF
// -------------------------

extern lighttable_t *dc_colormap;
extern INT32 dc_x, dc_yl, dc_yh;
extern fixed_t dc_iscale, dc_texturemid;

extern UINT8 *dc_source; // first pixel in a column

// translucency stuff here
extern UINT8 *dc_transmap;

// translation stuff here

extern UINT8 *dc_translation;

extern struct r_lightlist_s *dc_lightlist;
extern INT32 dc_numlights, dc_maxlights;

extern INT32 dc_texheight, dc_postlength;

// -----------------------
// SPAN DRAWING CODE STUFF
// -----------------------

extern INT32 ds_y, ds_x1, ds_x2;
extern lighttable_t *ds_colormap;
extern lighttable_t *ds_translation;

extern fixed_t ds_xfrac, ds_yfrac, ds_xstep, ds_ystep;
extern INT32 ds_waterofs, ds_bgofs;

extern UINT16 ds_flatwidth, ds_flatheight;
extern boolean ds_powersoftwo, ds_solidcolor, ds_fog;

extern UINT8 *ds_source;
extern UINT8 *ds_transmap;

// Vectors for Software's tilted slope drawers
extern dvector3_t ds_su, ds_sv, ds_sz, ds_slopelight;
extern double zeroheight;
extern float focallengthf;

// Variable flat sizes
extern UINT32 nflatxshift;
extern UINT32 nflatyshift;
extern UINT32 nflatshiftup;
extern UINT32 nflatmask;

// ------------------------------------------------
// r_draw.c COMMON ROUTINES FOR BOTH 8bpp and 16bpp
// ------------------------------------------------

#define GTC_CACHE 1

enum
{
	TC_BOSS       = INT8_MIN,
	TC_METALSONIC, // For Metal Sonic battle
	TC_ALLWHITE,   // For Cy-Brak-demon
	TC_RAINBOW,    // For single colour
	TC_BLINK,      // For item blinking, according to kart
	TC_DASHMODE,   // For Metal Sonic's dashmode

	TC_DEFAULT
};

INT32 R_SkinTranslationToCacheIndex(INT32 translation);

// Amount of colors in the palette
#define NUM_PALETTE_ENTRIES 256

typedef struct colorcache_s
{
	UINT8 colors[NUM_PALETTE_ENTRIES];
} colorcache_t;

enum
{
	DEFAULT_TT_CACHE_INDEX = MAXSKINS,
	BOSS_TT_CACHE_INDEX,
	METALSONIC_TT_CACHE_INDEX,
	ALLWHITE_TT_CACHE_INDEX,
	RAINBOW_TT_CACHE_INDEX,
	BLINK_TT_CACHE_INDEX,
	DASHMODE_TT_CACHE_INDEX,

	TT_CACHE_SIZE
};

// Custom player skin translation
// Initialize color translation tables, for player rendering etc.
UINT8* R_GetTranslationColormap(INT32 skinnum, skincolornum_t color, UINT8 flags);
void R_FlushTranslationColormapCache(void);
UINT16 R_GetColorByName(const char *name);
UINT16 R_GetSuperColorByName(const char *name);

extern UINT8 *transtables; // translucency tables, should be (*transtables)[5][256][256]

enum
{
	blendtab_add,
	blendtab_subtract,
	blendtab_reversesubtract,
	blendtab_modulate,
	NUMBLENDMAPS
};

extern UINT8 *blendtables[NUMBLENDMAPS];

void R_InitTranslucencyTables(void);
void R_GenerateBlendTables(void);

UINT8 *R_GetTranslucencyTable(INT32 alphalevel);
UINT8 *R_GetBlendTable(int style, INT32 alphalevel);

boolean R_BlendLevelVisible(INT32 blendmode, INT32 alphalevel);

// Color ramp modification should force a recache
extern boolean skincolor_modified[];

void R_InitViewBuffer(INT32 width, INT32 height);
void R_VideoErase(size_t ofs, INT32 count);

#define TRANSPARENTPIXEL 255

// -----------------
// 8bpp DRAWING CODE
// -----------------

void R_DrawColumn_8(void);
void R_DrawColumnClamped_8(void);
void R_Draw2sMultiPatchColumn_8(void);
void R_DrawShadeColumn_8(void);
void R_DrawTranslucentColumn_8(void);
void R_DrawTranslucentColumnClamped_8(void);
void R_Draw2sMultiPatchTranslucentColumn_8(void);
void R_DrawDropShadowColumn_8(void);
void R_DrawTranslatedColumn_8(void);
void R_DrawTranslatedTranslucentColumn_8(void);
void R_DrawFogColumn_8(void);
void R_DrawColumnShadowed_8(void);

void R_DrawSpan_8(void);
void R_DrawTranslucentSpan_8(void);
void R_DrawTiltedSpan_8(void);
void R_DrawTiltedTranslucentSpan_8(void);

void R_DrawSplat_8(void);
void R_DrawTranslucentSplat_8(void);
void R_DrawTiltedSplat_8(void);
void R_DrawTiltedTranslucentSplat_8(void);

void R_DrawFloorSprite_8(void);
void R_DrawTranslucentFloorSprite_8(void);
void R_DrawTiltedFloorSprite_8(void);
void R_DrawTiltedTranslucentFloorSprite_8(void);

void R_DrawWaterSpan_8(void);
void R_DrawTiltedWaterSpan_8(void);

void R_DrawFogSpan_8(void);
void R_DrawTiltedFogSpan_8(void);

// Lactozilla: Non-powers-of-two
void R_DrawSpan_NPO2_8(void);
void R_DrawTranslucentSpan_NPO2_8(void);
void R_DrawTiltedSpan_NPO2_8(void);
void R_DrawTiltedTranslucentSpan_NPO2_8(void);

void R_DrawSplat_NPO2_8(void);
void R_DrawTranslucentSplat_NPO2_8(void);
void R_DrawTiltedSplat_NPO2_8(void);
void R_DrawTiltedTranslucentSplat_NPO2_8(void);

void R_DrawFloorSprite_NPO2_8(void);
void R_DrawTranslucentFloorSprite_NPO2_8(void);
void R_DrawTiltedFloorSprite_NPO2_8(void);
void R_DrawTiltedTranslucentFloorSprite_NPO2_8(void);

void R_DrawWaterSpan_NPO2_8(void);
void R_DrawTiltedWaterSpan_NPO2_8(void);

void R_DrawSolidColorSpan_8(void);
void R_DrawTransSolidColorSpan_8(void);
void R_DrawTiltedSolidColorSpan_8(void);
void R_DrawTiltedTransSolidColorSpan_8(void);
void R_DrawWaterSolidColorSpan_8(void);
void R_DrawTiltedWaterSolidColorSpan_8(void);

// ------------------
// 32bpp DRAWING CODE
// ------------------

// The truecolor drawers take the same inputs as the 8bpp ones: textures are
// still palettized, and light tables and translucency tables are still given
// as pointers to their 8bpp versions. Lighting and blending are done in RGB.
// Pixels in the frame buffer are stored the same way as RGBA_t.

// Current palette, as frame buffer pixels
extern UINT32 tc_palette[NUM_PALETTE_ENTRIES];

enum
{
	TCBLEND_TRANSLUCENT,
	TCBLEND_ADD,
	TCBLEND_SUBTRACT,
	TCBLEND_REVERSESUBTRACT,
	TCBLEND_MODULATE
};

// A blend is (TCBLEND_ mode << 16) | amount
#define TCBLEND_MODESHIFT 16
#define TCBLEND_AMOUNTMASK 0xFFFF

// What a row of a light table does to a color, for applying it to the frame buffer
typedef struct
{
	boolean tinted;
	UINT32 tint[3];  // 16.16
	UINT32 untinted; // 0-256
	UINT8 fade[3];
	UINT32 fadeamount; // 0-256
} tcshade_t;

void R_SetTruecolorPalette(const RGBA_t *palette, boolean flash);
void R_RegisterLightTable(const lighttable_t *table, size_t size, const extracolormap_t *extra_colormap);
void R_ClearTruecolorLightTables(void);

const UINT32 *R_GetTruecolorColormap(const lighttable_t *colormap);
void R_GetTruecolorShade(const lighttable_t *colormap, tcshade_t *shade);
UINT32 R_GetTruecolorBlend(const UINT8 *transmap);

UINT32 R_ShadeTruecolorPixel(const tcshade_t *shade, UINT32 pixel);
UINT32 R_BlendTruecolorPixel(UINT32 source, UINT32 dest, UINT32 blend);

// Mixes two pixels. amount is how much of the source to use, from 0 to 256.
FUNCINLINE static ATTRINLINE UINT32 TC_Mix(UINT32 source, UINT32 dest, UINT32 amount)
{
	const UINT32 rest = 256 - amount;
	const UINT32 lo = (((source & 0x00FF00FF) * amount) + ((dest & 0x00FF00FF) * rest)) >> 8;
	const UINT32 hi = ((((source >> 8) & 0x00FF00FF) * amount) + (((dest >> 8) & 0x00FF00FF) * rest));
	return (lo & 0x00FF00FF) | (hi & 0xFF00FF00);
}

// Adds two pixels, clamping each channel.
FUNCINLINE static ATTRINLINE UINT32 TC_Add(UINT32 a, UINT32 b)
{
	UINT32 lo = (a & 0x00FF00FF) + (b & 0x00FF00FF);
	UINT32 hi = ((a >> 8) & 0x00FF00FF) + ((b >> 8) & 0x00FF00FF);
	const UINT32 lo_over = lo & 0x01000100;
	const UINT32 hi_over = hi & 0x01000100;
	lo |= lo_over - (lo_over >> 8);
	hi |= hi_over - (hi_over >> 8);
	return (lo & 0x00FF00FF) | ((hi & 0x00FF00FF) << 8);
}

// Subtracts b from a, clamping each channel.
FUNCINLINE static ATTRINLINE UINT32 TC_Subtract(UINT32 a, UINT32 b)
{
	UINT32 lo = ((a & 0x00FF00FF) | 0x01000100) - (b & 0x00FF00FF);
	UINT32 hi = (((a >> 8) & 0x00FF00FF) | 0x01000100) - ((b >> 8) & 0x00FF00FF);
	const UINT32 lo_keep = lo & 0x01000100;
	const UINT32 hi_keep = hi & 0x01000100;
	lo &= lo_keep - (lo_keep >> 8);
	hi &= hi_keep - (hi_keep >> 8);
	return (lo & 0x00FF00FF) | ((hi & 0x00FF00FF) << 8);
}

FUNCINLINE static ATTRINLINE UINT32 TC_Blend(UINT32 source, UINT32 dest, UINT32 blend)
{
	if (blend <= TCBLEND_AMOUNTMASK) // TCBLEND_TRANSLUCENT
		return TC_Mix(source, dest, blend);
	return R_BlendTruecolorPixel(source, dest, blend);
}

void R_DrawColumn_32(void);
void R_DrawColumnClamped_32(void);
void R_Draw2sMultiPatchColumn_32(void);
void R_DrawShadeColumn_32(void);
void R_DrawTranslucentColumn_32(void);
void R_DrawTranslucentColumnClamped_32(void);
void R_Draw2sMultiPatchTranslucentColumn_32(void);
void R_DrawDropShadowColumn_32(void);
void R_DrawTranslatedColumn_32(void);
void R_DrawTranslatedTranslucentColumn_32(void);
void R_DrawFogColumn_32(void);
void R_DrawColumnShadowed_32(void);

void R_DrawSpan_32(void);
void R_DrawTranslucentSpan_32(void);
void R_DrawTiltedSpan_32(void);
void R_DrawTiltedTranslucentSpan_32(void);

void R_DrawSplat_32(void);
void R_DrawTranslucentSplat_32(void);
void R_DrawTiltedSplat_32(void);
void R_DrawTiltedTranslucentSplat_32(void);

void R_DrawFloorSprite_32(void);
void R_DrawTranslucentFloorSprite_32(void);
void R_DrawTiltedFloorSprite_32(void);
void R_DrawTiltedTranslucentFloorSprite_32(void);

void R_DrawWaterSpan_32(void);
void R_DrawTiltedWaterSpan_32(void);

void R_DrawFogSpan_32(void);
void R_DrawTiltedFogSpan_32(void);

void R_DrawSpan_NPO2_32(void);
void R_DrawTranslucentSpan_NPO2_32(void);
void R_DrawTiltedSpan_NPO2_32(void);
void R_DrawTiltedTranslucentSpan_NPO2_32(void);

void R_DrawSplat_NPO2_32(void);
void R_DrawTranslucentSplat_NPO2_32(void);
void R_DrawTiltedSplat_NPO2_32(void);
void R_DrawTiltedTranslucentSplat_NPO2_32(void);

void R_DrawFloorSprite_NPO2_32(void);
void R_DrawTranslucentFloorSprite_NPO2_32(void);
void R_DrawTiltedFloorSprite_NPO2_32(void);
void R_DrawTiltedTranslucentFloorSprite_NPO2_32(void);

void R_DrawWaterSpan_NPO2_32(void);
void R_DrawTiltedWaterSpan_NPO2_32(void);

void R_DrawSolidColorSpan_32(void);
void R_DrawTransSolidColorSpan_32(void);
void R_DrawTiltedSolidColorSpan_32(void);
void R_DrawTiltedTransSolidColorSpan_32(void);
void R_DrawWaterSolidColorSpan_32(void);
void R_DrawTiltedWaterSolidColorSpan_32(void);

// =========================================================================
#endif  // __R_DRAW__

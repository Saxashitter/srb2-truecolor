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
/// \file  r_main.c
/// \brief Rendering main loop and setup functions,
///        utility functions (BSP, geometry, trigonometry).
///        See tables.c, too.

#include "doomdef.h"
#include "g_game.h"
#include "g_input.h"
#include "r_local.h"
#include "r_splats.h" // faB(21jan): testing
#include "r_sky.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "p_local.h"
#include "keys.h"
#include "i_video.h"
#include "m_menu.h"
#include "am_map.h"
#include "d_main.h"
#include "v_video.h"
#include "p_spec.h" // skyboxmo
#include "p_setup.h"
#include "z_zone.h"
#include "m_random.h" // quake camera shake
#include "r_portal.h"
#include "r_main.h"
#include "i_system.h" // I_GetPreciseTime
#include "r_fps.h" // Frame interpolation/uncapped

#ifdef HWRENDER
#include "hardware/hw_main.h"
#endif

// Fineangles in the SCREENWIDTH wide window.
#define FIELDOFVIEW 2048

// increment every time a check is made
size_t validcount = 1;

INT32 centerx, centery;

fixed_t centerxfrac, centeryfrac;
fixed_t projection;
fixed_t projectiony; // aspect ratio
fixed_t fovtan; // field of view

// just for profiling purposes
size_t framecount;

size_t loopcount;

fixed_t viewx, viewy, viewz;
angle_t viewangle, aimingangle;
fixed_t viewcos, viewsin;
sector_t *viewsector;
player_t *viewplayer;
mobj_t *r_viewmobj;

boolean r_renderwalls;
boolean r_renderfloors;
boolean r_renderthings;

fixed_t rendertimefrac;
fixed_t renderdeltatics;
boolean renderisnewtic;

//
// precalculated math tables
//
angle_t clipangle;
angle_t doubleclipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
INT32 viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t xtoviewangle[MAXVIDWIDTH+1];

lighttable_t *scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
lighttable_t *scalelightfixed[MAXLIGHTSCALE];
lighttable_t *zlight[LIGHTLEVELS][MAXLIGHTZ];

// Hack to support extra boom colormaps.
extracolormap_t *extra_colormaps = NULL;

// Render stats
precise_t ps_prevframetime = 0;
ps_metric_t ps_rendercalltime = {0};
ps_metric_t ps_otherrendertime = {0};
ps_metric_t ps_uitime = {0};
ps_metric_t ps_swaptime = {0};

ps_metric_t ps_bsptime = {0};

ps_metric_t ps_sw_spritecliptime = {0};
ps_metric_t ps_sw_portaltime = {0};
ps_metric_t ps_sw_planetime = {0};
ps_metric_t ps_sw_maskedtime = {0};

ps_metric_t ps_numbspcalls = {0};
ps_metric_t ps_numsprites = {0};
ps_metric_t ps_numdrawnodes = {0};
ps_metric_t ps_numpolyobjects = {0};

static CV_PossibleValue_t drawdist_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{3072, "3072"},	{4096, "4096"},	{6144, "6144"},
	{8192, "8192"},	{0, "Infinite"},	{0, NULL}};

//static CV_PossibleValue_t precipdensity_cons_t[] = {{0, "None"}, {1, "Light"}, {2, "Moderate"}, {4, "Heavy"}, {6, "Thick"}, {8, "V.Thick"}, {0, NULL}};

static CV_PossibleValue_t drawdist_precip_cons_t[] = {
	{256, "256"},	{512, "512"},	{768, "768"},
	{1024, "1024"},	{1536, "1536"},	{2048, "2048"},
	{0, "None"},	{0, NULL}};

static CV_PossibleValue_t fov_cons_t[] = {{MINFOV*FRACUNIT, "MIN"}, {MAXFOV*FRACUNIT, "MAX"}, {0, NULL}};
static CV_PossibleValue_t translucenthud_cons_t[] = {{0, "MIN"}, {10, "MAX"}, {0, NULL}};
static CV_PossibleValue_t maxportals_cons_t[] = {{0, "MIN"}, {12, "MAX"}, {0, NULL}}; // lmao rendering 32 portals, you're a card
static CV_PossibleValue_t homremoval_cons_t[] = {{0, "No"}, {1, "Yes"}, {2, "Flash"}, {0, NULL}};

static void R_SetFov(fixed_t playerfov);

static void Fov_OnChange(void);
static void ChaseCam_OnChange(void);
static void ChaseCam2_OnChange(void);
static void FlipCam_OnChange(void);
static void FlipCam2_OnChange(void);
void SendWeaponPref(void);
void SendWeaponPref2(void);

consvar_t cv_tailspickup = CVAR_INIT ("tailspickup", "On", CV_NETVAR|CV_ALLOWLUA, CV_OnOff, NULL);
consvar_t cv_chasecam = CVAR_INIT ("chasecam", "On", CV_CALL, CV_OnOff, ChaseCam_OnChange);
consvar_t cv_chasecam2 = CVAR_INIT ("chasecam2", "On", CV_CALL, CV_OnOff, ChaseCam2_OnChange);
consvar_t cv_flipcam = CVAR_INIT ("flipcam", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam_OnChange);
consvar_t cv_flipcam2 = CVAR_INIT ("flipcam2", "No", CV_SAVE|CV_CALL|CV_NOINIT, CV_YesNo, FlipCam2_OnChange);

consvar_t cv_shadow = CVAR_INIT ("shadow", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_skybox = CVAR_INIT ("skybox", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_allowmlook = CVAR_INIT ("allowmlook", "Yes", CV_NETVAR|CV_ALLOWLUA, CV_YesNo, NULL);
consvar_t cv_showhud = CVAR_INIT ("showhud", "Yes", CV_CALL|CV_ALLOWLUA,  CV_YesNo, R_SetViewSize);
consvar_t cv_translucenthud = CVAR_INIT ("translucenthud", "10", CV_SAVE, translucenthud_cons_t, NULL);

consvar_t cv_translucency = CVAR_INIT ("translucency", "On", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_drawdist = CVAR_INIT ("drawdist", "Infinite", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_nights = CVAR_INIT ("drawdist_nights", "2048", CV_SAVE, drawdist_cons_t, NULL);
consvar_t cv_drawdist_precip = CVAR_INIT ("drawdist_precip", "1024", CV_SAVE, drawdist_precip_cons_t, NULL);
consvar_t cv_fov = CVAR_INIT ("fov", "90", CV_SAVE|CV_FLOAT|CV_CALL, fov_cons_t, Fov_OnChange);

// Software renderer only: look up and down with real perspective, instead of shearing the view
consvar_t cv_trueperspective = CVAR_INIT ("trueperspective", "Off", CV_SAVE, CV_OnOff, NULL);

// The most times the view is rendered for it, not counting the ones for looking straight up or down.
// More is sharper when looking far up or down, and slower.
static CV_PossibleValue_t trueperspectivebands_cons_t[] = {{2, "MIN"}, {8, "MAX"}, {0, NULL}};
consvar_t cv_trueperspectivebands = CVAR_INIT ("trueperspective_bands", "6", CV_SAVE, trueperspectivebands_cons_t, NULL);
consvar_t cv_fovchange = CVAR_INIT ("fovchange", "Off", CV_SAVE, CV_OnOff, NULL);
consvar_t cv_maxportals = CVAR_INIT ("maxportals", "2", CV_SAVE, maxportals_cons_t, NULL);

consvar_t cv_renderview = CVAR_INIT ("renderview", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderwalls = CVAR_INIT ("r_renderwalls", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderfloors = CVAR_INIT ("r_renderfloors", "On", 0, CV_OnOff, NULL);
consvar_t cv_renderthings = CVAR_INIT ("r_renderthings", "On", 0, CV_OnOff, NULL);
consvar_t cv_ffloorclip = CVAR_INIT ("r_ffloorclip", "On", 0, CV_OnOff, NULL);
consvar_t cv_spriteclip = CVAR_INIT ("r_spriteclip", "On", 0, CV_OnOff, NULL);

consvar_t cv_homremoval = CVAR_INIT ("homremoval", "No", CV_SAVE, homremoval_cons_t, NULL);

consvar_t cv_renderstats = CVAR_INIT ("renderstats", "Off", 0, CV_OnOff, NULL);

void SplitScreen_OnChange(void)
{
	if (!cv_debug && netgame)
	{
		if (splitscreen)
		{
			CONS_Alert(CONS_NOTICE, M_GetText("Splitscreen not supported in netplay, sorry!\n"));
			splitscreen = false;
		}
		return;
	}

	// recompute screen size
	R_ExecuteSetViewSize();

	if (!demoplayback && !botingame)
	{
		if (splitscreen)
			CL_AddSplitscreenPlayer();
		else
			CL_RemoveSplitscreenPlayer();

		if (server && !netgame)
			multiplayer = splitscreen;
	}
	else
	{
		INT32 i;
		secondarydisplayplayer = consoleplayer;
		for (i = 0; i < MAXPLAYERS; i++)
			if (playeringame[i] && i != consoleplayer)
			{
				secondarydisplayplayer = i;
				break;
			}
	}
}
static void Fov_OnChange(void)
{
	R_SetViewSize();
}

static void ChaseCam_OnChange(void)
{
	if (!cv_chasecam.value || !cv_useranalog[0].value)
		CV_SetValue(&cv_analog[0], 0);
	else
		CV_SetValue(&cv_analog[0], 1);
}

static void ChaseCam2_OnChange(void)
{
	if (botingame)
		return;
	if (!cv_chasecam2.value || !cv_useranalog[1].value)
		CV_SetValue(&cv_analog[1], 0);
	else
		CV_SetValue(&cv_analog[1], 1);
}

static void FlipCam_OnChange(void)
{
	SendWeaponPref();
}

static void FlipCam2_OnChange(void)
{
	SendWeaponPref2();
}

//
// R_PointOnSide
// Traverse BSP (sub) tree,
// check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//
INT32 R_PointOnSide(fixed_t x, fixed_t y, node_t *restrict node)
{
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;

	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;

	fixed_t dx = (x >> 1) - (node->x >> 1);
	fixed_t dy = (y >> 1) - (node->y >> 1);

	// Try to quickly decide by looking at sign bits.
	// also use a mask to avoid branch prediction
	INT32 mask = (node->dy ^ node->dx ^ dx ^ dy) >> 31;
	return (mask & ((node->dy ^ dx) < 0)) |  // (left is negative)
		(~mask & (FixedMul(dy, node->dx>>FRACBITS) >= FixedMul(node->dy>>FRACBITS, dx)));
}

// killough 5/2/98: reformatted
INT32 R_PointOnSegSide(fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t lx = line->v1->x;
	fixed_t ly = line->v1->y;
	fixed_t ldx = line->v2->x - lx;
	fixed_t ldy = line->v2->y - ly;

	if (!ldx)
		return x <= lx ? ldy > 0 : ldy < 0;

	if (!ldy)
		return y <= ly ? ldx < 0 : ldx > 0;

	fixed_t dx = (x >> 1) - (lx >> 1);
	fixed_t dy = (y >> 1) - (ly >> 1);

	// Try to quickly decide by looking at sign bits.
	if ((ldy ^ ldx ^ dx ^ dy) < 0)
		return (ldy ^ dx) < 0;          // (left is negative)
	return FixedMul(dy, ldx>>FRACBITS) >= FixedMul(ldy>>FRACBITS, dx);
}

//
// R_PointToAngle
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up

angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

// This version uses 64-bit variables to avoid overflows with large values.
angle_t R_PointToAngle64(INT64 x, INT64 y)
{
	return (y -= viewy, (x -= viewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDivEx(y,x)] :                            // octant 0
		ANGLE_90-tantoangle[SlopeDivEx(x,y)] :                               // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDivEx(y,x)] :                    // octant 8
		ANGLE_270+tantoangle[SlopeDivEx(x,y)] :                              // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDivEx(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDivEx(x,y)] :                             // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDivEx(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDivEx(x,y)] :                              // octant 5
		0;
}

angle_t R_PointToAngle2(fixed_t pviewx, fixed_t pviewy, fixed_t x, fixed_t y)
{
	return (y -= pviewy, (x -= pviewx) || y) ?
	x >= 0 ?
	y >= 0 ?
		(x > y) ? tantoangle[SlopeDiv(y,x)] :                          // octant 0
		ANGLE_90-tantoangle[SlopeDiv(x,y)] :                           // octant 1
		x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :                   // octant 8
		ANGLE_270+tantoangle[SlopeDiv(x,y)] :                          // octant 7
		y >= 0 ? (x = -x) > y ? ANGLE_180-tantoangle[SlopeDiv(y,x)] :  // octant 3
		ANGLE_90 + tantoangle[SlopeDiv(x,y)] :                         // octant 2
		(x = -x) > (y = -y) ? ANGLE_180+tantoangle[SlopeDiv(y,x)] :    // octant 4
		ANGLE_270-tantoangle[SlopeDiv(x,y)] :                          // octant 5
		0;
}

fixed_t R_PointToDist2(fixed_t px2, fixed_t py2, fixed_t px1, fixed_t py1)
{
	angle_t angle;
	ufixed_t dx, dy, dist;

	dx = abs(px1 - px2);
	dy = abs(py1 - py2);

	if (dy > dx)
	{
		fixed_t temp;

		temp = dx;
		dx = dy;
		dy = temp;
	}
	if (!dy)
		return dx;

	angle = (tantoangle[FixedDiv(dy, dx)>>DBITS] + ANGLE_90) >> ANGLETOFINESHIFT;

	// use as cosine
	dist = FixedDiv(dx, FINESINE(angle));

	return dist;
}

// Little extra utility. Works in the same way as R_PointToAngle2
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	return R_PointToDist2(viewx, viewy, x, y);
}

line_t *R_GetFFloorLine(const line_t *line, const ffloor_t *pfloor, const sector_t *sector)
{
	if (pfloor->master->flags & ML_TFERLINE)
	{
		size_t linenum = min((size_t)(line - sector->lines[0]), pfloor->master->frontsector->linecount);
		return pfloor->master->frontsector->lines[0] + linenum;
	}
	else
		return pfloor->master;
}

side_t *R_GetFFloorSide(const line_t *line, const ffloor_t *pfloor, const sector_t *sector)
{
	if (pfloor->master->flags & ML_TFERLINE)
	{
		line_t *newline = R_GetFFloorLine(line, pfloor, sector);
		return &sides[newline->sidenum[0]];
	}
	else
		return &sides[pfloor->master->sidenum[0]];
}

//
// R_DoCulling
// Checks viewz and top/bottom heights of an item against culling planes
// Returns true if the item is to be culled, i.e it shouldn't be drawn!
// if args[1] is set, the camera view is required to be in the same area for culling to occur
boolean R_DoCulling(line_t *cullheight, line_t *viewcullheight, fixed_t vz, fixed_t bottomh, fixed_t toph)
{
	fixed_t cullplane;

	if (!cullheight)
		return false;

	cullplane = cullheight->frontsector->floorheight;
	if (cullheight->args[1]) // Group culling
	{
		if (!viewcullheight)
			return false;

		// Make sure this is part of the same group
		if (viewcullheight->frontsector == cullheight->frontsector)
		{
			// OK, we can cull
			if (vz > cullplane && toph < cullplane) // Cull if below plane
				return true;

			if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
				return true;
		}
	}
	else // Quick culling
	{
		if (vz > cullplane && toph < cullplane) // Cull if below plane
			return true;

		if (bottomh > cullplane && vz <= cullplane) // Cull if above plane
			return true;
	}

	return false;
}

//
// R_InitTextureMapping
//
static void R_InitTextureMapping(void)
{
	INT32 i;
	INT32 x;
	INT32 t;
	fixed_t focallength;

	// Use tangent table to generate viewangletox:
	//  viewangletox will give the next greatest x
	//  after the view angle.
	//
	// Calc focallength
	//  so FIELDOFVIEW angles covers SCREENWIDTH.
	focallength = FixedDiv(projection,
		FINETANGENT(FINEANGLES/4+FIELDOFVIEW/2));

	focallengthf = FIXED_TO_FLOAT(focallength);

	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (FINETANGENT(i) > fovtan*2)
			t = -1;
		else if (FINETANGENT(i) < -fovtan*2)
			t = viewwidth+1;
		else
		{
			t = FixedMul(FINETANGENT(i), focallength);
			t = (centerxfrac - t+FRACUNIT-1)>>FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > viewwidth+1)
				t = viewwidth+1;
		}
		viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//  xtoviewangle will give the smallest view angle
	//  that maps to x.
	// viewangletox[] only ever goes down, so going through x backwards,
	// each search can start from where the last one stopped.
	i = 0;
	for (x = viewwidth; x >= 0; x--)
	{
		while (viewangletox[i] > x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT) - ANGLE_90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == viewwidth+1)
			viewangletox[i]  = viewwidth;
	}

	clipangle = xtoviewangle[0];
	doubleclipangle = clipangle*2;
}



//
// R_InitLightTables
// Only inits the zlight table,
//  because the scalelight table changes with view size.
//
#define DISTMAP 2

static inline void R_InitLightTables(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;
	INT32 scale;

	// Calculate the light levels to use
	//  for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTZ; j++)
		{
			//added : 02-02-98 : use BASEVIDWIDTH, vid.width is not set already,
			// and it seems it needs to be calculated only once.
			scale = FixedDiv((BASEVIDWIDTH/2*FRACUNIT), (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT;
			level = startmapl - scale/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = colormaps + level*256;
		}
	}
}

//#define WOUGHMP_WOUGHMP // I got a fish-eye lens - I'll make a rap video with a couple of friends
// it's kinda laggy sometimes

static struct {
	angle_t rollangle; // pre-shifted by fineshift
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye;
#endif

	fixed_t zoomneeded;
	INT32 *scrmap;
	INT32 scrmapsize;

	INT32 x1; // clip rendering horizontally for efficiency
	INT16 ceilingclip[MAXVIDWIDTH], floorclip[MAXVIDWIDTH];

	boolean use;
} viewmorph = {
	0,
#ifdef WOUGHMP_WOUGHMP
	0,
#endif

	FRACUNIT,
	NULL,
	0,

	0,
	{0}, {0},

	false
};

// True perspective for looking up and down.
// All the renderer can do is shear the view, which is looking straight ahead and
// sliding the picture up or down. What a tilted camera would see is that same
// picture seen at an angle: every row of it is one row of the sheared view, scaled
// horizontally. So the view is rendered sheared and wider, then warped.
//
// Rows that look further up or down need a much wider sheared view than the rest,
// and one that wide leaves little detail for the others. So the screen is split
// into bands of rows, and the view is rendered once for each, as wide as it needs.
//
// Rows that look past straight up or down are behind the camera as far as a
// sheared view goes. Those come from bands rendered facing the other way.
//
// That leaves the rows in between, which look straight up or down in the middle
// and out to the sides at the ends. No view facing ahead or back is wide enough
// for those, so the ends of them come from two more views, facing left and right.
#define TRUEPERSPECTIVE_MAXBANDS 8

typedef struct
{
	UINT8 facing;                // which way it is rendered facing: 0 ahead, 1 right, 2 back, 3 left
	float validtop, validbottom; // the rows it can show, from the middle of the screen
	INT32 row, endrow;           // the screen rows it is used for
	INT32 validrow;              // the first of those that it can show
	float sidenear, sidefar;     // facing left or right: how far from the middle of the screen it is used
} trueperspectiveband_t;

static struct {
	boolean use;
	fixed_t zoom;       // how much wider the sheared view has to be
	fixed_t basefovtan; // fovtan without the zoom
	fixed_t fov;        // last field of view given to R_SetFov
	float sine, cosine; // of the angle the camera is tilted by
	float horizon;      // the row of the sheared view that is level with the camera
	// The part of the sheared view that the band is warped from, which is all that gets drawn
	INT32 clipleft, clipright;
	INT16 cliptop[MAXVIDWIDTH], clipbottom[MAXVIDWIDTH];

	INT32 band, numbands;
	trueperspectiveband_t bands[TRUEPERSPECTIVE_MAXBANDS + 4];
} trueperspective = {false, FRACUNIT, FRACUNIT, 0, 0.0f, 1.0f, 0.0f, 0, 0, {0}, {0}, 0, 1, {{0, 0.0f, 0.0f, 0, 0, 0, 0.0f, 0.0f}}};

// The sheared view can't reach straight up or down however wide it gets.
// This is as wide as it goes, for the views facing ahead and back.
#define TRUEPERSPECTIVE_MAXZOOM 32.0f

// How much more one edge of a band may be scaled down than the other.
// This goes up if it would take too many bands.
#define TRUEPERSPECTIVE_BANDRATIO 1.6f

// Bands are never made smaller than this many rows.
#define TRUEPERSPECTIVE_MINROWS 16

void R_CheckViewMorph(void)
{
	float zoomfactor, rollcos, rollsin;
	float x1, y1, x2, y2;
	fixed_t temp;
	INT32 end, vx, vy, pos, usedpos;
	INT32 usedx, usedy, halfwidth = vid.width/2, halfheight = vid.height/2;
#ifdef WOUGHMP_WOUGHMP
	float fisheyemap[MAXVIDWIDTH/2 + 1];
#endif

	angle_t rollangle = players[displayplayer].viewrollangle;
#ifdef WOUGHMP_WOUGHMP
	fixed_t fisheye = cv_cam2_turnmultiplier.value; // temporary test value
#endif

	rollangle >>= ANGLETOFINESHIFT;
	rollangle = ((rollangle+2) & ~3) & FINEMASK; // Limit the distinct number of angles to reduce recalcs from angles changing a lot.

#ifdef WOUGHMP_WOUGHMP
	fisheye &= ~0x7FF; // Same
#endif

	if (rollangle == viewmorph.rollangle &&
#ifdef WOUGHMP_WOUGHMP
		fisheye == viewmorph.fisheye &&
#endif
		viewmorph.scrmapsize == vid.width*vid.height)
		return; // No change

	viewmorph.rollangle = rollangle;
#ifdef WOUGHMP_WOUGHMP
	viewmorph.fisheye = fisheye;
#endif

	if (viewmorph.rollangle == 0
#ifdef WOUGHMP_WOUGHMP
		 && viewmorph.fisheye == 0
#endif
	 )
	{
		viewmorph.use = false;
		viewmorph.x1 = 0;
		if (viewmorph.zoomneeded != FRACUNIT)
			R_SetViewSize();
		viewmorph.zoomneeded = FRACUNIT;

		return;
	}

	if (viewmorph.scrmapsize != vid.width*vid.height)
	{
		if (viewmorph.scrmap)
			free(viewmorph.scrmap);
		viewmorph.scrmap = malloc(vid.width*vid.height * sizeof(INT32));
		viewmorph.scrmapsize = vid.width*vid.height;
	}

	temp = FINECOSINE(rollangle);
	rollcos = FIXED_TO_FLOAT(temp);
	temp = FINESINE(rollangle);
	rollsin = FIXED_TO_FLOAT(temp);

	// Calculate maximum zoom needed
	x1 = (vid.width*fabsf(rollcos) + vid.height*fabsf(rollsin)) / vid.width;
	y1 = (vid.height*fabsf(rollcos) + vid.width*fabsf(rollsin)) / vid.height;

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		float f = FIXED_TO_FLOAT(fisheye);
		for (vx = 0; vx <= halfwidth; vx++)
			fisheyemap[vx] = 1.0f / cos(atan(vx * f / halfwidth));

		f = cos(atan(f));
		if (f < 1.0f)
		{
			x1 /= f;
			y1 /= f;
		}
	}
#endif

	temp = max(x1, y1)*FRACUNIT;
	if (temp < FRACUNIT)
		temp = FRACUNIT;
	else
		temp |= 0x3FFF; // Limit how many times the viewport needs to be recalculated

	//CONS_Printf("Setting zoom to %f\n", FIXED_TO_FLOAT(temp));

	if (temp != viewmorph.zoomneeded)
	{
		viewmorph.zoomneeded = temp;
		R_SetViewSize();
	}

	zoomfactor = FIXED_TO_FLOAT(viewmorph.zoomneeded);

	end = vid.width * vid.height - 1;

	pos = 0;

	// Pre-multiply rollcos and rollsin to use for positional stuff
	rollcos /= zoomfactor;
	rollsin /= zoomfactor;

	x1 = -(halfwidth * rollcos - halfheight * rollsin);
	y1 = -(halfheight * rollcos + halfwidth * rollsin);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
		viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)) * fisheyemap[halfwidth]);
	else
#endif
	viewmorph.x1 = (INT32)(halfwidth - (halfwidth * fabsf(rollcos) + halfheight * fabsf(rollsin)));
	//CONS_Printf("saving %d cols\n", viewmorph.x1);

	// Set ceilingclip and floorclip
	for (vx = 0; vx < vid.width; vx++)
	{
		viewmorph.ceilingclip[vx] = vid.height;
		viewmorph.floorclip[vx] = -1;
	}
	x2 = x1;
	y2 = y1;
	for (vx = 0; vx < vid.width; vx++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight-1;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 += rollcos;
		y2 += rollsin;
	}
	x2 = x1;
	y2 = y1;
	for (vy = 0; vy < vid.height; vy++)
	{
		INT16 xa, ya, xb, yb;
		xa = x2+halfwidth;
		ya = y2+halfheight;
		xb = vid.width-1-xa;
		yb = vid.height-1-ya;

		viewmorph.ceilingclip[xa] = min(viewmorph.ceilingclip[xa], ya);
		viewmorph.floorclip[xa] = max(viewmorph.floorclip[xa], ya);
		viewmorph.ceilingclip[xb] = min(viewmorph.ceilingclip[xb], yb);
		viewmorph.floorclip[xb] = max(viewmorph.floorclip[xb], yb);
		x2 -= rollsin;
		y2 += rollcos;
	}

	//CONS_Printf("Top left corner is %f %f\n", x1, y1);

#ifdef WOUGHMP_WOUGHMP
	if (fisheye)
	{
		for (vy = 0; vy < halfheight; vy++)
		{
			x2 = x1;
			y2 = y1;
			x1 -= rollsin;
			y1 += rollcos;

			for (vx = 0; vx < vid.width; vx++)
			{
				usedx = halfwidth + x2*fisheyemap[(int) floorf(fabsf(y2*zoomfactor))];
				usedy = halfheight + y2*fisheyemap[(int) floorf(fabsf(x2*zoomfactor))];

				usedpos = usedx + usedy*vid.width;

				viewmorph.scrmap[pos] = usedpos;
				viewmorph.scrmap[end-pos] = end-usedpos;

				x2 += rollcos;
				y2 += rollsin;
				pos++;
			}
		}
	}
	else
	{
#endif
	x1 += halfwidth;
	y1 += halfheight;

	for (vy = 0; vy < halfheight; vy++)
	{
		x2 = x1;
		y2 = y1;
		x1 -= rollsin;
		y1 += rollcos;

		for (vx = 0; vx < vid.width; vx++)
		{
			usedx = x2;
			usedy = y2;

			usedpos = usedx + usedy*vid.width;

			viewmorph.scrmap[pos] = usedpos;
			viewmorph.scrmap[end-pos] = end-usedpos;

			x2 += rollcos;
			y2 += rollsin;
			pos++;
		}
	}
#ifdef WOUGHMP_WOUGHMP
	}
#endif

	viewmorph.use = true;
}

// The warp smooths truecolor pixels between two rows, then along the row.
// Both are done a row at a time, which is a lot quicker than a pixel at a time.
#ifdef __SSE2__
#include <emmintrin.h>
#endif

// Mixes two rows of pixels. weight is how much of the second row to use, from 0 to 128.
static void R_MixTruecolorRows(UINT32 *dest, const UINT32 *row1, const UINT32 *row2, INT32 weight, INT32 count)
{
	INT32 x = 0;

#ifdef __SSE2__
	const __m128i zero = _mm_setzero_si128();
	const __m128i weights = _mm_set1_epi16((short)weight);

	for (; x + 4 <= count; x += 4)
	{
		const __m128i pixels1 = _mm_loadu_si128((const __m128i *)(const void *)&row1[x]);
		const __m128i pixels2 = _mm_loadu_si128((const __m128i *)(const void *)&row2[x]);
		const __m128i low1 = _mm_unpacklo_epi8(pixels1, zero), high1 = _mm_unpackhi_epi8(pixels1, zero);
		const __m128i low2 = _mm_unpacklo_epi8(pixels2, zero), high2 = _mm_unpackhi_epi8(pixels2, zero);
		const __m128i low = _mm_add_epi16(low1, _mm_srai_epi16(_mm_mullo_epi16(_mm_sub_epi16(low2, low1), weights), 7));
		const __m128i high = _mm_add_epi16(high1, _mm_srai_epi16(_mm_mullo_epi16(_mm_sub_epi16(high2, high1), weights), 7));
		_mm_storeu_si128((__m128i *)(void *)&dest[x], _mm_packus_epi16(low, high));
	}
#endif

	for (; x < count; x++)
		dest[x] = TC_Mix(row2[x], row1[x], weight * 2);
}

// Stretches a row of pixels, smoothing between them.
// src needs a pixel to spare before and after the ones that are used.
static void R_ScaleTruecolorRow(UINT32 *dest, const UINT32 *src, fixed_t xfrac, fixed_t xstep, INT32 count)
{
	INT32 x = 0;

#ifdef __SSE2__
	const __m128i zero = _mm_setzero_si128();

	for (; x < count; x++, xfrac += xstep)
	{
		// both pixels, as 16 bits a channel
		const __m128i pixels = _mm_unpacklo_epi8(_mm_loadl_epi64((const __m128i *)(const void *)&src[xfrac >> FRACBITS]), zero);
		const __m128i next = _mm_srli_si128(pixels, 8);
		const __m128i weights = _mm_set1_epi16((short)((xfrac >> (FRACBITS - 7)) & 0x7F));
		const __m128i mixed = _mm_add_epi16(pixels, _mm_srai_epi16(_mm_mullo_epi16(_mm_sub_epi16(next, pixels), weights), 7));
		dest[x] = (UINT32)_mm_cvtsi128_si32(_mm_packus_epi16(mixed, zero));
	}
#else
	for (; x < count; x++, xfrac += xstep)
	{
		const UINT32 *pixels = &src[xfrac >> FRACBITS];
		dest[x] = TC_Mix(pixels[1], pixels[0], (xfrac >> (FRACBITS - 8)) & 0xFF);
	}
#endif
}

// Warps the ends of the rows that only a view facing left or right can show.
static void R_ApplyTruePerspectiveSide(void)
{
	const trueperspectiveband_t *band = &trueperspective.bands[trueperspective.band];
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	const float focal = (viewwidth / 2.0f) / FIXED_TO_FLOAT(trueperspective.basefovtan);
	const float srcfocal = FIXED_TO_FLOAT(projection);
	const float halfwidth = viewwidth / 2.0f;
	const INT32 startx = (INT32)((band->facing == 1) ? (halfwidth + band->sidenear) : (halfwidth - band->sidefar));
	const INT32 endx = (INT32)((band->facing == 1) ? (halfwidth + band->sidefar) : (halfwidth - band->sidenear));
	INT32 x, y;

	for (y = band->row; y < band->endrow; y++)
	{
		const float row = (y + 0.5f) - (viewheight / 2.0f);
		const float depth = (focal * trueperspective.cosine) + (row * trueperspective.sine);
		const float height = (focal * trueperspective.sine) - (row * trueperspective.cosine);

		for (x = max(startx, 0); x < min(endx, viewwidth); x++)
		{
			// The side view looks along the row, so how far along it is how far away.
			const float column = (x + 0.5f) - halfwidth;
			const float distance = max(fabsf(column), 1.0f);
			const float srcx = max(min(halfwidth - ((srcfocal * depth) / column) - 0.5f, (float)(viewwidth - 1)), 0.0f);
			const float srcy = max(min(trueperspective.horizon - ((srcfocal * height) / distance) - 0.5f, (float)(viewheight - 1)), 0.0f);
			// Fade in from the middle, which the bands before this filled in as well as they could.
			const float fadein = ((distance - band->sidenear) * 512.0f) / band->sidenear;
			const UINT32 fade = (UINT32)max(min(fadein, 256.0f), 0.0f);

			if (vid.bpp == 4)
			{
				const INT32 x1 = min((INT32)srcx, viewwidth - 2), y1 = min((INT32)srcy, viewheight - 2);
				const UINT32 xweight = (UINT32)((srcx - x1) * 256.0f), yweight = (UINT32)((srcy - y1) * 256.0f);
				const UINT32 *src = (const UINT32 *)(const void *)srcscr + (y1 * vid.width) + x1;
				UINT32 *dest = (UINT32 *)(void *)tmpscr + (y * vid.width) + x;
				const UINT32 pixel = TC_Mix(TC_Mix(src[vid.width + 1], src[vid.width], xweight), TC_Mix(src[1], src[0], xweight), yweight);
				*dest = TC_Mix(pixel, *dest, fade);
			}
			else if (fade >= 128)
				tmpscr[(y * vid.width) + x] = srcscr[((INT32)(srcy + 0.5f) * vid.width) + (INT32)(srcx + 0.5f)];
		}
	}
}

// Warps the rows of the band that was just rendered into screens[4].
static void R_ApplyTruePerspective(void)
{
	const trueperspectiveband_t *band = &trueperspective.bands[trueperspective.band];
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	const float focal = (viewwidth / 2.0f) / FIXED_TO_FLOAT(trueperspective.basefovtan);
	const float srcfocal = FIXED_TO_FLOAT(projection);
	const float halfwidth = viewwidth / 2.0f;
	INT32 x, y;

	if (band->facing & 1)
	{
		R_ApplyTruePerspectiveSide();
		return;
	}

	for (y = band->row; y < band->endrow; y++)
	{
		// Where this row of the tilted camera's screen is in the sheared view
		const float row = max(min((y + 0.5f) - (viewheight / 2.0f), band->validbottom), band->validtop);
		const float depth = max(fabsf((focal * trueperspective.cosine) + (row * trueperspective.sine)), 0.001f);
		const float level = ((focal * trueperspective.sine) - (row * trueperspective.cosine)) / depth;
		const float scale = min(srcfocal / depth, 1.0f);
		const float srcrow = trueperspective.horizon - (srcfocal * level);
		fixed_t xstep = (fixed_t)(scale * FRACUNIT);
		fixed_t xfrac = (fixed_t)((halfwidth - ((halfwidth - 0.5f) * scale)) * FRACUNIT);
		UINT32 fade = 256; // how much of this band to show

		if (y < band->validrow && trueperspective.band > 0)
			fade = ((y - band->row + 1) * 256) / (band->validrow - band->row + 1);

		if (band->facing == 2)
		{
			// Looking the other way, so left and right are swapped.
			xfrac = (fixed_t)((halfwidth + ((halfwidth - 0.5f) * scale)) * FRACUNIT);
			xstep = -xstep;
		}

		if (vid.bpp == 4)
		{
			// Enough colors to smooth it out
			static UINT32 rowbuffer[MAXVIDWIDTH + 2], fadebuffer[MAXVIDWIDTH];
			const float filterrow = max(min(srcrow - 0.5f, (float)(viewheight - 1)), 0.0f);
			const INT32 srcy = min((INT32)filterrow, viewheight - 2);
			const UINT32 *src = (const UINT32 *)(const void *)srcscr + (srcy * vid.width);
			UINT32 *dest = (UINT32 *)(void *)tmpscr + (y * vid.width);
			UINT32 *mixed = &rowbuffer[1];

			R_MixTruecolorRows(mixed, src, src + vid.width, (INT32)((filterrow - srcy) * 128.0f), viewwidth);
			mixed[-1] = mixed[0];
			mixed[viewwidth] = mixed[viewwidth - 1];

			// Nothing here goes off the ends of the row, give or take a rounding error.
			xfrac -= (FRACUNIT/2);
			if (xstep < 0)
				xfrac = min(xfrac, ((viewwidth - 1) << FRACBITS) + (FRACUNIT/2));
			else
				xfrac = max(xfrac, -(FRACUNIT/2));

			if (fade < 256)
			{
				R_ScaleTruecolorRow(fadebuffer, mixed, xfrac, xstep, viewwidth);
				for (x = 0; x < viewwidth; x++)
					dest[x] = TC_Mix(fadebuffer[x], dest[x], fade);
			}
			else
				R_ScaleTruecolorRow(dest, mixed, xfrac, xstep, viewwidth);
		}
		else
		{
			const INT32 srcy = max(min((INT32)srcrow, viewheight - 1), 0);
			const UINT8 *src = srcscr + (srcy * vid.width);
			UINT8 *dest = tmpscr + (y * vid.width);

			if (fade < 128)
				continue; // no colors to fade with, so leave the half nearest the other band

			for (x = 0; x < viewwidth; x++, xfrac += xstep)
				dest[x] = src[max(min(xfrac >> FRACBITS, viewwidth - 1), 0)];
		}
	}
}

// Writes a pixel for the view morph debug grid.
#define VIEWMORPH_PUT(pos, color) \
	do { \
		if (vid.bpp == 4) \
			((UINT32 *)(void *)tmpscr)[pos] = (color); \
		else \
			tmpscr[pos] = (UINT8)(color); \
	} while (0)

void R_ApplyViewMorph(void)
{
	UINT8 *tmpscr = screens[4];
	UINT8 *srcscr = screens[0];
	INT32 p, end = vid.width * vid.height;

	if (!viewmorph.use)
		return;

	if (cv_debug & DBG_VIEWMORPH)
	{
		UINT32 border = 32;
		UINT32 grid = 160;
		INT32 ws = vid.width / 4;
		INT32 hs = vid.width * (vid.height / 4);

		memcpy(tmpscr, srcscr, vid.width*vid.height*vid.bpp);
		if (vid.bpp == 4)
		{
			border = tc_palette[border];
			grid = tc_palette[grid];
		}
		for (p = 0; p < vid.width; p++)
		{
			VIEWMORPH_PUT(viewmorph.scrmap[p], border);
			VIEWMORPH_PUT(viewmorph.scrmap[p + hs], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[p + hs*2], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[p + hs*3], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[end - 1 - p], border);
		}
		for (p = vid.width; p < end; p += vid.width)
		{
			VIEWMORPH_PUT(viewmorph.scrmap[p], border);
			VIEWMORPH_PUT(viewmorph.scrmap[p + ws], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[p + ws*2], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[p + ws*3], grid);
			VIEWMORPH_PUT(viewmorph.scrmap[end - 1 - p], border);
		}
	}
	else if (vid.bpp == 4)
	{
		const UINT32 *src32 = (const UINT32 *)(const void *)srcscr;
		UINT32 *tmp32 = (UINT32 *)(void *)tmpscr;
		for (p = 0; p < end; p++)
			tmp32[p] = src32[viewmorph.scrmap[p]];
	}
	else
		for (p = 0; p < end; p++)
			tmpscr[p] = srcscr[viewmorph.scrmap[p]];

	VID_BlitLinearScreen(tmpscr, screens[0],
			vid.width*vid.bpp, vid.height, vid.width*vid.bpp, vid.rowbytes);
}

#undef VIEWMORPH_PUT


//
// R_SetViewSize
// Do not really change anything here,
// because it might be in the middle of a refresh.
// The change will take effect next refresh.
//
boolean setsizeneeded;

void R_SetViewSize(void)
{
	setsizeneeded = true;
}

//
// R_ExecuteSetViewSize
//
void R_ExecuteSetViewSize(void)
{
	INT32 i;
	INT32 j;
	INT32 level;
	INT32 startmapl;

	setsizeneeded = false;

	if (rendermode == render_none)
		return;

	// status bar overlay
	st_overlay = cv_showhud.value;

	scaledviewwidth = vid.width;
	viewheight = vid.height;

	if (splitscreen)
		viewheight >>= 1;

	viewwidth = scaledviewwidth;

	centerx = viewwidth/2;
	centery = viewheight/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	R_SetFov(cv_fov.value);

	R_InitViewBuffer(scaledviewwidth, viewheight);

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (INT16)viewheight;

	memset(scalelight, 0xFF, sizeof(scalelight));

	// Calculate the light levels to use for each level/scale combination.
	for (i = 0; i< LIGHTLEVELS; i++)
	{
		startmapl = ((LIGHTLEVELS - 1 - i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmapl - j*vid.width/(viewwidth)/DISTMAP;

			if (level < 0)
				level = 0;

			if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS - 1;

			scalelight[i][j] = colormaps + level*256;
		}
	}

	// continue to do the software setviewsize as long as we use the reference software view
#ifdef HWRENDER
	if (rendermode != render_soft)
		HWR_SetViewSize();
#endif

	am_recalc = true;
}

fixed_t R_GetPlayerFov(player_t *player)
{
	fixed_t fov = cv_fov.value + player->fovadd;
	return max(MINFOV*FRACUNIT, min(fov, MAXFOV*FRACUNIT));
}

static void R_SetFov(fixed_t playerfov)
{
	angle_t fov = FixedAngle(playerfov/2) + ANGLE_90;
	trueperspective.fov = playerfov;
	trueperspective.basefovtan = FixedMul(FINETANGENT(fov >> ANGLETOFINESHIFT), viewmorph.zoomneeded);
	fovtan = FixedMul(trueperspective.basefovtan, trueperspective.zoom);
	if (splitscreen == 1) // Splitscreen FOV should be adjusted to maintain expected vertical view
		fovtan = 17*fovtan/10;

	// this is only used for planes rendering in software mode
	INT32 j = viewheight*16;
	for (INT32 i = 0; i < j; i++)
	{
		// FixedDiv(centerx*FRACUNIT, FixedMul(abs(i - viewheight*8)<<FRACBITS, fovtan)),
		// but this is done every time the field of view changes, and doubles divide a lot quicker.
		const INT32 rows = abs(i - viewheight*8);
		const double dy = (double)rows * fovtan;
		if ((double)centerx * 4.0 >= dy)
			yslopetab[i] = INT32_MAX;
		else
			yslopetab[i] = (fixed_t)(((double)centerx * 4294967296.0) / dy);
	}

	projection = projectiony = FixedDiv(centerxfrac, fovtan);

	R_InitTextureMapping();

	// setup sky scaling
	R_SetSkyScale();
}

// How far above the horizon a row of the tilted camera's screen looks, as a slope.
// row is from the middle of the screen. Also gives how far away that row's part of
// the tilted screen is from the camera, going straight ahead (or straight back).
static float R_GetTruePerspectiveSlope(float row, float focal, float *depth)
{
	*depth = max(fabsf((focal * trueperspective.cosine) + (row * trueperspective.sine)), 0.001f);
	return ((focal * trueperspective.sine) - (row * trueperspective.cosine)) / *depth;
}

// Adds bands for the rows that are between these distances from the camera,
// starting from the furthest. Returns how many bands there would be now.
static INT32 R_AddTruePerspectiveBands(INT32 num, float neardepth, float fardepth, boolean back, float ratio, float focal)
{
	const float minspan = TRUEPERSPECTIVE_MINROWS * fabsf(trueperspective.sine);
	const float side = back ? -1.0f : 1.0f;
	float depth = fardepth;

	for (;;)
	{
		float next = depth / ratio;

		if (next - neardepth < minspan)
			next = neardepth;

		if (num < TRUEPERSPECTIVE_MAXBANDS)
		{
			trueperspectiveband_t *band = &trueperspective.bands[num];
			const float row1 = ((side * next) - (focal * trueperspective.cosine)) / trueperspective.sine;
			const float row2 = ((side * depth) - (focal * trueperspective.cosine)) / trueperspective.sine;

			band->facing = back ? 2 : 0;
			band->validtop = min(row1, row2);
			band->validbottom = max(row1, row2);
		}

		num++;

		if (next <= neardepth)
			break;

		depth = next;
	}

	return num;
}

// Works out the bands for the current aiming angle.
// This is needed before the view is set up, since it can turn it around.
void R_SetupTruePerspective(void)
{
	const float limit = 80.0f * (float)(M_PIl / 180.0); // G_SoftwareClipAimingPitch
	float focal, halfheight, angle, neardepth, low, high, ratio;
	INT32 i, num, front;

	trueperspective.use = (rendermode == render_soft && cv_trueperspective.value && aimingangle && !splitscreen && !viewmorph.use);

	if (!trueperspective.use || trueperspective.band > 0)
		return;

	focal = (viewwidth / 2.0f) / FIXED_TO_FLOAT(trueperspective.basefovtan);
	halfheight = viewheight / 2.0f;

	angle = (float)(INT32)aimingangle * (float)(M_PIl / 2147483648.0);
	angle = max(min(angle, limit), -limit);

	trueperspective.sine = sinf(angle);
	trueperspective.cosine = cosf(angle);

	if (fabsf(trueperspective.sine) < 0.001f)
	{
		// As good as level
		trueperspective.bands[0].facing = 0;
		trueperspective.bands[0].validtop = -halfheight;
		trueperspective.bands[0].validbottom = halfheight;
		trueperspective.bands[0].row = 0;
		trueperspective.bands[0].endrow = viewheight;
		trueperspective.numbands = 1;
		return;
	}

	// The field of view has to stay under 180 degrees.
	neardepth = focal / min(TRUEPERSPECTIVE_MAXZOOM, 100.0f / FIXED_TO_FLOAT(trueperspective.basefovtan));

	// How far away the top and bottom rows are
	low = (focal * trueperspective.cosine) - (halfheight * fabsf(trueperspective.sine));
	high = (focal * trueperspective.cosine) + (halfheight * fabsf(trueperspective.sine));

	for (ratio = TRUEPERSPECTIVE_BANDRATIO;; ratio *= 1.25f)
	{
		num = 0;

		// Bands go in order of distance, counting behind the camera as negative.
		if (-low - neardepth >= 4.0f * fabsf(trueperspective.sine))
			num = R_AddTruePerspectiveBands(num, neardepth, -low, true, ratio, focal);

		front = num;
		num = R_AddTruePerspectiveBands(num, max(low, neardepth), high, false, ratio, focal);

		if (num <= min(cv_trueperspectivebands.value, TRUEPERSPECTIVE_MAXBANDS))
			break;
	}

	// The ones in front were added furthest first.
	for (i = 0; i < (num - front) / 2; i++)
	{
		const trueperspectiveband_t swap = trueperspective.bands[front + i];
		trueperspective.bands[front + i] = trueperspective.bands[num - 1 - i];
		trueperspective.bands[num - 1 - i] = swap;
	}

	// That is from the top of the screen down if looking up. Otherwise, it's upside down.
	if (trueperspective.sine < 0.0f)
	{
		for (i = 0; i < num / 2; i++)
		{
			const trueperspectiveband_t swap = trueperspective.bands[i];
			trueperspective.bands[i] = trueperspective.bands[num - 1 - i];
			trueperspective.bands[num - 1 - i] = swap;
		}
	}

	// Rows between two bands can't be shown by either. Both fill them in with
	// their nearest row, and the second one is faded in over the first.
	for (i = 0; i < num; i++)
	{
		trueperspectiveband_t *band = &trueperspective.bands[i];

		band->validrow = (INT32)(band->validtop + halfheight + 0.999f);
		band->validrow = max(min(band->validrow, viewheight), 0);

		band->row = 0;
		if (i > 0)
		{
			band->row = (INT32)(trueperspective.bands[i - 1].validbottom + halfheight + 0.999f);
			band->row = max(min(band->row, band->validrow), 0);
		}

		band->endrow = viewheight;
		if (i < num - 1)
		{
			band->endrow = (INT32)(trueperspective.bands[i + 1].validtop + halfheight + 0.999f);
			band->endrow = max(min(band->endrow, viewheight), band->validrow);
		}
	}

	// The rows that are too near straight up or down for any of those
	{
		const float row1 = ((-neardepth - (focal * trueperspective.cosine)) / trueperspective.sine) + halfheight;
		const float row2 = ((neardepth - (focal * trueperspective.cosine)) / trueperspective.sine) + halfheight;
		const INT32 row = max((INT32)min(row1, row2), 0);
		const INT32 endrow = min((INT32)(max(row1, row2) + 0.999f), viewheight);

		if (endrow > row)
		{
			// Looking along the rows, the middle of the screen is as hard to reach as it was
			// before. So first a wide view for the part near the middle, then one for the rest.
			for (i = 0; i < 4; i++, num++)
			{
				trueperspectiveband_t *band = &trueperspective.bands[num];
				band->facing = (i & 1) ? 3 : 1;
				band->sidenear = viewwidth / ((i < 2) ? 72.0f : 12.0f);
				band->sidefar = viewwidth / ((i < 2) ? 8.0f : 2.0f);
				band->validtop = row - halfheight;
				band->validbottom = endrow - halfheight;
				band->row = band->validrow = row;
				band->endrow = endrow;
			}
		}
	}

	trueperspective.numbands = num;
}

// How far the band being rendered is turned away from where the camera is facing.
angle_t R_GetTruePerspectiveTurn(void)
{
	if (!trueperspective.use)
		return 0;

	// Angles go anticlockwise, so turning right is going backwards.
	return (angle_t)(0 - trueperspective.bands[trueperspective.band].facing) * ANGLE_90;
}

// Returns how far to shear the view for the current aiming angle.
// With true perspective, this is for the band being rendered.
fixed_t R_GetAimingShear(void)
{
	fixed_t zoom = FRACUNIT;
	float top = 0.0f, bottom = 0.0f;

	if (trueperspective.use)
	{
		const trueperspectiveband_t *band = &trueperspective.bands[trueperspective.band];
		const float focal = (viewwidth / 2.0f) / FIXED_TO_FLOAT(trueperspective.basefovtan);
		float topdepth, bottomdepth, needed;

		if (band->facing & 1)
		{
			// Facing along the rows, from the side of the screen in to where this view starts
			const float height1 = (focal * trueperspective.sine) - (band->validtop * trueperspective.cosine);
			const float height2 = (focal * trueperspective.sine) - (band->validbottom * trueperspective.cosine);
			const float nearest = band->sidenear, furthest = band->sidefar;

			top = max(max(height1, height2) / nearest, max(height1, height2) / furthest);
			bottom = min(min(height1, height2) / nearest, min(height1, height2) / furthest);

			needed = (focal * (top - bottom)) / (viewheight - 2);
		}
		else
		{
			top = R_GetTruePerspectiveSlope(band->validtop, focal, &topdepth);
			bottom = R_GetTruePerspectiveSlope(band->validbottom, focal, &bottomdepth);

			needed = focal / min(topdepth, bottomdepth); // rows are never stretched
			needed = max(needed, (focal * fabsf(top - bottom)) / (viewheight - 2)); // and all of them fit
		}

		needed = max(min(needed, 100.0f / FIXED_TO_FLOAT(trueperspective.basefovtan)), 1.0f);

		// A little spare, and not a new value for every tiny change
		zoom = (fixed_t)(needed * 1.01f * FRACUNIT) | 0x3FF;
	}

	if (zoom != trueperspective.zoom)
	{
		trueperspective.zoom = zoom;
		R_SetFov(trueperspective.fov);
	}

	if (!trueperspective.use)
		return FixedMul(AIMINGTODY(aimingangle), FixedDiv(viewwidth<<FRACBITS, BASEVIDWIDTH<<FRACBITS));

	// Put what the band sees in the middle of the sheared view.
	{
		const trueperspectiveband_t *band = &trueperspective.bands[trueperspective.band];
		const float focal = (viewwidth / 2.0f) / FIXED_TO_FLOAT(trueperspective.basefovtan);
		const float srcfocal = FIXED_TO_FLOAT(projection);
		const float halfwidth = viewwidth / 2.0f;
		float shear = srcfocal * (top + bottom) / 2.0f;
		INT32 x;

		shear = max(min(shear, viewheight * 7.0f), viewheight * -7.0f);
		trueperspective.horizon = (viewheight / 2.0f) + shear;

		if (band->facing & 1)
		{
			// These rows are all about as far ahead as each other, which is
			// to say they're in a narrow strip down the middle of a side view.
			const float depth1 = fabsf((focal * trueperspective.cosine) + (band->validtop * trueperspective.sine));
			const float depth2 = fabsf((focal * trueperspective.cosine) + (band->validbottom * trueperspective.sine));
			const float strip = ((srcfocal * max(depth1, depth2)) / band->sidenear) + 3.0f;
			const INT32 cliptop = max((INT32)(trueperspective.horizon - (srcfocal * top)) - 2, 0);
			const INT32 clipbottom = min((INT32)(trueperspective.horizon - (srcfocal * bottom)) + 3, viewheight);

			trueperspective.clipleft = max((INT32)(halfwidth - strip), 0);
			trueperspective.clipright = min((INT32)(halfwidth + strip) + 1, viewwidth);

			for (x = 0; x < viewwidth; x++)
			{
				trueperspective.cliptop[x] = (INT16)cliptop;
				trueperspective.clipbottom[x] = (INT16)clipbottom;
			}
		}
		else
		{
			// The further away a row is, the less of its width is used. So every column
			// is used from the nearest row to as far as rows are still wide enough to reach it.
			float neardepth, fardepth, nearslope, farslope;

			nearslope = R_GetTruePerspectiveSlope(band->validtop, focal, &neardepth);
			farslope = R_GetTruePerspectiveSlope(band->validbottom, focal, &fardepth);

			if (neardepth > fardepth)
			{
				const float swapdepth = neardepth, swapslope = nearslope;
				neardepth = fardepth, nearslope = farslope;
				fardepth = swapdepth, farslope = swapslope;
			}

			trueperspective.clipleft = 0;
			trueperspective.clipright = viewwidth;

			for (x = 0; x < viewwidth; x++)
			{
				const float column = max(fabsf((x + 0.5f) - halfwidth) - 2.0f, 1.0f);
				// how far along the band rows stop being this wide
				const float reach = min(max((((srcfocal * halfwidth) / column) - neardepth) / (fardepth - neardepth + 0.001f), 0.0f), 1.0f);
				// slopes don't go evenly with distance, but one over them does
				const float reachdepth = neardepth + ((fardepth - neardepth) * reach);
				const float height = (nearslope * neardepth) + (((farslope * fardepth) - (nearslope * neardepth)) * reach);
				const float reachrow = trueperspective.horizon - (srcfocal * (height / reachdepth));
				const float nearrow = trueperspective.horizon - (srcfocal * nearslope);

				trueperspective.cliptop[x] = (INT16)max((INT32)min(nearrow, reachrow) - 3, 0);
				trueperspective.clipbottom[x] = (INT16)min((INT32)max(nearrow, reachrow) + 4, viewheight);
			}
		}

		return (fixed_t)(shear * FRACUNIT);
	}
}

//
// R_Init
//

void R_Init(void)
{
	// screensize independent
	//I_OutputMsg("\nR_InitData");
	R_InitData();

	R_SetViewSize(); // setsizeneeded is set true

	// this is now done by SCR_Recalc() at the first mode set
	//I_OutputMsg("\nR_InitLightTables");
	R_InitLightTables();

	//I_OutputMsg("\nR_InitTranslucencyTables\n");
	R_InitTranslucencyTables();

	R_InitDrawNodes();

	framecount = 0;
}

//
// R_IsPointInSector
//
boolean R_IsPointInSector(sector_t *sector, fixed_t x, fixed_t y)
{
	size_t i;
	size_t passes = 0;

	for (i = 0; i < sector->linecount; i++)
	{
		line_t *line = sector->lines[i];
		vertex_t *v1, *v2;

		if (line->frontsector == line->backsector)
			continue;

		v1 = line->v1;
		v2 = line->v2;

		// make sure v1 is below v2
		if (v1->y > v2->y)
		{
			vertex_t *tmp = v1;
			v1 = v2;
			v2 = tmp;
		}
		else if (v1->y == v2->y)
			// horizontal line, we can't match this
			continue;

		if (v1->y < y && y <= v2->y)
		{
			// if the y axis in inside the line, find the point where we intersect on the x axis...
			fixed_t vx = v1->x + (INT64)(v2->x - v1->x) * (y - v1->y) / (v2->y - v1->y);

			// ...and if that point is to the left of the point, count it as inside.
			if (vx < x)
				passes++;
		}
	}

	// and odd number of passes means we're inside the polygon.
	return passes % 2;
}

//
// R_PointInSubsector
//
subsector_t *R_PointInSubsector(fixed_t x, fixed_t y)
{
	size_t nodenum = numnodes-1;

	while (!(nodenum & NF_SUBSECTOR))
		nodenum = nodes[nodenum].children[R_PointOnSide(x, y, nodes+nodenum)];

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
// R_PointInSubsectorOrNull, same as above but returns 0 if not in subsector
//
subsector_t *R_PointInSubsectorOrNull(fixed_t x, fixed_t y)
{
	node_t *node;
	INT32 side, i;
	size_t nodenum;
	subsector_t *ret;
	seg_t *seg;

	// single subsector is a special case
	if (numnodes == 0)
		return subsectors;

	nodenum = numnodes - 1;

	while (!(nodenum & NF_SUBSECTOR))
	{
		node = &nodes[nodenum];
		side = R_PointOnSide(x, y, node);
		nodenum = node->children[side];
	}

	ret = &subsectors[nodenum & ~NF_SUBSECTOR];
	for (i = 0, seg = &segs[ret->firstline]; i < ret->numlines; i++, seg++)
	{
		if (seg->glseg)
			continue;

		//if (R_PointOnSegSide(x, y, seg)) -- breaks in ogl because polyvertex_t cast over vertex pointers
		if (P_PointOnLineSide(x, y, seg->linedef) != seg->side)
			return 0;
	}

	return ret;
}

//
// R_SetupFrame
//
void R_SetupFrame(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = R_ViewpointHasChasecam(player);
	boolean ispaused = paused || P_AutoPause();

	if (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer])
		thiscam = &camera2;
	else
		thiscam = &camera;

	newview->sky = false;

	if (player->awayviewtics)
	{
		// cut-away view stuff
		r_viewmobj = player->awayviewmobj; // should be a MT_ALTVIEWMAN
		I_Assert(r_viewmobj != NULL);
		newview->z = r_viewmobj->z + 20*FRACUNIT;
		newview->aim = player->awayviewaiming;
		newview->angle = r_viewmobj->angle;
	}
	else if (!player->spectator && chasecam)
	// use outside cam view
	{
		r_viewmobj = NULL;
		newview->z = thiscam->z + (thiscam->height>>1);
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;
	}
	else
	// use the player's eyes view
	{
		newview->z = player->viewz;

		r_viewmobj = player->mo;
		I_Assert(r_viewmobj != NULL);

		newview->aim = player->aiming;
		newview->angle = r_viewmobj->angle;

		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle; // WARNING: camera uses this
				newview->aim = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;
			}
		}
	}

	if (trueperspective.band > 0)
	{
		// the same view as the first band
	}
	else if (quake.time && !ispaused)
	{
		fixed_t ir = quake.intensity>>1;

		if (quake.epicenter) {
			// Calculate 3D distance from epicenter, using the camera.
			fixed_t xydist, dist;
			if (P_MobjWasRemoved(r_viewmobj)) {
				xydist = R_PointToDist2(thiscam->x, thiscam->y, quake.epicenter->x, quake.epicenter->y);
				dist = R_PointToDist2(0, thiscam->z, xydist, quake.epicenter->z);
			} else {
				xydist = R_PointToDist2(r_viewmobj->x, r_viewmobj->y, quake.epicenter->x, quake.epicenter->y);
				dist = R_PointToDist2(0, r_viewmobj->z, xydist, quake.epicenter->z);
			}

			// More effect closer to epicenter, outside of radius = no effect
			if (!quake.radius || dist > quake.radius)
				ir = 0;
			else
				ir = FixedMul(ir, FRACUNIT - FixedDiv(dist, quake.radius));
		}

		quake.x = M_RandomRange(-ir,ir);
		quake.y = M_RandomRange(-ir,ir);
		quake.z = M_RandomRange(-ir,ir);
	}
	else if (!ispaused)
		quake.x = quake.y = quake.z = 0;

	newview->z += quake.z;

	newview->player = player;

	if (chasecam && !player->awayviewtics && !player->spectator)
	{
		newview->x = thiscam->x;
		newview->y = thiscam->y;
		newview->x += quake.x;
		newview->y += quake.y;

		if (thiscam->subsector)
			newview->sector = thiscam->subsector->sector;
		else
			newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;
	}
	else
	{
		newview->x = r_viewmobj->x;
		newview->y = r_viewmobj->y;
		newview->x += quake.x;
		newview->y += quake.y;

		if (!P_MobjWasRemoved(r_viewmobj) && r_viewmobj->subsector)
			newview->sector = r_viewmobj->subsector->sector;
		else
			newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;
	}

	// newview->sin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	// newview->cos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
}

void R_SkyboxFrame(player_t *player)
{
	camera_t *thiscam;

	if (splitscreen && player == &players[secondarydisplayplayer]
	&& player != &players[consoleplayer])
	{
		thiscam = &camera2;
		R_SetViewContext(VIEWCONTEXT_SKY2);
	}
	else
	{
		thiscam = &camera;
		R_SetViewContext(VIEWCONTEXT_SKY1);
	}

	// cut-away view stuff
	newview->sky = true;
	r_viewmobj = skyboxmo[0];
#ifdef PARANOIA
	if (!r_viewmobj)
	{
		const size_t playeri = (size_t)(player - players);
		I_Error("R_SkyboxFrame: r_viewmobj null (player %s)", sizeu1(playeri));
	}
#endif
	if (player->awayviewtics)
	{
		newview->aim = player->awayviewaiming;
		newview->angle = player->awayviewmobj->angle;
	}
	else if (thiscam->chase)
	{
		newview->aim = thiscam->aiming;
		newview->angle = thiscam->angle;
	}
	else
	{
		newview->aim = player->aiming;
		newview->angle = player->mo->angle;
		if (!demoplayback && player->playerstate != PST_DEAD)
		{
			if (player == &players[consoleplayer])
			{
				newview->angle = localangle; // WARNING: camera uses this
				newview->aim = localaiming;
			}
			else if (player == &players[secondarydisplayplayer])
			{
				newview->angle = localangle2;
				newview->aim = localaiming2;
			}
		}
	}
	newview->angle += r_viewmobj->angle;

	newview->player = player;

	newview->x = r_viewmobj->x;
	newview->y = r_viewmobj->y;
	newview->z = r_viewmobj->z; // 26/04/17: use actual Z position instead of spawnpoint angle!

	if (mapheaderinfo[gamemap-1])
	{
		mapheader_t *mh = mapheaderinfo[gamemap-1];
		vector3_t campos = {0,0,0}; // Position of player's actual view point

		if (player->awayviewtics) {
			campos.x = player->awayviewmobj->x;
			campos.y = player->awayviewmobj->y;
			campos.z = player->awayviewmobj->z + 20*FRACUNIT;
		} else if (thiscam->chase) {
			campos.x = thiscam->x;
			campos.y = thiscam->y;
			campos.z = thiscam->z + (thiscam->height>>1);
		} else {
			campos.x = player->mo->x;
			campos.y = player->mo->y;
			campos.z = player->viewz;
		}

		// Earthquake effects should be scaled in the skybox
		// (if an axis isn't used, the skybox won't shake in that direction)
		campos.x += quake.x;
		campos.y += quake.y;
		campos.z += quake.z;

		if (skyboxmo[1]) // Is there a viewpoint?
		{
			fixed_t x = 0, y = 0;
			if (mh->skybox_scalex > 0)
				x = (campos.x - skyboxmo[1]->x) / mh->skybox_scalex;
			else if (mh->skybox_scalex < 0)
				x = (campos.x - skyboxmo[1]->x) * -mh->skybox_scalex;

			if (mh->skybox_scaley > 0)
				y = (campos.y - skyboxmo[1]->y) / mh->skybox_scaley;
			else if (mh->skybox_scaley < 0)
				y = (campos.y - skyboxmo[1]->y) * -mh->skybox_scaley;

			if (r_viewmobj->angle == 0)
			{
				newview->x += x;
				newview->y += y;
			}
			else if (r_viewmobj->angle == ANGLE_90)
			{
				newview->x -= y;
				newview->y += x;
			}
			else if (r_viewmobj->angle == ANGLE_180)
			{
				newview->x -= x;
				newview->y -= y;
			}
			else if (r_viewmobj->angle == ANGLE_270)
			{
				newview->x += y;
				newview->y -= x;
			}
			else
			{
				angle_t ang = r_viewmobj->angle>>ANGLETOFINESHIFT;
				newview->x += FixedMul(x,FINECOSINE(ang)) - FixedMul(y,  FINESINE(ang));
				newview->y += FixedMul(x,  FINESINE(ang)) + FixedMul(y,FINECOSINE(ang));
			}
		}
		if (mh->skybox_scalez > 0)
			newview->z += campos.z / mh->skybox_scalez;
		else if (mh->skybox_scalez < 0)
			newview->z += campos.z * -mh->skybox_scalez;
	}

	if (!P_MobjWasRemoved(r_viewmobj) && r_viewmobj->subsector)
		newview->sector = r_viewmobj->subsector->sector;
	else
		newview->sector = R_PointInSubsector(newview->x, newview->y)->sector;

	// newview->sin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	// newview->cos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	R_InterpolateView(R_UsingFrameInterpolation() ? rendertimefrac : FRACUNIT);
}

boolean R_ViewpointHasChasecam(player_t *player)
{
	camera_t *thiscam;
	boolean chasecam = false;
	boolean isplayer2 = (splitscreen && player == &players[secondarydisplayplayer] && player != &players[consoleplayer]);

	if (isplayer2)
	{
		thiscam = &camera2;
		chasecam = (cv_chasecam2.value != 0);
	}
	else
	{
		thiscam = &camera;
		chasecam = (cv_chasecam.value != 0);
	}

	if (player->climbing || (player->powers[pw_carry] == CR_NIGHTSMODE) || player->playerstate == PST_DEAD || gamestate == GS_TITLESCREEN || tutorialmode)
		chasecam = true; // force chasecam on
	else if (player->spectator) // no spectator chasecam
		chasecam = false; // force chasecam off

	if (chasecam && !thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = true;
	}
	else if (!chasecam && thiscam->chase)
	{
		P_ResetCamera(player, thiscam);
		thiscam->chase = false;
	}

	if (isplayer2)
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER2);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(2);
			thiscam->reset = false;
		}
	}
	else
	{
		R_SetViewContext(VIEWCONTEXT_PLAYER1);
		if (thiscam->reset)
		{
			R_ResetViewInterpolation(1);
			thiscam->reset = false;
		}
	}

	return chasecam;
}

boolean R_IsViewpointThirdPerson(player_t *player, boolean skybox)
{
	boolean chasecam = R_ViewpointHasChasecam(player);

	// cut-away view stuff
	if (player->awayviewtics || skybox)
		return chasecam;
	// use outside cam view
	else if (!player->spectator && chasecam)
		return true;

	// use the player's eyes view
	return false;
}

static void R_PortalFrame(portal_t *portal)
{
	viewx = portal->viewx;
	viewy = portal->viewy;
	viewz = portal->viewz;

	viewangle = portal->viewangle;
	viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
	viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);

	if (!P_MobjWasRemoved(portal->viewmobj))
		r_viewmobj = portal->viewmobj;

	portalclipstart = portal->start;
	portalclipend = portal->end;

	if (portal->clipline != -1)
	{
		portalclipline = &lines[portal->clipline];
		portalcullsector = portalclipline->frontsector;
		viewsector = portalclipline->frontsector;
	}
	else
	{
		portalclipline = NULL;
		portalcullsector = NULL;
		viewsector = R_PointInSubsector(viewx, viewy)->sector;
	}
}

static void Mask_Pre (maskcount_t* m)
{
	m->drawsegs[0] = ds_p - drawsegs;
	m->vissprites[0] = visspritecount;
	m->viewx = viewx;
	m->viewy = viewy;
	m->viewz = viewz;
	m->viewsector = viewsector;
}

static void Mask_Post (maskcount_t* m)
{
	m->drawsegs[1] = ds_p - drawsegs;
	m->vissprites[1] = visspritecount;
}

// ================
// R_RenderView
// ================

//                     FAB NOTE FOR WIN32 PORT !! I'm not finished already,
// but I suspect network may have problems with the video buffer being locked
// for all duration of rendering, and being released only once at the end..
// I mean, there is a win16lock() or something that lasts all the rendering,
// so maybe we should release screen lock before each netupdate below..?

static fixed_t viewfov[2];

static void R_RenderView(player_t *player);

void R_RenderPlayerView(player_t *player)
{
	trueperspective.band = 0;
	trueperspective.numbands = 1;

	// With true perspective, this is once for every band.
	do
	{
		R_RenderView(player);

		if (trueperspective.use)
			R_ApplyTruePerspective();
	}
	while (trueperspective.use && ++trueperspective.band < trueperspective.numbands);

	if (trueperspective.use)
	{
		VID_BlitLinearScreen(screens[4], screens[0],
				vid.width*vid.bpp, viewheight, vid.width*vid.bpp, vid.rowbytes);

		// Leave the view facing forwards for anything that asks afterwards.
		trueperspective.band = trueperspective.numbands - 1;
		if (R_GetTruePerspectiveTurn())
		{
			viewangle -= R_GetTruePerspectiveTurn();
			viewsin = FINESINE(viewangle>>ANGLETOFINESHIFT);
			viewcos = FINECOSINE(viewangle>>ANGLETOFINESHIFT);
		}
	}

	trueperspective.band = 0;
}

static void R_RenderView(player_t *player)
{
	INT32			nummasks	= 1;
	maskcount_t*	masks		= malloc(sizeof(maskcount_t));

	if (cv_homremoval.value && player == &players[displayplayer]) // if this is display player 1
	{
		if (cv_homremoval.value == 1)
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 31); // No HOM effect!
		else //'development' HOM removal -- makes it blindingly obvious if HOM is spotted.
			V_DrawFill(0, 0, BASEVIDWIDTH, BASEVIDHEIGHT, 32+(timeinmap&15));
	}

	fixed_t fov = R_GetPlayerFov(player);

	if (player == &players[displayplayer] && viewfov[0] != fov)
	{
		viewfov[0] = fov;
		R_SetFov(fov);
	}
	else if (player == &players[secondarydisplayplayer] && viewfov[1] != fov)
	{
		viewfov[1] = fov;
		R_SetFov(fov);
	}

	R_SetupFrame(player);
	framecount++;
	validcount++;

	// Clear buffers.
	R_ClearPlanes();
	if (viewmorph.use)
	{
		portalclipstart = viewmorph.x1;
		portalclipend = viewwidth-viewmorph.x1-1;
		R_PortalClearClipSegs(portalclipstart, portalclipend);
		memcpy(ceilingclip, viewmorph.ceilingclip, sizeof(INT16)*vid.width);
		memcpy(floorclip, viewmorph.floorclip, sizeof(INT16)*vid.width);
	}
	else
	{
		portalclipstart = 0;
		portalclipend = viewwidth;
		R_ClearClipSegs();
	}
	if (trueperspective.use)
	{
		// Only the part that the band is warped from has to be drawn.
		INT32 i;

		if (trueperspective.clipleft > 0 || trueperspective.clipright < viewwidth)
		{
			portalclipstart = trueperspective.clipleft;
			portalclipend = trueperspective.clipright;
			R_PortalClearClipSegs(portalclipstart, portalclipend);
		}

		for (i = 0; i < viewwidth; i++)
		{
			ceilingclip[i] = (INT16)(trueperspective.cliptop[i] - 1);
			floorclip[i] = trueperspective.clipbottom[i];
		}
	}
	R_ClearDrawSegs();
	R_ClearSegTables();
	R_ClearSprites();
	Portal_InitList();

	// check for new console commands.
	NetUpdate();

	// The head node is the last node output.

	Mask_Pre(&masks[nummasks - 1]);
	curdrawsegs = ds_p;
	ps_numbspcalls.value.i = ps_numpolyobjects.value.i = ps_numdrawnodes.value.i = 0;
	PS_START_TIMING(ps_bsptime);
	R_RenderBSPNode((INT32)numnodes - 1);
	PS_STOP_TIMING(ps_bsptime);
	Mask_Post(&masks[nummasks - 1]);

	PS_START_TIMING(ps_sw_spritecliptime);
	R_ClipSprites(drawsegs, NULL);
	PS_STOP_TIMING(ps_sw_spritecliptime);

	ps_numsprites.value.i = numvisiblesprites;

	// Add portals caused by visplanes.
	Portal_AddPlanePortals(cv_skybox.value);

	// Portal rendering. Hijacks the BSP traversal.
	PS_START_TIMING(ps_sw_portaltime);
	if (portal_base)
	{
		portal_t *portal;

		for(portal = portal_base; portal; portal = portal_base)
		{
			portalrender = portal->pass; // Recursiveness depth.

			R_ClearFFloorClips();

			// Apply the viewpoint stored for the portal.
			R_PortalFrame(portal);

			// Hack in the clipsegs to delimit the starting
			// clipping for sprites and possibly other similar
			// future items.
			R_PortalClearClipSegs(portal->start, portal->end);

			// Hack in the top/bottom clip values for the window
			// that were previously stored.
			Portal_ClipApply(portal);

			validcount++;

			masks = realloc(masks, (++nummasks)*sizeof(maskcount_t));

			Mask_Pre(&masks[nummasks - 1]);
			curdrawsegs = ds_p;

			if (portal->is_horizon)
			{
				// If the portal is a plane or a horizon portal, then we just render a horizon line
				R_RenderPortalHorizonLine(portal->horizon_sector);
			}
			else
			{
				// Render the BSP from the new viewpoint, and clip
				// any sprites with the new clipsegs and window.
				R_RenderBSPNode((INT32)numnodes - 1);
			}

			// Don't add skybox portals while already rendering a skybox view, because that'll cause an infinite loop
			Portal_AddPlanePortals(cv_skybox.value && !portal->is_skybox);

			Mask_Post(&masks[nummasks - 1]);

			R_ClipSprites(ds_p - (masks[nummasks - 1].drawsegs[1] - masks[nummasks - 1].drawsegs[0]), portal);

			Portal_Remove(portal);
		}
	}
	PS_STOP_TIMING(ps_sw_portaltime);

	PS_START_TIMING(ps_sw_planetime);
	R_DrawPlanes();
	PS_STOP_TIMING(ps_sw_planetime);

	// draw mid texture and sprite
	// And now 3D floors/sides!
	PS_START_TIMING(ps_sw_maskedtime);
	R_DrawMasked(masks, nummasks);
	PS_STOP_TIMING(ps_sw_maskedtime);

	free(masks);
}

// =========================================================================
//                    ENGINE COMMANDS & VARS
// =========================================================================

void R_RegisterEngineStuff(void)
{
	// Do nothing for dedicated server
	if (dedicated)
		return;

	CV_RegisterVar(&cv_homremoval);
	CV_RegisterVar(&cv_translucency);
	CV_RegisterVar(&cv_drawdist);
	CV_RegisterVar(&cv_drawdist_nights);
	CV_RegisterVar(&cv_drawdist_precip);
	CV_RegisterVar(&cv_fovchange);
	CV_RegisterVar(&cv_fov);
	CV_RegisterVar(&cv_trueperspective);
	CV_RegisterVar(&cv_trueperspectivebands);

	CV_RegisterVar(&cv_chasecam);
	CV_RegisterVar(&cv_chasecam2);

	CV_RegisterVar(&cv_shadow);
	CV_RegisterVar(&cv_skybox);
	CV_RegisterVar(&cv_renderview);
	CV_RegisterVar(&cv_renderhitboxinterpolation);
	CV_RegisterVar(&cv_renderhitboxgldepth);
	CV_RegisterVar(&cv_renderhitbox);
	CV_RegisterVar(&cv_renderwalls);
	CV_RegisterVar(&cv_renderfloors);
	CV_RegisterVar(&cv_renderthings);
	CV_RegisterVar(&cv_ffloorclip);
	CV_RegisterVar(&cv_spriteclip);

	CV_RegisterVar(&cv_cam_dist);
	CV_RegisterVar(&cv_cam_still);
	CV_RegisterVar(&cv_cam_height);
	CV_RegisterVar(&cv_cam_speed);
	CV_RegisterVar(&cv_cam_rotate);
	CV_RegisterVar(&cv_cam_rotspeed);
	CV_RegisterVar(&cv_cam_turnmultiplier);
	CV_RegisterVar(&cv_cam_orbit);
	CV_RegisterVar(&cv_cam_adjust);

	CV_RegisterVar(&cv_cam2_dist);
	CV_RegisterVar(&cv_cam2_still);
	CV_RegisterVar(&cv_cam2_height);
	CV_RegisterVar(&cv_cam2_speed);
	CV_RegisterVar(&cv_cam2_rotate);
	CV_RegisterVar(&cv_cam2_rotspeed);
	CV_RegisterVar(&cv_cam2_turnmultiplier);
	CV_RegisterVar(&cv_cam2_orbit);
	CV_RegisterVar(&cv_cam2_adjust);

	CV_RegisterVar(&cv_cam_savedist[0][0]);
	CV_RegisterVar(&cv_cam_savedist[0][1]);
	CV_RegisterVar(&cv_cam_savedist[1][0]);
	CV_RegisterVar(&cv_cam_savedist[1][1]);

	CV_RegisterVar(&cv_cam_saveheight[0][0]);
	CV_RegisterVar(&cv_cam_saveheight[0][1]);
	CV_RegisterVar(&cv_cam_saveheight[1][0]);
	CV_RegisterVar(&cv_cam_saveheight[1][1]);

	CV_RegisterVar(&cv_showhud);
	CV_RegisterVar(&cv_translucenthud);

	CV_RegisterVar(&cv_maxportals);

	// Frame interpolation/uncapped
	CV_RegisterVar(&cv_fpscap);
}

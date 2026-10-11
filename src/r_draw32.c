// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 1998-2000 by DooM Legacy Team.
// Copyright (C) 1999-2024 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  r_draw32.c
/// \brief 32bpp span/column drawer functions
/// \note  no includes because this is included as part of r_draw.c

// ==========================================================================
// COLUMNS
// ==========================================================================

// A column is a vertical slice/span of a wall texture that uses
// a has a constant z depth from top to bottom.
//

/**	\brief The R_DrawColumn_32 function
	Experiment to make software go faster. Taken from the Boom source
*/
void R_DrawColumn_32(void)
{
	INT32 count;
	UINT32 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		register INT32 heightmask = dc_texheight-1;
		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				*dest = colormap[source[frac>>FRACBITS]];
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = colormap[source[(frac>>FRACBITS) & heightmask]];
		}
	}
}

/**	\brief The R_DrawColumnClamped_32 function
	Same as R_DrawColumn_32, but prevents artifacts from showing up (caused by fixed-point imprecisions)
*/
void R_DrawColumnClamped_32(void)
{
	INT32 count;
	UINT32 *dest;
	fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const UINT8 *source = dc_source;
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		INT32 heightmask = dc_texheight-1;
		INT32 idx;
		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS) & heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = colormap[source[idx]];
			}
		}
	}
}

void R_Draw2sMultiPatchColumn_32(void)
{
	INT32 count;
	UINT32 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];

				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
			}
		}
	}
}

void R_Draw2sMultiPatchTranslucentColumn_32(void)
{
	INT32 count;
	UINT32 *dest;
	register fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;

	if (count < 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		return;
#endif

	// Framebuffer destination address.
	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	count++;

	// Determine scaling, which is the only mapping to be done.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		const UINT32 blend = R_GetTruecolorBlend(dc_transmap);
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		register INT32 heightmask = dc_texheight-1;
		register UINT8 val;
		if (dc_texheight & heightmask)   // not a power of 2 -- killough
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) <  0);
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				val = source[frac>>FRACBITS];

				if (val != TRANSPARENTPIXEL)
					*dest = TC_Blend(colormap[val], *dest, blend);

				dest += vid.width;

				// Avoid overflow.
				if (fracstep > 0x7FFFFFFF - frac)
					frac += fracstep - heightmask;
				else
					frac += fracstep;

				while (frac >= heightmask)
					frac -= heightmask;
			} while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = TC_Blend(colormap[val], *dest, blend);
				dest += vid.width;
				frac += fracstep;
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = TC_Blend(colormap[val], *dest, blend);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				val = source[(frac>>FRACBITS) & heightmask];
				if (val != TRANSPARENTPIXEL)
					*dest = TC_Blend(colormap[val], *dest, blend);
			}
		}
	}
}


/**	\brief The R_DrawTranslucentColumn_32 function
	I've made an asm routine for the transparency, because it slows down
	a lot in 640x480 with big sprites (bfg on all screen, or transparent
	walls on fullscreen)
*/
void R_DrawTranslucentColumn_32(void)
{
	register INT32 count;
	UINT32 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumn_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register const UINT8 *source = dc_source;
		const UINT32 blend = R_GetTruecolorBlend(dc_transmap);
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		register INT32 heightmask = dc_texheight - 1;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				// using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				*dest = TC_Blend(colormap[source[frac>>FRACBITS]], *dest, blend);
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = TC_Blend(colormap[source[(frac>>FRACBITS)&heightmask]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
				*dest = TC_Blend(colormap[source[(frac>>FRACBITS)&heightmask]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = TC_Blend(colormap[source[(frac>>FRACBITS)&heightmask]], *dest, blend);
		}
	}
}

/**	\brief The R_DrawTranslucentColumnClamped_32 function
	Same as R_DrawTranslucentColumn_32, but prevents artifacts from showing up (caused by fixed-point imprecisions)
*/
void R_DrawTranslucentColumnClamped_32(void)
{
	INT32 count;
	UINT32 *dest;
	fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslucentColumnClamped_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		const UINT8 *source = dc_source;
		const UINT32 blend = R_GetTruecolorBlend(dc_transmap);
		const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
		INT32 heightmask = dc_texheight - 1;
		INT32 idx;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				// using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix
				idx = frac>>FRACBITS;
				if (idx >= 0 && idx < dc_postlength)
					*dest = TC_Blend(colormap[source[idx]], *dest, blend);
				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = TC_Blend(colormap[source[idx]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = TC_Blend(colormap[source[idx]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
			{
				idx = (frac>>FRACBITS)&heightmask;
				if (idx >= 0 && idx < dc_postlength)
					*dest = TC_Blend(colormap[source[idx]], *dest, blend);
			}
		}
	}
}


/**	\brief The R_DrawTranslatedTranslucentColumn_32 function
	Spiffy function. Not only does it colormap a sprite, but does translucency as well.
	Uber-kudos to Cyan Helkaraxe
*/
void R_DrawTranslatedTranslucentColumn_32(void)
{
	const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
	const UINT32 blend = R_GetTruecolorBlend(dc_transmap);
	register INT32 count;
	UINT32 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Inner loop that does the actual texture mapping, e.g. a DDA-like scaling.
	// This is as fast as it gets.
	{
		register INT32 heightmask = dc_texheight - 1;
		if (dc_texheight & heightmask)
		{
			heightmask++;
			heightmask <<= FRACBITS;

			if (frac < 0)
				while ((frac += heightmask) < 0)
					;
			else
				while (frac >= heightmask)
					frac -= heightmask;

			do
			{
				// Re-map color indices from wall texture column
				//  using a lighting/special effects LUT.
				// heightmask is the Tutti-Frutti fix

				*dest = TC_Blend(colormap[dc_translation[dc_source[frac>>FRACBITS]]], *dest, blend);

				dest += vid.width;
				if ((frac += fracstep) >= heightmask)
					frac -= heightmask;
			}
			while (--count);
		}
		else
		{
			while ((count -= 2) >= 0) // texture height is a power of 2
			{
				*dest = TC_Blend(colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
				*dest = TC_Blend(colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]], *dest, blend);
				dest += vid.width;
				frac += fracstep;
			}
			if (count & 1)
				*dest = TC_Blend(colormap[dc_translation[dc_source[(frac>>FRACBITS)&heightmask]]], *dest, blend);
		}
	}
}

/**	\brief The R_DrawTranslatedColumn_32 function
	Draw columns up to 128 high but remap the green ramp to other colors

  \warning STILL NOT IN ASM, TO DO..
*/
void R_DrawTranslatedColumn_32(void)
{
	const UINT32 *colormap = R_GetTruecolorColormap(dc_colormap);
	register INT32 count;
	UINT32 *dest;
	register fixed_t frac, fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawTranslatedColumn_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	// Here we do an additional index re-mapping.
	do
	{
		// Translation tables are used
		//  to map certain colorramps to other ones,
		//  used with PLAY sprites.
		// Thus the "green" ramp of the player 0 sprite
		//  is mapped to gray, red, black/indigo.
		*dest = colormap[dc_translation[dc_source[frac>>FRACBITS]]];

		dest += vid.width;

		frac += fracstep;
	} while (count--);
}

// ==========================================================================
// SPANS
// ==========================================================================

#define SPANSIZE 16
#define INVSPAN 0.0625f

/**	\brief The R_DrawSpan_32 function
	Draws the actual span.
*/
void R_DrawSpan_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	if (dest+8 > deststop)
		return;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[1] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[2] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[3] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[4] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[5] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[6] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest[7] = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		*dest++ = colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]];
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedSpan_32 function
	Draw slopes! Holy sheit!
*/
void R_DrawTiltedSpan_32(void)
{
	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);

		*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
				*dest = colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedTranslucentSpan_32 function
	Like DrawTiltedSpan, but translucent
*/
void R_DrawTiltedTranslucentSpan_32(void)
{
	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
		*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dest, blend);
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dest, blend);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dest, blend);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
				*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dest, blend);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawTiltedWaterSpan_32 function
	Like DrawTiltedTranslucentSpan, but for water
*/
void R_DrawTiltedWaterSpan_32(void)
{
	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	UINT32 *dsrc;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	dsrc = (UINT32 *)screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
		*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dsrc++, blend);
		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dsrc++, blend);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dsrc++, blend);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
				*dest = TC_Blend(colormap[source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)]], *dsrc++, blend);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedSplat_32(void)
{
	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];

		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = colormap[val];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val != TRANSPARENTPIXEL)
					*dest = colormap[val];
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

void R_DrawTiltedTranslucentSplat_32(void)
{
	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;

	UINT8 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	R_CalcSlopeLight();

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = ds_source;
	//colormap = ds_colormap;

#if 0	// The "perfect" reference version of this routine. Pretty slow.
		// Use it only to see how things are supposed to look.
	i = 0;
	do
	{
		double z = 1.f/iz;
		u = (INT64)(uz*z);
		v = (INT64)(vz*z);

		colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);

		val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = TC_Blend(colormap[val], *dest, blend);

		dest++;
		iz += ds_sz.x;
		uz += ds_su.x;
		vz += ds_sv.x;
	} while (--width >= 0);
#else
	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = TC_Blend(colormap[val], *dest, blend);
			dest++;
			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val != TRANSPARENTPIXEL)
				*dest = TC_Blend(colormap[val], *dest, blend);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val != TRANSPARENTPIXEL)
					*dest = TC_Blend(colormap[val], *dest, blend);
				dest++;
				u += stepu;
				v += stepv;
			}
		}
	}
#endif
}

/**	\brief The R_DrawSplat_32 function
	Just like R_DrawSpan_32, but skips transparent pixels.
*/
void R_DrawSplat_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		//
		// <Callum> 4194303 = (2048x2048)-1 (2048x2048 is maximum flat size)
		// Why decimal? 0x3FFFFF == 4194303... ~Golden
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[0] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[1] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[2] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[3] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[4] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[5] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[6] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val &= 0x3FFFFF;
		val = source[val];
		if (val != TRANSPARENTPIXEL)
			dest[7] = colormap[val];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = colormap[val];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentSplat_32 function
	Just like R_DrawSplat_32, but is translucent!
*/
void R_DrawTranslucentSplat_32 (void)
{
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[0] = TC_Blend(colormap[val], dest[0], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[1] = TC_Blend(colormap[val], dest[1], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[2] = TC_Blend(colormap[val], dest[2], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[3] = TC_Blend(colormap[val], dest[3], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[4] = TC_Blend(colormap[val], dest[4], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[5] = TC_Blend(colormap[val], dest[5], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[6] = TC_Blend(colormap[val], dest[6], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			dest[7] = TC_Blend(colormap[val], dest[7], blend);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val != TRANSPARENTPIXEL)
			*dest = TC_Blend(colormap[val], *dest, blend);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawFloorSprite_32 function
	Just like R_DrawSplat_32, but for floor sprites.
*/
void R_DrawFloorSprite_32 (void)
{
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	const UINT32 *colormap;
	UINT8 *translation;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	translation = ds_translation;
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[0] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[1] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[2] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[3] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[4] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[5] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[6] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		val = source[val];
		if (val & 0xFF00)
			dest[7] = colormap[translation[val & 0xFF]];
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			*dest = colormap[translation[val & 0xFF]];
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTranslucentFloorSplat_32 function
	Just like R_DrawFloorSprite_32, but is translucent!
*/
void R_DrawTranslucentFloorSprite_32 (void)
{
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT16 *source;
	const UINT32 *colormap;
	UINT8 *translation;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = (UINT16 *)ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	translation = ds_translation;
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[0] = TC_Blend(colormap[translation[val & 0xFF]], dest[0], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[1] = TC_Blend(colormap[translation[val & 0xFF]], dest[1], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[2] = TC_Blend(colormap[translation[val & 0xFF]], dest[2], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[3] = TC_Blend(colormap[translation[val & 0xFF]], dest[3], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[4] = TC_Blend(colormap[translation[val & 0xFF]], dest[4], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[5] = TC_Blend(colormap[translation[val & 0xFF]], dest[5], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[6] = TC_Blend(colormap[translation[val & 0xFF]], dest[6], blend);
		xposition += xstep;
		yposition += ystep;

		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			dest[7] = TC_Blend(colormap[translation[val & 0xFF]], dest[7], blend);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)];
		if (val & 0xFF00)
			*dest = TC_Blend(colormap[translation[val & 0xFF]], *dest, blend);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

/**	\brief The R_DrawTiltedFloorSprite_32 function
	Draws a tilted floor sprite.
*/
void R_DrawTiltedFloorSprite_32(void)
{
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	const UINT32 *colormap;
	UINT8 *translation;
	UINT32 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = colormap[translation[val & 0xFF]];
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val & 0xFF00)
					*dest = colormap[translation[val & 0xFF]];
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTiltedTranslucentFloorSprite_32 function
	Draws a tilted, translucent, floor sprite.
*/
void R_DrawTiltedTranslucentFloorSprite_32(void)
{
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	// x1, x2 = ds_x1, ds_x2
	int width = ds_x2 - ds_x1;
	double iz, uz, vz;
	UINT32 u, v;
	int i;

	UINT16 *source;
	const UINT32 *colormap;
	UINT8 *translation;
	UINT32 *dest;
	UINT16 val;

	double startz, startu, startv;
	double izstep, uzstep, vzstep;
	double endz, endu, endv;
	UINT32 stepu, stepv;

	iz = ds_sz.z + ds_sz.y*(centery-ds_y) + ds_sz.x*(ds_x1-centerx);
	uz = ds_su.z + ds_su.y*(centery-ds_y) + ds_su.x*(ds_x1-centerx);
	vz = ds_sv.z + ds_sv.y*(centery-ds_y) + ds_sv.x*(ds_x1-centerx);

	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	source = (UINT16 *)ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	translation = ds_translation;

	startz = 1.f/iz;
	startu = uz*startz;
	startv = vz*startz;

	izstep = ds_sz.x * SPANSIZE;
	uzstep = ds_su.x * SPANSIZE;
	vzstep = ds_sv.x * SPANSIZE;
	//x1 = 0;
	width++;

	while (width >= SPANSIZE)
	{
		iz += izstep;
		uz += uzstep;
		vz += vzstep;

		endz = 1.f/iz;
		endu = uz*endz;
		endv = vz*endz;
		stepu = (INT64)((endu - startu) * INVSPAN);
		stepv = (INT64)((endv - startv) * INVSPAN);
		u = (INT64)(startu);
		v = (INT64)(startv);

		for (i = SPANSIZE-1; i >= 0; i--)
		{
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = TC_Blend(colormap[translation[val & 0xFF]], *dest, blend);
			dest++;

			u += stepu;
			v += stepv;
		}
		startu = endu;
		startv = endv;
		width -= SPANSIZE;
	}
	if (width > 0)
	{
		if (width == 1)
		{
			u = (INT64)(startu);
			v = (INT64)(startv);
			val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
			if (val & 0xFF00)
				*dest = TC_Blend(colormap[translation[val & 0xFF]], *dest, blend);
		}
		else
		{
			double left = width;
			iz += ds_sz.x * left;
			uz += ds_su.x * left;
			vz += ds_sv.x * left;

			endz = 1.f/iz;
			endu = uz*endz;
			endv = vz*endz;
			left = 1.f/left;
			stepu = (INT64)((endu - startu) * left);
			stepv = (INT64)((endv - startv) * left);
			u = (INT64)(startu);
			v = (INT64)(startv);

			for (; width != 0; width--)
			{
				val = source[((v >> nflatyshift) & nflatmask) | (u >> nflatxshift)];
				if (val & 0xFF00)
					*dest = TC_Blend(colormap[translation[val & 0xFF]], *dest, blend);
				dest++;

				u += stepu;
				v += stepv;
			}
		}
	}
}

/**	\brief The R_DrawTranslucentSpan_32 function
	Draws the actual span with translucency.
*/
void R_DrawTranslucentSpan_32 (void)
{
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	fixed_t xposition;
	fixed_t yposition;
	fixed_t xstep, ystep;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	size_t count = (ds_x2 - ds_x1 + 1);
	UINT32 val;

	xposition = ds_xfrac; yposition = ds_yfrac;
	xstep = ds_xstep; ystep = ds_ystep;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition <<= nflatshiftup; yposition <<= nflatshiftup;
	xstep <<= nflatshiftup; ystep <<= nflatshiftup;

	source = ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[0], blend);
		xposition += xstep;
		yposition += ystep;

		dest[1] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[1], blend);
		xposition += xstep;
		yposition += ystep;

		dest[2] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[2], blend);
		xposition += xstep;
		yposition += ystep;

		dest[3] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[3], blend);
		xposition += xstep;
		yposition += ystep;

		dest[4] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[4], blend);
		xposition += xstep;
		yposition += ystep;

		dest[5] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[5], blend);
		xposition += xstep;
		yposition += ystep;

		dest[6] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[6], blend);
		xposition += xstep;
		yposition += ystep;

		dest[7] = TC_Blend(colormap[source[(((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift)]], dest[7], blend);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count-- && dest <= deststop)
	{
		val = (((UINT32)yposition >> nflatyshift) & nflatmask) | ((UINT32)xposition >> nflatxshift);
		*dest = TC_Blend(colormap[source[val]], *dest, blend);
		dest++;
		xposition += xstep;
		yposition += ystep;
	}
}

void R_DrawWaterSpan_32(void)
{
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	UINT32 xposition;
	UINT32 yposition;
	UINT32 xstep, ystep;

	UINT8 *source;
	const UINT32 *colormap;
	UINT32 *dest;
	UINT32 *dsrc;

	size_t count;

	// SoM: we only need 6 bits for the integer part (0 thru 63) so the rest
	// can be used for the fraction part. This allows calculation of the memory address in the
	// texture with two shifts, an OR and one AND. (see below)
	// for texture sizes > 64 the amount of precision we can allow will decrease, but only by one
	// bit per power of two (obviously)
	// Ok, because I was able to eliminate the variable spot below, this function is now FASTER
	// than the original span renderer. Whodathunkit?
	xposition = ds_xfrac << nflatshiftup; yposition = (ds_yfrac + ds_waterofs) << nflatshiftup;
	xstep = ds_xstep << nflatshiftup; ystep = ds_ystep << nflatshiftup;

	source = ds_source;
	colormap = R_GetTruecolorColormap(ds_colormap);
	dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	dsrc = (UINT32 *)screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;
	count = ds_x2 - ds_x1 + 1;

	while (count >= 8)
	{
		// SoM: Why didn't I see this earlier? the spot variable is a waste now because we don't
		// have the uber complicated math to calculate it now, so that was a memory write we didn't
		// need!
		dest[0] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[1] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[2] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[3] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[4] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[5] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[6] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest[7] = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;

		dest += 8;
		count -= 8;
	}
	while (count--)
	{
		*dest++ = TC_Blend(colormap[source[((yposition >> nflatyshift) & nflatmask) | (xposition >> nflatxshift)]], *dsrc++, blend);
		xposition += xstep;
		yposition += ystep;
	}
}

// ==========================================================================
// The drawers below work on what's already in the frame buffer, or don't
// have a texture, so they can't be the 8bpp ones with different tables.
// ==========================================================================

/**	\brief The R_DrawShadeColumn_32 function
	Darkens the screen, using the column as light levels.
*/
void R_DrawShadeColumn_32(void)
{
	register INT32 count;
	UINT32 *dest;
	register fixed_t frac, fracstep;
	tcshade_t shade;
	INT32 level, lastlevel = -1;

	// check out coords for src*
	if ((dc_yl < 0) || (dc_x >= vid.width))
		return;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawShadeColumn_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Looks familiar.
	fracstep = dc_iscale;
	frac = dc_texturemid + FixedMul((dc_yl << FRACBITS) - centeryfrac, fracstep);

	do
	{
		level = min(dc_source[frac>>FRACBITS], NUMCOLORMAPS-1);
		if (level != lastlevel)
		{
			R_GetTruecolorShade(colormaps + (level << 8), &shade);
			lastlevel = level;
		}

		*dest = R_ShadeTruecolorPixel(&shade, *dest);
		dest += vid.width;
		frac += fracstep;
	} while (count--);
}

/**	\brief The R_DrawDropShadowColumn_32 function
	See R_DrawDropShadowColumn_8.
*/
void R_DrawDropShadowColumn_32(void)
{
	register INT32 count;
	UINT32 *dest;

	count = dc_yh - dc_yl + 1;

	if (count <= 0) // Zero length, column does not exceed a pixel.
		return;

	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	{
#define DSCOLOR 31 // palette index for the color of the shadow
		const UINT32 color = R_GetTruecolorColormap(dc_colormap)[DSCOLOR];
#undef DSCOLOR
		const UINT32 blend = R_GetTruecolorBlend(dc_transmap);

		while (count--)
		{
			*dest = TC_Blend(color, *dest, blend);
			dest += vid.width;
		}
	}
}

/**	\brief The R_DrawFogColumn_32 function
	Fog wall.
*/
void R_DrawFogColumn_32(void)
{
	INT32 count;
	UINT32 *dest;
	tcshade_t shade;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawFogColumn_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// Framebuffer destination address.
	dest = &((UINT32 *)topleft)[dc_yl*vid.width + dc_x];

	// Apply the light table to what's already on the screen.
	R_GetTruecolorShade(dc_colormap, &shade);

	do
	{
		*dest = R_ShadeTruecolorPixel(&shade, *dest);
		dest += vid.width;
	} while (count--);
}

/**	\brief The R_DrawFogSpan_32 function
	Draws the actual span with fogging.
*/
void R_DrawFogSpan_32(void)
{
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	size_t count = ds_x2 - ds_x1 + 1;
	tcshade_t shade;

	R_GetTruecolorShade(ds_colormap, &shade);

	while (count--)
	{
		*dest = R_ShadeTruecolorPixel(&shade, *dest);
		dest++;
	}
}

/**	\brief The R_DrawTiltedFogSpan_32 function
	Draws a tilted span with fogging.
*/
void R_DrawTiltedFogSpan_32(void)
{
	int width = ds_x2 - ds_x1;

	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	const lighttable_t *colormap, *lastcolormap = NULL;
	tcshade_t shade;

	R_CalcSlopeLight();

	do
	{
		colormap = planezlight[tiltlighting[ds_x1++]] + (ds_colormap - colormaps);
		if (colormap != lastcolormap)
		{
			R_GetTruecolorShade(colormap, &shade);
			lastcolormap = colormap;
		}

		*dest = R_ShadeTruecolorPixel(&shade, *dest);
		dest++;
	} while (--width >= 0);
}

/**	\brief The R_DrawSolidColorSpan_32 function
	Draws a solid color span.
*/
void R_DrawSolidColorSpan_32(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT32 source = R_GetTruecolorColormap(ds_colormap)[ds_source[0]];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	while (count--)
		*dest++ = source;
}

/**	\brief The R_DrawTransSolidColorSpan_32 function
	Draws a translucent solid color span.
*/
void R_DrawTransSolidColorSpan_32(void)
{
	size_t count = (ds_x2 - ds_x1 + 1);

	UINT32 source = R_GetTruecolorColormap(ds_colormap)[ds_source[0]];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = TC_Blend(source, *dest, blend);
		dest++;
	}
}

/**	\brief The R_DrawTiltedSolidColorSpan_32 function
	Draws a tilted solid color span.
*/
void R_DrawTiltedSolidColorSpan_32(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);

	R_CalcSlopeLight();

	do
	{
		const UINT32 *colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
		*dest++ = colormap[source];
	} while (--width >= 0);
}

/**	\brief The R_DrawTiltedTransSolidColorSpan_32 function
	Draws a tilted and translucent solid color span.
*/
void R_DrawTiltedTransSolidColorSpan_32(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];

	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);

	R_CalcSlopeLight();

	do
	{
		const UINT32 *colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
		*dest = TC_Blend(colormap[source], *dest, blend);
		dest++;
	} while (--width >= 0);
}

/**	\brief The R_DrawWaterSolidColorSpan_32 function
	Draws a water solid color span.
*/
void R_DrawWaterSolidColorSpan_32(void)
{
	UINT32 source = R_GetTruecolorColormap(ds_colormap)[ds_source[0]];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	UINT32 *dsrc = (UINT32 *)screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	size_t count = (ds_x2 - ds_x1 + 1);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);
	const UINT32 *deststop = (UINT32 *)screens[0] + vid.width * vid.height;

	while (count-- && dest <= deststop)
	{
		*dest = TC_Blend(source, *dsrc++, blend);
		dest++;
	}
}

/**	\brief The R_DrawTiltedWaterSolidColorSpan_32 function
	Draws a tilted water solid color span.
*/
void R_DrawTiltedWaterSolidColorSpan_32(void)
{
	int width = ds_x2 - ds_x1;

	UINT8 source = ds_source[0];
	UINT32 *dest = &((UINT32 *)topleft)[ds_y*vid.width + ds_x1];
	UINT32 *dsrc = (UINT32 *)screens[1] + (ds_y+ds_bgofs)*vid.width + ds_x1;

	const UINT32 *lights = R_GetTruecolorColormap(ds_colormap);
	const UINT32 blend = R_GetTruecolorBlend(ds_transmap);

	R_CalcSlopeLight();

	do
	{
		const UINT32 *colormap = lights + (planezlight[tiltlighting[ds_x1++]] - colormaps);
		*dest++ = TC_Blend(colormap[source], *dsrc++, blend);
	} while (--width >= 0);
}

/**	\brief The R_DrawShadeColumn_32 function
	This is for 3D floors that cast shadows on walls.

	This function just cuts the column up into sections and calls R_DrawColumn_32
*/
void R_DrawColumnShadowed_32(void)
{
	INT32 count, realyh, i, height, bheight = 0, solid = 0;

	realyh = dc_yh;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

#ifdef RANGECHECK
	if ((unsigned)dc_x >= (unsigned)vid.width || dc_yl < 0 || dc_yh >= vid.height)
		I_Error("R_DrawColumnShadowed_32: %d to %d at %d", dc_yl, dc_yh, dc_x);
#endif

	// This runs through the lightlist from top to bottom and cuts up the column accordingly.
	for (i = 0; i < dc_numlights; i++)
	{
		// If the height of the light is above the column, get the colormap
		// anyway because the lighting of the top should be affected.
		solid = dc_lightlist[i].flags & FOF_CUTSOLIDS;

		height = dc_lightlist[i].height >> LIGHTSCALESHIFT;
		if (solid)
		{
			bheight = dc_lightlist[i].botheight >> LIGHTSCALESHIFT;
			if (bheight < height)
			{
				// confounded slopes sometimes allow partial invertedness,
				// even including cases where the top and bottom heights
				// should actually be the same!
				// swap the height values as a workaround for this quirk
				INT32 temp = height;
				height = bheight;
				bheight = temp;
			}
		}
		if (height <= dc_yl)
		{
			dc_colormap = dc_lightlist[i].rcolormap;
			if (solid && dc_yl < bheight)
				dc_yl = bheight;
			continue;
		}
		// Found a break in the column!
		dc_yh = height;

		if (dc_yh > realyh)
			dc_yh = realyh;
		(colfuncs[BASEDRAWFUNC])();		// R_DrawColumn_32 for the appropriate architecture
		if (solid)
			dc_yl = bheight;
		else
			dc_yl = dc_yh + 1;

		dc_colormap = dc_lightlist[i].rcolormap;
	}
	dc_yh = realyh;
	if (dc_yl <= realyh)
		(colfuncs[BASEDRAWFUNC])();		// R_DrawWallColumn_32 for the appropriate architecture
}

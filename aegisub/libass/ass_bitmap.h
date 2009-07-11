/*
 * Copyright (C) 2006 Evgeniy Stepanov <eugeni.stepanov@gmail.com>
 *
 * This file is part of libass.
 *
 * libass is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libass is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libass; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LIBASS_BITMAP_H
#define LIBASS_BITMAP_H

#include <ft2build.h>
#include FT_GLYPH_H

typedef struct ass_synth_priv_s ass_synth_priv_t;

ass_synth_priv_t *ass_synth_init(double);
void ass_synth_done(ass_synth_priv_t *priv);

typedef struct bitmap_s {
    int left, top;
    int w, h;                   // width, height
    unsigned char *buffer;      // w x h buffer
} bitmap_t;

/**
 * \brief perform glyph rendering
 * \param glyph original glyph
 * \param outline_glyph "border" glyph, produced from original by FreeType's glyph stroker
 * \param bm_g out: pointer to the bitmap of original glyph is returned here
 * \param bm_o out: pointer to the bitmap of outline (border) glyph is returned here
 * \param bm_g out: pointer to the bitmap of glyph shadow is returned here
 * \param be 1 = produces blurred bitmaps, 0 = normal bitmaps
 */
int glyph_to_bitmap(ass_synth_priv_t *priv_blur, FT_Glyph glyph,
                    FT_Glyph outline_glyph, bitmap_t **bm_g,
                    bitmap_t **bm_o, bitmap_t **bm_s, int be,
                    double blur_radius, FT_Vector shadow_offset);

void ass_free_bitmap(bitmap_t *bm);

#endif                          /* LIBASS_BITMAP_H */

/* Xviewer -- Affine Transformations
 *
 * Copyright (C) 2003-2009 The Free Software Foundation
 *
 * Portions based on code from libart_lgpl by Raph Levien.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include <cairo/cairo.h>

#include "xviewer-transform.h"
#include "xviewer-jobs.h"

/* The number of progress updates per transformation */
#define XVIEWER_TRANSFORM_N_PROG_UPDATES 20

struct _XviewerTransformPrivate {
	cairo_matrix_t affine;
};

typedef struct {
	gdouble x;
	gdouble y;
} XviewerPoint;

/* Convert degrees into radians */
#define XVIEWER_DEG_TO_RAD(degree) ((degree) * (G_PI/180.0)) 

G_DEFINE_TYPE_WITH_PRIVATE (XviewerTransform, xviewer_transform, G_TYPE_OBJECT)

static void
xviewer_transform_init (XviewerTransform *trans)
{
	trans->priv = xviewer_transform_get_instance_private (trans);
}

static void
xviewer_transform_class_init (XviewerTransformClass *klass)
{

}

/**
 * xviewer_transform_apply:
 * @trans: a #XviewerTransform
 * @pixbuf: a #GdkPixbuf
 * @job: a #XviewerJob
 *
 * Applies the transformation in @trans to @pixbuf, setting its progress in @job.
 *
 * Returns: (transfer full): A new #GdkPixbuf with the transformation applied.
 **/
GdkPixbuf*
xviewer_transform_apply (XviewerTransform *trans, GdkPixbuf *pixbuf, XviewerJob *job)
{
	XviewerPoint dest_top_left;
	XviewerPoint dest_bottom_right;
	XviewerPoint vertices[4] = { {0, 0}, {1, 0}, {1, 1}, {0, 1} };
	double r_det;
	int inverted [6];
	XviewerPoint dest;

	int src_width;
	int src_height;
	int src_rowstride;
	int src_n_channels;
	guchar *src_buffer;

	GdkPixbuf *dest_pixbuf;
	int dest_width;
	int dest_height;
	int dest_rowstride;
	int dest_n_channels;
	guchar *dest_buffer;

	guchar *src_pos;
	guchar *dest_pos;
	int dx, dy, sx, sy;
	int i, x, y;

	int progress_delta;

	g_return_val_if_fail (pixbuf != NULL, NULL);

	g_object_ref (pixbuf);

	src_width = gdk_pixbuf_get_width (pixbuf);
	src_height = gdk_pixbuf_get_height (pixbuf);
	src_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	src_n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	src_buffer = gdk_pixbuf_get_pixels (pixbuf);

	/* find out the dimension of the destination pixbuf */
	dest_top_left.x = 100000;
	dest_top_left.y = 100000;
	dest_bottom_right.x = -100000;
	dest_bottom_right.y = -100000;

	for (i = 0; i < 4; i++) {
		dest.x = vertices[i].x * (src_width - 1);
		dest.y = vertices[i].y * (src_height -1);

		cairo_matrix_transform_point (&trans->priv->affine,
					      &dest.x, &dest.y);

		dest_top_left.x = MIN (dest_top_left.x, dest.x);
		dest_top_left.y = MIN (dest_top_left.y, dest.y);

		dest_bottom_right.x = MAX (dest_bottom_right.x, dest.x);
		dest_bottom_right.y = MAX (dest_bottom_right.y, dest.y);
	}

	/* create the resulting pixbuf */
	dest_width = abs (dest_bottom_right.x - dest_top_left.x + 1);
	dest_height = abs (dest_bottom_right.y - dest_top_left.y + 1);

	dest_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
			       gdk_pixbuf_get_has_alpha (pixbuf),
			       gdk_pixbuf_get_bits_per_sample (pixbuf),
			       dest_width,
			       dest_height);
	dest_rowstride = gdk_pixbuf_get_rowstride (dest_pixbuf);
	dest_n_channels = gdk_pixbuf_get_n_channels (dest_pixbuf);
	dest_buffer = gdk_pixbuf_get_pixels (dest_pixbuf);

	/* invert the matrix so that we can compute the source pixel
	   from the target pixel and convert the values to integer
	   ones (faster!)  FIXME: Maybe we can do some more
	   improvements by using special mmx/3dnow features if
	   available.
	*/
	r_det = 1.0 / (trans->priv->affine.xx * trans->priv->affine.yy - trans->priv->affine.yx * trans->priv->affine.xy);
	inverted[0] =  trans->priv->affine.yy * r_det;
	inverted[1] = -trans->priv->affine.yx * r_det;
	inverted[2] = -trans->priv->affine.xy * r_det;
	inverted[3] =  trans->priv->affine.xx * r_det;
	inverted[4] = -trans->priv->affine.x0 * inverted[0] - trans->priv->affine.y0 * inverted[2];
	inverted[5] = -trans->priv->affine.x0 * inverted[1] - trans->priv->affine.y0 * inverted[3];

	progress_delta = MAX (1, dest_height / XVIEWER_TRANSFORM_N_PROG_UPDATES);

	/*
	 * for every destination pixel (dx,dy) compute the source pixel (sx, sy)
	 * and copy the color values
	 */
	for (y = 0, dy = dest_top_left.y; y < dest_height; y++, dy++) {
		for (x = 0, dx = dest_top_left.x; x < dest_width; x++, dx++) {

			sx = dx * inverted[0] + dy * inverted[2] + inverted[4];
			sy = dx * inverted[1] + dy * inverted[3] + inverted[5];

			if (sx >= 0 && sx < src_width && sy >= 0 && sy < src_height) {
				src_pos  = src_buffer  + sy * src_rowstride  + sx * src_n_channels;
				dest_pos = dest_buffer +  y * dest_rowstride +  x * dest_n_channels;

				for (i = 0; i <  src_n_channels; i++) {
					dest_pos[i] = src_pos[i];
				}
			}
		}

		if (job != NULL && y % progress_delta == 0) {
			gfloat progress;

			progress = (gfloat) (y + 1.0) / (gfloat) dest_height;

			xviewer_job_set_progress (job, progress);
		}
	}

	g_object_unref (pixbuf);

	if (job != NULL) {
		xviewer_job_set_progress (job, 1.0);
	}

	return dest_pixbuf;
}

static void
_xviewer_cairo_matrix_copy (const cairo_matrix_t *src, cairo_matrix_t *dest)
{
	cairo_matrix_init (dest, src->xx, src->yx, src->xy, src->yy, src->x0, src->y0);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)
/* art_affine_equal modified to work with cairo_matrix_t */
static gboolean
_xviewer_cairo_matrix_equal (const cairo_matrix_t *a, const cairo_matrix_t *b)
{
	return (DOUBLE_EQUAL (a->xx, b->xx) && DOUBLE_EQUAL (a->yx, b->yx) &&
		DOUBLE_EQUAL (a->xy, b->xy) && DOUBLE_EQUAL (a->yy, b->yy) &&
		DOUBLE_EQUAL (a->x0, b->x0) && DOUBLE_EQUAL (a->y0, b->y0) );
}

/* art_affine_flip modified to work with cairo_matrix_t */
static void
_xviewer_cairo_matrix_flip (cairo_matrix_t *dst, const cairo_matrix_t *src, gboolean horiz, gboolean vert)
{
	dst->xx = horiz ? -src->xx : src->xx;
	dst->yx = horiz ? -src->yx : src->yx;
	dst->xy = vert ? -src->xy : src->xy;
	dst->yy = vert ? -src->yy : src->yy;
	dst->x0 = horiz ? -src->x0 : src->x0;
	dst->y0 = vert ? -src->y0 : src->y0;
}

/**
 * xviewer_transform_reverse:
 * @trans: a #XviewerTransform
 *
 * Creates the reverse transformation of @trans
 *
 * Returns: (transfer full): a new transformation
 **/
XviewerTransform*
xviewer_transform_reverse (XviewerTransform *trans)
{
	XviewerTransform *reverse;

	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (trans), NULL);

	reverse = XVIEWER_TRANSFORM (g_object_new (XVIEWER_TYPE_TRANSFORM, NULL));

	_xviewer_cairo_matrix_copy (&trans->priv->affine, &reverse->priv->affine);

	g_return_val_if_fail (cairo_matrix_invert (&reverse->priv->affine) == CAIRO_STATUS_SUCCESS, reverse);

	return reverse;
}

/**
 * xviewer_transform_compose:
 * @trans: a #XviewerTransform
 * @compose: another #XviewerTransform
 *
 * 
 *
 * Returns: (transfer full): a new transform
 **/
XviewerTransform*
xviewer_transform_compose (XviewerTransform *trans, XviewerTransform *compose)
{
	XviewerTransform *composition;

	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (trans), NULL);
	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (compose), NULL);

	composition = XVIEWER_TRANSFORM (g_object_new (XVIEWER_TYPE_TRANSFORM, NULL));

	cairo_matrix_multiply (&composition->priv->affine,
			       &trans->priv->affine,
			       &compose->priv->affine);

	return composition;
}

gboolean
xviewer_transform_is_identity (XviewerTransform *trans)
{
	static const cairo_matrix_t identity = { 1, 0, 0, 1, 0, 0 };

	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (trans), FALSE);

	return _xviewer_cairo_matrix_equal (&identity, &trans->priv->affine);
}

XviewerTransform*
xviewer_transform_identity_new (void)
{
	XviewerTransform *trans;

	trans = XVIEWER_TRANSFORM (g_object_new (XVIEWER_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_identity (&trans->priv->affine);

	return trans;
}

XviewerTransform*
xviewer_transform_rotate_new (int degree)
{
	XviewerTransform *trans;

	trans = XVIEWER_TRANSFORM (g_object_new (XVIEWER_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_rotate (&trans->priv->affine, XVIEWER_DEG_TO_RAD(degree));

	return trans;
}

XviewerTransform*
xviewer_transform_flip_new   (XviewerTransformType type)
{
	XviewerTransform *trans;
	gboolean horiz, vert;

	trans = XVIEWER_TRANSFORM (g_object_new (XVIEWER_TYPE_TRANSFORM, NULL));

	cairo_matrix_init_identity (&trans->priv->affine);

	horiz = (type == XVIEWER_TRANSFORM_FLIP_HORIZONTAL);
	vert = (type == XVIEWER_TRANSFORM_FLIP_VERTICAL);

	_xviewer_cairo_matrix_flip (&trans->priv->affine,
				&trans->priv->affine,
				horiz, vert);

	return trans;
}

XviewerTransform*
xviewer_transform_new (XviewerTransformType type)
{
	XviewerTransform *trans = NULL;
	XviewerTransform *temp1 = NULL, *temp2 = NULL;

	switch (type) {
	case XVIEWER_TRANSFORM_NONE:
		trans = xviewer_transform_identity_new ();
		break;
	case XVIEWER_TRANSFORM_FLIP_HORIZONTAL:
		trans = xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_HORIZONTAL);
		break;
	case XVIEWER_TRANSFORM_ROT_180:
		trans = xviewer_transform_rotate_new (180);
		break;
	case XVIEWER_TRANSFORM_FLIP_VERTICAL:
		trans = xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_VERTICAL);
		break;
	case XVIEWER_TRANSFORM_TRANSPOSE:
		temp1 = xviewer_transform_rotate_new (90);
		temp2 = xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_HORIZONTAL);
		trans = xviewer_transform_compose (temp1, temp2);
		g_object_unref (temp1);
		g_object_unref (temp2);
		break;
	case XVIEWER_TRANSFORM_ROT_90:
		trans = xviewer_transform_rotate_new (90);
		break;
	case XVIEWER_TRANSFORM_TRANSVERSE:
		temp1 = xviewer_transform_rotate_new (90);
		temp2 = xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_VERTICAL);
		trans = xviewer_transform_compose (temp1, temp2);
		g_object_unref (temp1);
		g_object_unref (temp2);
		break;
	case XVIEWER_TRANSFORM_ROT_270:
		trans = xviewer_transform_rotate_new (270);
		break;
	default:
		trans = xviewer_transform_identity_new ();
		break;
	}

	return trans;
}

XviewerTransformType
xviewer_transform_get_transform_type (XviewerTransform *trans)
{
	cairo_matrix_t affine, a1, a2;
	XviewerTransformPrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (trans), XVIEWER_TRANSFORM_NONE);

	priv = trans->priv;

	cairo_matrix_init_rotate (&affine, XVIEWER_DEG_TO_RAD(90));
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_ROT_90;
	}

	cairo_matrix_init_rotate (&affine, XVIEWER_DEG_TO_RAD(180));
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_ROT_180;
	}

	cairo_matrix_init_rotate (&affine, XVIEWER_DEG_TO_RAD(270));
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_ROT_270;
	}

	cairo_matrix_init_identity (&affine);
	_xviewer_cairo_matrix_flip (&affine, &affine, TRUE, FALSE);
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_FLIP_HORIZONTAL;
	}

	cairo_matrix_init_identity (&affine);
	_xviewer_cairo_matrix_flip (&affine, &affine, FALSE, TRUE);
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_FLIP_VERTICAL;
	}

	cairo_matrix_init_rotate (&a1, XVIEWER_DEG_TO_RAD(90));
	cairo_matrix_init_identity (&a2);
	_xviewer_cairo_matrix_flip (&a2, &a2, TRUE, FALSE);
	cairo_matrix_multiply(&affine, &a1, &a2);
	if (_xviewer_cairo_matrix_equal (&affine, &priv->affine)) {
		return XVIEWER_TRANSFORM_TRANSPOSE;
	}

	/* A transversion is a 180° rotation followed by a transposition */
	/* Reuse the transposition from the previous step for this. */
	cairo_matrix_init_rotate (&a1, XVIEWER_DEG_TO_RAD(180));
	cairo_matrix_multiply(&a2, &a1, &affine);
	if (_xviewer_cairo_matrix_equal (&a2, &priv->affine)) {
		return XVIEWER_TRANSFORM_TRANSVERSE;
	}

	return XVIEWER_TRANSFORM_NONE;
}

gboolean
xviewer_transform_get_affine (XviewerTransform *trans, cairo_matrix_t *affine)
{
	g_return_val_if_fail (XVIEWER_IS_TRANSFORM (trans), FALSE);

	_xviewer_cairo_matrix_copy (&trans->priv->affine, affine);

	return TRUE;
}


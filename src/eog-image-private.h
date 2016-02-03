/* Eye Of Gnome - Image Private Data
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __EOG_IMAGE_PRIVATE_H__
#define __EOG_IMAGE_PRIVATE_H__

#include "eog-image.h"

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

G_BEGIN_DECLS

struct _EogImagePrivate {
	GFile            *file;

	EogImageStatus    status;
	EogImageStatus    prev_status;
	EogImageMetadataStatus metadata_status;

	gboolean          is_playing;
	GdkPixbufAnimation     *anim;
	GdkPixbufAnimationIter *anim_iter;
	GdkPixbuf        *image;
	GdkPixbuf        *thumbnail;
#ifdef HAVE_RSVG
	RsvgHandle       *svg;
#endif

	gint              width;
	gint              height;

	goffset           bytes;
	gchar            *file_type;
	gboolean          threadsafe_format;

	/* Holds EXIF raw data */
	guint             exif_chunk_len;
	guchar           *exif_chunk;

#if 0
	/* Holds IPTC raw data */
	guchar           *iptc_chunk;
	guint             iptc_chunk_len;
#endif

	gboolean          modified;
	gboolean          file_is_changed;

	gboolean          autorotate;
	gint              orientation;
#ifdef HAVE_EXIF
	ExifData         *exif;
#endif
#ifdef HAVE_EXEMPI
 	XmpPtr   xmp;
#endif

#ifdef HAVE_LCMS
	cmsHPROFILE       profile;
#endif

	gchar            *caption;

	gchar            *collate_key;

	GMutex            status_mutex;

	gboolean          cancel_loading;
	guint             data_ref_count;

	GSList           *undo_stack;

	EogTransform     *trans;
	EogTransform     *trans_autorotate;
};

G_END_DECLS

#endif /* __EOG_IMAGE_PRIVATE_H__ */

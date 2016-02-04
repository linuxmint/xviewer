/* Xviewer -- Metadata Reader Interface
 *
 * Copyright (C) 2008-2011 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xviewer-metadata-reader.h"
#include "xviewer-metadata-reader-jpg.h"
#include "xviewer-metadata-reader-png.h"
#include "xviewer-debug.h"

G_DEFINE_INTERFACE (XviewerMetadataReader, xviewer_metadata_reader, G_TYPE_INVALID)

XviewerMetadataReader*
xviewer_metadata_reader_new (XviewerMetadataFileType type)
{
	XviewerMetadataReader *emr;

	switch (type) {
	case XVIEWER_METADATA_JPEG:
		emr = XVIEWER_METADATA_READER (g_object_new (XVIEWER_TYPE_METADATA_READER_JPG, NULL));
		break;
	case XVIEWER_METADATA_PNG:
		emr = XVIEWER_METADATA_READER (g_object_new (XVIEWER_TYPE_METADATA_READER_PNG, NULL));
		break;
	default:
		emr = NULL;
		break;
	}

	return emr;
}

gboolean
xviewer_metadata_reader_finished (XviewerMetadataReader *emr)
{
	g_return_val_if_fail (XVIEWER_IS_METADATA_READER (emr), TRUE);

	return XVIEWER_METADATA_READER_GET_INTERFACE (emr)->finished (emr);
}


void
xviewer_metadata_reader_consume (XviewerMetadataReader *emr, const guchar *buf, guint len)
{
	XVIEWER_METADATA_READER_GET_INTERFACE (emr)->consume (emr, buf, len);
}

/* Returns the raw exif data. NOTE: The caller of this function becomes
 * the new owner of this piece of memory and is responsible for freeing it!
 */
void
xviewer_metadata_reader_get_exif_chunk (XviewerMetadataReader *emr, guchar **data, guint *len)
{
	g_return_if_fail (data != NULL && len != NULL);

	XVIEWER_METADATA_READER_GET_INTERFACE (emr)->get_raw_exif (emr, data, len);
}

#ifdef HAVE_EXIF
ExifData*
xviewer_metadata_reader_get_exif_data (XviewerMetadataReader *emr)
{
	return XVIEWER_METADATA_READER_GET_INTERFACE (emr)->get_exif_data (emr);
}
#endif

#ifdef HAVE_EXEMPI
XmpPtr
xviewer_metadata_reader_get_xmp_data (XviewerMetadataReader *emr)
{
	return XVIEWER_METADATA_READER_GET_INTERFACE (emr)->get_xmp_ptr (emr);
}
#endif

#ifdef HAVE_LCMS
cmsHPROFILE
xviewer_metadata_reader_get_icc_profile (XviewerMetadataReader *emr)
{
	return XVIEWER_METADATA_READER_GET_INTERFACE (emr)->get_icc_profile (emr);
}
#endif

/* Default vfunc that simply clears the output if not overriden by the
   implementing class. This mimics the old behaviour of get_exif_chunk(). */
static void
_xviewer_metadata_reader_default_get_raw_exif (XviewerMetadataReader *emr,
					   guchar **data, guint *length)
{
	g_return_if_fail (data != NULL && length != NULL);

	*data = NULL;
	*length = 0;
}

/* Default vfunc that simply returns NULL if not overriden by the implementing
   class. Mimics the old fallback behaviour of the getter functions. */
static gpointer
_xviewer_metadata_reader_default_get_null (XviewerMetadataReader *emr)
{
	return NULL;
}

static void
xviewer_metadata_reader_default_init (XviewerMetadataReaderInterface *iface)
{
	/* consume and finished are required to be implemented */
	/* Not-implemented funcs return NULL by default */
	iface->get_raw_exif = _xviewer_metadata_reader_default_get_raw_exif;
	iface->get_exif_data = _xviewer_metadata_reader_default_get_null;
	iface->get_icc_profile = _xviewer_metadata_reader_default_get_null;
	iface->get_xmp_ptr = _xviewer_metadata_reader_default_get_null;
}

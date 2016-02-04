/* Xviewer -- Metadata Reader Interface
 *
 * Copyright (C) 2008 The Free Software Foundation
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

#ifndef _XVIEWER_METADATA_READER_H_
#define _XVIEWER_METADATA_READER_H_

#include <glib-object.h>
#if HAVE_EXIF
#include "xviewer-exif-util.h"
#endif
#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif
#if HAVE_LCMS
#include <lcms2.h>
#endif

G_BEGIN_DECLS

#define XVIEWER_TYPE_METADATA_READER	      (xviewer_metadata_reader_get_type ())
#define XVIEWER_METADATA_READER(o)		      (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_METADATA_READER, XviewerMetadataReader))
#define XVIEWER_IS_METADATA_READER(o)	      (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_METADATA_READER))
#define XVIEWER_METADATA_READER_GET_INTERFACE(o)  (G_TYPE_INSTANCE_GET_INTERFACE ((o), XVIEWER_TYPE_METADATA_READER, XviewerMetadataReaderInterface))

typedef struct _XviewerMetadataReader XviewerMetadataReader;
typedef struct _XviewerMetadataReaderInterface XviewerMetadataReaderInterface;

struct _XviewerMetadataReaderInterface {
	GTypeInterface parent;

	void		(*consume)		(XviewerMetadataReader *self,
						 const guchar *buf,
						 guint len);

	gboolean	(*finished)		(XviewerMetadataReader *self);

	void		(*get_raw_exif)		(XviewerMetadataReader *self,
						 guchar **data,
						 guint *len);

	gpointer	(*get_exif_data)	(XviewerMetadataReader *self);

	gpointer	(*get_icc_profile)	(XviewerMetadataReader *self);

	gpointer	(*get_xmp_ptr)		(XviewerMetadataReader *self);
};

typedef enum {
	XVIEWER_METADATA_JPEG,
	XVIEWER_METADATA_PNG
} XviewerMetadataFileType;

G_GNUC_INTERNAL
GType                xviewer_metadata_reader_get_type	(void) G_GNUC_CONST;

G_GNUC_INTERNAL
XviewerMetadataReader*   xviewer_metadata_reader_new 		(XviewerMetadataFileType type);

G_GNUC_INTERNAL
void                 xviewer_metadata_reader_consume	(XviewerMetadataReader *emr,
							 const guchar *buf,
							 guint len);

G_GNUC_INTERNAL
gboolean             xviewer_metadata_reader_finished	(XviewerMetadataReader *emr);

G_GNUC_INTERNAL
void                 xviewer_metadata_reader_get_exif_chunk (XviewerMetadataReader *emr,
							 guchar **data,
							 guint *len);

#ifdef HAVE_EXIF
G_GNUC_INTERNAL
ExifData*         xviewer_metadata_reader_get_exif_data	(XviewerMetadataReader *emr);
#endif

#ifdef HAVE_EXEMPI
G_GNUC_INTERNAL
XmpPtr	     	     xviewer_metadata_reader_get_xmp_data	(XviewerMetadataReader *emr);
#endif

#if 0
gpointer             xviewer_metadata_reader_get_iptc_chunk	(XviewerMetadataReader *emr);
IptcData*            xviewer_metadata_reader_get_iptc_data	(XviewerMetadataReader *emr);
#endif

#ifdef HAVE_LCMS
G_GNUC_INTERNAL
cmsHPROFILE          xviewer_metadata_reader_get_icc_profile (XviewerMetadataReader *emr);
#endif

G_END_DECLS

#endif /* _XVIEWER_METADATA_READER_H_ */

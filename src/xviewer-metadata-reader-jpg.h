/* Xviewer -- JPEG Metadata Reader
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Felix Riemann <friemann@svn.gnome.org>
 *
 * Based on the original XviewerMetadataReader code.
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

#ifndef _XVIEWER_METADATA_READER_JPG_H_
#define _XVIEWER_METADATA_READER_JPG_H_

G_BEGIN_DECLS

#define XVIEWER_TYPE_METADATA_READER_JPG		(xviewer_metadata_reader_jpg_get_type ())
#define XVIEWER_METADATA_READER_JPG(o)         	(G_TYPE_CHECK_INSTANCE_CAST ((o),XVIEWER_TYPE_METADATA_READER_JPG, XviewerMetadataReaderJpg))
#define XVIEWER_METADATA_READER_JPG_CLASS(k)   	(G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_METADATA_READER_JPG, XviewerMetadataReaderJpgClass))
#define XVIEWER_IS_METADATA_READER_JPG(o)      	(G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_METADATA_READER_JPG))
#define XVIEWER_IS_METADATA_READER_JPG_CLASS(k)   	(G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_METADATA_READER_JPG))
#define XVIEWER_METADATA_READER_JPG_GET_CLASS(o)  	(G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_METADATA_READER_JPG, XviewerMetadataReaderJpgClass))

typedef struct _XviewerMetadataReaderJpg XviewerMetadataReaderJpg;
typedef struct _XviewerMetadataReaderJpgClass XviewerMetadataReaderJpgClass;
typedef struct _XviewerMetadataReaderJpgPrivate XviewerMetadataReaderJpgPrivate;

struct _XviewerMetadataReaderJpg {
	GObject parent;

	XviewerMetadataReaderJpgPrivate *priv;
};

struct _XviewerMetadataReaderJpgClass {
	GObjectClass parent_klass;
};

G_GNUC_INTERNAL
GType		      xviewer_metadata_reader_jpg_get_type	(void) G_GNUC_CONST;

G_END_DECLS

#endif /* _XVIEWER_METADATA_READER_JPG_H_ */

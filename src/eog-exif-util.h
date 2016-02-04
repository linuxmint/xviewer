/* Xviewer - EXIF Utilities
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 * Author: Felix Riemann <felix@hsgheli.de>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
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

#ifndef __XVIEWER_EXIF_UTIL_H__
#define __XVIEWER_EXIF_UTIL_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libexif/exif-data.h>

G_BEGIN_DECLS

#define XVIEWER_TYPE_EXIF_DATA xviewer_exif_data_get_type ()

gchar       *xviewer_exif_util_format_date           (const gchar *date);
void         xviewer_exif_util_set_label_text        (GtkLabel *label,
                                                  ExifData *exif_data,
                                                  gint tag_id);

void         xviewer_exif_util_set_focal_length_label_text (GtkLabel *label,
                                                        ExifData *exif_data);


const gchar *xviewer_exif_data_get_value             (ExifData *exif_data,
                                                  gint tag_id, gchar *buffer,
                                                  guint buf_size);

GType        xviewer_exif_data_get_type              (void) G_GNUC_CONST;

ExifData    *xviewer_exif_data_copy                  (ExifData *data);
void         xviewer_exif_data_free                  (ExifData *data);

G_END_DECLS

#endif /* __XVIEWER_EXIF_UTIL_H__ */

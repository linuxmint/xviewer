/* Eye Of Gnome - Thumbnailing functions
 *
 * Copyright (C) 2000-2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on nautilus code (libnautilus-private/nautilus-thumbnail.c) by:
 * 	- Andy Hertzfeld <andy@eazel.com>
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

#ifndef _EOG_THUMBNAIL_H_
#define _EOG_THUMBNAIL_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "eog-image.h"

G_BEGIN_DECLS

void          eog_thumbnail_init        (void);

GdkPixbuf*    eog_thumbnail_fit_to_size (GdkPixbuf *thumbnail,
					 gint        dimension);

GdkPixbuf*    eog_thumbnail_add_frame   (GdkPixbuf *thumbnail);

GdkPixbuf*    eog_thumbnail_load        (EogImage *image,
					 GError **error);

#define EOG_THUMBNAIL_ORIGINAL_WIDTH  "eog-thumbnail-orig-width"
#define EOG_THUMBNAIL_ORIGINAL_HEIGHT "eog-thumbnail-orig-height"

G_END_DECLS

#endif /* _EOG_THUMBNAIL_H_ */

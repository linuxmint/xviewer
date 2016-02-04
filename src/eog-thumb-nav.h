/* Xviewer - Thumbnail Navigator
 *
 * Copyright (C) 2006 The Free Software Foundation
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

#ifndef __XVIEWER_THUMB_NAV_H__
#define __XVIEWER_THUMB_NAV_H__

#include "xviewer-thumb-view.h"

#include <gtk/gtk.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _XviewerThumbNav XviewerThumbNav;
typedef struct _XviewerThumbNavClass XviewerThumbNavClass;
typedef struct _XviewerThumbNavPrivate XviewerThumbNavPrivate;

#define XVIEWER_TYPE_THUMB_NAV            (xviewer_thumb_nav_get_type ())
#define XVIEWER_THUMB_NAV(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), XVIEWER_TYPE_THUMB_NAV, XviewerThumbNav))
#define XVIEWER_THUMB_NAV_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  XVIEWER_TYPE_THUMB_NAV, XviewerThumbNavClass))
#define XVIEWER_IS_THUMB_NAV(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), XVIEWER_TYPE_THUMB_NAV))
#define XVIEWER_IS_THUMB_NAV_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  XVIEWER_TYPE_THUMB_NAV))
#define XVIEWER_THUMB_NAV_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  XVIEWER_TYPE_THUMB_NAV, XviewerThumbNavClass))

typedef enum {
	XVIEWER_THUMB_NAV_MODE_ONE_ROW,
	XVIEWER_THUMB_NAV_MODE_ONE_COLUMN,
	XVIEWER_THUMB_NAV_MODE_MULTIPLE_ROWS,
	XVIEWER_THUMB_NAV_MODE_MULTIPLE_COLUMNS
} XviewerThumbNavMode;

struct _XviewerThumbNav {
	GtkBox base_instance;

	XviewerThumbNavPrivate *priv;
};

struct _XviewerThumbNavClass {
	GtkBoxClass parent_class;
};

GType	         xviewer_thumb_nav_get_type          (void) G_GNUC_CONST;

GtkWidget       *xviewer_thumb_nav_new               (GtkWidget         *thumbview,
						  XviewerThumbNavMode    mode,
	             			          gboolean           show_buttons);

gboolean         xviewer_thumb_nav_get_show_buttons  (XviewerThumbNav       *nav);

void             xviewer_thumb_nav_set_show_buttons  (XviewerThumbNav       *nav,
                                                  gboolean           show_buttons);

XviewerThumbNavMode  xviewer_thumb_nav_get_mode          (XviewerThumbNav       *nav);

void             xviewer_thumb_nav_set_mode          (XviewerThumbNav       *nav,
                                                  XviewerThumbNavMode    mode);

G_END_DECLS

#endif /* __XVIEWER_THUMB_NAV_H__ */

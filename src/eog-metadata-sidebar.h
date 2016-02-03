/*
 * eog-metadata-sidebar.h
 * This file is part of eog
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2011 GNOME Foundation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#ifndef __EOG_METADATA_SIDEBAR_H__
#define __EOG_METADATA_SIDEBAR_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "eog-window.h"

G_BEGIN_DECLS

#define EOG_TYPE_METADATA_SIDEBAR          (eog_metadata_sidebar_get_type ())
#define EOG_METADATA_SIDEBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_METADATA_SIDEBAR, EogMetadataSidebar))
#define EOG_METADATA_SIDEBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_METADATA_SIDEBAR, EogMetadataSidebarClass))
#define EOG_IS_METADATA_SIDEBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_METADATA_SIDEBAR))
#define EOG_IS_METADATA_SIDEBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_METADATA_SIDEBAR))
#define EOG_METADATA_SIDEBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_METADATA_SIDEBAR, EogMetadataSidebarClass))

typedef struct _EogMetadataSidebar EogMetadataSidebar;
typedef struct _EogMetadataSidebarClass EogMetadataSidebarClass;
typedef struct _EogMetadataSidebarPrivate EogMetadataSidebarPrivate;

struct _EogMetadataSidebar {
	GtkScrolledWindow parent;

	EogMetadataSidebarPrivate *priv;
};

struct _EogMetadataSidebarClass {
	GtkScrolledWindowClass parent_klass;
};

G_GNUC_INTERNAL
GType eog_metadata_sidebar_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget* eog_metadata_sidebar_new (EogWindow *window);

G_END_DECLS

#endif /* __EOG_METADATA_SIDEBAR_H__ */

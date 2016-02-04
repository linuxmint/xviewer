/*
 * xviewer-metadata-sidebar.h
 * This file is part of xviewer
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

#ifndef __XVIEWER_METADATA_SIDEBAR_H__
#define __XVIEWER_METADATA_SIDEBAR_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "xviewer-window.h"

G_BEGIN_DECLS

#define XVIEWER_TYPE_METADATA_SIDEBAR          (xviewer_metadata_sidebar_get_type ())
#define XVIEWER_METADATA_SIDEBAR(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_METADATA_SIDEBAR, XviewerMetadataSidebar))
#define XVIEWER_METADATA_SIDEBAR_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_METADATA_SIDEBAR, XviewerMetadataSidebarClass))
#define XVIEWER_IS_METADATA_SIDEBAR(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_METADATA_SIDEBAR))
#define XVIEWER_IS_METADATA_SIDEBAR_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_METADATA_SIDEBAR))
#define XVIEWER_METADATA_SIDEBAR_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_METADATA_SIDEBAR, XviewerMetadataSidebarClass))

typedef struct _XviewerMetadataSidebar XviewerMetadataSidebar;
typedef struct _XviewerMetadataSidebarClass XviewerMetadataSidebarClass;
typedef struct _XviewerMetadataSidebarPrivate XviewerMetadataSidebarPrivate;

struct _XviewerMetadataSidebar {
	GtkScrolledWindow parent;

	XviewerMetadataSidebarPrivate *priv;
};

struct _XviewerMetadataSidebarClass {
	GtkScrolledWindowClass parent_klass;
};

G_GNUC_INTERNAL
GType xviewer_metadata_sidebar_get_type (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget* xviewer_metadata_sidebar_new (XviewerWindow *window);

G_END_DECLS

#endif /* __XVIEWER_METADATA_SIDEBAR_H__ */

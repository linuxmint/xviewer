/* Xviewer - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.h) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
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

#ifndef __XVIEWER_SIDEBAR_H__
#define __XVIEWER_SIDEBAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XviewerSidebar XviewerSidebar;
typedef struct _XviewerSidebarClass XviewerSidebarClass;
typedef struct _XviewerSidebarPrivate XviewerSidebarPrivate;

#define XVIEWER_TYPE_SIDEBAR	    (xviewer_sidebar_get_type())
#define XVIEWER_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_CAST((obj), XVIEWER_TYPE_SIDEBAR, XviewerSidebar))
#define XVIEWER_SIDEBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  XVIEWER_TYPE_SIDEBAR, XviewerSidebarClass))
#define XVIEWER_IS_SIDEBAR(obj)	    (G_TYPE_CHECK_INSTANCE_TYPE((obj), XVIEWER_TYPE_SIDEBAR))
#define XVIEWER_IS_SIDEBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  XVIEWER_TYPE_SIDEBAR))
#define XVIEWER_SIDEBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  XVIEWER_TYPE_SIDEBAR, XviewerSidebarClass))

struct _XviewerSidebar {
	GtkBox base_instance;

	XviewerSidebarPrivate *priv;
};

struct _XviewerSidebarClass {
	GtkBoxClass base_class;

	void (* page_added)   (XviewerSidebar *xviewer_sidebar,
			       GtkWidget  *main_widget);

	void (* page_removed) (XviewerSidebar *xviewer_sidebar,
			       GtkWidget  *main_widget);
};

GType      xviewer_sidebar_get_type     (void);

GtkWidget *xviewer_sidebar_new          (void);

void       xviewer_sidebar_add_page     (XviewerSidebar  *xviewer_sidebar,
				     const gchar *title,
				     GtkWidget   *main_widget);

void       xviewer_sidebar_remove_page  (XviewerSidebar  *xviewer_sidebar,
				     GtkWidget   *main_widget);

void       xviewer_sidebar_set_page     (XviewerSidebar  *xviewer_sidebar,
				     GtkWidget   *main_widget);

gint       xviewer_sidebar_get_n_pages  (XviewerSidebar  *xviewer_sidebar);

gboolean   xviewer_sidebar_is_empty     (XviewerSidebar  *xviewer_sidebar);

G_END_DECLS

#endif /* __XVIEWER_SIDEBAR_H__ */



/* Xviewer - Application Facade
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-application.h) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#ifndef __XVIEWER_APPLICATION_H__
#define __XVIEWER_APPLICATION_H__


#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>
#include "xviewer-window.h"

G_BEGIN_DECLS

typedef struct _XviewerApplication XviewerApplication;
typedef struct _XviewerApplicationClass XviewerApplicationClass;
typedef struct _XviewerApplicationPrivate XviewerApplicationPrivate;

#define XVIEWER_TYPE_APPLICATION            (xviewer_application_get_type ())
#define XVIEWER_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), XVIEWER_TYPE_APPLICATION, XviewerApplication))
#define XVIEWER_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  XVIEWER_TYPE_APPLICATION, XviewerApplicationClass))
#define XVIEWER_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), XVIEWER_TYPE_APPLICATION))
#define XVIEWER_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  XVIEWER_TYPE_APPLICATION))
#define XVIEWER_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  XVIEWER_TYPE_APPLICATION, XviewerApplicationClass))

#define XVIEWER_APP				(xviewer_application_get_instance ())

struct _XviewerApplication {
	GtkApplication base_instance;

	XviewerApplicationPrivate *priv;
};

struct _XviewerApplicationClass {
	GtkApplicationClass parent_class;
};

GType	          xviewer_application_get_type	      (void) G_GNUC_CONST;

XviewerApplication   *xviewer_application_get_instance        (void);

gboolean          xviewer_application_open_window         (XviewerApplication   *application,
						       guint             timestamp,
						       XviewerStartupFlags   flags,
						       GError          **error);

gboolean          xviewer_application_open_uri_list      (XviewerApplication   *application,
						      GSList           *uri_list,
						      guint             timestamp,
						      XviewerStartupFlags   flags,
						      GError          **error);

gboolean          xviewer_application_open_file_list     (XviewerApplication  *application,
						      GSList          *file_list,
						      guint           timestamp,
						      XviewerStartupFlags flags,
						      GError         **error);

gboolean          xviewer_application_open_uris           (XviewerApplication *application,
						       gchar         **uris,
						       guint           timestamp,
						       XviewerStartupFlags flags,
						       GError        **error);

G_END_DECLS

#endif /* __XVIEWER_APPLICATION_H__ */

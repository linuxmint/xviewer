/* Eye Of Gnome - Application Facade
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

#ifndef __EOG_APPLICATION_H__
#define __EOG_APPLICATION_H__


#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>
#include "eog-window.h"

G_BEGIN_DECLS

typedef struct _EogApplication EogApplication;
typedef struct _EogApplicationClass EogApplicationClass;
typedef struct _EogApplicationPrivate EogApplicationPrivate;

#define EOG_TYPE_APPLICATION            (eog_application_get_type ())
#define EOG_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), EOG_TYPE_APPLICATION, EogApplication))
#define EOG_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  EOG_TYPE_APPLICATION, EogApplicationClass))
#define EOG_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), EOG_TYPE_APPLICATION))
#define EOG_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  EOG_TYPE_APPLICATION))
#define EOG_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  EOG_TYPE_APPLICATION, EogApplicationClass))

#define EOG_APP				(eog_application_get_instance ())

struct _EogApplication {
	GtkApplication base_instance;

	EogApplicationPrivate *priv;
};

struct _EogApplicationClass {
	GtkApplicationClass parent_class;
};

GType	          eog_application_get_type	      (void) G_GNUC_CONST;

EogApplication   *eog_application_get_instance        (void);

gboolean          eog_application_open_window         (EogApplication   *application,
						       guint             timestamp,
						       EogStartupFlags   flags,
						       GError          **error);

gboolean          eog_application_open_uri_list      (EogApplication   *application,
						      GSList           *uri_list,
						      guint             timestamp,
						      EogStartupFlags   flags,
						      GError          **error);

gboolean          eog_application_open_file_list     (EogApplication  *application,
						      GSList          *file_list,
						      guint           timestamp,
						      EogStartupFlags flags,
						      GError         **error);

gboolean          eog_application_open_uris           (EogApplication *application,
						       gchar         **uris,
						       guint           timestamp,
						       EogStartupFlags flags,
						       GError        **error);

G_END_DECLS

#endif /* __EOG_APPLICATION_H__ */

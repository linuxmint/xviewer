/* Statusbar Date -- Shows the EXIF date in EOG's statusbar
 *
 * Copyright (C) 2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra  <csaavedra@gnome.org>
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

#ifndef __EOG_STATUSBAR_DATE_PLUGIN_H__
#define __EOG_STATUSBAR_DATE_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

#include <eog-window.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define EOG_TYPE_STATUSBAR_DATE_PLUGIN		(eog_statusbar_date_plugin_get_type ())
#define EOG_STATUSBAR_DATE_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_STATUSBAR_DATE_PLUGIN, EogStatusbarDatePlugin))
#define EOG_STATUSBAR_DATE_PLUGIN_CLASS(k)		G_TYPE_CHECK_CLASS_CAST((k),      EOG_TYPE_STATUSBAR_DATE_PLUGIN, EogStatusbarDatePluginClass))
#define EOG_IS_STATUSBAR_DATE_PLUGIN(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_STATUSBAR_DATE_PLUGIN))
#define EOG_IS_STATUSBAR_DATE_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k),    EOG_TYPE_STATUSBAR_DATE_PLUGIN))
#define EOG_STATUSBAR_DATE_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o),  EOG_TYPE_STATUSBAR_DATE_PLUGIN, EogStatusbarDatePluginClass))

/* Private structure type */
typedef struct _EogStatusbarDatePluginPrivate	EogStatusbarDatePluginPrivate;

/*
 * Main object structure
 */
typedef struct _EogStatusbarDatePlugin		EogStatusbarDatePlugin;

struct _EogStatusbarDatePlugin
{
	PeasExtensionBase parent_instance;

	EogWindow *window;
	GtkWidget *statusbar_date;
	gulong signal_id;
};

/*
 * Class definition
 */
typedef struct _EogStatusbarDatePluginClass	EogStatusbarDatePluginClass;

struct _EogStatusbarDatePluginClass
{
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType	eog_statusbar_date_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __EOG_STATUSBAR_DATE_PLUGIN_H__ */

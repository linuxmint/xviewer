/* Reload Image -- Allows to reload an image from disk
 *
 * Copyright (C) 2007-2012 The Free Software Foundation
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

#ifndef __XVIEWER_RELOAD_PLUGIN_H__
#define __XVIEWER_RELOAD_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

#include <xviewer-window.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define XVIEWER_TYPE_RELOAD_PLUGIN		(xviewer_reload_plugin_get_type ())
#define XVIEWER_RELOAD_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_RELOAD_PLUGIN, XviewerReloadPlugin))
#define XVIEWER_RELOAD_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k),     XVIEWER_TYPE_RELOAD_PLUGIN, XviewerReloadPluginClass))
#define XVIEWER_IS_RELOAD_PLUGIN(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_RELOAD_PLUGIN))
#define XVIEWER_IS_RELOAD_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k),    XVIEWER_TYPE_RELOAD_PLUGIN))
#define XVIEWER_RELOAD_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o),  XVIEWER_TYPE_RELOAD_PLUGIN, XviewerReloadPluginClass))

/* Private structure type */
typedef struct _XviewerReloadPluginPrivate	XviewerReloadPluginPrivate;

/*
 * Main object structure
 */
typedef struct _XviewerReloadPlugin		XviewerReloadPlugin;

struct _XviewerReloadPlugin
{
	PeasExtensionBase parent_instance;

	XviewerWindow *window;
	GtkActionGroup *ui_action_group;
	guint ui_id;
};

/*
 * Class definition
 */
typedef struct _XviewerReloadPluginClass	XviewerReloadPluginClass;

struct _XviewerReloadPluginClass
{
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType	xviewer_reload_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __XVIEWER_RELOAD_PLUGIN_H__ */

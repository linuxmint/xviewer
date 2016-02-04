/* Fullscreen with double-click -- Sets xviewer to fullscreen by double-clicking
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

#ifndef __XVIEWER_FULLSCREEN_PLUGIN_H__
#define __XVIEWER_FULLSCREEN_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

#include <xviewer-window.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define XVIEWER_TYPE_FULLSCREEN_PLUGIN		(xviewer_fullscreen_plugin_get_type ())
#define XVIEWER_FULLSCREEN_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_FULLSCREEN_PLUGIN, XviewerFullscreenPlugin))
#define XVIEWER_FULLSCREEN_PLUGIN_CLASS(k)		G_TYPE_CHECK_CLASS_CAST((k),      XVIEWER_TYPE_FULLSCREEN_PLUGIN, XviewerFullscreenPluginClass))
#define XVIEWER_IS_FULLSCREEN_PLUGIN(o)	        (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_FULLSCREEN_PLUGIN))
#define XVIEWER_IS_FULLSCREEN_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k),    XVIEWER_TYPE_FULLSCREEN_PLUGIN))
#define XVIEWER_FULLSCREEN_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o),  XVIEWER_TYPE_FULLSCREEN_PLUGIN, XviewerFullscreenPluginClass))

/* Private structure type */
typedef struct _XviewerFullscreenPluginPrivate	XviewerFullscreenPluginPrivate;

/*
 * Main object structure
 */
typedef struct _XviewerFullscreenPlugin		XviewerFullscreenPlugin;

struct _XviewerFullscreenPlugin
{
	PeasExtensionBase parent_instance;

	XviewerWindow *window;
	gulong signal_id;
};

/*
 * Class definition
 */
typedef struct _XviewerFullscreenPluginClass	XviewerFullscreenPluginClass;

struct _XviewerFullscreenPluginClass
{
	PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType	xviewer_fullscreen_plugin_get_type		(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS

#endif /* __XVIEWER_FULLSCREEN_PLUGIN_H__ */

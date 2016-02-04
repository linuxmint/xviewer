/* Xviewer - XVIEWER Plugin Manager
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-module.c) by:
 * 	- Paolo Maggi <paolo@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xviewer-plugin-engine.h"
#include "xviewer-debug.h"
#include "xviewer-config-keys.h"
#include "xviewer-util.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>
#include <girepository.h>

#define XVIEWER_PLUGIN_DATA_DIR XVIEWER_DATA_DIR G_DIR_SEPARATOR_S "plugins"

struct _XviewerPluginEnginePrivate {
    GSettings *plugins_settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (XviewerPluginEngine, xviewer_plugin_engine, PEAS_TYPE_ENGINE);

static void
xviewer_plugin_engine_dispose (GObject *object)
{
	XviewerPluginEngine *engine = XVIEWER_PLUGIN_ENGINE (object);

	if (engine->priv->plugins_settings != NULL)
	{
		g_object_unref (engine->priv->plugins_settings);
		engine->priv->plugins_settings = NULL;
	}

	G_OBJECT_CLASS (xviewer_plugin_engine_parent_class)->dispose (object);
}

static void
xviewer_plugin_engine_class_init (XviewerPluginEngineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = xviewer_plugin_engine_dispose;
}

static void
xviewer_plugin_engine_init (XviewerPluginEngine *engine)
{
	xviewer_debug (DEBUG_PLUGINS);

	engine->priv = xviewer_plugin_engine_get_instance_private (engine);

	engine->priv->plugins_settings = g_settings_new ("org.gnome.xviewer.plugins");
}

XviewerPluginEngine *
xviewer_plugin_engine_new (void)
{
	XviewerPluginEngine *engine;
	gchar *user_plugin_path;
	gchar *private_path;
	const gchar * const * system_data_dirs;
	GError *error = NULL;

	private_path = g_build_filename (LIBDIR, "xviewer",
					"girepository-1.0", NULL);

	/* This should be moved to libpeas */
	if (g_irepository_require (g_irepository_get_default (),
				   "Peas", "1.0", 0, &error) == NULL)
	{
		g_warning ("Error loading Peas typelib: %s\n",
			   error->message);
		g_clear_error (&error);
	}


	if (g_irepository_require (g_irepository_get_default (),
				   "PeasGtk", "1.0", 0, &error) == NULL)
	{
		g_warning ("Error loading PeasGtk typelib: %s\n",
			   error->message);
		g_clear_error (&error);
	}


	if (g_irepository_require_private (g_irepository_get_default (),
					   private_path, "Xviewer", "3.0", 0,
					   &error) == NULL)
	{
		g_warning ("Error loading Xviewer typelib: %s\n",
			   error->message);
		g_clear_error (&error);
	}

	g_free (private_path);

	engine = XVIEWER_PLUGIN_ENGINE (g_object_new (XVIEWER_TYPE_PLUGIN_ENGINE,
						  NULL));

	peas_engine_enable_loader (PEAS_ENGINE (engine), "python3");

	user_plugin_path = g_build_filename (g_get_user_data_dir (),
					     "xviewer", "plugins", NULL);
	/* Find per-user plugins */
	xviewer_debug_message (DEBUG_PLUGINS,
	                   "Adding XDG_DATA_HOME (%s) to plugins search path",
	                   user_plugin_path);
	peas_engine_add_search_path (PEAS_ENGINE (engine),
				     user_plugin_path, user_plugin_path);

	system_data_dirs = g_get_system_data_dirs();

	while (*system_data_dirs != NULL)
	{
		gchar *plugin_path;

		plugin_path = g_build_filename (*system_data_dirs,
		                                 "xviewer", "plugins", NULL);
		xviewer_debug_message (DEBUG_PLUGINS,
		                "Adding XDG_DATA_DIR %s to plugins search path",
		                plugin_path);
		peas_engine_add_search_path (PEAS_ENGINE (engine),
		                             plugin_path, plugin_path);
		g_free (plugin_path);
		++system_data_dirs;
	}

	/* Find system-wide plugins */
	xviewer_debug_message (DEBUG_PLUGINS, "Adding system plugin dir ("
	                   XVIEWER_PLUGIN_DIR ")to plugins search path");
	peas_engine_add_search_path (PEAS_ENGINE (engine),
				     XVIEWER_PLUGIN_DIR, XVIEWER_PLUGIN_DATA_DIR);

	g_settings_bind (engine->priv->plugins_settings,
			 XVIEWER_CONF_PLUGINS_ACTIVE_PLUGINS,
			 engine,
			 "loaded-plugins",
			 G_SETTINGS_BIND_DEFAULT);

	g_free (user_plugin_path);

	return engine;
}

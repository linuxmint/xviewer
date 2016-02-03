/* Eye Of Gnome - EOG Plugin Manager
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

#include "eog-plugin-engine.h"
#include "eog-debug.h"
#include "eog-config-keys.h"
#include "eog-util.h"

#include <glib/gi18n.h>
#include <glib.h>
#include <gio/gio.h>
#include <girepository.h>

#define EOG_PLUGIN_DATA_DIR EOG_DATA_DIR G_DIR_SEPARATOR_S "plugins"

struct _EogPluginEnginePrivate {
    GSettings *plugins_settings;
};

G_DEFINE_TYPE_WITH_PRIVATE (EogPluginEngine, eog_plugin_engine, PEAS_TYPE_ENGINE);

static void
eog_plugin_engine_dispose (GObject *object)
{
	EogPluginEngine *engine = EOG_PLUGIN_ENGINE (object);

	if (engine->priv->plugins_settings != NULL)
	{
		g_object_unref (engine->priv->plugins_settings);
		engine->priv->plugins_settings = NULL;
	}

	G_OBJECT_CLASS (eog_plugin_engine_parent_class)->dispose (object);
}

static void
eog_plugin_engine_class_init (EogPluginEngineClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = eog_plugin_engine_dispose;
}

static void
eog_plugin_engine_init (EogPluginEngine *engine)
{
	eog_debug (DEBUG_PLUGINS);

	engine->priv = eog_plugin_engine_get_instance_private (engine);

	engine->priv->plugins_settings = g_settings_new ("org.gnome.eog.plugins");
}

EogPluginEngine *
eog_plugin_engine_new (void)
{
	EogPluginEngine *engine;
	gchar *user_plugin_path;
	gchar *private_path;
	const gchar * const * system_data_dirs;
	GError *error = NULL;

	private_path = g_build_filename (LIBDIR, "eog",
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
					   private_path, "Eog", "3.0", 0,
					   &error) == NULL)
	{
		g_warning ("Error loading Eog typelib: %s\n",
			   error->message);
		g_clear_error (&error);
	}

	g_free (private_path);

	engine = EOG_PLUGIN_ENGINE (g_object_new (EOG_TYPE_PLUGIN_ENGINE,
						  NULL));

	peas_engine_enable_loader (PEAS_ENGINE (engine), "python3");

	user_plugin_path = g_build_filename (g_get_user_data_dir (),
					     "eog", "plugins", NULL);
	/* Find per-user plugins */
	eog_debug_message (DEBUG_PLUGINS,
	                   "Adding XDG_DATA_HOME (%s) to plugins search path",
	                   user_plugin_path);
	peas_engine_add_search_path (PEAS_ENGINE (engine),
				     user_plugin_path, user_plugin_path);

	system_data_dirs = g_get_system_data_dirs();

	while (*system_data_dirs != NULL)
	{
		gchar *plugin_path;

		plugin_path = g_build_filename (*system_data_dirs,
		                                 "eog", "plugins", NULL);
		eog_debug_message (DEBUG_PLUGINS,
		                "Adding XDG_DATA_DIR %s to plugins search path",
		                plugin_path);
		peas_engine_add_search_path (PEAS_ENGINE (engine),
		                             plugin_path, plugin_path);
		g_free (plugin_path);
		++system_data_dirs;
	}

	/* Find system-wide plugins */
	eog_debug_message (DEBUG_PLUGINS, "Adding system plugin dir ("
	                   EOG_PLUGIN_DIR ")to plugins search path");
	peas_engine_add_search_path (PEAS_ENGINE (engine),
				     EOG_PLUGIN_DIR, EOG_PLUGIN_DATA_DIR);

	g_settings_bind (engine->priv->plugins_settings,
			 EOG_CONF_PLUGINS_ACTIVE_PLUGINS,
			 engine,
			 "loaded-plugins",
			 G_SETTINGS_BIND_DEFAULT);

	g_free (user_plugin_path);

	return engine;
}

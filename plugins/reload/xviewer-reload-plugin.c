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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xviewer-reload-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <libpeas/peas.h>

#include <xviewer-debug.h>
#include <xviewer-scroll-view.h>
#include <xviewer-thumb-view.h>
#include <xviewer-image.h>
#include <xviewer-window.h>
#include <xviewer-window-activatable.h>

static void xviewer_window_activatable_iface_init (XviewerWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (XviewerReloadPlugin,
                                xviewer_reload_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (XVIEWER_TYPE_WINDOW_ACTIVATABLE,
                                                               xviewer_window_activatable_iface_init))

enum {
  PROP_0,
  PROP_WINDOW
};

static void
reload_cb (GtkAction	*action, 
	   XviewerWindow *window)
{
        xviewer_window_reload_image (window);
}

static const gchar * const ui_definition =
	"<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\"><separator/>"
	"<menuitem name=\"XviewerPluginReload\" action=\"XviewerPluginRunReload\"/>"
	"<separator/></menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
	"<menuitem action=\"XviewerPluginRunReload\"/><separator/>"
	"</popup></ui>";

static const GtkActionEntry action_entries[] =
{
	{ "XviewerPluginRunReload",
	  "view-refresh",
	  N_("Reload Image"),
	  "R",
	  N_("Reload current image"),
	  G_CALLBACK (reload_cb) }
};

static void
xviewer_reload_plugin_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	XviewerReloadPlugin *plugin = XVIEWER_RELOAD_PLUGIN (object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		plugin->window = XVIEWER_WINDOW (g_value_dup_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xviewer_reload_plugin_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	XviewerReloadPlugin *plugin = XVIEWER_RELOAD_PLUGIN (object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		g_value_set_object (value, plugin->window);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xviewer_reload_plugin_init (XviewerReloadPlugin *plugin)
{
	xviewer_debug_message (DEBUG_PLUGINS, "XviewerReloadPlugin initializing");
}

static void
xviewer_reload_plugin_dispose (GObject *object)
{
	XviewerReloadPlugin *plugin = XVIEWER_RELOAD_PLUGIN (object);

	xviewer_debug_message (DEBUG_PLUGINS, "XviewerReloadPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (xviewer_reload_plugin_parent_class)->dispose (object);
}

static void
xviewer_reload_plugin_activate (XviewerWindowActivatable *activatable)
{
	GtkUIManager *manager;
	XviewerReloadPlugin *plugin = XVIEWER_RELOAD_PLUGIN (activatable);

	xviewer_debug (DEBUG_PLUGINS);

	manager = xviewer_window_get_ui_manager (plugin->window);

	plugin->ui_action_group = gtk_action_group_new ("XviewerReloadPluginActions");

	gtk_action_group_set_translation_domain (plugin->ui_action_group,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (plugin->ui_action_group,
				      action_entries,
				      G_N_ELEMENTS (action_entries),
				      plugin->window);

	gtk_ui_manager_insert_action_group (manager,
					    plugin->ui_action_group,
					    -1);

	plugin->ui_id = gtk_ui_manager_add_ui_from_string (manager,
							   ui_definition,
							   -1, NULL);
	g_warn_if_fail (plugin->ui_id != 0);
}

static void
xviewer_reload_plugin_deactivate (XviewerWindowActivatable *activatable)
{
	GtkUIManager *manager;
	XviewerReloadPlugin *plugin = XVIEWER_RELOAD_PLUGIN (activatable);

	xviewer_debug (DEBUG_PLUGINS);

	manager = xviewer_window_get_ui_manager (plugin->window);

	gtk_ui_manager_remove_ui (manager,
				  plugin->ui_id);

	gtk_ui_manager_remove_action_group (manager,
					    plugin->ui_action_group);
}

static void
xviewer_reload_plugin_class_init (XviewerReloadPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose= xviewer_reload_plugin_dispose;
	object_class->set_property = xviewer_reload_plugin_set_property;
	object_class->get_property = xviewer_reload_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
xviewer_reload_plugin_class_finalize (XviewerReloadPluginClass *klass)
{
}

static void
xviewer_window_activatable_iface_init (XviewerWindowActivatableInterface *iface)
{
	iface->activate = xviewer_reload_plugin_activate;
	iface->deactivate = xviewer_reload_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	xviewer_reload_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
						    XVIEWER_TYPE_WINDOW_ACTIVATABLE,
						    XVIEWER_TYPE_RELOAD_PLUGIN);
}

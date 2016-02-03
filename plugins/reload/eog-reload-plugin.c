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

#include "eog-reload-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>

#include <libpeas/peas.h>

#include <eog-debug.h>
#include <eog-scroll-view.h>
#include <eog-thumb-view.h>
#include <eog-image.h>
#include <eog-window.h>
#include <eog-window-activatable.h>

static void eog_window_activatable_iface_init (EogWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EogReloadPlugin,
                                eog_reload_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE_DYNAMIC (EOG_TYPE_WINDOW_ACTIVATABLE,
                                                               eog_window_activatable_iface_init))

enum {
  PROP_0,
  PROP_WINDOW
};

static void
reload_cb (GtkAction	*action, 
	   EogWindow *window)
{
        eog_window_reload_image (window);
}

static const gchar * const ui_definition =
	"<ui><menubar name=\"MainMenu\">"
	"<menu name=\"ToolsMenu\" action=\"Tools\"><separator/>"
	"<menuitem name=\"EogPluginReload\" action=\"EogPluginRunReload\"/>"
	"<separator/></menu></menubar>"
	"<popup name=\"ViewPopup\"><separator/>"
	"<menuitem action=\"EogPluginRunReload\"/><separator/>"
	"</popup></ui>";

static const GtkActionEntry action_entries[] =
{
	{ "EogPluginRunReload",
	  "view-refresh",
	  N_("Reload Image"),
	  "R",
	  N_("Reload current image"),
	  G_CALLBACK (reload_cb) }
};

static void
eog_reload_plugin_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	EogReloadPlugin *plugin = EOG_RELOAD_PLUGIN (object);

	switch (prop_id)
	{
	case PROP_WINDOW:
		plugin->window = EOG_WINDOW (g_value_dup_object (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
eog_reload_plugin_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	EogReloadPlugin *plugin = EOG_RELOAD_PLUGIN (object);

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
eog_reload_plugin_init (EogReloadPlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogReloadPlugin initializing");
}

static void
eog_reload_plugin_dispose (GObject *object)
{
	EogReloadPlugin *plugin = EOG_RELOAD_PLUGIN (object);

	eog_debug_message (DEBUG_PLUGINS, "EogReloadPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eog_reload_plugin_parent_class)->dispose (object);
}

static void
eog_reload_plugin_activate (EogWindowActivatable *activatable)
{
	GtkUIManager *manager;
	EogReloadPlugin *plugin = EOG_RELOAD_PLUGIN (activatable);

	eog_debug (DEBUG_PLUGINS);

	manager = eog_window_get_ui_manager (plugin->window);

	plugin->ui_action_group = gtk_action_group_new ("EogReloadPluginActions");

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
eog_reload_plugin_deactivate (EogWindowActivatable *activatable)
{
	GtkUIManager *manager;
	EogReloadPlugin *plugin = EOG_RELOAD_PLUGIN (activatable);

	eog_debug (DEBUG_PLUGINS);

	manager = eog_window_get_ui_manager (plugin->window);

	gtk_ui_manager_remove_ui (manager,
				  plugin->ui_id);

	gtk_ui_manager_remove_action_group (manager,
					    plugin->ui_action_group);
}

static void
eog_reload_plugin_class_init (EogReloadPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose= eog_reload_plugin_dispose;
	object_class->set_property = eog_reload_plugin_set_property;
	object_class->get_property = eog_reload_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eog_reload_plugin_class_finalize (EogReloadPluginClass *klass)
{
}

static void
eog_window_activatable_iface_init (EogWindowActivatableInterface *iface)
{
	iface->activate = eog_reload_plugin_activate;
	iface->deactivate = eog_reload_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eog_reload_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
						    EOG_TYPE_WINDOW_ACTIVATABLE,
						    EOG_TYPE_RELOAD_PLUGIN);
}

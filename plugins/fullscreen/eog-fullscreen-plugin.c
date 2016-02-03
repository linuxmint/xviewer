/* Fullscreen with double-click -- Sets eog to fullscreen by double-clicking
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

#include "eog-fullscreen-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>
#include <libpeas/peas-activatable.h>

#include <eog-debug.h>
#include <eog-scroll-view.h>
#include <eog-window-activatable.h>

static void eog_window_activatable_iface_init (EogWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (EogFullscreenPlugin,
		eog_fullscreen_plugin,
		PEAS_TYPE_EXTENSION_BASE,
		0,
		G_IMPLEMENT_INTERFACE_DYNAMIC (EOG_TYPE_WINDOW_ACTIVATABLE,
					     eog_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static gboolean
on_button_press (GtkWidget *widget, GdkEventButton *event, EogWindow *window)
{
	EogScrollView *view = EOG_SCROLL_VIEW (widget);

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		EogWindowMode mode = eog_window_get_mode (window);
		GdkEvent *ev = (GdkEvent*) event;

		if(!eog_scroll_view_event_is_over_image (view, ev))
			return FALSE;

		if (mode == EOG_WINDOW_MODE_SLIDESHOW ||
		    mode == EOG_WINDOW_MODE_FULLSCREEN)
			eog_window_set_mode (window, EOG_WINDOW_MODE_NORMAL);
		else if (mode == EOG_WINDOW_MODE_NORMAL)
			eog_window_set_mode (window, EOG_WINDOW_MODE_FULLSCREEN);

		return TRUE;
	}

	return FALSE;
}

static void
eog_fullscreen_plugin_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	EogFullscreenPlugin *plugin = EOG_FULLSCREEN_PLUGIN (object);

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
eog_fullscreen_plugin_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	EogFullscreenPlugin *plugin = EOG_FULLSCREEN_PLUGIN (object);

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
eog_fullscreen_plugin_init (EogFullscreenPlugin *plugin)
{
	eog_debug_message (DEBUG_PLUGINS, "EogFullscreenPlugin initializing");
}

static void
eog_fullscreen_plugin_dispose (GObject *object)
{
	EogFullscreenPlugin *plugin = EOG_FULLSCREEN_PLUGIN (object);

	eog_debug_message (DEBUG_PLUGINS, "EogFullscreenPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (eog_fullscreen_plugin_parent_class)->dispose (object);
}

static void
eog_fullscreen_plugin_activate (EogWindowActivatable *activatable)
{
	EogFullscreenPlugin *plugin = EOG_FULLSCREEN_PLUGIN (activatable);
	GtkWidget *view = eog_window_get_view (plugin->window);

	eog_debug (DEBUG_PLUGINS);

	plugin->signal_id = g_signal_connect (G_OBJECT (view),
					      "button-press-event",
					      G_CALLBACK (on_button_press),
					      plugin->window);
}

static void
eog_fullscreen_plugin_deactivate (EogWindowActivatable *activatable)
{
	EogFullscreenPlugin *plugin = EOG_FULLSCREEN_PLUGIN (activatable);
	GtkWidget *view = eog_window_get_view (plugin->window);

	g_signal_handler_disconnect (view, plugin->signal_id);
}

static void
eog_fullscreen_plugin_class_init (EogFullscreenPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = eog_fullscreen_plugin_dispose;
	object_class->set_property = eog_fullscreen_plugin_set_property;
	object_class->get_property = eog_fullscreen_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
eog_fullscreen_plugin_class_finalize (EogFullscreenPluginClass *klass)
{
}

static void
eog_window_activatable_iface_init (EogWindowActivatableInterface *iface)
{
	iface->activate = eog_fullscreen_plugin_activate;
	iface->deactivate = eog_fullscreen_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	eog_fullscreen_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
						    EOG_TYPE_WINDOW_ACTIVATABLE,
						    EOG_TYPE_FULLSCREEN_PLUGIN);
}

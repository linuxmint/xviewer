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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xviewer-fullscreen-plugin.h"

#include <gmodule.h>
#include <glib/gi18n-lib.h>
#include <libpeas/peas-activatable.h>

#include <xviewer-debug.h>
#include <xviewer-scroll-view.h>
#include <xviewer-window-activatable.h>

static void xviewer_window_activatable_iface_init (XviewerWindowActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (XviewerFullscreenPlugin,
		xviewer_fullscreen_plugin,
		PEAS_TYPE_EXTENSION_BASE,
		0,
		G_IMPLEMENT_INTERFACE_DYNAMIC (XVIEWER_TYPE_WINDOW_ACTIVATABLE,
					     xviewer_window_activatable_iface_init))

enum {
	PROP_0,
	PROP_WINDOW
};

static gboolean
on_button_press (GtkWidget *widget, GdkEventButton *event, XviewerWindow *window)
{
	XviewerScrollView *view = XVIEWER_SCROLL_VIEW (widget);

	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		XviewerWindowMode mode = xviewer_window_get_mode (window);
		GdkEvent *ev = (GdkEvent*) event;

		if(!xviewer_scroll_view_event_is_over_image (view, ev))
			return FALSE;

		if (mode == XVIEWER_WINDOW_MODE_SLIDESHOW ||
		    mode == XVIEWER_WINDOW_MODE_FULLSCREEN)
			xviewer_window_set_mode (window, XVIEWER_WINDOW_MODE_NORMAL);
		else if (mode == XVIEWER_WINDOW_MODE_NORMAL)
			xviewer_window_set_mode (window, XVIEWER_WINDOW_MODE_FULLSCREEN);

		return TRUE;
	}

	return FALSE;
}

static void
xviewer_fullscreen_plugin_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	XviewerFullscreenPlugin *plugin = XVIEWER_FULLSCREEN_PLUGIN (object);

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
xviewer_fullscreen_plugin_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	XviewerFullscreenPlugin *plugin = XVIEWER_FULLSCREEN_PLUGIN (object);

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
xviewer_fullscreen_plugin_init (XviewerFullscreenPlugin *plugin)
{
	xviewer_debug_message (DEBUG_PLUGINS, "XviewerFullscreenPlugin initializing");
}

static void
xviewer_fullscreen_plugin_dispose (GObject *object)
{
	XviewerFullscreenPlugin *plugin = XVIEWER_FULLSCREEN_PLUGIN (object);

	xviewer_debug_message (DEBUG_PLUGINS, "XviewerFullscreenPlugin disposing");

	if (plugin->window != NULL) {
		g_object_unref (plugin->window);
		plugin->window = NULL;
	}

	G_OBJECT_CLASS (xviewer_fullscreen_plugin_parent_class)->dispose (object);
}

static void
xviewer_fullscreen_plugin_activate (XviewerWindowActivatable *activatable)
{
	XviewerFullscreenPlugin *plugin = XVIEWER_FULLSCREEN_PLUGIN (activatable);
	GtkWidget *view = xviewer_window_get_view (plugin->window);

	xviewer_debug (DEBUG_PLUGINS);

	plugin->signal_id = g_signal_connect (G_OBJECT (view),
					      "button-press-event",
					      G_CALLBACK (on_button_press),
					      plugin->window);
}

static void
xviewer_fullscreen_plugin_deactivate (XviewerWindowActivatable *activatable)
{
	XviewerFullscreenPlugin *plugin = XVIEWER_FULLSCREEN_PLUGIN (activatable);
	GtkWidget *view = xviewer_window_get_view (plugin->window);

	g_signal_handler_disconnect (view, plugin->signal_id);
}

static void
xviewer_fullscreen_plugin_class_init (XviewerFullscreenPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = xviewer_fullscreen_plugin_dispose;
	object_class->set_property = xviewer_fullscreen_plugin_set_property;
	object_class->get_property = xviewer_fullscreen_plugin_get_property;

	g_object_class_override_property (object_class, PROP_WINDOW, "window");
}

static void
xviewer_fullscreen_plugin_class_finalize (XviewerFullscreenPluginClass *klass)
{
}

static void
xviewer_window_activatable_iface_init (XviewerWindowActivatableInterface *iface)
{
	iface->activate = xviewer_fullscreen_plugin_activate;
	iface->deactivate = xviewer_fullscreen_plugin_deactivate;
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
	xviewer_fullscreen_plugin_register_type (G_TYPE_MODULE (module));
	peas_object_module_register_extension_type (module,
						    XVIEWER_TYPE_WINDOW_ACTIVATABLE,
						    XVIEWER_TYPE_FULLSCREEN_PLUGIN);
}

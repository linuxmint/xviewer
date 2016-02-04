/*
 * xviewer-window-activatable.c
 * This file is part of xviewer
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2011 Felix Riemann
 * 
 * Base on code by:
 * 	- Steve Frécinaux <code@istique.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include "config.h"
#endif

#include "xviewer-window-activatable.h"

#include <glib-object.h>
#include "xviewer-window.h"

G_DEFINE_INTERFACE(XviewerWindowActivatable, xviewer_window_activatable, G_TYPE_OBJECT)

void
xviewer_window_activatable_default_init (XviewerWindowActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * XviewerWindowActivatable:window:
		 *
		 * This is the #XviewerWindow this #XviewerWindowActivatable instance
		 * should be attached to.
		 */
		g_object_interface_install_property (iface,
				g_param_spec_object ("window", "Window",
						     "The XviewerWindow this "
						     "instance it attached to",
						     XVIEWER_TYPE_WINDOW,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_STATIC_STRINGS));
		initialized = TRUE;
	}
}

void
xviewer_window_activatable_activate (XviewerWindowActivatable *activatable)
{
	XviewerWindowActivatableInterface *iface;

	g_return_if_fail (XVIEWER_IS_WINDOW_ACTIVATABLE (activatable));

	iface = XVIEWER_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->activate != NULL))
		iface->activate (activatable);
}

void
xviewer_window_activatable_deactivate (XviewerWindowActivatable *activatable)
{
	XviewerWindowActivatableInterface *iface;

	g_return_if_fail (XVIEWER_IS_WINDOW_ACTIVATABLE (activatable));

	iface = XVIEWER_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->deactivate != NULL))
		iface->deactivate (activatable);
}

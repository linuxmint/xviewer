/*
 * eog-window-activatable.c
 * This file is part of eog
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

#include "eog-window-activatable.h"

#include <glib-object.h>
#include "eog-window.h"

G_DEFINE_INTERFACE(EogWindowActivatable, eog_window_activatable, G_TYPE_OBJECT)

void
eog_window_activatable_default_init (EogWindowActivatableInterface *iface)
{
	static gboolean initialized = FALSE;

	if (!initialized) {
		/**
		 * EogWindowActivatable:window:
		 *
		 * This is the #EogWindow this #EogWindowActivatable instance
		 * should be attached to.
		 */
		g_object_interface_install_property (iface,
				g_param_spec_object ("window", "Window",
						     "The EogWindow this "
						     "instance it attached to",
						     EOG_TYPE_WINDOW,
						     G_PARAM_READWRITE |
						     G_PARAM_CONSTRUCT_ONLY |
						     G_PARAM_STATIC_STRINGS));
		initialized = TRUE;
	}
}

void
eog_window_activatable_activate (EogWindowActivatable *activatable)
{
	EogWindowActivatableInterface *iface;

	g_return_if_fail (EOG_IS_WINDOW_ACTIVATABLE (activatable));

	iface = EOG_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->activate != NULL))
		iface->activate (activatable);
}

void
eog_window_activatable_deactivate (EogWindowActivatable *activatable)
{
	EogWindowActivatableInterface *iface;

	g_return_if_fail (EOG_IS_WINDOW_ACTIVATABLE (activatable));

	iface = EOG_WINDOW_ACTIVATABLE_GET_IFACE (activatable);

	if (G_LIKELY (iface->deactivate != NULL))
		iface->deactivate (activatable);
}

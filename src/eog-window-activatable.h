/*
 * eog-window-activatable.h
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

#ifndef __EOG_WINDOW_ACTIVATABLE_H__
#define __EOG_WINDOW_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EOG_TYPE_WINDOW_ACTIVATABLE	(eog_window_activatable_get_type ())
#define EOG_WINDOW_ACTIVATABLE(obj) 	(G_TYPE_CHECK_INSTANCE_CAST ((obj), \
					 EOG_TYPE_WINDOW_ACTIVATABLE, \
					 EogWindowActivatable))
#define EOG_WINDOW_ACTIVATABLE_IFACE(obj) \
					(G_TYPE_CHECK_CLASS_CAST ((obj), \
					 EOG_TYPE_WINDOW_ACTIVATABLE, \
					 EogWindowActivatableInterface))
#define EOG_IS_WINDOW_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
					 EOG_TYPE_WINDOW_ACTIVATABLE))
#define EOG_WINDOW_ACTIVATABLE_GET_IFACE(obj) \
					(G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
					 EOG_TYPE_WINDOW_ACTIVATABLE, \
					 EogWindowActivatableInterface))

typedef struct _EogWindowActivatable		EogWindowActivatable;
typedef struct _EogWindowActivatableInterface	EogWindowActivatableInterface;

struct _EogWindowActivatableInterface
{
	GTypeInterface g_iface;

	/* vfuncs */

	void	(*activate)	(EogWindowActivatable *activatable);
	void	(*deactivate)	(EogWindowActivatable *activatable);
};

GType	eog_window_activatable_get_type	(void) G_GNUC_CONST;

void	eog_window_activatable_activate	    (EogWindowActivatable *activatable);
void	eog_window_activatable_deactivate   (EogWindowActivatable *activatable);

G_END_DECLS
#endif /* __EOG_WINDOW_ACTIVATABLE_H__ */

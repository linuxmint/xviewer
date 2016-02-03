/*
 * eog-application-activatable.h
 * This file is part of eog
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2012 Felix Riemann
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

#ifndef __EOG_APPLICATION_ACTIVATABLE_H__
#define __EOG_APPLICATION_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define EOG_TYPE_APPLICATION_ACTIVATABLE (eog_application_activatable_get_type ())
#define EOG_APPLICATION_ACTIVATABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          EOG_TYPE_APPLICATION_ACTIVATABLE, \
                                          EogApplicationActivatable))
#define EOG_APPLICATION_ACTIVATABLE_IFACE(obj) \
                                          (G_TYPE_CHECK_CLASS_CAST ((obj), \
                                           EOG_TYPE_APPLICATION_ACTIVATABLE, \
                                           EogApplicationActivatableInterface))
#define EOG_IS_APPLICATION_ACTIVATABLE(obj) \
                                          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           EOG_TYPE_APPLICATION_ACTIVATABLE))
#define EOG_APPLICATION_ACTIVATABLE_GET_IFACE(obj) \
                                        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
                                         EOG_TYPE_APPLICATION_ACTIVATABLE, \
                                         EogApplicationActivatableInterface))

typedef struct _EogApplicationActivatable		EogApplicationActivatable;
typedef struct _EogApplicationActivatableInterface	EogApplicationActivatableInterface;

struct _EogApplicationActivatableInterface
{
	GTypeInterface g_iface;

	/* vfuncs */

    void	(*activate)	(EogApplicationActivatable *activatable);
    void	(*deactivate)	(EogApplicationActivatable *activatable);
};

GType	eog_application_activatable_get_type     (void) G_GNUC_CONST;

void	eog_application_activatable_activate     (EogApplicationActivatable *activatable);
void	eog_application_activatable_deactivate   (EogApplicationActivatable *activatable);

G_END_DECLS
#endif /* __EOG_APPLICATION_ACTIVATABLE_H__ */

/*
 * xviewer-application-activatable.h
 * This file is part of xviewer
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

#ifndef __XVIEWER_APPLICATION_ACTIVATABLE_H__
#define __XVIEWER_APPLICATION_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XVIEWER_TYPE_APPLICATION_ACTIVATABLE (xviewer_application_activatable_get_type ())
#define XVIEWER_APPLICATION_ACTIVATABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          XVIEWER_TYPE_APPLICATION_ACTIVATABLE, \
                                          XviewerApplicationActivatable))
#define XVIEWER_APPLICATION_ACTIVATABLE_IFACE(obj) \
                                          (G_TYPE_CHECK_CLASS_CAST ((obj), \
                                           XVIEWER_TYPE_APPLICATION_ACTIVATABLE, \
                                           XviewerApplicationActivatableInterface))
#define XVIEWER_IS_APPLICATION_ACTIVATABLE(obj) \
                                          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                           XVIEWER_TYPE_APPLICATION_ACTIVATABLE))
#define XVIEWER_APPLICATION_ACTIVATABLE_GET_IFACE(obj) \
                                        (G_TYPE_INSTANCE_GET_INTERFACE ((obj), \
                                         XVIEWER_TYPE_APPLICATION_ACTIVATABLE, \
                                         XviewerApplicationActivatableInterface))

typedef struct _XviewerApplicationActivatable		XviewerApplicationActivatable;
typedef struct _XviewerApplicationActivatableInterface	XviewerApplicationActivatableInterface;

struct _XviewerApplicationActivatableInterface
{
	GTypeInterface g_iface;

	/* vfuncs */

    void	(*activate)	(XviewerApplicationActivatable *activatable);
    void	(*deactivate)	(XviewerApplicationActivatable *activatable);
};

GType	xviewer_application_activatable_get_type     (void) G_GNUC_CONST;

void	xviewer_application_activatable_activate     (XviewerApplicationActivatable *activatable);
void	xviewer_application_activatable_deactivate   (XviewerApplicationActivatable *activatable);

G_END_DECLS
#endif /* __XVIEWER_APPLICATION_ACTIVATABLE_H__ */

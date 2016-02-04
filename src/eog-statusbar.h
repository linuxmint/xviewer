/* Xviewer - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnome.org>
 *	   Jens Finke <jens@gnome.org>
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

#ifndef __XVIEWER_STATUSBAR_H__
#define __XVIEWER_STATUSBAR_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XviewerStatusbar        XviewerStatusbar;
typedef struct _XviewerStatusbarPrivate XviewerStatusbarPrivate;
typedef struct _XviewerStatusbarClass   XviewerStatusbarClass;

#define XVIEWER_TYPE_STATUSBAR            (xviewer_statusbar_get_type ())
#define XVIEWER_STATUSBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_STATUSBAR, XviewerStatusbar))
#define XVIEWER_STATUSBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),   XVIEWER_TYPE_STATUSBAR, XviewerStatusbarClass))
#define XVIEWER_IS_STATUSBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_STATUSBAR))
#define XVIEWER_IS_STATUSBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_STATUSBAR))
#define XVIEWER_STATUSBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_STATUSBAR, XviewerStatusbarClass))

struct _XviewerStatusbar
{
        GtkStatusbar parent;

        XviewerStatusbarPrivate *priv;
};

struct _XviewerStatusbarClass
{
        GtkStatusbarClass parent_class;
};

GType		 xviewer_statusbar_get_type			(void) G_GNUC_CONST;

GtkWidget	*xviewer_statusbar_new			(void);

void		 xviewer_statusbar_set_image_number		(XviewerStatusbar   *statusbar,
							 gint           num,
							 gint           tot);

void		 xviewer_statusbar_set_progress		(XviewerStatusbar   *statusbar,
							 gdouble        progress);

G_END_DECLS

#endif /* __XVIEWER_STATUSBAR_H__ */

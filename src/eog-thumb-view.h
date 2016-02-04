/* Xviewer - Thumbnail View
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#ifndef XVIEWER_THUMB_VIEW_H
#define XVIEWER_THUMB_VIEW_H

#include "xviewer-image.h"
#include "xviewer-list-store.h"

G_BEGIN_DECLS

#define XVIEWER_TYPE_THUMB_VIEW            (xviewer_thumb_view_get_type ())
#define XVIEWER_THUMB_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_THUMB_VIEW, XviewerThumbView))
#define XVIEWER_THUMB_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_THUMB_VIEW, XviewerThumbViewClass))
#define XVIEWER_IS_THUMB_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_THUMB_VIEW))
#define XVIEWER_IS_THUMB_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_THUMB_VIEW))
#define XVIEWER_THUMB_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_THUMB_VIEW, XviewerThumbViewClass))

typedef struct _XviewerThumbView XviewerThumbView;
typedef struct _XviewerThumbViewClass XviewerThumbViewClass;
typedef struct _XviewerThumbViewPrivate XviewerThumbViewPrivate;

typedef enum {
	XVIEWER_THUMB_VIEW_SELECT_CURRENT = 0,
	XVIEWER_THUMB_VIEW_SELECT_LEFT,
	XVIEWER_THUMB_VIEW_SELECT_RIGHT,
	XVIEWER_THUMB_VIEW_SELECT_FIRST,
	XVIEWER_THUMB_VIEW_SELECT_LAST,
	XVIEWER_THUMB_VIEW_SELECT_RANDOM
} XviewerThumbViewSelectionChange;

struct _XviewerThumbView {
	GtkIconView icon_view;
	XviewerThumbViewPrivate *priv;
};

struct _XviewerThumbViewClass {
	 GtkIconViewClass icon_view_class;
};

GType       xviewer_thumb_view_get_type 		    (void) G_GNUC_CONST;

GtkWidget  *xviewer_thumb_view_new 			    (void);

void	    xviewer_thumb_view_set_model 		    (XviewerThumbView *thumbview,
						     XviewerListStore *store);

void        xviewer_thumb_view_set_item_height          (XviewerThumbView *thumbview,
						     gint          height);

guint	    xviewer_thumb_view_get_n_selected 	    (XviewerThumbView *thumbview);

XviewerImage   *xviewer_thumb_view_get_first_selected_image (XviewerThumbView *thumbview);

GList      *xviewer_thumb_view_get_selected_images 	    (XviewerThumbView *thumbview);

void        xviewer_thumb_view_select_single 	    (XviewerThumbView *thumbview,
						     XviewerThumbViewSelectionChange change);

void        xviewer_thumb_view_set_current_image	    (XviewerThumbView *thumbview,
						     XviewerImage     *image,
						     gboolean     deselect_other);

void        xviewer_thumb_view_set_thumbnail_popup      (XviewerThumbView *thumbview,
						     GtkMenu      *menu);

G_END_DECLS

#endif /* XVIEWER_THUMB_VIEW_H */

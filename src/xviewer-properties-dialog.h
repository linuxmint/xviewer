/* Xviewer - Image Properties Dialog
 *
 * Copyright (C) 2006 The Free Software Foundation
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

#ifndef __XVIEWER_PROPERTIES_DIALOG_H__
#define __XVIEWER_PROPERTIES_DIALOG_H__

#include "xviewer-image.h"
#include "xviewer-thumb-view.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XviewerPropertiesDialog XviewerPropertiesDialog;
typedef struct _XviewerPropertiesDialogClass XviewerPropertiesDialogClass;
typedef struct _XviewerPropertiesDialogPrivate XviewerPropertiesDialogPrivate;

#define XVIEWER_TYPE_PROPERTIES_DIALOG            (xviewer_properties_dialog_get_type ())
#define XVIEWER_PROPERTIES_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), XVIEWER_TYPE_PROPERTIES_DIALOG, XviewerPropertiesDialog))
#define XVIEWER_PROPERTIES_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  XVIEWER_TYPE_PROPERTIES_DIALOG, XviewerPropertiesDialogClass))
#define XVIEWER_IS_PROPERTIES_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), XVIEWER_TYPE_PROPERTIES_DIALOG))
#define XVIEWER_IS_PROPERTIES_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  XVIEWER_TYPE_PROPERTIES_DIALOG))
#define XVIEWER_PROPERTIES_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  XVIEWER_TYPE_PROPERTIES_DIALOG, XviewerPropertiesDialogClass))

typedef enum {
	XVIEWER_PROPERTIES_DIALOG_PAGE_GENERAL = 0,
	XVIEWER_PROPERTIES_DIALOG_PAGE_EXIF,
	XVIEWER_PROPERTIES_DIALOG_PAGE_DETAILS,
	XVIEWER_PROPERTIES_DIALOG_N_PAGES
} XviewerPropertiesDialogPage;

struct _XviewerPropertiesDialog {
	GtkDialog dialog;

	XviewerPropertiesDialogPrivate *priv;
};

struct _XviewerPropertiesDialogClass {
	GtkDialogClass parent_class;
};

GType	    xviewer_properties_dialog_get_type	(void) G_GNUC_CONST;

GtkWidget  *xviewer_properties_dialog_new		(GtkWindow               *parent,
						 XviewerThumbView            *thumbview,
						 GtkAction               *next_image_action,
						 GtkAction               *previous_image_action);

void	    xviewer_properties_dialog_update  	(XviewerPropertiesDialog     *prop,
						 XviewerImage                *image);

void	    xviewer_properties_dialog_set_page  	(XviewerPropertiesDialog     *prop,
						 XviewerPropertiesDialogPage  page);

void	    xviewer_properties_dialog_set_netbook_mode (XviewerPropertiesDialog *dlg,
						    gboolean enable);
G_END_DECLS

#endif /* __XVIEWER_PROPERTIES_DIALOG_H__ */

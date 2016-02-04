/*
 * xviewer-close-confirmation-dialog.h
 * This file is part of xviewer
 *
 * Author: Marcus Carlson <marcus@mejlamej.nu>
 *
 * Based on gedit code (gedit/gedit-close-confirmation.h) by gedit Team
 *
 * Copyright (C) 2004-2009 GNOME Foundation
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

#ifndef __XVIEWER_CLOSE_CONFIRMATION_DIALOG_H__
#define __XVIEWER_CLOSE_CONFIRMATION_DIALOG_H__

#include <glib.h>
#include <gtk/gtk.h>

#include <xviewer-image.h>

#define XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG		(xviewer_close_confirmation_dialog_get_type ())
#define XVIEWER_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG, XviewerCloseConfirmationDialog))
#define XVIEWER_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG, XviewerCloseConfirmationDialogClass))
#define XVIEWER_IS_CLOSE_CONFIRMATION_DIALOG(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define XVIEWER_IS_CLOSE_CONFIRMATION_DIALOG_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG))
#define XVIEWER_CLOSE_CONFIRMATION_DIALOG_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj),XVIEWER_TYPE_CLOSE_CONFIRMATION_DIALOG, XviewerCloseConfirmationDialogClass))

typedef struct _XviewerCloseConfirmationDialog 		XviewerCloseConfirmationDialog;
typedef struct _XviewerCloseConfirmationDialogClass 	XviewerCloseConfirmationDialogClass;
typedef struct _XviewerCloseConfirmationDialogPrivate 	XviewerCloseConfirmationDialogPrivate;

struct _XviewerCloseConfirmationDialog
{
	GtkDialog parent;

	/*< private > */
	XviewerCloseConfirmationDialogPrivate *priv;
};

struct _XviewerCloseConfirmationDialogClass
{
	GtkDialogClass parent_class;
};

/**
 * XviewerCloseConfirmationDialogResponseType:
 * @XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_NONE: Returned if the dialog has no response id,
 * or if the message area gets programmatically hidden or destroyed
 * @XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CLOSE: Returned by CLOSE button in the dialog
 * @XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CANCEL: Returned by CANCEL button in the dialog
 * @XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVE: Returned by SAVE button in the dialog
 * @XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVEAS: Returned by SAVE AS button in the dialog
 *
 */
typedef enum
{
	XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_NONE   = 0,
	XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CLOSE  = 1,
	XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CANCEL = 2,
	XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVE   = 3,
	XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVEAS = 4
} XviewerCloseConfirmationDialogResponseType;

G_GNUC_INTERNAL
GType 		 xviewer_close_confirmation_dialog_get_type		(void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget	*xviewer_close_confirmation_dialog_new			(GtkWindow     *parent,
									 GList         *unsaved_documents);
G_GNUC_INTERNAL
GtkWidget 	*xviewer_close_confirmation_dialog_new_single 		(GtkWindow     *parent,
									 XviewerImage      *image);

G_GNUC_INTERNAL
const GList	*xviewer_close_confirmation_dialog_get_unsaved_images	(XviewerCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
GList		*xviewer_close_confirmation_dialog_get_selected_images	(XviewerCloseConfirmationDialog *dlg);

G_GNUC_INTERNAL
void		 xviewer_close_confirmation_dialog_set_sensitive		(XviewerCloseConfirmationDialog *dlg, gboolean value);

#endif /* __XVIEWER_CLOSE_CONFIRMATION_DIALOG_H__ */


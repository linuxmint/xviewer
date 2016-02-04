/*
 * xviewer-clipboard-handler.h
 * This file is part of xviewer
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Copyright (C) 2010 GNOME Foundation
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

#ifndef __XVIEWER_CLIPBOARD_HANDLER_H__
#define __XVIEWER_CLIPBOARD_HANDLER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "xviewer-image.h"

G_BEGIN_DECLS

#define XVIEWER_TYPE_CLIPBOARD_HANDLER          (xviewer_clipboard_handler_get_type ())
#define XVIEWER_CLIPBOARD_HANDLER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_CLIPBOARD_HANDLER, XviewerClipboardHandler))
#define XVIEWER_CLIPBOARD_HANDLER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_CLIPBOARD_HANDLER, XviewerClipboardHandlerClass))
#define XVIEWER_IS_CLIPBOARD_HANDLER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_CLIPBOARD_HANDLER))
#define XVIEWER_IS_CLIPBOARD_HANDLER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_CLIPBOARD_HANDLER))
#define XVIEWER_CLIPBOARD_HANDLER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_CLIPBOARD_HANDLER, XviewerClipboardHandlerClass))

typedef struct _XviewerClipboardHandler XviewerClipboardHandler;
typedef struct _XviewerClipboardHandlerClass XviewerClipboardHandlerClass;
typedef struct _XviewerClipboardHandlerPrivate XviewerClipboardHandlerPrivate;

struct _XviewerClipboardHandler {
	GObject parent;

	XviewerClipboardHandlerPrivate *priv;
};

struct _XviewerClipboardHandlerClass {
	GObjectClass parent_klass;
};

GType xviewer_clipboard_handler_get_type (void) G_GNUC_CONST;

XviewerClipboardHandler* xviewer_clipboard_handler_new (XviewerImage *img);

void xviewer_clipboard_handler_copy_to_clipboard (XviewerClipboardHandler *handler,
					      GtkClipboard *clipboard);

G_END_DECLS
#endif /* __XVIEWER_CLIPBOARD_HANDLER_H__ */

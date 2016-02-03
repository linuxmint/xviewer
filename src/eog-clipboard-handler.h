/*
 * eog-clipboard-handler.h
 * This file is part of eog
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

#ifndef __EOG_CLIPBOARD_HANDLER_H__
#define __EOG_CLIPBOARD_HANDLER_H__

#include <glib-object.h>
#include <gtk/gtk.h>

#include "eog-image.h"

G_BEGIN_DECLS

#define EOG_TYPE_CLIPBOARD_HANDLER          (eog_clipboard_handler_get_type ())
#define EOG_CLIPBOARD_HANDLER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), EOG_TYPE_CLIPBOARD_HANDLER, EogClipboardHandler))
#define EOG_CLIPBOARD_HANDLER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), EOG_TYPE_CLIPBOARD_HANDLER, EogClipboardHandlerClass))
#define EOG_IS_CLIPBOARD_HANDLER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), EOG_TYPE_CLIPBOARD_HANDLER))
#define EOG_IS_CLIPBOARD_HANDLER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), EOG_TYPE_CLIPBOARD_HANDLER))
#define EOG_CLIPBOARD_HANDLER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), EOG_TYPE_CLIPBOARD_HANDLER, EogClipboardHandlerClass))

typedef struct _EogClipboardHandler EogClipboardHandler;
typedef struct _EogClipboardHandlerClass EogClipboardHandlerClass;
typedef struct _EogClipboardHandlerPrivate EogClipboardHandlerPrivate;

struct _EogClipboardHandler {
	GObject parent;

	EogClipboardHandlerPrivate *priv;
};

struct _EogClipboardHandlerClass {
	GObjectClass parent_klass;
};

GType eog_clipboard_handler_get_type (void) G_GNUC_CONST;

EogClipboardHandler* eog_clipboard_handler_new (EogImage *img);

void eog_clipboard_handler_copy_to_clipboard (EogClipboardHandler *handler,
					      GtkClipboard *clipboard);

G_END_DECLS
#endif /* __EOG_CLIPBOARD_HANDLER_H__ */

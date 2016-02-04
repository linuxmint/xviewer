/* Xviewer - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnome.org>
 *	- Jens Finke <jens@gnome.org>
 * Based on evince code (shell/ev-window.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#ifndef __XVIEWER_WINDOW_H__
#define __XVIEWER_WINDOW_H__

#include "xviewer-list-store.h"
#include "xviewer-image.h"

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _XviewerWindow XviewerWindow;
typedef struct _XviewerWindowClass XviewerWindowClass;
typedef struct _XviewerWindowPrivate XviewerWindowPrivate;

#define XVIEWER_TYPE_WINDOW            (xviewer_window_get_type ())
#define XVIEWER_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_WINDOW, XviewerWindow))
#define XVIEWER_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_WINDOW, XviewerWindowClass))
#define XVIEWER_IS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_WINDOW))
#define XVIEWER_IS_WINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_WINDOW))
#define XVIEWER_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_WINDOW, XviewerWindowClass))

#define XVIEWER_WINDOW_ERROR           (xviewer_window_error_quark ())

typedef enum {
	XVIEWER_WINDOW_MODE_UNKNOWN,
	XVIEWER_WINDOW_MODE_NORMAL,
	XVIEWER_WINDOW_MODE_FULLSCREEN,
	XVIEWER_WINDOW_MODE_SLIDESHOW
} XviewerWindowMode;

typedef enum {
	XVIEWER_WINDOW_GALLERY_POS_BOTTOM,
	XVIEWER_WINDOW_GALLERY_POS_LEFT,
	XVIEWER_WINDOW_GALLERY_POS_TOP,
	XVIEWER_WINDOW_GALLERY_POS_RIGHT
} XviewerWindowGalleryPos;

//TODO
typedef enum {
	XVIEWER_WINDOW_ERROR_CONTROL_NOT_FOUND,
	XVIEWER_WINDOW_ERROR_UI_NOT_FOUND,
	XVIEWER_WINDOW_ERROR_NO_PERSIST_FILE_INTERFACE,
	XVIEWER_WINDOW_ERROR_IO,
	XVIEWER_WINDOW_ERROR_TRASH_NOT_FOUND,
	XVIEWER_WINDOW_ERROR_GENERIC,
	XVIEWER_WINDOW_ERROR_UNKNOWN
} XviewerWindowError;

typedef enum {
	XVIEWER_STARTUP_FULLSCREEN         = 1 << 0,
	XVIEWER_STARTUP_SLIDE_SHOW         = 1 << 1,
	XVIEWER_STARTUP_DISABLE_GALLERY    = 1 << 2,
	XVIEWER_STARTUP_SINGLE_WINDOW      = 1 << 3
} XviewerStartupFlags;

struct _XviewerWindow {
	GtkApplicationWindow win;

	XviewerWindowPrivate *priv;
};

struct _XviewerWindowClass {
	GtkApplicationWindowClass parent_class;

	void (* prepared) (XviewerWindow *window);
};

GType         xviewer_window_get_type  	(void) G_GNUC_CONST;

GtkWidget    *xviewer_window_new		(XviewerStartupFlags  flags);

XviewerWindowMode xviewer_window_get_mode       (XviewerWindow       *window);

void          xviewer_window_set_mode       (XviewerWindow       *window,
					 XviewerWindowMode    mode);

GtkUIManager *xviewer_window_get_ui_manager (XviewerWindow       *window);

XviewerListStore *xviewer_window_get_store      (XviewerWindow       *window);

GtkWidget    *xviewer_window_get_view       (XviewerWindow       *window);

GtkWidget    *xviewer_window_get_sidebar    (XviewerWindow       *window);

GtkWidget    *xviewer_window_get_thumb_view (XviewerWindow       *window);

GtkWidget    *xviewer_window_get_thumb_nav  (XviewerWindow       *window);

GtkWidget    *xviewer_window_get_statusbar  (XviewerWindow       *window);

XviewerImage     *xviewer_window_get_image      (XviewerWindow       *window);

void          xviewer_window_open_file_list	(XviewerWindow       *window,
					 GSList          *file_list);

gboolean      xviewer_window_is_empty 	(XviewerWindow       *window);
gboolean      xviewer_window_is_not_initializing (const XviewerWindow *window);

void          xviewer_window_reload_image (XviewerWindow *window);
GtkWidget    *xviewer_window_get_properties_dialog (XviewerWindow *window);

void          xviewer_window_show_about_dialog (XviewerWindow    *window);
void          xviewer_window_show_preferences_dialog (XviewerWindow *window);

void          xviewer_window_close          (XviewerWindow *window);

G_END_DECLS

#endif

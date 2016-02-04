/* Xviewer - Image Store
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
 *
 * Based on code by: Jens Finke <jens@triq.net>
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

#ifndef XVIEWER_LIST_STORE_H
#define XVIEWER_LIST_STORE_H

#include <gtk/gtk.h>
#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#ifndef __XVIEWER_IMAGE_DECLR__
#define __XVIEWER_IMAGE_DECLR__
  typedef struct _XviewerImage XviewerImage;
#endif

typedef struct _XviewerListStore XviewerListStore;
typedef struct _XviewerListStoreClass XviewerListStoreClass;
typedef struct _XviewerListStorePrivate XviewerListStorePrivate;

#define XVIEWER_TYPE_LIST_STORE            xviewer_list_store_get_type()
#define XVIEWER_LIST_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_LIST_STORE, XviewerListStore))
#define XVIEWER_LIST_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_LIST_STORE, XviewerListStoreClass))
#define XVIEWER_IS_LIST_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_LIST_STORE))
#define XVIEWER_IS_LIST_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_LIST_STORE))
#define XVIEWER_LIST_STORE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_LIST_STORE, XviewerListStoreClass))

#define XVIEWER_LIST_STORE_THUMB_SIZE 90

typedef enum {
	XVIEWER_LIST_STORE_THUMBNAIL = 0,
	XVIEWER_LIST_STORE_THUMB_SET,
	XVIEWER_LIST_STORE_XVIEWER_IMAGE,
	XVIEWER_LIST_STORE_XVIEWER_JOB,
	XVIEWER_LIST_STORE_NUM_COLUMNS
} XviewerListStoreColumn;

struct _XviewerListStore {
        GtkListStore parent;
	XviewerListStorePrivate *priv;
};

struct _XviewerListStoreClass {
        GtkListStoreClass parent_class;

	/* Padding for future expansion */
	void (* _xviewer_reserved1) (void);
	void (* _xviewer_reserved2) (void);
	void (* _xviewer_reserved3) (void);
	void (* _xviewer_reserved4) (void);
};

GType           xviewer_list_store_get_type 	     (void) G_GNUC_CONST;

GtkListStore   *xviewer_list_store_new 		     (void);

GtkListStore   *xviewer_list_store_new_from_glist 	     (GList *list);

void            xviewer_list_store_append_image 	     (XviewerListStore *store,
						      XviewerImage     *image);

void            xviewer_list_store_add_files 	     (XviewerListStore *store,
						      GList        *file_list);

void            xviewer_list_store_remove_image 	     (XviewerListStore *store,
						      XviewerImage     *image);

gint            xviewer_list_store_get_pos_by_image      (XviewerListStore *store,
						      XviewerImage     *image);

XviewerImage       *xviewer_list_store_get_image_by_pos      (XviewerListStore *store,
						      gint   pos);

gint            xviewer_list_store_get_pos_by_iter 	     (XviewerListStore *store,
						      GtkTreeIter  *iter);

gint            xviewer_list_store_length                (XviewerListStore *store);

gint            xviewer_list_store_get_initial_pos 	     (XviewerListStore *store);

void            xviewer_list_store_thumbnail_set         (XviewerListStore *store,
						      GtkTreeIter *iter);

void            xviewer_list_store_thumbnail_unset       (XviewerListStore *store,
						      GtkTreeIter *iter);

void            xviewer_list_store_thumbnail_refresh     (XviewerListStore *store,
						      GtkTreeIter *iter);

G_END_DECLS

#endif

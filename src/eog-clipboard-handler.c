/*
 * xviewer-clipboard-handler.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include "xviewer-clipboard-handler.h"

enum {
	PROP_0,
	PROP_PIXBUF,
	PROP_URI
};

enum {
	TARGET_PIXBUF,
	TARGET_TEXT,
	TARGET_URI
};

struct _XviewerClipboardHandlerPrivate {
	GdkPixbuf *pixbuf;
	gchar     *uri;
};

G_DEFINE_TYPE_WITH_PRIVATE(XviewerClipboardHandler, xviewer_clipboard_handler, G_TYPE_INITIALLY_UNOWNED)

static GdkPixbuf*
xviewer_clipboard_handler_get_pixbuf (XviewerClipboardHandler *handler)
{
	g_return_val_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (handler), NULL);

	return handler->priv->pixbuf;
}

static const gchar *
xviewer_clipboard_handler_get_uri (XviewerClipboardHandler *handler)
{
	g_return_val_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (handler), NULL);

	return handler->priv->uri;
}

static void
xviewer_clipboard_handler_set_pixbuf (XviewerClipboardHandler *handler, GdkPixbuf *pixbuf)
{
	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (handler));
	g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

	if (handler->priv->pixbuf == pixbuf)
		return;
	
	if (handler->priv->pixbuf)
		g_object_unref (handler->priv->pixbuf);

	handler->priv->pixbuf = g_object_ref (pixbuf);

	g_object_notify (G_OBJECT (handler), "pixbuf");
}

static void
xviewer_clipboard_handler_set_uri (XviewerClipboardHandler *handler, const gchar *uri)
{
	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (handler));

	if (handler->priv->uri != NULL)
		g_free (handler->priv->uri);

	handler->priv->uri = g_strdup (uri);
	g_object_notify (G_OBJECT (handler), "uri");
}

static void
xviewer_clipboard_handler_get_property (GObject *object, guint property_id,
				    GValue *value, GParamSpec *pspec)
{
	XviewerClipboardHandler *handler;

	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (object));

	handler = XVIEWER_CLIPBOARD_HANDLER (object);

	switch (property_id) {
	case PROP_PIXBUF:
		g_value_set_object (value,
				    xviewer_clipboard_handler_get_pixbuf (handler));
		break;
	case PROP_URI:
		g_value_set_string (value,
				    xviewer_clipboard_handler_get_uri (handler));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
xviewer_clipboard_handler_set_property (GObject *object, guint property_id,
				    const GValue *value, GParamSpec *pspec)
{
	XviewerClipboardHandler *handler;

	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (object));

	handler = XVIEWER_CLIPBOARD_HANDLER (object);

	switch (property_id) {
	case PROP_PIXBUF:
	{
		GdkPixbuf *pixbuf;

		pixbuf = g_value_get_object (value);
		xviewer_clipboard_handler_set_pixbuf (handler, pixbuf);
		break;
	}
	case PROP_URI:
	{
		const gchar *uri;

		uri = g_value_get_string (value);
		xviewer_clipboard_handler_set_uri (handler, uri);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}

}

static void
xviewer_clipboard_handler_dispose (GObject *obj)
{
	XviewerClipboardHandlerPrivate *priv;

	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (obj));

	priv = XVIEWER_CLIPBOARD_HANDLER (obj)->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}
	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	G_OBJECT_CLASS (xviewer_clipboard_handler_parent_class)->dispose (obj);
}

static void
xviewer_clipboard_handler_init (XviewerClipboardHandler *handler)
{
	handler->priv = xviewer_clipboard_handler_get_instance_private (handler);
}

static void
xviewer_clipboard_handler_class_init (XviewerClipboardHandlerClass *klass)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (klass);

	g_obj_class->get_property = xviewer_clipboard_handler_get_property;
	g_obj_class->set_property = xviewer_clipboard_handler_set_property;
	g_obj_class->dispose = xviewer_clipboard_handler_dispose;

	g_object_class_install_property (
		g_obj_class, PROP_PIXBUF,
		g_param_spec_object ("pixbuf", NULL, NULL, GDK_TYPE_PIXBUF,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (
		g_obj_class, PROP_URI,
		g_param_spec_string ("uri", NULL, NULL, NULL,
				     G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY |
				     G_PARAM_STATIC_STRINGS));
}

XviewerClipboardHandler*
xviewer_clipboard_handler_new (XviewerImage *img)
{
	GObject *obj;
	GFile *file;
	GdkPixbuf *pbuf;
	gchar *uri;

	g_object_ref (img);
	pbuf = xviewer_image_get_pixbuf (img);
	file = xviewer_image_get_file (img);
	uri = g_file_get_uri (file);
	obj = g_object_new (XVIEWER_TYPE_CLIPBOARD_HANDLER,
			    "pixbuf", pbuf,
			    "uri", uri,
			    NULL);
	g_free (uri);
	g_object_unref (file);
	g_object_unref (pbuf);
	g_object_unref (img);

	return XVIEWER_CLIPBOARD_HANDLER (obj);

}

static void
xviewer_clipboard_handler_get_func (GtkClipboard *clipboard,
				GtkSelectionData *selection,
				guint info, gpointer owner)
{
	XviewerClipboardHandler *handler;

	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (owner));

	handler = XVIEWER_CLIPBOARD_HANDLER (owner);

	switch (info) {
	case TARGET_PIXBUF:
	{
		GdkPixbuf *pixbuf = xviewer_clipboard_handler_get_pixbuf (handler);
		g_object_ref (pixbuf);
		gtk_selection_data_set_pixbuf (selection, pixbuf);
		g_object_unref (pixbuf);
		break;
	}
	case TARGET_TEXT:
	{
		gtk_selection_data_set_text (selection,
					     xviewer_clipboard_handler_get_uri (handler),
					     -1);
		break;
	}
	case TARGET_URI:
	{
		gchar *uris[2];
		uris[0] = g_strdup (xviewer_clipboard_handler_get_uri (handler));
		uris[1] = NULL;

		gtk_selection_data_set_uris (selection, uris);
		g_free (uris[0]);
		break;
	}
	default:
		g_return_if_reached ();
	}

}

static void
xviewer_clipboard_handler_clear_func (GtkClipboard *clipboard, gpointer owner)
{
	g_return_if_fail (XVIEWER_IS_CLIPBOARD_HANDLER (owner));

	g_object_unref (G_OBJECT (owner));
}

void
xviewer_clipboard_handler_copy_to_clipboard (XviewerClipboardHandler *handler,
					 GtkClipboard *clipboard)
{
	GtkTargetList *tlist;
	GtkTargetEntry *targets;
	gint n_targets = 0;
	gboolean set = FALSE;

	tlist = gtk_target_list_new (NULL, 0);

	if (handler->priv->pixbuf != NULL)
		gtk_target_list_add_image_targets (tlist, TARGET_PIXBUF, TRUE);

	if (handler->priv->uri != NULL) {
		gtk_target_list_add_text_targets (tlist, TARGET_TEXT);
		gtk_target_list_add_uri_targets (tlist, TARGET_URI);
	}

	targets = gtk_target_table_new_from_list (tlist, &n_targets);

	// We need to take ownership here if nobody else did
	g_object_ref_sink (handler);

	if (n_targets > 0) {
		set = gtk_clipboard_set_with_owner (clipboard,
						    targets, n_targets,
						    xviewer_clipboard_handler_get_func,
						    xviewer_clipboard_handler_clear_func,
						    G_OBJECT (handler));

	} 
	
	if (!set) {
		gtk_clipboard_clear (clipboard);
		g_object_unref (handler);
	}

	gtk_target_table_free (targets, n_targets);
	gtk_target_list_unref (tlist);
}


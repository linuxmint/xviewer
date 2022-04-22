/* Xviewer - Thumbnail View
 *
 * Copyright (C) 2006-2008 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xviewer-thumb-view.h"
#include "xviewer-list-store.h"
#include "xviewer-image.h"
#include "xviewer-job-scheduler.h"

#ifdef HAVE_EXIF
#include "xviewer-exif-util.h"
#include <libexif/exif-data.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

enum {
  PROP_0,
  PROP_ORIENTATION
};

#define XVIEWER_THUMB_VIEW_SPACING 0

static void xviewer_thumb_view_init (XviewerThumbView *thumbview);

static XviewerImage* xviewer_thumb_view_get_image_from_path (XviewerThumbView      *thumbview,
						     GtkTreePath       *path);

static void      xviewer_thumb_view_popup_menu          (XviewerThumbView      *widget,
						     GdkEventButton    *event);
static void      xviewer_thumb_view_update_columns      (XviewerThumbView *view);

static gboolean
thumbview_on_query_tooltip_cb (GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       GtkTooltip *tooltip,
			       gpointer    user_data);
static void
thumbview_on_parent_set_cb (GtkWidget *widget,
			    GtkWidget *old_parent,
			    gpointer   user_data);

static void
thumbview_on_drag_data_get_cb (GtkWidget        *widget,
			       GdkDragContext   *drag_context,
			       GtkSelectionData *data,
			       guint             info,
			       guint             time,
			       gpointer          user_data);
struct _XviewerThumbViewPrivate {
	gint start_thumb; /* the first visible thumbnail */
	gint end_thumb;   /* the last visible thumbnail  */
	GtkWidget *menu;  /* a contextual menu for thumbnails */
	GtkCellRenderer *pixbuf_cell;
	gint visible_range_changed_id;

	GtkOrientation orientation;
	gint n_images;
	gulong image_add_id;
	gulong image_removed_id;
};

G_DEFINE_TYPE_WITH_CODE (XviewerThumbView, xviewer_thumb_view, GTK_TYPE_ICON_VIEW,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE, NULL) \
			 G_ADD_PRIVATE (XviewerThumbView));

/* Drag 'n Drop */

static void
xviewer_thumb_view_constructed (GObject *object)
{
	XviewerThumbView *thumbview;

	if (G_OBJECT_CLASS (xviewer_thumb_view_parent_class)->constructed)
		G_OBJECT_CLASS (xviewer_thumb_view_parent_class)->constructed (object);

	thumbview = XVIEWER_THUMB_VIEW (object);

	thumbview->priv->pixbuf_cell = gtk_cell_renderer_pixbuf_new ();

	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (thumbview),
				    thumbview->priv->pixbuf_cell,
				    FALSE);

	g_object_set (thumbview->priv->pixbuf_cell,
	              "follow-state", FALSE,
	              "height", 100,
	              "width", 115,
	              "yalign", 0.5,
	              "xalign", 0.5,
	              NULL);

	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (thumbview),
					thumbview->priv->pixbuf_cell,
					"pixbuf", XVIEWER_LIST_STORE_THUMBNAIL,
					NULL);

	gtk_icon_view_set_selection_mode (GTK_ICON_VIEW (thumbview),
					  GTK_SELECTION_MULTIPLE);

	gtk_icon_view_set_column_spacing (GTK_ICON_VIEW (thumbview),
					  XVIEWER_THUMB_VIEW_SPACING);

	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (thumbview),
				       XVIEWER_THUMB_VIEW_SPACING);

	g_object_set (thumbview, "has-tooltip", TRUE, NULL);

	g_signal_connect (thumbview,
			  "query-tooltip",
			  G_CALLBACK (thumbview_on_query_tooltip_cb),
			  NULL);

	thumbview->priv->start_thumb = 0;
	thumbview->priv->end_thumb = 0;
	thumbview->priv->menu = NULL;

	g_signal_connect (G_OBJECT (thumbview), "parent-set",
			  G_CALLBACK (thumbview_on_parent_set_cb), NULL);

	gtk_icon_view_enable_model_drag_source (GTK_ICON_VIEW (thumbview), 0,
						NULL, 0,
						GDK_ACTION_COPY |
						GDK_ACTION_MOVE |
						GDK_ACTION_LINK |
						GDK_ACTION_ASK);
	gtk_drag_source_add_uri_targets (GTK_WIDGET (thumbview));

	g_signal_connect (G_OBJECT (thumbview), "drag-data-get",
			  G_CALLBACK (thumbview_on_drag_data_get_cb), NULL);
}

static void
xviewer_thumb_view_dispose (GObject *object)
{
	XviewerThumbViewPrivate *priv = XVIEWER_THUMB_VIEW (object)->priv;
	GtkTreeModel *model;

	if (priv->visible_range_changed_id != 0) {
		g_source_remove (priv->visible_range_changed_id);
		priv->visible_range_changed_id = 0;
	}

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (object));

	if (model && priv->image_add_id != 0) {
		g_signal_handler_disconnect (model, priv->image_add_id);
		priv->image_add_id = 0;
	}

	if (model && priv->image_removed_id) {
		g_signal_handler_disconnect (model, priv->image_removed_id);
		priv->image_removed_id = 0;
	}

	G_OBJECT_CLASS (xviewer_thumb_view_parent_class)->dispose (object);
}

static void
xviewer_thumb_view_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	XviewerThumbView *view = XVIEWER_THUMB_VIEW (object);

	switch (prop_id)
	{
	case PROP_ORIENTATION:
		g_value_set_enum (value, view->priv->orientation);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xviewer_thumb_view_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	XviewerThumbView *view = XVIEWER_THUMB_VIEW (object);

	switch (prop_id)
	{
	case PROP_ORIENTATION:
		view->priv->orientation = g_value_get_enum (value);
		xviewer_thumb_view_update_columns (view);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
xviewer_thumb_view_class_init (XviewerThumbViewClass *class)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (class);

	gobject_class->constructed = xviewer_thumb_view_constructed;
	gobject_class->dispose = xviewer_thumb_view_dispose;
	gobject_class->get_property = xviewer_thumb_view_get_property;
	gobject_class->set_property = xviewer_thumb_view_set_property;

	g_object_class_override_property (gobject_class, PROP_ORIENTATION,
	                                  "orientation");
}

static void
xviewer_thumb_view_clear_range (XviewerThumbView *thumbview,
			    const gint start_thumb,
			    const gint end_thumb)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	XviewerListStore *store = XVIEWER_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = gtk_tree_path_new_from_indices (start_thumb, -1);
	for (result = gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter), thumb++) {
		xviewer_list_store_thumbnail_unset (store, &iter);
	}
	gtk_tree_path_free (path);
}

static void
xviewer_thumb_view_add_range (XviewerThumbView *thumbview,
			  const gint start_thumb,
			  const gint end_thumb)
{
	GtkTreePath *path;
	GtkTreeIter iter;
	XviewerListStore *store = XVIEWER_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	gint thumb = start_thumb;
	gboolean result;

	g_assert (start_thumb <= end_thumb);

	path = gtk_tree_path_new_from_indices (start_thumb, -1);
	for (result = gtk_tree_model_get_iter (GTK_TREE_MODEL (store), &iter, path);
	     result && thumb <= end_thumb;
	     result = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter), thumb++) {
		xviewer_list_store_thumbnail_set (store, &iter);
	}
	gtk_tree_path_free (path);
}

static void
xviewer_thumb_view_update_visible_range (XviewerThumbView *thumbview,
				     const gint start_thumb,
				     const gint end_thumb)
{
	XviewerThumbViewPrivate *priv = thumbview->priv;
	int old_start_thumb, old_end_thumb;

	old_start_thumb= priv->start_thumb;
	old_end_thumb = priv->end_thumb;

	if (start_thumb == old_start_thumb &&
	    end_thumb == old_end_thumb) {
		return;
	}

	if (old_start_thumb < start_thumb)
		xviewer_thumb_view_clear_range (thumbview, old_start_thumb, MIN (start_thumb - 1, old_end_thumb));

	if (old_end_thumb > end_thumb)
		xviewer_thumb_view_clear_range (thumbview, MAX (end_thumb + 1, old_start_thumb), old_end_thumb);

	xviewer_thumb_view_add_range (thumbview, start_thumb, end_thumb);

	priv->start_thumb = start_thumb;
	priv->end_thumb = end_thumb;
}

static gboolean
visible_range_changed_cb (XviewerThumbView *thumbview)
{
	GtkTreePath *path1, *path2;

	thumbview->priv->visible_range_changed_id = 0;

	if (!gtk_icon_view_get_visible_range (GTK_ICON_VIEW (thumbview), &path1, &path2)) {
		return FALSE;
	}

	if (path1 == NULL) {
		path1 = gtk_tree_path_new_first ();
	}
	if (path2 == NULL) {
		gint n_items = gtk_tree_model_iter_n_children (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)), NULL);
		path2 = gtk_tree_path_new_from_indices (n_items - 1 , -1);
	}

	xviewer_thumb_view_update_visible_range (thumbview, gtk_tree_path_get_indices (path1) [0],
					     gtk_tree_path_get_indices (path2) [0]);

	gtk_tree_path_free (path1);
	gtk_tree_path_free (path2);

	return FALSE;
}

static void
xviewer_thumb_view_visible_range_changed (XviewerThumbView *thumbview)
{
	if (thumbview->priv->visible_range_changed_id == 0) {
		g_idle_add ((GSourceFunc)visible_range_changed_cb, thumbview);
	}
}

static void
thumbview_on_visible_range_changed_cb (XviewerThumbView *thumbview,
				       gpointer user_data)
{
	xviewer_thumb_view_visible_range_changed (thumbview);
}

static void
thumbview_on_adjustment_changed_cb (XviewerThumbView *thumbview,
				    gpointer user_data)
{
	xviewer_thumb_view_visible_range_changed (thumbview);
}

static void
thumbview_on_parent_set_cb (GtkWidget *widget,
			    GtkWidget *old_parent,
			    gpointer   user_data)
{
	XviewerThumbView *thumbview = XVIEWER_THUMB_VIEW (widget);
	GtkScrolledWindow *sw;
	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;

	GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (thumbview));
	if (!GTK_IS_SCROLLED_WINDOW (parent)) {
		return;
	}

	/* if we have been set to a ScrolledWindow, we connect to the callback
	   to set and unset thumbnails. */
	sw = GTK_SCROLLED_WINDOW (parent);
	hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (sw));
	vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (sw));

	/* when scrolling */
	g_signal_connect_data (G_OBJECT (hadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "value-changed",
			       G_CALLBACK (thumbview_on_visible_range_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when the adjustment is changed, ie. probably we have new images added. */
	g_signal_connect_data (G_OBJECT (hadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
	g_signal_connect_data (G_OBJECT (vadjustment), "changed",
			       G_CALLBACK (thumbview_on_adjustment_changed_cb),
			       thumbview, NULL, G_CONNECT_SWAPPED | G_CONNECT_AFTER);

	/* when resizing the scrolled window */
	g_signal_connect_swapped (G_OBJECT (sw), "size-allocate",
				  G_CALLBACK (thumbview_on_visible_range_changed_cb),
				  thumbview);
}

static gboolean
thumbview_on_button_press_event_cb (GtkWidget *thumbview, GdkEventButton *event,
				    gpointer user_data)
{
	GtkTreePath *path;

	/* Ignore double-clicks and triple-clicks */
	if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
	{
		path = gtk_icon_view_get_path_at_pos (GTK_ICON_VIEW (thumbview),
						      (gint) event->x, (gint) event->y);
		if (path == NULL) {
			return FALSE;
		}

		if (!gtk_icon_view_path_is_selected (GTK_ICON_VIEW (thumbview), path) ||
		    xviewer_thumb_view_get_n_selected (XVIEWER_THUMB_VIEW (thumbview)) != 1) {
			gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));
			gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
			gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		}
		xviewer_thumb_view_popup_menu (XVIEWER_THUMB_VIEW (thumbview), event);

		gtk_tree_path_free (path);

		return TRUE;
	}

	return FALSE;
}

static void
thumbview_on_drag_data_get_cb (GtkWidget        *widget,
			       GdkDragContext   *drag_context,
			       GtkSelectionData *data,
			       guint             info,
			       guint             time,
			       gpointer          user_data)
{
	GList *list;
	GList *node;
	XviewerImage *image;
	GFile *file;
	gchar **uris = NULL;
	gint i = 0, n_images;

	list = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (widget));
	n_images = xviewer_thumb_view_get_n_selected (XVIEWER_THUMB_VIEW (widget));

	uris = g_new (gchar *, n_images + 1);

	for (node = list; node != NULL; node = node->next, i++) {
		image = XVIEWER_IMAGE (node->data);
		file = xviewer_image_get_file (image);
		uris[i] = g_file_get_uri (file);
		g_object_unref (image);
		g_object_unref (file);
	}
	uris[i] = NULL;

	gtk_selection_data_set_uris (data, uris);
	g_strfreev (uris);
	g_list_free (list);
}

static gchar *
thumbview_get_tooltip_string (XviewerImage *image)
{
	gchar *bytes;
	char *type_str;
	gint width, height;
	GFile *file;
	GFileInfo *file_info;
	const char *mime_str;
	gchar *tooltip_string;
#ifdef HAVE_EXIF
	ExifData *exif_data;
#endif

	bytes = g_format_size (xviewer_image_get_bytes (image));

	xviewer_image_get_size (image, &width, &height);

	file = xviewer_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	g_object_unref (file);
	if (file_info == NULL) {
		g_free(bytes);
		return NULL;
	}

	mime_str = g_file_info_get_content_type (file_info);

	if (G_UNLIKELY (mime_str == NULL)) {
		g_free (bytes);
		g_object_unref (image);
		return NULL;
	}

	type_str = g_content_type_get_description (mime_str);
	g_object_unref (file_info);

	if (width > -1 && height > -1) {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%i x %i %s\n"
							  "%s\n"
							  "%s",
							  xviewer_image_get_caption (image),
							  width,
							  height,
							  ngettext ("pixel",
								    "pixels",
								    height),
							  bytes,
							  type_str);
	} else {
		tooltip_string = g_markup_printf_escaped ("<b><big>%s</big></b>\n"
							  "%s\n"
							  "%s",
							  xviewer_image_get_caption (image),
							  bytes,
							  type_str);

	}

#ifdef HAVE_EXIF
	exif_data = (ExifData *) xviewer_image_get_exif_info (image);

	if (exif_data) {
		gchar *extra_info, *tmp, *date;
		/* The EXIF standard says that the DATE_TIME tag is
		 * 20 bytes long. A 32-byte buffer should be large enough. */
		gchar time_buffer[32];

		date = xviewer_exif_util_format_date (
			xviewer_exif_data_get_value (exif_data, EXIF_TAG_DATE_TIME_ORIGINAL, time_buffer, 32));

		if (date) {
			extra_info = g_strdup_printf ("\n%s %s", _("Taken on"), date);

			tmp = g_strconcat (tooltip_string, extra_info, NULL);

			g_free (date);
			g_free (extra_info);
			g_free (tooltip_string);

			tooltip_string = tmp;
		}
		exif_data_unref (exif_data);
	}
#endif

	g_free (type_str);
	g_free (bytes);

	return tooltip_string;
}

static void
on_data_loaded_cb (XviewerJob *job, gpointer data)
{
	if (!job->error) {
		gtk_tooltip_trigger_tooltip_query (gdk_display_get_default());
	}
}

static gboolean
thumbview_on_query_tooltip_cb (GtkWidget  *widget,
			       gint        x,
			       gint        y,
			       gboolean    keyboard_mode,
			       GtkTooltip *tooltip,
			       gpointer    user_data)
{
	GtkTreePath *path;
	XviewerImage *image;
	gchar *tooltip_string;
	XviewerImageData data = 0;

	if (!gtk_icon_view_get_tooltip_context (GTK_ICON_VIEW (widget),
						&x, &y, keyboard_mode,
						NULL, &path, NULL)) {
		return FALSE;
	}

	image = xviewer_thumb_view_get_image_from_path (XVIEWER_THUMB_VIEW (widget),
						    path);
	gtk_tree_path_free (path);

	if (image == NULL) {
		return FALSE;
	}

	if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_EXIF) &&
            xviewer_image_get_metadata_status (image) == XVIEWER_IMAGE_METADATA_NOT_READ) {
		data = XVIEWER_IMAGE_DATA_EXIF;
	}

	if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_DIMENSION)) {
		data |= XVIEWER_IMAGE_DATA_DIMENSION;
	}

	if (data) {
		XviewerJob *job;

		job = xviewer_job_load_new (image, data);
		g_signal_connect (G_OBJECT (job), "finished",
				  G_CALLBACK (on_data_loaded_cb),
				  widget);
		xviewer_job_scheduler_add_job (job);
		g_object_unref (image);
		g_object_unref (job);
		return FALSE;
	}

	tooltip_string = thumbview_get_tooltip_string (image);
	g_object_unref (image);

	if (tooltip_string == NULL) {
		return FALSE;
	}

	gtk_tooltip_set_markup (tooltip, tooltip_string);
	g_free (tooltip_string);

	return TRUE;
}

static void
xviewer_thumb_view_init (XviewerThumbView *thumbview)
{
	thumbview->priv = xviewer_thumb_view_get_instance_private (thumbview);

	thumbview->priv->visible_range_changed_id = 0;
	thumbview->priv->image_add_id = 0;
	thumbview->priv->image_removed_id = 0;

}

/**
 * xviewer_thumb_view_new:
 *
 * Creates a new #XviewerThumbView object.
 *
 * Returns: a newly created #XviewerThumbView.
 **/
GtkWidget *
xviewer_thumb_view_new (void)
{
	XviewerThumbView *thumbview;

	thumbview = g_object_new (XVIEWER_TYPE_THUMB_VIEW, NULL);

	return GTK_WIDGET (thumbview);
}

static void
xviewer_thumb_view_update_columns (XviewerThumbView *view)
{
	XviewerThumbViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_THUMB_VIEW (view));

	priv = view->priv;

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
			gtk_icon_view_set_columns (GTK_ICON_VIEW (view),
			                           priv->n_images);
}

static void
xviewer_thumb_view_row_inserted_cb (GtkTreeModel    *tree_model,
                                GtkTreePath     *path,
                                GtkTreeIter     *iter,
                                XviewerThumbView    *view)
{
	XviewerThumbViewPrivate *priv = view->priv;

	priv->n_images++;
	xviewer_thumb_view_update_columns (view);
}

static void
xviewer_thumb_view_row_deleted_cb (GtkTreeModel    *tree_model,
                               GtkTreePath     *path,
                               XviewerThumbView    *view)
{
	XviewerThumbViewPrivate *priv = view->priv;

	priv->n_images--;
	xviewer_thumb_view_update_columns (view);
}

/**
 * xviewer_thumb_view_set_model:
 * @thumbview: A #XviewerThumbView.
 * @store: A #XviewerListStore.
 *
 * Sets the #XviewerListStore to be used with @thumbview. If an initial image
 * was set during @store creation, its thumbnail will be selected and visible.
 *
 **/
void
xviewer_thumb_view_set_model (XviewerThumbView *thumbview, XviewerListStore *store)
{
	gint index;
	XviewerThumbViewPrivate *priv;
	GtkTreeModel *existing;

	g_return_if_fail (XVIEWER_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (XVIEWER_IS_LIST_STORE (store));

	priv = thumbview->priv;

	existing = gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview));

	if (existing != NULL) {
		if (priv->image_add_id != 0) {
			g_signal_handler_disconnect (existing,
			                             priv->image_add_id);
		}
		if (priv->image_removed_id != 0) {
			g_signal_handler_disconnect (existing,
			                             priv->image_removed_id);

		}
	}

	priv->image_add_id = g_signal_connect (G_OBJECT (store), "row-inserted",
	                            G_CALLBACK (xviewer_thumb_view_row_inserted_cb),
	                            thumbview);
	priv->image_removed_id = g_signal_connect (G_OBJECT (store),
	                             "row-deleted",
	                             G_CALLBACK (xviewer_thumb_view_row_deleted_cb),
	                             thumbview);

	thumbview->priv->n_images = xviewer_list_store_length (store);

	index = xviewer_list_store_get_initial_pos (store);

	gtk_icon_view_set_model (GTK_ICON_VIEW (thumbview),
	                         GTK_TREE_MODEL (store));

	xviewer_thumb_view_update_columns (thumbview);

	if (index >= 0) {
		GtkTreePath *path = gtk_tree_path_new_from_indices (index, -1);
		gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
		gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
		gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
		gtk_tree_path_free (path);
	}
}

/**
 * xviewer_thumb_view_set_item_height:
 * @thumbview: A #XviewerThumbView.
 * @height: The desired height.
 *
 * Sets the height of each thumbnail in @thumbview.
 *
 **/
void
xviewer_thumb_view_set_item_height (XviewerThumbView *thumbview, gint height)
{
	g_return_if_fail (XVIEWER_IS_THUMB_VIEW (thumbview));

	g_object_set (thumbview->priv->pixbuf_cell,
	              "height", height,
	              NULL);
}

static void
xviewer_thumb_view_get_n_selected_helper (GtkIconView *thumbview,
				      GtkTreePath *path,
				      gpointer data)
{
	/* data is of type (guint *) */
	(*(guint *) data) ++;
}

/**
 * xviewer_thumb_view_get_n_selected:
 * @thumbview: An #XviewerThumbView.
 *
 * Gets the number of images that are currently selected in @thumbview.
 *
 * Returns: the number of selected images in @thumbview.
 **/
guint
xviewer_thumb_view_get_n_selected (XviewerThumbView *thumbview)
{
	guint count = 0;
	gtk_icon_view_selected_foreach (GTK_ICON_VIEW (thumbview),
					xviewer_thumb_view_get_n_selected_helper,
					(&count));
	return count;
}

/**
 * xviewer_thumb_view_get_image_from_path:
 * @thumbview: A #XviewerThumbView.
 * @path: A #GtkTreePath pointing to a #XviewerImage in the model for @thumbview.
 *
 * Gets the #XviewerImage stored in @thumbview's #XviewerListStore at the position indicated
 * by @path.
 *
 * Returns: A #XviewerImage.
 **/
static XviewerImage *
xviewer_thumb_view_get_image_from_path (XviewerThumbView *thumbview, GtkTreePath *path)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	XviewerImage *image;

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview));
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter,
			    XVIEWER_LIST_STORE_XVIEWER_IMAGE, &image,
			    -1);

	return image;
}

/**
 * xviewer_thumb_view_get_first_selected_image:
 * @thumbview: A #XviewerThumbView.
 *
 * Returns the first selected image. Note that the returned #XviewerImage
 * is not ensured to be really the first selected image in @thumbview, but
 * generally, it will be.
 *
 * Returns: (transfer full): A #XviewerImage.
 **/
XviewerImage *
xviewer_thumb_view_get_first_selected_image (XviewerThumbView *thumbview)
{
	/* The returned list is not sorted! We need to find the
	   smaller tree path value => tricky and expensive. Do we really need this?
	*/
	XviewerImage *image;
	GtkTreePath *path;
	GList *list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));

	if (list == NULL) {
		return NULL;
	}

	path = (GtkTreePath *) (list->data);

	image = xviewer_thumb_view_get_image_from_path (thumbview, path);

	g_list_foreach (list, (GFunc) gtk_tree_path_free , NULL);
	g_list_free (list);

	return image;
}

/**
 * xviewer_thumb_view_get_selected_images:
 * @thumbview: A #XviewerThumbView.
 *
 * Gets a list with the currently selected images. Note that a new reference is
 * hold for each image and the list must be freed with g_list_free().
 *
 * Returns: (element-type XviewerImage) (transfer full): A newly allocated list of #XviewerImage's.
 **/
GList *
xviewer_thumb_view_get_selected_images (XviewerThumbView *thumbview)
{
	GList *l, *item;
	GList *list = NULL;

	GtkTreePath *path;

	l = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));

	for (item = l; item != NULL; item = item->next) {
		path = (GtkTreePath *) item->data;
		list = g_list_prepend (list, xviewer_thumb_view_get_image_from_path (thumbview, path));
		gtk_tree_path_free (path);
	}

	g_list_free (l);
	list = g_list_reverse (list);

	return list;
}

/**
 * xviewer_thumb_view_set_current_image:
 * @thumbview: A #XviewerThumbView.
 * @image: The image to be selected.
 * @deselect_other: Whether to deselect currently selected images.
 *
 * Changes the status of a image, marking it as currently selected.
 * If @deselect_other is %TRUE, all other selected images will be
 * deselected.
 *
 **/
void
xviewer_thumb_view_set_current_image (XviewerThumbView *thumbview, XviewerImage *image,
				  gboolean deselect_other)
{
	GtkTreePath *path;
	XviewerListStore *store;
	gint pos;

	store = XVIEWER_LIST_STORE (gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview)));
	pos = xviewer_list_store_get_pos_by_image (store, image);
	path = gtk_tree_path_new_from_indices (pos, -1);

	if (path == NULL) {
		return;
	}

	if (deselect_other) {
		gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));
	}

	gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);

	gtk_tree_path_free (path);
}

/**
 * xviewer_thumb_view_select_single:
 * @thumbview: A #XviewerThumbView.
 * @change: A #XviewerThumbViewSelectionChange, describing the
 * desired selection change.
 *
 * Changes the current selection according to a single movement
 * described by #XviewerThumbViewSelectionChange. If there are no
 * thumbnails currently selected, one is selected according to the
 * natural selection according to the #XviewerThumbViewSelectionChange
 * used, p.g., when %XVIEWER_THUMB_VIEW_SELECT_RIGHT is the selected change,
 * the first thumbnail will be selected.
 *
 **/
void
xviewer_thumb_view_select_single (XviewerThumbView *thumbview,
			      XviewerThumbViewSelectionChange change)
{
  	GtkTreePath *path = NULL;
	GtkTreeModel *model;
	GList *list;
	gint n_items;

	g_return_if_fail (XVIEWER_IS_THUMB_VIEW (thumbview));

	model = gtk_icon_view_get_model (GTK_ICON_VIEW (thumbview));

	n_items = xviewer_list_store_length (XVIEWER_LIST_STORE (model));

	if (n_items == 0) {
		return;
	}

	if (xviewer_thumb_view_get_n_selected (thumbview) == 0) {
		switch (change) {
		case XVIEWER_THUMB_VIEW_SELECT_CURRENT:
			break;
		case XVIEWER_THUMB_VIEW_SELECT_RIGHT:
		case XVIEWER_THUMB_VIEW_SELECT_FIRST:
			path = gtk_tree_path_new_first ();
			break;
		case XVIEWER_THUMB_VIEW_SELECT_LEFT:
		case XVIEWER_THUMB_VIEW_SELECT_LAST:
			path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case XVIEWER_THUMB_VIEW_SELECT_RANDOM:
			path = gtk_tree_path_new_from_indices (g_random_int_range (0, n_items), -1);
			break;
		}
	} else {
		list = gtk_icon_view_get_selected_items (GTK_ICON_VIEW (thumbview));
		path = gtk_tree_path_copy ((GtkTreePath *) list->data);
		g_list_foreach (list, (GFunc) gtk_tree_path_free , NULL);
		g_list_free (list);

		gtk_icon_view_unselect_all (GTK_ICON_VIEW (thumbview));

		switch (change) {
		case XVIEWER_THUMB_VIEW_SELECT_CURRENT:
			break;
		case XVIEWER_THUMB_VIEW_SELECT_LEFT:
			if (!gtk_tree_path_prev (path)) {
				gtk_tree_path_free (path);
				path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			}
			break;
		case XVIEWER_THUMB_VIEW_SELECT_RIGHT:
			if (gtk_tree_path_get_indices (path) [0] == n_items - 1) {
				gtk_tree_path_free (path);
				path = gtk_tree_path_new_first ();
			} else {
				gtk_tree_path_next (path);
			}
			break;
		case XVIEWER_THUMB_VIEW_SELECT_FIRST:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_first ();
			break;
		case XVIEWER_THUMB_VIEW_SELECT_LAST:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_from_indices (n_items - 1, -1);
			break;
		case XVIEWER_THUMB_VIEW_SELECT_RANDOM:
			gtk_tree_path_free (path);
			path = gtk_tree_path_new_from_indices (g_random_int_range (0, n_items), -1);
			break;
		}
	}

	gtk_icon_view_select_path (GTK_ICON_VIEW (thumbview), path);
	gtk_icon_view_set_cursor (GTK_ICON_VIEW (thumbview), path, NULL, FALSE);
	gtk_icon_view_scroll_to_path (GTK_ICON_VIEW (thumbview), path, FALSE, 0, 0);
	gtk_tree_path_free (path);
}


/**
 * xviewer_thumb_view_set_thumbnail_popup:
 * @thumbview: An #XviewerThumbView.
 * @menu: A #GtkMenu.
 *
 * Set the contextual menu to be used with the thumbnails in the
 * widget. This can be done only once.
 *
 **/
void
xviewer_thumb_view_set_thumbnail_popup (XviewerThumbView *thumbview,
				    GtkMenu      *menu)
{
	g_return_if_fail (XVIEWER_IS_THUMB_VIEW (thumbview));
	g_return_if_fail (thumbview->priv->menu == NULL);

	thumbview->priv->menu = GTK_WIDGET (g_object_ref (menu));

	gtk_menu_attach_to_widget (GTK_MENU (thumbview->priv->menu),
				   GTK_WIDGET (thumbview),
				   NULL);

	g_signal_connect (G_OBJECT (thumbview), "button_press_event",
			  G_CALLBACK (thumbview_on_button_press_event_cb), NULL);

}


static void
xviewer_thumb_view_popup_menu (XviewerThumbView *thumbview, GdkEventButton *event)
{
	GtkWidget *popup;
	int button, event_time;

	popup = thumbview->priv->menu;

	if (event) {
		button = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

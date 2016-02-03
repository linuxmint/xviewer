/*
 * eog-metadata-sidebar.c
 * This file is part of eog
 *
 * Author: Felix Riemann <friemann@gnome.org>
 *
 * Portions based on code by: Lucas Rocha <lucasr@gnome.org>
 *                            Hubert Figuiere <hub@figuiere.net> (XMP support)
 *
 * Copyright (C) 2011 GNOME Foundation
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
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "eog-image.h"
#include "eog-metadata-sidebar.h"
#include "eog-properties-dialog.h"
#include "eog-scroll-view.h"
#include "eog-util.h"
#include "eog-window.h"

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#include <libexif/exif-tag.h>
#include "eog-exif-util.h"
#endif

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#include <exempi/xmpconsts.h>
#endif

#if HAVE_EXIF || HAVE_EXEMPI
#define HAVE_METADATA 1
#endif

enum {
	PROP_0,
	PROP_IMAGE,
	PROP_PARENT_WINDOW
};

struct _EogMetadataSidebarPrivate {
	EogWindow *parent_window;
	EogImage *image;

	gulong image_changed_id;
	gulong thumb_changed_id;

	GtkWidget *grid;

	GtkWidget *name_label;
	GtkWidget *height_label;
	GtkWidget *width_label;
	GtkWidget *type_label;
	GtkWidget *size_label;
	GtkWidget *folder_button;

#if HAVE_EXIF
	GtkWidget *aperture_label;
	GtkWidget *exposure_label;
	GtkWidget *focallen_label;
	GtkWidget *flash_label;
	GtkWidget *iso_label;
	GtkWidget *metering_label;
	GtkWidget *model_label;
	GtkWidget *date_label;
#endif

#if HAVE_EXEMPI
	GtkWidget *location_label;
	GtkWidget *desc_label;
	GtkWidget *keyword_label;
	GtkWidget *creator_label;
	GtkWidget *rights_label;
#endif

#if HAVE_METADATA
	GtkWidget *details_button;
#endif
};

G_DEFINE_TYPE_WITH_PRIVATE(EogMetadataSidebar, eog_metadata_sidebar, GTK_TYPE_SCROLLED_WINDOW)

static GtkWidget*
_gtk_grid_append_title_line (GtkGrid *grid, GtkWidget *sibling,
			     const gchar *text)
{
	GtkWidget *label;
	gchar *markup;

	label = gtk_label_new (NULL);

	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);

	gtk_grid_attach_next_to (grid, label, sibling, GTK_POS_BOTTOM,  2, 1);
	return label;
}

static GtkWidget*
_gtk_grid_append_prop_line (GtkGrid *grid, GtkWidget *sibling,
			    GtkWidget **data_label, const gchar *text)
{
	GtkWidget *label;
	gchar *markup;
	GtkWidget *box;

	box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	label = gtk_label_new (NULL);
	markup = g_markup_printf_escaped("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL(label), markup);
	g_free (markup);

	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_valign (label, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);

	if (G_LIKELY (data_label != NULL)) {
		*data_label = gtk_label_new (NULL);
		gtk_label_set_selectable (GTK_LABEL (*data_label), TRUE);
		gtk_label_set_line_wrap (GTK_LABEL(*data_label), TRUE);
		gtk_widget_set_halign (*data_label, GTK_ALIGN_START);
		gtk_widget_set_valign (*data_label, GTK_ALIGN_START);
		// Add a small margin to make it a sublabel to the first label
		gtk_widget_set_margin_start (*data_label, 12);
		gtk_box_pack_end (GTK_BOX(box), *data_label, FALSE, FALSE, 0);
	}
	gtk_grid_attach_next_to (grid, box, sibling, GTK_POS_BOTTOM,  2, 1);

	return box;
}

#if HAVE_EXEMPI
static void
eog_xmp_set_label (XmpPtr xmp,
		   const char *ns,
		   const char *propname,
		   GtkWidget *w)
{
	uint32_t options;

	XmpStringPtr value = xmp_string_new ();

	if (xmp && xmp_get_property (xmp, ns, propname, value, &options)) {
		if (XMP_IS_PROP_SIMPLE (options)) {
			gtk_label_set_text (GTK_LABEL (w), xmp_string_cstr (value));
		} else if (XMP_IS_PROP_ARRAY (options)) {
			XmpIteratorPtr iter = xmp_iterator_new (xmp,
							        ns,
								propname,
								XMP_ITER_JUSTLEAFNODES);

			GString *string = g_string_new ("");

			if (iter) {
				gboolean first = TRUE;

				while (xmp_iterator_next (iter, NULL, NULL, value, &options)
				       && !XMP_IS_PROP_QUALIFIER (options)) {

					if (!first) {
						g_string_append_printf(string, ", ");
					} else {
						first = FALSE;
					}

					g_string_append_printf (string,
								"%s",
								xmp_string_cstr (value));
				}

				xmp_iterator_free (iter);
			}

			gtk_label_set_text (GTK_LABEL (w), string->str);
			g_string_free (string, TRUE);
		}
	} else {
		/* Property was not found */
		/* Clear label so it won't show bogus data */
		gtk_label_set_text (GTK_LABEL (w), NULL);
	}

	xmp_string_free (value);
}
#endif

static void
eog_metadata_sidebar_update_general_section (EogMetadataSidebar *sidebar)
{
	EogMetadataSidebarPrivate *priv = sidebar->priv;
	EogImage *img = priv->image;
	GFile *file, *parent_file;
	GFileInfo *file_info;
	gchar *str;
	goffset bytes;
	gint width, height;

	if (G_UNLIKELY (img == NULL)) {
		gtk_label_set_text (GTK_LABEL (priv->name_label), NULL);
		gtk_label_set_text (GTK_LABEL (priv->height_label), NULL);
		gtk_label_set_text (GTK_LABEL (priv->width_label), NULL);
		gtk_label_set_text (GTK_LABEL (priv->type_label), NULL);
		gtk_label_set_text (GTK_LABEL (priv->size_label), NULL);
		return;		
	}

	gtk_label_set_text (GTK_LABEL (priv->name_label),
			    eog_image_get_caption (img));
	eog_image_get_size (img, &width, &height);
	str = g_strdup_printf ("%d %s", height,
			       ngettext ("pixel", "pixels", height));
	gtk_label_set_text (GTK_LABEL (priv->height_label), str);
	g_free (str);
	str = g_strdup_printf ("%d %s", width,
			       ngettext ("pixel", "pixels", width));
	gtk_label_set_text (GTK_LABEL (priv->width_label), str);
	g_free (str);

	file = eog_image_get_file (img);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL) {
		str = g_strdup (_("Unknown"));
	} else {
		const gchar *mime_str;

		mime_str = g_file_info_get_content_type (file_info);
		str = g_content_type_get_description (mime_str);
		g_object_unref (file_info);
	}
	gtk_label_set_text (GTK_LABEL (priv->type_label), str);
	g_free (str);

	bytes = eog_image_get_bytes (img);
	str = g_format_size (bytes);
	gtk_label_set_text (GTK_LABEL (priv->size_label), str);
	g_free (str);

	parent_file = g_file_get_parent (file);
	if (parent_file == NULL) {
		/* file is root directory itself */
		parent_file = g_object_ref (file);
	}
	str = g_file_get_basename (parent_file);
	gtk_button_set_label (GTK_BUTTON (priv->folder_button), str);
	g_free (str);
	g_object_unref (parent_file);
}

#if HAVE_METADATA
static void
eog_metadata_sidebar_update_metadata_section (EogMetadataSidebar *sidebar)
{
	EogMetadataSidebarPrivate *priv = sidebar->priv;
	EogImage *img = priv->image;
#if HAVE_EXIF
	ExifData *exif_data = NULL;
#endif
#if HAVE_EXEMPI
	XmpPtr xmp_data = NULL;
#endif

	if (img) {
#if HAVE_EXIF
		exif_data = eog_image_get_exif_info (img);
#endif
#if HAVE_EXEMPI
		xmp_data =  eog_image_get_xmp_info (img);
#endif
	}

#if HAVE_EXIF
	eog_exif_util_set_label_text (GTK_LABEL (priv->aperture_label),
				      exif_data, EXIF_TAG_FNUMBER);
	eog_exif_util_set_label_text (GTK_LABEL (priv->exposure_label),
				      exif_data,
				      EXIF_TAG_EXPOSURE_TIME);
	eog_exif_util_set_focal_length_label_text (
				       GTK_LABEL (priv->focallen_label),
				       exif_data);
	eog_exif_util_set_label_text (GTK_LABEL (priv->flash_label),
				      exif_data, EXIF_TAG_FLASH);
	eog_exif_util_set_label_text (GTK_LABEL (priv->iso_label),
				      exif_data,
				      EXIF_TAG_ISO_SPEED_RATINGS);
	eog_exif_util_set_label_text (GTK_LABEL (priv->metering_label),
				      exif_data,
				      EXIF_TAG_METERING_MODE);
	eog_exif_util_set_label_text (GTK_LABEL (priv->model_label),
				      exif_data, EXIF_TAG_MODEL);
	eog_exif_util_set_label_text (GTK_LABEL (priv->date_label),
				      exif_data,
				      EXIF_TAG_DATE_TIME_ORIGINAL);

	/* exif_data_unref can handle NULL-values */
	exif_data_unref(exif_data);
#endif /* HAVE_EXIF */

#if HAVE_EXEMPI
 	eog_xmp_set_label (xmp_data,
			   NS_IPTC4XMP,
			   "Location",
			   priv->location_label);

	eog_xmp_set_label (xmp_data,
			   NS_DC,
			   "description",
			   priv->desc_label);

	eog_xmp_set_label (xmp_data,
			   NS_DC,
			   "subject",
			   priv->keyword_label);

	eog_xmp_set_label (xmp_data,
			   NS_DC,
       	                   "creator",
			   priv->creator_label);

	eog_xmp_set_label (xmp_data,
			   NS_DC,
			   "rights",
			   priv->rights_label);


	if (xmp_data != NULL)
		xmp_free (xmp_data);
#endif /* HAVE_EXEMPI */
}
#endif /* HAVE_METADATA */

static void
eog_metadata_sidebar_update (EogMetadataSidebar *sidebar)
{
	g_return_if_fail (EOG_IS_METADATA_SIDEBAR (sidebar));

	eog_metadata_sidebar_update_general_section (sidebar);
#if HAVE_METADATA
	eog_metadata_sidebar_update_metadata_section (sidebar);
#endif
}

static void
_thumbnail_changed_cb (EogImage *image, gpointer user_data)
{
	eog_metadata_sidebar_update (EOG_METADATA_SIDEBAR (user_data));
}

static void
eog_metadata_sidebar_set_image (EogMetadataSidebar *sidebar, EogImage *image)
{
	EogMetadataSidebarPrivate *priv = sidebar->priv;

	if (image == priv->image)
		return;


	if (priv->thumb_changed_id != 0) {
		g_signal_handler_disconnect (priv->image,
					     priv->thumb_changed_id);
		priv->thumb_changed_id = 0;
	}

	if (priv->image)
		g_object_unref (priv->image);

	priv->image = image;

	if (priv->image) {
		g_object_ref (priv->image);
		priv->thumb_changed_id = 
			g_signal_connect (priv->image, "thumbnail-changed",
					  G_CALLBACK (_thumbnail_changed_cb),
					  sidebar);
		eog_metadata_sidebar_update (sidebar);
	}
	
	g_object_notify (G_OBJECT (sidebar), "image");
}

static void
_notify_image_cb (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	EogImage *image;

	g_return_if_fail (EOG_IS_METADATA_SIDEBAR (user_data));
	g_return_if_fail (EOG_IS_SCROLL_VIEW (gobject));

	image = eog_scroll_view_get_image (EOG_SCROLL_VIEW (gobject));

	eog_metadata_sidebar_set_image (EOG_METADATA_SIDEBAR (user_data),
					image);

	if (image)
		g_object_unref (image);
}

static void
_folder_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	EogMetadataSidebarPrivate *priv = EOG_METADATA_SIDEBAR(user_data)->priv;
	EogImage *img;
	GdkScreen *screen;
	GFile *file;

	g_return_if_fail (priv->parent_window != NULL);

	img = eog_window_get_image (priv->parent_window);
	screen = gtk_widget_get_screen (GTK_WIDGET (priv->parent_window));
	file = eog_image_get_file (img);

	eog_util_show_file_in_filemanager (file, screen);

	g_object_unref (file);
}

#ifdef HAVE_METADATA
static void
_details_button_clicked_cb (GtkButton *button, gpointer user_data)
{
	EogMetadataSidebarPrivate *priv = EOG_METADATA_SIDEBAR(user_data)->priv;
	GtkWidget *dlg;

	g_return_if_fail (priv->parent_window != NULL);

	dlg = eog_window_get_properties_dialog (
					EOG_WINDOW (priv->parent_window));
	g_return_if_fail (dlg != NULL);
	eog_properties_dialog_set_page (EOG_PROPERTIES_DIALOG (dlg),
					EOG_PROPERTIES_DIALOG_PAGE_DETAILS);
	gtk_widget_show (dlg);
}
#endif

static void
eog_metadata_sidebar_set_parent_window (EogMetadataSidebar *sidebar,
					EogWindow *window)
{
	EogMetadataSidebarPrivate *priv;
	GtkWidget *view;

	g_return_if_fail (EOG_IS_METADATA_SIDEBAR (sidebar));
	priv = sidebar->priv;
	g_return_if_fail (priv->parent_window == NULL);

	priv->parent_window = g_object_ref (window);
	eog_metadata_sidebar_update (sidebar);
	view = eog_window_get_view (window);
	priv->image_changed_id = g_signal_connect (view, "notify::image",
						  G_CALLBACK (_notify_image_cb),
						  sidebar);

	g_object_notify (G_OBJECT (sidebar), "parent-window");
	
}

static void
eog_metadata_sidebar_init (EogMetadataSidebar *sidebar)
{
	EogMetadataSidebarPrivate *priv;
	GtkWidget *label;

	priv = sidebar->priv = eog_metadata_sidebar_get_instance_private (sidebar);
	priv->grid = gtk_grid_new ();
	g_object_set (G_OBJECT (priv->grid),
	              "row-spacing", 6,
		      "column-spacing", 6,
		      NULL);

	label = _gtk_grid_append_title_line (GTK_GRID (priv->grid),
					     NULL, _("General"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->name_label, _("Name:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->width_label, _("Width:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->height_label, _("Height:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->type_label, _("Type:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->size_label, _("File size:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    NULL, _("Folder:"));

	/* Enable wrapping at char boundaries as fallback for the filename
	 * as it is possible for it to not contain any "words" to wrap on. */
	gtk_label_set_line_wrap_mode (GTK_LABEL (priv->name_label),
				      PANGO_WRAP_WORD_CHAR);

{
	priv->folder_button = gtk_button_new_with_label ("");
	g_signal_connect (priv->folder_button, "clicked",
			  G_CALLBACK (_folder_button_clicked_cb), sidebar);
	gtk_widget_set_margin_start (priv->folder_button, 12);
	gtk_widget_set_margin_end (priv->folder_button, 12);
	gtk_widget_set_margin_top (priv->folder_button, 3);
	gtk_widget_set_tooltip_text (priv->folder_button, _("Show the folder "
			       "which contains this file in the file manager"));
	gtk_box_pack_end (GTK_BOX (label), priv->folder_button, FALSE, FALSE, 0);
}

#if HAVE_METADATA
	label = _gtk_grid_append_title_line (GTK_GRID (priv->grid),
					     label, _("Metadata"));
#if HAVE_EXIF
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->aperture_label,
					    _("Aperture Value:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->exposure_label,
					    _("Exposure Time:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->focallen_label,
					    _("Focal Length:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->flash_label, _("Flash:"));
	gtk_label_set_line_wrap (GTK_LABEL (priv->flash_label), TRUE);
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->iso_label,
					    _("ISO Speed Rating:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->metering_label,
					    _("Metering Mode:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->model_label,
					    _("Camera Model:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->date_label, _("Date/Time:"));
#endif /* HAVE_EXIF */
#if HAVE_EXEMPI
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->desc_label,
					    _("Description:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->location_label,
					    _("Location:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->keyword_label,
					    _("Keywords:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->creator_label, _("Author:"));
	label = _gtk_grid_append_prop_line (GTK_GRID (priv->grid), label,
					    &priv->rights_label,
					    _("Copyright:"));
#endif /* HAVE_EXEMPI */

	priv->details_button = gtk_button_new_with_label (_("Details"));
	g_signal_connect (priv->details_button, "clicked",
			  G_CALLBACK (_details_button_clicked_cb), sidebar);
	gtk_grid_attach_next_to (GTK_GRID (priv->grid), priv->details_button,
				 label, GTK_POS_BOTTOM, 1, 1);
#endif /* HAVE_METADATA */



	gtk_widget_show_all (priv->grid);
}

static void
eog_metadata_sidebar_constructed (GObject *object)
{
	EogMetadataSidebarPrivate *priv;

	priv = EOG_METADATA_SIDEBAR (object)->priv;

	/* This can only happen after all construct properties for
	 * GtkScrolledWindow are set/handled. */
	gtk_container_add (GTK_CONTAINER (object), priv->grid);
	gtk_widget_show (GTK_WIDGET (object));

	G_OBJECT_CLASS (eog_metadata_sidebar_parent_class)->constructed (object);
}

static void
eog_metadata_sidebar_get_property (GObject *object, guint property_id,
				   GValue *value, GParamSpec *pspec)
{
	EogMetadataSidebar *sidebar;

	g_return_if_fail (EOG_IS_METADATA_SIDEBAR (object));

	sidebar = EOG_METADATA_SIDEBAR (object);

	switch (property_id) {
	case PROP_IMAGE:
	{
		g_value_set_object (value, sidebar->priv->image);
		break;
	}
	case PROP_PARENT_WINDOW:
		g_value_set_object (value, sidebar->priv->parent_window);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
eog_metadata_sidebar_set_property (GObject *object, guint property_id,
				   const GValue *value, GParamSpec *pspec)
{
	EogMetadataSidebar *sidebar;

	g_return_if_fail (EOG_IS_METADATA_SIDEBAR (object));

	sidebar = EOG_METADATA_SIDEBAR (object);

	switch (property_id) {
	case PROP_IMAGE:
	{
		break;
	}
	case PROP_PARENT_WINDOW:
	{
		EogWindow *window;

		window = g_value_get_object (value);
		eog_metadata_sidebar_set_parent_window (sidebar, window);
		break;
	}
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}

}
static void
eog_metadata_sidebar_class_init (EogMetadataSidebarClass *klass)
{
	GObjectClass *g_obj_class = G_OBJECT_CLASS (klass);

	g_obj_class->constructed = eog_metadata_sidebar_constructed;
	g_obj_class->get_property = eog_metadata_sidebar_get_property;
	g_obj_class->set_property = eog_metadata_sidebar_set_property;
/*	g_obj_class->dispose = eog_metadata_sidebar_dispose;*/

	g_object_class_install_property (
		g_obj_class, PROP_PARENT_WINDOW,
		g_param_spec_object ("parent-window", NULL, NULL,
				     EOG_TYPE_WINDOW, G_PARAM_READWRITE
				     | G_PARAM_CONSTRUCT_ONLY
				     | G_PARAM_STATIC_STRINGS));
	g_object_class_install_property (
		g_obj_class, PROP_IMAGE,
		g_param_spec_object ("image", NULL, NULL, EOG_TYPE_IMAGE,
				     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)
				    );
}


GtkWidget*
eog_metadata_sidebar_new (EogWindow *window)
{
	return gtk_widget_new (EOG_TYPE_METADATA_SIDEBAR,
			       "hadjustment", NULL,
			       "vadjustment", NULL,
	                       "hscrollbar-policy", GTK_POLICY_NEVER,
			       "vscrollbar-policy", GTK_POLICY_AUTOMATIC,
			       "border-width", 6,
			       "parent-window", window,
			       NULL);
}

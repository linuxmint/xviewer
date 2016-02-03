/* Eye of Gnome - Print Operations
 *
 * Copyright (C) 2005-2008 The Free Software Foundation
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "eog-image.h"
#include "eog-print.h"
#include "eog-print-image-setup.h"
#include "eog-util.h"
#include "eog-debug.h"

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#define EOG_PRINT_SETTINGS_FILE "eog-print-settings.ini"
#define EOG_PAGE_SETUP_GROUP "Page Setup"
#define EOG_PRINT_SETTINGS_GROUP "Print Settings"

typedef struct {
	EogImage *image;
	gdouble left_margin;
	gdouble top_margin;
	gdouble scale_factor;
	GtkUnit unit;
} EogPrintData;

/* art_affine_flip modified to work with cairo_matrix_t */
static void
_eog_cairo_matrix_flip (cairo_matrix_t *dst, const cairo_matrix_t *src, gboolean horiz, gboolean vert)
{
	dst->xx = horiz ? -src->xx : src->xx;
	dst->yx = horiz ? -src->yx : src->yx;
	dst->xy = vert ? -src->xy : src->xy;
	dst->yy = vert ? -src->yy : src->yy;
	dst->x0 = horiz ? -src->x0 : src->x0;
	dst->y0 = vert ? -src->y0 : src->y0;
}

static gboolean
_cairo_ctx_supports_jpg_metadata (cairo_t *cr)
{
	cairo_surface_t *surface = cairo_get_target (cr);
	cairo_surface_type_t type = cairo_surface_get_type (surface);

	/* Based on cairo-1.10 */
	return (type == CAIRO_SURFACE_TYPE_PDF || type == CAIRO_SURFACE_TYPE_PS ||
		type == CAIRO_SURFACE_TYPE_SVG || type == CAIRO_SURFACE_TYPE_WIN32_PRINTING);
}

static void
eog_print_draw_page (GtkPrintOperation *operation,
		     GtkPrintContext   *context,
		     gint               page_nr,
		     gpointer           user_data)
{
	cairo_t *cr;
	gdouble dpi_x, dpi_y;
	gdouble x0, y0;
	gdouble scale_factor;
	gdouble p_width, p_height;
	gint width, height;
	EogPrintData *data;
	GtkPageSetup *page_setup;

	eog_debug (DEBUG_PRINTING);

	data = (EogPrintData *) user_data;

	scale_factor = data->scale_factor/100;

	dpi_x = gtk_print_context_get_dpi_x (context);
	dpi_y = gtk_print_context_get_dpi_y (context);

	switch (data->unit) {
	case GTK_UNIT_INCH:
		x0 = data->left_margin * dpi_x;
		y0 = data->top_margin  * dpi_y;
		break;
	case GTK_UNIT_MM:
		x0 = data->left_margin * dpi_x/25.4;
		y0 = data->top_margin  * dpi_y/25.4;
		break;
	default:
		g_assert_not_reached ();
	}

	cr = gtk_print_context_get_cairo_context (context);

	cairo_translate (cr, x0, y0);

	page_setup = gtk_print_context_get_page_setup (context);
	p_width =  gtk_page_setup_get_page_width (page_setup, GTK_UNIT_POINTS);
	p_height = gtk_page_setup_get_page_height (page_setup, GTK_UNIT_POINTS);

	eog_image_get_size (data->image, &width, &height);

	/* this is both a workaround for a bug in cairo's PDF backend, and
	   a way to ensure we are not printing outside the page margins */
	cairo_rectangle (cr, 0, 0, MIN (width*scale_factor, p_width), MIN (height*scale_factor, p_height));
	cairo_clip (cr);

	cairo_scale (cr, scale_factor, scale_factor);

#ifdef HAVE_RSVG
	if (eog_image_is_svg (data->image))
	{
		RsvgHandle *svg = eog_image_get_svg (data->image);

		rsvg_handle_render_cairo (svg, cr);
		return;
	} else
#endif
	/* JPEGs can be attached to the cairo surface which simply embeds the JPEG file into the
	 * destination PDF skipping (PNG-)recompression. This should reduce PDF sizes enormously. */
	if (eog_image_is_jpeg (data->image) && _cairo_ctx_supports_jpg_metadata (cr))
	{
		GFile *file;
		char *img_data;
		gsize data_len;
		cairo_surface_t *surface = NULL;

		eog_debug_message (DEBUG_PRINTING, "Attaching image to cairo surface");

		file = eog_image_get_file (data->image);
		if (g_file_load_contents (file, NULL, &img_data, &data_len, NULL, NULL))
		{
			EogTransform *tf = eog_image_get_transform (data->image);
			EogTransform *auto_tf = eog_image_get_autorotate_transform (data->image);
			cairo_matrix_t mx, mx2;

			if (!tf && auto_tf) {
				/* If only autorotation data present,
				 * make it the normal rotation. */
				tf = auto_tf;
				auto_tf = NULL;
			}

			/* Care must be taken with height and width values. They are not the original
			 * values but were affected by the transformation. As the surface needs to be
			 * generated using the original dimensions they might need to be flipped. */
			if (tf) {
				if (auto_tf) {
					/* If we have an autorotation apply
					 * it before the others */
					tf = eog_transform_compose (auto_tf, tf);
				}

				switch (eog_transform_get_transform_type (tf)) {
					case EOG_TRANSFORM_ROT_90:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, height, width);
						cairo_rotate (cr, 90.0 * (G_PI/180.0));
						cairo_translate (cr, 0.0, -width);
						break;
					case EOG_TRANSFORM_ROT_180:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, width, height);
						cairo_rotate (cr, 180.0 * (G_PI/180.0));
						cairo_translate (cr, -width, -height);
						break;
					case EOG_TRANSFORM_ROT_270:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, height, width);
						cairo_rotate (cr, 270.0 * (G_PI/180.0));
						cairo_translate (cr, -height, 0.0);
						break;
					case EOG_TRANSFORM_FLIP_HORIZONTAL:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, width, height);
						cairo_matrix_init_identity (&mx);
						_eog_cairo_matrix_flip (&mx2, &mx, TRUE, FALSE);
						cairo_transform (cr, &mx2);
						cairo_translate (cr, -width, 0.0);
						break;
					case EOG_TRANSFORM_FLIP_VERTICAL:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, width, height);
						cairo_matrix_init_identity (&mx);
						_eog_cairo_matrix_flip (&mx2, &mx, FALSE, TRUE);
						cairo_transform (cr, &mx2);
						cairo_translate (cr, 0.0, -height);
						break;
					case EOG_TRANSFORM_TRANSPOSE:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, height, width);
						cairo_matrix_init_rotate (&mx, 90.0 * (G_PI/180.0));
						cairo_matrix_init_identity (&mx2);
						_eog_cairo_matrix_flip (&mx2, &mx2, TRUE, FALSE);
						cairo_matrix_multiply (&mx2, &mx, &mx2);
						cairo_transform (cr, &mx2);
						break;
					case EOG_TRANSFORM_TRANSVERSE:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, height, width);
						cairo_matrix_init_rotate (&mx, 90.0 * (G_PI/180.0));
						cairo_matrix_init_identity (&mx2);
						_eog_cairo_matrix_flip (&mx2, &mx2, FALSE, TRUE);
						cairo_matrix_multiply (&mx2, &mx, &mx2);
						cairo_transform (cr, &mx2);
						cairo_translate (cr, -height , -width);
						break;
					case EOG_TRANSFORM_NONE:
					default:
						surface = cairo_image_surface_create (
								CAIRO_FORMAT_RGB24, width, height);
						break;
				}
			}

			if (!surface)
				surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24,
								      width, height);
			cairo_surface_set_mime_data (surface,
			                             CAIRO_MIME_TYPE_JPEG,
			                             (unsigned char*)img_data, data_len,
						     g_free, img_data);
			cairo_set_source_surface (cr, surface, 0, 0);
			cairo_paint (cr);
			cairo_surface_destroy (surface);
			g_object_unref (file);
			return;
		}
		g_object_unref (file);

	}

	{
		GdkPixbuf *pixbuf;

		pixbuf = eog_image_get_pixbuf (data->image);
		gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
 		cairo_paint (cr);
		g_object_unref (pixbuf);
	}
}

static GObject *
eog_print_create_custom_widget (GtkPrintOperation *operation,
				       gpointer user_data)
{
	GtkPageSetup *page_setup;
	EogPrintData *data;

	eog_debug (DEBUG_PRINTING);

	data = (EogPrintData *)user_data;

	page_setup = gtk_print_operation_get_default_page_setup (operation);

	if (page_setup == NULL)
		page_setup = gtk_page_setup_new ();

	return G_OBJECT (eog_print_image_setup_new (data->image, page_setup));
}

static void
eog_print_custom_widget_apply (GtkPrintOperation *operation,
			       GtkWidget         *widget,
			       gpointer           user_data)
{
	EogPrintData *data;
	gdouble left_margin, top_margin, scale_factor;
	GtkUnit unit;

	eog_debug (DEBUG_PRINTING);

	data = (EogPrintData *)user_data;

	eog_print_image_setup_get_options (EOG_PRINT_IMAGE_SETUP (widget),
					   &left_margin, &top_margin,
					   &scale_factor, &unit);

	data->left_margin = left_margin;
	data->top_margin = top_margin;
	data->scale_factor = scale_factor;
	data->unit = unit;
}

static void
eog_print_end_print (GtkPrintOperation *operation,
		     GtkPrintContext   *context,
		     gpointer           user_data)
{
	EogPrintData *data = (EogPrintData*) user_data;

	eog_debug (DEBUG_PRINTING);

	g_object_unref (data->image);
	g_slice_free (EogPrintData, data);
}

GtkPrintOperation *
eog_print_operation_new (EogImage *image,
			 GtkPrintSettings *print_settings,
			 GtkPageSetup *page_setup)
{
	GtkPrintOperation *print;
	EogPrintData *data;
	gint width, height;

	eog_debug (DEBUG_PRINTING);

	print = gtk_print_operation_new ();

	data = g_slice_new0 (EogPrintData);

	data->left_margin = 0;
	data->top_margin = 0;
	data->scale_factor = 100;
	data->image = g_object_ref (image);
	data->unit = GTK_UNIT_INCH;

	eog_image_get_size (image, &width, &height);

	if (page_setup == NULL)
		page_setup = gtk_page_setup_new ();

	if (height >= width) {
		gtk_page_setup_set_orientation (page_setup,
						GTK_PAGE_ORIENTATION_PORTRAIT);
	} else {
		gtk_page_setup_set_orientation (page_setup,
						GTK_PAGE_ORIENTATION_LANDSCAPE);
	}

	gtk_print_operation_set_print_settings (print, print_settings);
	gtk_print_operation_set_default_page_setup (print,
						    page_setup);
	gtk_print_operation_set_n_pages (print, 1);
	gtk_print_operation_set_job_name (print,
					  eog_image_get_caption (image));
	gtk_print_operation_set_embed_page_setup (print, TRUE);

	g_signal_connect (print, "draw_page",
			  G_CALLBACK (eog_print_draw_page),
			  data);
	g_signal_connect (print, "create-custom-widget",
			  G_CALLBACK (eog_print_create_custom_widget),
			  data);
	g_signal_connect (print, "custom-widget-apply",
			  G_CALLBACK (eog_print_custom_widget_apply),
			  data);
	g_signal_connect (print, "end-print",
			  G_CALLBACK (eog_print_end_print),
			  data);
	g_signal_connect (print, "update-custom-widget",
			  G_CALLBACK (eog_print_image_setup_update),
			  data);

	gtk_print_operation_set_custom_tab_label (print, _("Image Settings"));

	return print;
}

static GKeyFile *
eog_print_get_key_file (void)
{
	GKeyFile *key_file;
	GError *error = NULL;
	gchar *filename;
	GFile *file;
	const gchar *dot_dir = eog_util_dot_dir ();

	filename = g_build_filename (dot_dir, EOG_PRINT_SETTINGS_FILE, NULL);

	file = g_file_new_for_path (filename);
	key_file = g_key_file_new ();

	if (g_file_query_exists (file, NULL)) {
		g_key_file_load_from_file (key_file, filename,
					   G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
					   &error);
		if (error) {
			g_warning ("Error loading print settings file: %s", error->message);
			g_error_free (error);
			g_object_unref (file);
			g_free (filename);
			g_key_file_free (key_file);
			return NULL;
		}
	}

	g_object_unref (file);
	g_free (filename);

	return key_file;
}

GtkPageSetup *
eog_print_get_page_setup (void)
{
	GtkPageSetup *page_setup;
	GKeyFile *key_file;
	GError *error = NULL;

	key_file = eog_print_get_key_file ();

	if (key_file && g_key_file_has_group (key_file, EOG_PAGE_SETUP_GROUP)) {
		page_setup = gtk_page_setup_new_from_key_file (key_file, EOG_PAGE_SETUP_GROUP, &error);
	} else {
		page_setup = gtk_page_setup_new ();
	}

	if (error) {
		page_setup = gtk_page_setup_new ();

		g_warning ("Error loading print settings file: %s", error->message);
		g_error_free (error);
	}

	if (key_file)
		g_key_file_free (key_file);

	return page_setup;
}

static void
eog_print_save_key_file (GKeyFile *key_file)
{
	gchar *filename;
	gchar *data;
	GError *error = NULL;
	const gchar *dot_dir = eog_util_dot_dir ();

	filename = g_build_filename (dot_dir, EOG_PRINT_SETTINGS_FILE, NULL);

	data = g_key_file_to_data (key_file, NULL, NULL);

	g_file_set_contents (filename, data, -1, &error);

	if (error) {
		g_warning ("Error saving print settings file: %s", error->message);
		g_error_free (error);
	}

	g_free (filename);
	g_free (data);
}

void
eog_print_set_page_setup (GtkPageSetup *page_setup)
{
	GKeyFile *key_file;

	key_file = eog_print_get_key_file ();

	if (key_file == NULL) {
		key_file = g_key_file_new ();
	}

	gtk_page_setup_to_key_file (page_setup, key_file, EOG_PAGE_SETUP_GROUP);
	eog_print_save_key_file (key_file);

	g_key_file_free (key_file);
}

GtkPrintSettings *
eog_print_get_print_settings (void)
{
	GtkPrintSettings *print_settings;
	GError *error = NULL;
	GKeyFile *key_file;

	key_file = eog_print_get_key_file ();

	if (key_file && g_key_file_has_group (key_file, EOG_PRINT_SETTINGS_GROUP)) {
		print_settings = gtk_print_settings_new_from_key_file (key_file, EOG_PRINT_SETTINGS_GROUP, &error);
	} else {
		print_settings = gtk_print_settings_new ();
	}

	if (error) {
		print_settings = gtk_print_settings_new ();

		g_warning ("Error loading print settings file: %s", error->message);
		g_error_free (error);
	}

	if (key_file)
		g_key_file_free (key_file);

	return print_settings;
}

void
eog_print_set_print_settings (GtkPrintSettings *print_settings)
{
	GKeyFile *key_file;

	key_file = eog_print_get_key_file ();

	if (key_file == NULL) {
		key_file = g_key_file_new ();
	}
	/* Override copies settings as we don't want to save
	 * this for the next time eog is used. */
	gtk_print_settings_set_n_copies (print_settings, 1);
	/* Unset output-uri as it would break setting output-base-name */
	gtk_print_settings_set (print_settings, GTK_PRINT_SETTINGS_OUTPUT_URI, NULL);
	gtk_print_settings_to_key_file (print_settings, key_file, EOG_PRINT_SETTINGS_GROUP);
	eog_print_save_key_file (key_file);

	g_key_file_free (key_file);
}

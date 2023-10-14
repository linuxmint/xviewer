/* Xviewer - Image
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define GDK_PIXBUF_ENABLE_BACKEND

#include "xviewer-image.h"
#include "xviewer-image-private.h"
#include "xviewer-debug.h"

#ifdef HAVE_JPEG
#include "xviewer-image-jpeg.h"
#endif

#include "xviewer-marshal.h"
#include "xviewer-pixbuf-util.h"
#include "xviewer-metadata-reader.h"
#include "xviewer-image-save-info.h"
#include "xviewer-transform.h"
#include "xviewer-util.h"
#include "xviewer-jobs.h"
#include "xviewer-thumbnail.h"

#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include "xviewer-exif-util.h"
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <libexif/exif-loader.h>
#endif

#ifdef HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#ifdef HAVE_LCMS
#include <lcms2.h>
#ifndef EXIF_TAG_GAMMA
#define EXIF_TAG_GAMMA 0xa500
#endif
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_DEFINE_TYPE_WITH_PRIVATE (XviewerImage, xviewer_image, G_TYPE_OBJECT)

enum {
	SIGNAL_CHANGED,
	SIGNAL_SIZE_PREPARED,
	SIGNAL_THUMBNAIL_CHANGED,
	SIGNAL_SAVE_PROGRESS,
	SIGNAL_NEXT_FRAME,
	SIGNAL_FILE_CHANGED,
	SIGNAL_LAST
};

static gint signals[SIGNAL_LAST];

static GList *supported_mime_types = NULL;

#define XVIEWER_IMAGE_READ_BUFFER_SIZE 65535

static void
xviewer_image_free_mem_private (XviewerImage *image)
{
	XviewerImagePrivate *priv;

	priv = image->priv;

	if (priv->status == XVIEWER_IMAGE_STATUS_LOADING) {
		xviewer_image_cancel_load (image);
	} else {
		if (priv->anim_iter != NULL) {
			g_object_unref (priv->anim_iter);
			priv->anim_iter = NULL;
		}

		if (priv->anim != NULL) {
			g_object_unref (priv->anim);
			priv->anim = NULL;
		}

		priv->is_playing = FALSE;

		if (priv->image != NULL) {
			g_object_unref (priv->image);
			priv->image = NULL;
		}

#ifdef HAVE_RSVG
		if (priv->svg != NULL) {
			g_object_unref (priv->svg);
			priv->svg = NULL;
		}
#endif

#ifdef HAVE_EXIF
		if (priv->exif != NULL) {
			exif_data_unref (priv->exif);
			priv->exif = NULL;
		}
#endif

		if (priv->exif_chunk != NULL) {
			g_free (priv->exif_chunk);
			priv->exif_chunk = NULL;
		}

		priv->exif_chunk_len = 0;

#ifdef HAVE_EXEMPI
		if (priv->xmp != NULL) {
			xmp_free (priv->xmp);
			priv->xmp = NULL;
		}
#endif

#ifdef HAVE_LCMS
		if (priv->profile != NULL) {
			cmsCloseProfile (priv->profile);
			priv->profile = NULL;
		}
#endif

		priv->status = XVIEWER_IMAGE_STATUS_UNKNOWN;
		priv->metadata_status = XVIEWER_IMAGE_METADATA_NOT_READ;
	}
}

static void
xviewer_image_dispose (GObject *object)
{
	XviewerImagePrivate *priv;

	priv = XVIEWER_IMAGE (object)->priv;

	xviewer_image_free_mem_private (XVIEWER_IMAGE (object));

	if (priv->file) {
		g_object_unref (priv->file);
		priv->file = NULL;
	}

	if (priv->caption) {
		g_free (priv->caption);
		priv->caption = NULL;
	}

	if (priv->collate_key) {
		g_free (priv->collate_key);
		priv->collate_key = NULL;
	}

	if (priv->file_type) {
		g_free (priv->file_type);
		priv->file_type = NULL;
	}

	g_mutex_clear (&priv->status_mutex);

	if (priv->trans) {
		g_object_unref (priv->trans);
		priv->trans = NULL;
	}

	if (priv->trans_autorotate) {
		g_object_unref (priv->trans_autorotate);
		priv->trans_autorotate = NULL;
	}

	if (priv->undo_stack) {
		g_slist_foreach (priv->undo_stack, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->undo_stack);
		priv->undo_stack = NULL;
	}

	G_OBJECT_CLASS (xviewer_image_parent_class)->dispose (object);
}

static void
xviewer_image_finalize (GObject *object)
{
	XviewerImagePrivate *priv;

	priv = XVIEWER_IMAGE (object)->priv;

	g_mutex_clear (&priv->status_mutex);

	G_OBJECT_CLASS (xviewer_image_parent_class)->finalize (object);
}

static void
xviewer_image_class_init (XviewerImageClass *klass)
{
	GObjectClass *object_class = (GObjectClass*) klass;

	object_class->dispose = xviewer_image_dispose;
	object_class->finalize = xviewer_image_finalize;

	signals[SIGNAL_SIZE_PREPARED] =
		g_signal_new ("size-prepared",
			      XVIEWER_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerImageClass, size_prepared),
			      NULL, NULL,
			      xviewer_marshal_VOID__INT_INT,
			      G_TYPE_NONE, 2,
			      G_TYPE_INT,
			      G_TYPE_INT);

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      XVIEWER_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerImageClass, changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[SIGNAL_THUMBNAIL_CHANGED] =
		g_signal_new ("thumbnail-changed",
			      XVIEWER_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerImageClass, thumbnail_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	signals[SIGNAL_SAVE_PROGRESS] =
		g_signal_new ("save-progress",
			      XVIEWER_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerImageClass, save_progress),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE, 1,
			      G_TYPE_FLOAT);
 	/**
 	 * XviewerImage::next-frame:
  	 * @img: the object which received the signal.
	 * @delay: number of milliseconds the current frame will be displayed.
	 *
	 * The ::next-frame signal will be emitted each time an animated image
	 * advances to the next frame.
	 */
	signals[SIGNAL_NEXT_FRAME] =
		g_signal_new ("next-frame",
			      XVIEWER_TYPE_IMAGE,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerImageClass, next_frame),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1,
			      G_TYPE_INT);

	signals[SIGNAL_FILE_CHANGED] = g_signal_new ("file-changed",
						     XVIEWER_TYPE_IMAGE,
						     G_SIGNAL_RUN_LAST,
						     G_STRUCT_OFFSET (XviewerImageClass, file_changed),
						     NULL, NULL,
						     g_cclosure_marshal_VOID__VOID,
						     G_TYPE_NONE, 0);
}

static void
xviewer_image_init (XviewerImage *img)
{
	img->priv = xviewer_image_get_instance_private (img);

	img->priv->file = NULL;
	img->priv->image = NULL;
	img->priv->anim = NULL;
	img->priv->anim_iter = NULL;
	img->priv->is_playing = FALSE;
	img->priv->thumbnail = NULL;
	img->priv->width = -1;
	img->priv->height = -1;
	img->priv->modified = FALSE;
	img->priv->file_is_changed = FALSE;
	g_mutex_init (&img->priv->status_mutex);
	img->priv->status = XVIEWER_IMAGE_STATUS_UNKNOWN;
	img->priv->metadata_status = XVIEWER_IMAGE_METADATA_NOT_READ;
	img->priv->undo_stack = NULL;
	img->priv->trans = NULL;
	img->priv->trans_autorotate = NULL;
	img->priv->data_ref_count = 0;
#ifdef HAVE_EXIF
	img->priv->orientation = 0;
	img->priv->autorotate = FALSE;
	img->priv->exif = NULL;
#endif
#ifdef HAVE_EXEMPI
	img->priv->xmp = NULL;
#endif
#ifdef HAVE_LCMS
	img->priv->profile = NULL;
#endif
#ifdef HAVE_RSVG
	img->priv->svg = NULL;
#endif
}

XviewerImage *
xviewer_image_new (const char *txt_uri)
{
	XviewerImage *img;

	img = XVIEWER_IMAGE (g_object_new (XVIEWER_TYPE_IMAGE, NULL));

	img->priv->file = g_file_new_for_uri (txt_uri);

	return img;
}

XviewerImage *
xviewer_image_new_file (GFile *file)
{
	XviewerImage *img;

	img = XVIEWER_IMAGE (g_object_new (XVIEWER_TYPE_IMAGE, NULL));

	img->priv->file = g_object_ref (file);

	return img;
}

GQuark
xviewer_image_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0) {
		q = g_quark_from_static_string ("xviewer-image-error-quark");
	}

	return q;
}

static void
xviewer_image_real_transform (XviewerImage     *img,
			  XviewerTransform *trans,
			  gboolean      is_undo,
			  XviewerJob       *job)
{
	XviewerImagePrivate *priv;
	GdkPixbuf *transformed;
	gboolean modified = FALSE;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));
	g_return_if_fail (XVIEWER_IS_TRANSFORM (trans));

	priv = img->priv;

	if (priv->image != NULL) {
		transformed = xviewer_transform_apply (trans, priv->image, job);

		g_object_unref (priv->image);
		priv->image = transformed;

		priv->width = gdk_pixbuf_get_width (transformed);
		priv->height = gdk_pixbuf_get_height (transformed);

		modified = TRUE;
	}

	if (priv->thumbnail != NULL) {
		transformed = xviewer_transform_apply (trans, priv->thumbnail, NULL);

		g_object_unref (priv->thumbnail);
		priv->thumbnail = transformed;

		modified = TRUE;
	}

	if (modified) {
                                    /* the code used to update EXIF information here but there is little point
                                       in doing so. If the user moves to the next image the EXIF data buffer is unreferenced
                                       and the updates are therefore lost. This leads to a physically rotated image but
                                       with the original orientation, width and height tags. Instead update the EXIF information
                                       if/when the image is saved */
        priv->modified = TRUE;
        priv->orientation = 1;
    }

	if (priv->trans == NULL) {
		g_object_ref (trans);
		priv->trans = trans;
	} else {
		XviewerTransform *composition;

		composition = xviewer_transform_compose (priv->trans, trans);

		g_object_unref (priv->trans);

		priv->trans = composition;
	}

	if (!is_undo) {
		g_object_ref (trans);
		priv->undo_stack = g_slist_prepend (priv->undo_stack, trans);
	}
}

static gboolean
do_emit_size_prepared_signal (XviewerImage *img)
{
	g_signal_emit (img, signals[SIGNAL_SIZE_PREPARED], 0,
		       img->priv->width, img->priv->height);
	return FALSE;
}

static void
xviewer_image_emit_size_prepared (XviewerImage *img)
{
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) do_emit_size_prepared_signal,
			 g_object_ref (img), g_object_unref);
}

static gboolean
check_loader_threadsafety (GdkPixbufLoader *loader, gboolean *result)
{
	GdkPixbufFormat *format;
	gboolean ret_val = FALSE;

	format = gdk_pixbuf_loader_get_format (loader);
	if (format) {
		ret_val = TRUE;
		if (result)
		/* FIXME: We should not be accessing this struct internals
 		 * directly. Keep track of bug #469209 to fix that. */
			*result = format->flags & GDK_PIXBUF_FORMAT_THREADSAFE;
	}

	return ret_val;
}

static void
xviewer_image_pre_size_prepared (GdkPixbufLoader *loader,
			     gint width,
			     gint height,
			     gpointer data)
{
	XviewerImage *img;

	xviewer_debug (DEBUG_IMAGE_LOAD);

	g_return_if_fail (XVIEWER_IS_IMAGE (data));

	img = XVIEWER_IMAGE (data);
	check_loader_threadsafety (loader, &img->priv->threadsafe_format);
}

static void
xviewer_image_size_prepared (GdkPixbufLoader *loader,
			 gint             width,
			 gint             height,
			 gpointer         data)
{
	XviewerImage *img;

	xviewer_debug (DEBUG_IMAGE_LOAD);

	g_return_if_fail (XVIEWER_IS_IMAGE (data));

	img = XVIEWER_IMAGE (data);

	g_mutex_lock (&img->priv->status_mutex);

	img->priv->width = width;
	img->priv->height = height;

	g_mutex_unlock (&img->priv->status_mutex);

#ifdef HAVE_EXIF
	if (img->priv->threadsafe_format && (!img->priv->autorotate || img->priv->exif))
#else
	if (img->priv->threadsafe_format)
#endif
		xviewer_image_emit_size_prepared (img);
}

static XviewerMetadataReader*
check_for_metadata_img_format (XviewerImage *img, guchar *buffer, guint bytes_read)
{
	XviewerMetadataReader *md_reader = NULL;

	xviewer_debug_message (DEBUG_IMAGE_DATA, "Check image format for jpeg: %x%x - length: %i",
			   buffer[0], buffer[1], bytes_read);

	if (bytes_read >= 2) {
		/* SOI (start of image) marker for JPEGs is 0xFFD8 */
		if ((buffer[0] == 0xFF) && (buffer[1] == 0xD8)) {
			md_reader = xviewer_metadata_reader_new (XVIEWER_METADATA_JPEG);
		}
		if (bytes_read >= 8 &&
		    memcmp (buffer, "\x89PNG\x0D\x0A\x1a\x0A", 8) == 0) {
			md_reader = xviewer_metadata_reader_new (XVIEWER_METADATA_PNG);
		}
	}

	return md_reader;
}

static gboolean
xviewer_image_needs_transformation (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	return (img->priv->trans != NULL || img->priv->trans_autorotate != NULL);
}

static gboolean
xviewer_image_apply_transformations (XviewerImage *img, GError **error)
{
	GdkPixbuf *transformed = NULL;
	XviewerTransform *composition = NULL;
	XviewerImagePrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	priv = img->priv;

	if (priv->trans == NULL && priv->trans_autorotate == NULL) {
		return TRUE;
	}

	if (priv->image == NULL) {
		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_NOT_LOADED,
			     _("Transformation on unloaded image."));

		return FALSE;
	}

	if (priv->trans != NULL && priv->trans_autorotate != NULL) {
		composition = xviewer_transform_compose (priv->trans,
						     priv->trans_autorotate);
	} else if (priv->trans != NULL) {
		composition = g_object_ref (priv->trans);
	} else if (priv->trans_autorotate != NULL) {
		composition = g_object_ref (priv->trans_autorotate);
	}

	if (composition != NULL) {
		transformed = xviewer_transform_apply (composition, priv->image, NULL);
	}

	g_object_unref (priv->image);
	priv->image = transformed;

	if (transformed != NULL) {
		priv->width = gdk_pixbuf_get_width (priv->image);
		priv->height = gdk_pixbuf_get_height (priv->image);
	} else {
		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_GENERIC,
			     _("Transformation failed."));
 	}

	g_object_unref (composition);

	return (transformed != NULL);
}

static void
xviewer_image_get_file_info (XviewerImage *img,
			 goffset *bytes,
			 gchar **mime_type,
			 GError **error)
{
    const gint BUFFER_SIZE = 100;            /* give the recognition based on content a reasonable chance */

    gboolean file_type_unknown = TRUE;

	static const struct magic {                                 /* 'magic number' code based on code from pix */
		const unsigned int offset;
		const unsigned int length;
		const char * const id_bytes;
		const char * const mime_type_str;
	}
	magic_type_ids [] = {
		{ 0,  8, "\x89PNG\x0d\x0a\x1a\x0a", "image/png" },
		{ 0,  4, "MM\x00\x2a",              "image/tiff" },
		{ 0,  4, "II\x2a\x00",              "image/tiff" },
		{ 0,  4, "GIF8",                    "image/gif" },
		{ 0,  3, "\xff\xd8\xff",            "image/jpeg" },
		{ 0,  2, "BM",                      "image/bmp" },
		{ 0,  4, "\x00\x00\x01\x00",        "image/vnd.microsoft.icon" },
		{ 8,  7, "WEBPVP8",                 "image/webp" }      /* whilst webp is not currently (9.12.2020)
                                                                   supported it has been included here in
                                                                   case support is added */

                                                                /* note that there are no magic numbers for
                                                                   AVIF and svg type files - so no tests here */
	};


    /* Note that as of 9.12.2020 g_file_query_info() and g_content_type_guess() both return application/octet-stream
       for AVIF files (with AVIF extensions. The correct MIME type is image/avif - assume that this will be corrected
       before xviewer supports AVIF files. */

	GFileInfo *file_info;

	file_info = g_file_query_info (img->priv->file,
				       G_FILE_ATTRIBUTE_STANDARD_SIZE ","
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, error);

	if (file_info == NULL) {
		if (bytes)
			*bytes = 0;

		if (mime_type)
			*mime_type = NULL;

		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_VFS,
			     "Error in getting image file info");
	} else {
		if (bytes)
			*bytes = g_file_info_get_size (file_info);

        /* g_file_query_info() (as of 9.12.2020) returns a MIME type based on the file
            extension - even if the extension is not the correct one for the file content.
            If there is no extension it examines the content of the file to determine
            the type. Use the result returned by g_file_query_info() as the default to
            be used if we can't determine the type based on content, otherwise use the type based
            on content rather than the file extension */
		if (mime_type)
        {
            GFileInputStream *input_stream;
            guchar ip_buffer [BUFFER_SIZE];
            gint bytes_read;
            char *mime_type_by_extn;

            input_stream = g_file_read (img->priv->file,
                                       NULL, error);
            if (input_stream)
            {
                bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
                                                 ip_buffer,
                                                 BUFFER_SIZE,
                                                 NULL, error);
                g_object_unref(G_OBJECT(input_stream));

                if (bytes_read != 0)
                {                               /* try to determine file type based on content */
                    gint i;

                    for (i = 0; i < G_N_ELEMENTS (magic_type_ids); i++) {
                        if ((magic_type_ids[i].offset + magic_type_ids[i].length) <= bytes_read)
                            if (! memcmp (ip_buffer + magic_type_ids[i].offset, magic_type_ids[i].id_bytes, magic_type_ids[i].length))
                                {
                                    *mime_type =  g_strdup (magic_type_ids[i].mime_type_str);
                                    file_type_unknown = FALSE;
                                    break;
                                }
                    }
                }

                if (file_type_unknown)
                {
                    char *content_type;
	                gboolean    result_uncertain = FALSE;
                    content_type = g_content_type_guess (NULL, ip_buffer, bytes_read, &result_uncertain);
                    if (content_type != NULL)
                    {
                        *mime_type =  g_strdup (content_type);
                        file_type_unknown = FALSE;
                        g_free (content_type);
                    }
                }
            }

            mime_type_by_extn = g_strdup (g_file_info_get_content_type (file_info));

            if (file_type_unknown)
               *mime_type = g_strdup (mime_type_by_extn);
            else
            {
                if ((strstr (*mime_type,"application/") == *mime_type) &&
                    (strstr (mime_type_by_extn,"image/") == mime_type_by_extn))
                    *mime_type = g_strdup (mime_type_by_extn);
            }

            g_free (mime_type_by_extn);
        }
		g_object_unref (file_info);
	}
}

#ifdef HAVE_LCMS
void
xviewer_image_apply_display_profile (XviewerImage *img, cmsHPROFILE screen)
{
	XviewerImagePrivate *priv;
	cmsHTRANSFORM transform;
	gint row, width, rows, stride;
	guchar *p;

	g_return_if_fail (img != NULL);

	priv = img->priv;

	if (screen == NULL) return;

	if (priv->profile == NULL) {
		/* Check whether GdkPixbuf was able to extract a profile */
		const char* data = gdk_pixbuf_get_option (priv->image,
		                                          "icc-profile");

		if(data) {
			gsize   profile_size = 0;
			guchar *profile_data = g_base64_decode(data,
			                                       &profile_size);

			if (profile_data && profile_size > 0) {
				xviewer_debug_message (DEBUG_LCMS,
				                   "Using ICC profile "
				                   "extracted by GdkPixbuf");
				priv->profile =
					cmsOpenProfileFromMem(profile_data,
					                      profile_size);
				g_free(profile_data);
			}
		}

		if(priv->profile == NULL) {
			/* Assume sRGB color space for images without ICC profile */
			xviewer_debug_message (DEBUG_LCMS, "Image has no ICC profile. "
					   "Assuming sRGB.");
			priv->profile = cmsCreate_sRGBProfile ();
		}
	}

	/* TODO: support other colorspaces than RGB */
	if (cmsGetColorSpace (priv->profile) != cmsSigRgbData ||
	    cmsGetColorSpace (screen) != cmsSigRgbData) {
		xviewer_debug_message (DEBUG_LCMS, "One or both ICC profiles not in RGB colorspace; not correcting");
		return;
	}

	cmsUInt32Number color_type = TYPE_RGB_8;

	if (gdk_pixbuf_get_has_alpha (priv->image))
		color_type = TYPE_RGBA_8;

	transform = cmsCreateTransform (priv->profile,
	                                color_type,
	                                screen,
	                                color_type,
	                                INTENT_PERCEPTUAL,
	                                0);

	if (G_LIKELY (transform != NULL)) {
		rows = gdk_pixbuf_get_height (priv->image);
		width = gdk_pixbuf_get_width (priv->image);
		stride = gdk_pixbuf_get_rowstride (priv->image);
		p = gdk_pixbuf_get_pixels (priv->image);

		for (row = 0; row < rows; ++row) {
			cmsDoTransform (transform, p, p, width);
			p += stride;
		}
		cmsDeleteTransform (transform);
	}
}

static void
xviewer_image_set_icc_data (XviewerImage *img, XviewerMetadataReader *md_reader)
{
	XviewerImagePrivate *priv = img->priv;

	priv->profile = xviewer_metadata_reader_get_icc_profile (md_reader);


}
#endif

static void
xviewer_image_set_orientation (XviewerImage *img)
{
	XviewerImagePrivate *priv;
#ifdef HAVE_EXIF
	ExifData* exif;
#endif

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

#ifdef HAVE_EXIF
	exif = (ExifData*) xviewer_image_get_exif_info (img);

	if (exif != NULL) {
		ExifByteOrder o = exif_data_get_byte_order (exif);

		ExifEntry *entry = exif_data_get_entry (exif,
							EXIF_TAG_ORIENTATION);

        if (priv->modified)
        {
            priv->orientation = 1;
        }
        else
		if (entry && entry->data != NULL) {
			priv->orientation = exif_get_short (entry->data, o);
		}

		exif_data_unref (exif);
	} else
#endif
	{
		GdkPixbuf *pbuf;

		pbuf = xviewer_image_get_pixbuf (img);

		if (pbuf) {
			const gchar *o_str;

			o_str = gdk_pixbuf_get_option (pbuf, "orientation");
			if (o_str) {
				short t = (short) g_ascii_strtoll (o_str,
								   NULL, 10);
				if (t >= 0 && t < 9)
					priv->orientation = t;
			}
			g_object_unref (pbuf);
		}
	}

	if (priv->orientation > 4 &&
        priv->orientation < 9 &&
        priv->autorotate) {
        gint tmp;

		tmp = priv->width;
		priv->width = priv->height;
		priv->height = tmp;
	}
}

static void
xviewer_image_real_autorotate (XviewerImage *img)
{
	static const XviewerTransformType lookup[8] = {XVIEWER_TRANSFORM_NONE,
					     XVIEWER_TRANSFORM_FLIP_HORIZONTAL,
					     XVIEWER_TRANSFORM_ROT_180,
					     XVIEWER_TRANSFORM_FLIP_VERTICAL,
					     XVIEWER_TRANSFORM_TRANSPOSE,
					     XVIEWER_TRANSFORM_ROT_90,
					     XVIEWER_TRANSFORM_TRANSVERSE,
					     XVIEWER_TRANSFORM_ROT_270};
	XviewerImagePrivate *priv;
	XviewerTransformType type;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

	type = (priv->orientation >= 1 && priv->orientation <= 8 ?
		lookup[priv->orientation - 1] : XVIEWER_TRANSFORM_NONE);

	if (type != XVIEWER_TRANSFORM_NONE) {
		img->priv->trans_autorotate = xviewer_transform_new (type);
	}

	/* Disable auto orientation for next loads */
	priv->autorotate = FALSE;
}

void
xviewer_image_autorotate (XviewerImage *img)
{
	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	/* Schedule auto orientation */
	img->priv->autorotate = TRUE;
}

#ifdef HAVE_EXEMPI
static void
xviewer_image_set_xmp_data (XviewerImage *img, XviewerMetadataReader *md_reader)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

	if (priv->xmp) {
		xmp_free (priv->xmp);
	}
	priv->xmp = xviewer_metadata_reader_get_xmp_data (md_reader);
}
#endif

static void
xviewer_image_set_exif_data (XviewerImage *img, XviewerMetadataReader *md_reader)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

#ifdef HAVE_EXIF
	g_mutex_lock (&priv->status_mutex);
	if (priv->exif) {
		exif_data_unref (priv->exif);
	}
	priv->exif = xviewer_metadata_reader_get_exif_data (md_reader);
	g_mutex_unlock (&priv->status_mutex);

	priv->exif_chunk = NULL;
	priv->exif_chunk_len = 0;

	/* EXIF data is already available, set the image orientation */
	if (priv->autorotate) {
		xviewer_image_set_orientation (img);

		/* Emit size prepared signal if we have the size */
		if (priv->width > 0 &&
		    priv->height > 0) {
			xviewer_image_emit_size_prepared (img);
		}
	}
#else
	if (priv->exif_chunk) {
		g_free (priv->exif_chunk);
	}
	xviewer_metadata_reader_get_exif_chunk (md_reader,
					    &priv->exif_chunk,
					    &priv->exif_chunk_len);
#endif
}

/*
 * Attempts to get the image dimensions from the thumbnail.
 * Returns FALSE if this information is not found.
 **/
static gboolean
xviewer_image_get_dimension_from_thumbnail (XviewerImage *image,
			                gint     *width,
			                gint     *height)
{
	if (image->priv->thumbnail == NULL)
		return FALSE;

	*width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image->priv->thumbnail),
						     XVIEWER_THUMBNAIL_ORIGINAL_WIDTH));
	*height = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (image->priv->thumbnail),
						      XVIEWER_THUMBNAIL_ORIGINAL_HEIGHT));

	return (*width || *height);
}

static gboolean
xviewer_image_real_load (XviewerImage *img,
		     guint     data2read,
		     XviewerJob   *job,
		     GError  **error)
{
	XviewerImagePrivate *priv;
	GFileInputStream *input_stream;
	XviewerMetadataReader *md_reader = NULL;
	GdkPixbufFormat *format;
	gchar *mime_type;
	GdkPixbufLoader *loader = NULL;
	guchar *buffer;
	goffset bytes_read, bytes_read_total = 0;
	gboolean failed = FALSE;
	gboolean first_run = TRUE;
	gboolean set_metadata = TRUE;
	gboolean use_rsvg = FALSE;
	gboolean read_image_data = (data2read & XVIEWER_IMAGE_DATA_IMAGE);
	gboolean read_only_dimension = (data2read & XVIEWER_IMAGE_DATA_DIMENSION) &&
				  ((data2read ^ XVIEWER_IMAGE_DATA_DIMENSION) == 0);


	priv = img->priv;

 	g_assert (!read_image_data || priv->image == NULL);

	if (read_image_data && priv->file_type != NULL) {
		g_free (priv->file_type);
		priv->file_type = NULL;
	}

	priv->threadsafe_format = FALSE;

	xviewer_image_get_file_info (img, &priv->bytes, &mime_type, error);

	if (error && *error) {
		g_free (mime_type);
		return FALSE;
	}

	if (read_only_dimension) {
		gint width, height;
		gboolean done;

		done = xviewer_image_get_dimension_from_thumbnail (img,
							       &width,
							       &height);

		if (done) {
			priv->width = width;
			priv->height = height;

			g_free (mime_type);
			return TRUE;
		}
	}

	input_stream = g_file_read (priv->file, NULL, error);

	if (input_stream == NULL) {
		g_free (mime_type);

		if (error != NULL) {
			g_clear_error (error);
			g_set_error (error,
				     XVIEWER_IMAGE_ERROR,
				     XVIEWER_IMAGE_ERROR_VFS,
				     "Failed to open input stream for file");
		}
		return FALSE;
	}

	buffer = g_new0 (guchar, XVIEWER_IMAGE_READ_BUFFER_SIZE);

	if (read_image_data || read_only_dimension) {
		gboolean checked_threadsafety = FALSE;

#ifdef HAVE_RSVG
		if (priv->svg != NULL) {
			g_object_unref (priv->svg);
			priv->svg = NULL;
		}

		if (!strcmp (mime_type, "image/svg+xml")
#if LIBRSVG_CHECK_FEATURE(SVGZ)
		    || !strcmp (mime_type, "image/svg+xml-compressed")
#endif
		) {
			gchar *file_path;
			/* Keep the object for rendering */
			priv->svg = rsvg_handle_new ();
			use_rsvg = (priv->svg != NULL);
			file_path = g_file_get_path (priv->file);
			rsvg_handle_set_base_uri (priv->svg, file_path);
			g_free (file_path);
		}
#endif

		if (!use_rsvg) {
			loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);

			if (error && *error) {
				g_error_free (*error);
				*error = NULL;

				loader = gdk_pixbuf_loader_new ();
			} else {
				/* The mimetype-based loader should know the
				 * format here already. */
				checked_threadsafety = check_loader_threadsafety (loader, &priv->threadsafe_format);
			}

		/* This is used to detect non-threadsafe loaders and disable
 		 * any possible asyncronous task that could bring deadlocks
 		 * to image loading process. */
			if (!checked_threadsafety)
				g_signal_connect (loader,
					  "size-prepared",
					  G_CALLBACK (xviewer_image_pre_size_prepared),
					  img);

			g_signal_connect_object (G_OBJECT (loader),
					 "size-prepared",
					 G_CALLBACK (xviewer_image_size_prepared),
					 img,
					 0);
		}
	}
	g_free (mime_type);

	while (!priv->cancel_loading) {
		/* FIXME: make this async */
		bytes_read = g_input_stream_read (G_INPUT_STREAM (input_stream),
						  buffer,
						  XVIEWER_IMAGE_READ_BUFFER_SIZE,
						  NULL, error);

		if (bytes_read == 0) {
			/* End of the file */
			break;
		} else if (bytes_read == -1) {
			failed = TRUE;

			g_set_error (error,
				     XVIEWER_IMAGE_ERROR,
				     XVIEWER_IMAGE_ERROR_VFS,
				     "Failed to read from input stream");

			break;
		}

        /* GIMP (version 2.10.22 and probably all versions since the option of saving colour space information was added)
           saves BMP files with a BITMAPV5HEADER when the colour space information is to be included. It sets the bV5Compression
           member to BI_BITFIELDS (3) for all six non-indexed variants of BMPs that it saves. This indicates that there is no compression of
           the data and that the header members bV5RedMask, bV5GreenMask and bV5BlueMask hold the bit masks for the three colours.
           The problem is that the Microsoft specification for BITMAPV5HEADER says that BI_BITFIELDS is only valid for 16 and 32-bit
           files (quite why is questionable) and GIMP sets the mask members correctly but technically the setting of the compression
           field is illegal and causes  gdk_pixbuf_loader_write() to return an error. (The three variants of 16-bit files and the
           two variants of 32-bit files have no problems)

           The same problem also applies to 1, 4 and 8 bpp index files saved with colour space information.

           The following statements check for the specific cases of 1, 4, 8 and 24-bit BMP files with colour space information included and
           with the compression type set incorrectly. For such files the bV5Compression member value is changed to BI_RGB (0) - a simple
           uncompressed format. */

        if ((bytes_read >= 0x1F) && (bytes_read_total == 0)){   /* only check the start of the file */
            if ((buffer[0] == 'B') && (buffer[1] == 'M')   /* if the file has the 'magic numbers' for a BMP... */
                && (buffer[0x0E] == 0x7C)                  /* ...and it has the 124. byte BITMAPv5HEADER (colour space info included)... */
                && ((buffer[0x1C] == 1)                    /* ...and it has 1, 4, 8 or 24 bits per pixel... */
                || (buffer[0x1C] == 4)
                || (buffer[0x1C] == 8)
                || (buffer[0x1C] == 24))
                && (buffer[0x1E] == 3))                    /* ...and bV5Compression is set to BI_BITFIELDS */
            {
                buffer[0x1E] = 0;                          /* set bV5Compression member to BI_RGB */
            }
        }

		if ((read_image_data || read_only_dimension)) {
#ifdef HAVE_RSVG
			if (use_rsvg) {
				gboolean res;

				res = rsvg_handle_write (priv->svg, buffer,
							 bytes_read, error);

				if (G_UNLIKELY (!res)) {
					failed = TRUE;
					break;
				}
			} else
#endif
			if (!gdk_pixbuf_loader_write (loader, buffer, bytes_read, error)) {
				failed = TRUE;
				break;
			}
		}

		bytes_read_total += bytes_read;

		/* For now allow calling from outside of jobs */
		if (job != NULL)
		{
			/* check that load job wasn't cancelled */
			if (xviewer_job_is_cancelled (job)) {
				xviewer_image_cancel_load (img);
				continue;
			} else {
				float progress = (float) bytes_read_total / (float) priv->bytes;
				xviewer_job_set_progress (job, progress);
			}
		}

		if (first_run) {
			md_reader = check_for_metadata_img_format (img, buffer, bytes_read);

			if (md_reader == NULL) {
				if (data2read == XVIEWER_IMAGE_DATA_EXIF) {
					g_set_error (error,
						     XVIEWER_IMAGE_ERROR,
                                	             XVIEWER_IMAGE_ERROR_GENERIC,
                                	             _("EXIF not supported for this file format."));
					break;
				}

				if (priv->threadsafe_format)
					xviewer_image_emit_size_prepared (img);

                                priv->metadata_status = XVIEWER_IMAGE_METADATA_NOT_AVAILABLE;
                        }

			first_run = FALSE;
		}

		if (md_reader != NULL) {
			xviewer_metadata_reader_consume (md_reader, buffer, bytes_read);

			if (xviewer_metadata_reader_finished (md_reader)) {
				if (set_metadata) {
					xviewer_image_set_exif_data (img, md_reader);

#ifdef HAVE_LCMS
					xviewer_image_set_icc_data (img, md_reader);
#endif

#ifdef HAVE_EXEMPI
					xviewer_image_set_xmp_data (img, md_reader);
#endif
					set_metadata = FALSE;
                                        priv->metadata_status = XVIEWER_IMAGE_METADATA_READY;
				}

				if (data2read == XVIEWER_IMAGE_DATA_EXIF)
					break;
			}
		}

		if (read_only_dimension &&
		    xviewer_image_has_data (img, XVIEWER_IMAGE_DATA_DIMENSION)) {
			break;
		}
	}

	if (read_image_data || read_only_dimension) {
#ifdef HAVE_RSVG
		if (use_rsvg) {
			rsvg_handle_close (priv->svg, error);
		} else
#endif
		if (failed) {
			gdk_pixbuf_loader_close (loader, NULL);
		} else if (!gdk_pixbuf_loader_close (loader, error)) {
			if (gdk_pixbuf_loader_get_pixbuf (loader) != NULL) {
				/* Clear error in order to support partial
				 * images as well. */
				g_clear_error (error);
			}
	        }
        }

	g_free (buffer);

	g_object_unref (G_OBJECT (input_stream));

	failed = (failed ||
		  priv->cancel_loading ||
		  bytes_read_total == 0 ||
		  (error && *error != NULL));

	if (failed) {
		if (priv->cancel_loading) {
			priv->cancel_loading = FALSE;
			priv->status = XVIEWER_IMAGE_STATUS_UNKNOWN;
		} else {
			priv->status = XVIEWER_IMAGE_STATUS_FAILED;
		}
	} else if (read_image_data) {
		if (priv->image != NULL) {
			g_object_unref (priv->image);
		}

#ifdef HAVE_RSVG
		if (use_rsvg) {
			priv->image = rsvg_handle_get_pixbuf (priv->svg);
		} else
#endif
		{

		priv->anim = gdk_pixbuf_loader_get_animation (loader);

		if (gdk_pixbuf_animation_is_static_image (priv->anim)) {
			priv->image = gdk_pixbuf_animation_get_static_image (priv->anim);
			priv->anim = NULL;
		} else {
			priv->anim_iter = gdk_pixbuf_animation_get_iter (priv->anim,NULL);
			priv->image = gdk_pixbuf_animation_iter_get_pixbuf (priv->anim_iter);
		}

		}

		if (G_LIKELY (priv->image != NULL)) {
			if (!use_rsvg)
				g_object_ref (priv->image);

			priv->width = gdk_pixbuf_get_width (priv->image);
			priv->height = gdk_pixbuf_get_height (priv->image);

			if (use_rsvg) {
				format = NULL;
				priv->file_type = g_strdup ("svg");
			} else {
				format = gdk_pixbuf_loader_get_format (loader);
			}

			if (format != NULL) {
				priv->file_type = gdk_pixbuf_format_get_name (format);
			}

			priv->file_is_changed = FALSE;

			/* Set orientation again for safety, eg. if we don't
			 * have Exif data or HAVE_EXIF is undefined. */
			xviewer_image_set_orientation (img);

			/* If it's non-threadsafe loader, then trigger window
 			 * showing in the end of the process. */
			if (!priv->threadsafe_format)
				xviewer_image_emit_size_prepared (img);
		} else {
			/* Some loaders don't report errors correctly.
			 * Error will be set below. */
			failed = TRUE;
			priv->status = XVIEWER_IMAGE_STATUS_FAILED;
		}
	}

	if (loader != NULL) {
		g_object_unref (loader);
	}

	if (md_reader != NULL) {
		g_object_unref (md_reader);
		md_reader = NULL;
	}

	/* Catch-all in case of poor-error reporting */
	if (failed && error && *error == NULL) {
		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_GENERIC,
			     _("Image loading failed."));
	}

	return !failed;
}

gboolean
xviewer_image_has_data (XviewerImage *img, XviewerImageData req_data)
{
	XviewerImagePrivate *priv;
	gboolean has_data = TRUE;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	priv = img->priv;

	if ((req_data & XVIEWER_IMAGE_DATA_IMAGE) > 0) {
		req_data = (req_data & ~XVIEWER_IMAGE_DATA_IMAGE);
		has_data = has_data && (priv->image != NULL);
	}

	if ((req_data & XVIEWER_IMAGE_DATA_DIMENSION) > 0 ) {
		req_data = (req_data & ~XVIEWER_IMAGE_DATA_DIMENSION);
		has_data = has_data && (priv->width >= 0) && (priv->height >= 0);
	}

	if ((req_data & XVIEWER_IMAGE_DATA_EXIF) > 0) {
		req_data = (req_data & ~XVIEWER_IMAGE_DATA_EXIF);
#ifdef HAVE_EXIF
		has_data = has_data && (priv->exif != NULL);
#else
		has_data = has_data && (priv->exif_chunk != NULL);
#endif
	}

	if ((req_data & XVIEWER_IMAGE_DATA_XMP) > 0) {
		req_data = (req_data & ~XVIEWER_IMAGE_DATA_XMP);
#ifdef HAVE_EXEMPI
		has_data = has_data && (priv->xmp != NULL);
#endif
	}

	if (req_data != 0) {
		g_warning ("Asking for unknown data, remaining: %i\n", req_data);
		has_data = FALSE;
	}

	return has_data;
}

gboolean
xviewer_image_load (XviewerImage *img, XviewerImageData data2read, XviewerJob *job, GError **error)
{
	XviewerImagePrivate *priv;
	gboolean success = FALSE;

	xviewer_debug (DEBUG_IMAGE_LOAD);

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	priv = XVIEWER_IMAGE (img)->priv;

	if (data2read == 0) {
		return TRUE;
	}

	if (xviewer_image_has_data (img, data2read)) {
		return TRUE;
	}

	priv->status = XVIEWER_IMAGE_STATUS_LOADING;

	success = xviewer_image_real_load (img, data2read, job, error);


	/* Check that the metadata was loaded at least once before
	 * trying to autorotate. Also only an image load job should try to
	 * autorotate an image. */
	if (priv->autorotate &&
#ifdef HAVE_EXIF
	    priv->metadata_status != XVIEWER_IMAGE_METADATA_NOT_READ &&
#endif
	    data2read & XVIEWER_IMAGE_DATA_IMAGE) {
		xviewer_image_real_autorotate (img);
	}

	if (success && xviewer_image_needs_transformation (img)) {
		success = xviewer_image_apply_transformations (img, error);
	}

	if (success) {
		priv->status = XVIEWER_IMAGE_STATUS_LOADED;
	} else {
		priv->status = XVIEWER_IMAGE_STATUS_FAILED;
	}

	return success;
}

void
xviewer_image_set_thumbnail (XviewerImage *img, GdkPixbuf *thumbnail)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));
	g_return_if_fail (GDK_IS_PIXBUF (thumbnail) || thumbnail == NULL);

	priv = img->priv;

	if (priv->thumbnail != NULL) {
		g_object_unref (priv->thumbnail);
		priv->thumbnail = NULL;
	}

	if (thumbnail != NULL && priv->trans != NULL) {
		priv->thumbnail = xviewer_transform_apply (priv->trans, thumbnail, NULL);
	} else {
		priv->thumbnail = thumbnail;

		if (thumbnail != NULL) {
			g_object_ref (priv->thumbnail);
		}
	}

	if (priv->thumbnail != NULL) {
		g_signal_emit (img, signals[SIGNAL_THUMBNAIL_CHANGED], 0);
	}
}

/**
 * xviewer_image_get_pixbuf:
 * @img: a #XviewerImage
 *
 * Gets the #GdkPixbuf of the image
 *
 * Returns: (transfer full): a #GdkPixbuf
 **/
GdkPixbuf *
xviewer_image_get_pixbuf (XviewerImage *img)
{
	GdkPixbuf *image = NULL;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	g_mutex_lock (&img->priv->status_mutex);
	image = img->priv->image;
	g_mutex_unlock (&img->priv->status_mutex);

	if (image != NULL) {
		g_object_ref (image);
	}

	return image;
}

#ifdef HAVE_LCMS
cmsHPROFILE
xviewer_image_get_profile (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	return img->priv->profile;
}
#endif

/**
 * xviewer_image_get_thumbnail:
 * @img: a #XviewerImage
 *
 * Gets the thumbnail pixbuf for @img
 *
 * Returns: (transfer full): a #GdkPixbuf with a thumbnail
 **/
GdkPixbuf *
xviewer_image_get_thumbnail (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	if (img->priv->thumbnail != NULL) {
		return g_object_ref (img->priv->thumbnail);
	}

	return NULL;
}

void
xviewer_image_get_size (XviewerImage *img, int *width, int *height)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

	*width = priv->width;
	*height = priv->height;
}

void
xviewer_image_transform (XviewerImage *img, XviewerTransform *trans, XviewerJob *job)
{
	xviewer_image_real_transform (img, trans, FALSE, job);
}

void
xviewer_image_undo (XviewerImage *img)
{
	XviewerImagePrivate *priv;
	XviewerTransform *trans;
	XviewerTransform *inverse;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

	if (priv->undo_stack != NULL) {
		trans = XVIEWER_TRANSFORM (priv->undo_stack->data);

		inverse = xviewer_transform_reverse (trans);

		xviewer_image_real_transform (img, inverse, TRUE, NULL);

		priv->undo_stack = g_slist_delete_link (priv->undo_stack, priv->undo_stack);

		g_object_unref (trans);
		g_object_unref (inverse);

		if (xviewer_transform_is_identity (priv->trans)) {
			g_object_unref (priv->trans);
			priv->trans = NULL;
		}
	}

	priv->modified = (priv->undo_stack != NULL);
}

static GFile *
tmp_file_get (void)
{
	GFile *tmp_file;
	char *tmp_file_path;
	gint fd;

	tmp_file_path = g_build_filename (g_get_tmp_dir (), "xviewer-save-XXXXXX", NULL);
	fd = g_mkstemp (tmp_file_path);
	if (fd == -1) {
		g_free (tmp_file_path);
		return NULL;
	}
	else {
		tmp_file = g_file_new_for_path (tmp_file_path);
		g_free (tmp_file_path);
		return tmp_file;
	}
}

static void
transfer_progress_cb (goffset cur_bytes,
		      goffset total_bytes,
		      gpointer user_data)
{
	XviewerImage *image = XVIEWER_IMAGE (user_data);

	if (cur_bytes > 0) {
		g_signal_emit (G_OBJECT(image),
			       signals[SIGNAL_SAVE_PROGRESS],
			       0,
			       (gfloat) cur_bytes / (gfloat) total_bytes);
	}
}

static void
tmp_file_restore_unix_attributes (GFile *temp_file,
				  GFile *target_file)
{
	GFileInfo *file_info;
	guint      uid;
	guint      gid;
	guint      mode;
	guint      mode_mask = 00600;

	GError    *error = NULL;

	g_return_if_fail (G_IS_FILE (temp_file));
	g_return_if_fail (G_IS_FILE (target_file));

	/* check if file exists */
	if (!g_file_query_exists (target_file, NULL)) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "Target file doesn't exist. Setting default attributes.");
		return;
	}

	/* retrieve UID, GID, and MODE of the original file info */
	file_info = g_file_query_info (target_file,
				       "unix::uid,unix::gid,unix::mode",
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       &error);

	/* check that there aren't any error */
	if (error != NULL) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "File information not available. Setting default attributes.");

		/* free objects */
		g_object_unref (file_info);
		g_clear_error (&error);

		return;
	}

	/* save UID, GID and MODE values */
	uid = g_file_info_get_attribute_uint32 (file_info,
						G_FILE_ATTRIBUTE_UNIX_UID);

	gid = g_file_info_get_attribute_uint32 (file_info,
						G_FILE_ATTRIBUTE_UNIX_GID);

	mode = g_file_info_get_attribute_uint32 (file_info,
						 G_FILE_ATTRIBUTE_UNIX_MODE);

	/* apply default mode mask to file mode */
	mode |= mode_mask;

	/* restore original UID, GID, and MODE into the temporal file */
	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_UID,
				     uid,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file UID.");

		g_clear_error (&error);
	}

	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_GID,
				     gid,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file GID. Setting user default GID.");

		g_clear_error (&error);
	}

	g_file_set_attribute_uint32 (temp_file,
				     G_FILE_ATTRIBUTE_UNIX_MODE,
				     mode,
				     G_FILE_QUERY_INFO_NONE,
				     NULL,
				     &error);

	/* check that there aren't any error */
	if (error != NULL) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "You do not have the permissions necessary to change the file MODE.");

		g_clear_error (&error);
	}

	/* free objects */
	g_object_unref (file_info);
}

static gboolean
tmp_file_move_to_uri (XviewerImage *image,
		      GFile *tmpfile,
		      GFile *file,
		      gboolean overwrite,
		      GError **error)
{
	gboolean result;
	GError *ioerror = NULL;

	/* try to restore target file unix attributes */
	tmp_file_restore_unix_attributes (tmpfile, file);

	/* replace target file with temporal file */
	result = g_file_move (tmpfile,
			      file,
			      (overwrite ? G_FILE_COPY_OVERWRITE : 0) |
			      G_FILE_COPY_ALL_METADATA,
			      NULL,
			      (GFileProgressCallback) transfer_progress_cb,
			      image,
			      &ioerror);

	if (result == FALSE) {
		if (g_error_matches (ioerror, G_IO_ERROR,
				     G_IO_ERROR_EXISTS)) {
			g_set_error (error, XVIEWER_IMAGE_ERROR,
				     XVIEWER_IMAGE_ERROR_FILE_EXISTS,
				     "File exists");
		} else {
			g_set_error (error, XVIEWER_IMAGE_ERROR,
				     XVIEWER_IMAGE_ERROR_VFS,
				     "VFS error moving the temp file");
		}
		g_clear_error (&ioerror);
	}

	return result;
}

static gboolean
tmp_file_delete (GFile *tmpfile)
{
	gboolean result;
	GError *err = NULL;

	if (tmpfile == NULL) return FALSE;

	result = g_file_delete (tmpfile, NULL, &err);
	if (result == FALSE) {
		char *tmpfile_path;
		if (err != NULL) {
			if (err->code == G_IO_ERROR_NOT_FOUND) {
				g_error_free (err);
				return TRUE;
			}
			g_error_free (err);
		}
		tmpfile_path = g_file_get_path (tmpfile);
		g_warning ("Couldn't delete temporary file: %s", tmpfile_path);
		g_free (tmpfile_path);
	}

	return result;
}

static void
xviewer_image_reset_modifications (XviewerImage *image)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	priv = image->priv;

	g_slist_foreach (priv->undo_stack, (GFunc) g_object_unref, NULL);
	g_slist_free (priv->undo_stack);
	priv->undo_stack = NULL;

	if (priv->trans != NULL) {
		g_object_unref (priv->trans);
		priv->trans = NULL;
	}

	if (priv->trans_autorotate != NULL) {
		g_object_unref (priv->trans_autorotate);
		priv->trans_autorotate = NULL;
	}

	priv->modified = FALSE;
}

static void
xviewer_image_link_with_target (XviewerImage *image, XviewerImageSaveInfo *target)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (image));
	g_return_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (target));

	priv = image->priv;

	/* update file location */
	if (priv->file != NULL) {
		g_object_unref (priv->file);
	}
	priv->file = g_object_ref (target->file);

	/* Clear caption and caption key, these will be
	 * updated on next xviewer_image_get_caption call.
	 */
	if (priv->caption != NULL) {
		g_free (priv->caption);
		priv->caption = NULL;
	}
	if (priv->collate_key != NULL) {
		g_free (priv->collate_key);
		priv->collate_key = NULL;
	}

	/* update file format */
	if (priv->file_type != NULL) {
		g_free (priv->file_type);
	}
	priv->file_type = g_strdup (target->format);
}

static gboolean
check_if_file_is_writable (GFile *file)
{
	GFile           *file_to_check;
	GFileInfo	*file_info;
	GError		*error = NULL;
	gboolean	 is_writable;

	g_return_val_if_fail (G_IS_FILE (file), FALSE);

	/* check if file exists */
	if (!g_file_query_exists (file, NULL)) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE, "File doesn't exist. "
				   "Checking parent directory.");

		file_to_check = g_file_get_parent (file);
	} else {
		/* Add another ref so we don't need to split between file and
		 * parent file again after querying and can simply unref it. */
		file_to_check = g_object_ref (file);
	}

	/* recover file information */
	file_info = g_file_query_info (file_to_check,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
				       G_FILE_QUERY_INFO_NONE,
				       NULL,
				       &error);

	/* we assume that the image can't be saved when
	   we can't retrieve any file information */
	if (G_UNLIKELY (file_info == NULL)) {
		xviewer_debug_message (DEBUG_IMAGE_SAVE,
				   "Couldn't query file info: %s",
				   error->message);
		g_error_free (error);
		g_object_unref (file_to_check);
		return FALSE;
	}

	/* check if file can be writed */
	is_writable = g_file_info_get_attribute_boolean (file_info,
							 G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE);

	/* free objects */
	g_object_unref (file_info);
	g_object_unref (file_to_check);

	return is_writable;
}

gboolean
xviewer_image_save_by_info (XviewerImage *img, XviewerImageSaveInfo *source, GError **error)
{
	XviewerImagePrivate *priv;
	XviewerImageStatus prev_status;
	gboolean success = FALSE;
	GFile *tmp_file;
	char *tmp_file_path;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (source), FALSE);

	priv = img->priv;

	prev_status = priv->status;

	/* Image is now being saved */
	priv->status = XVIEWER_IMAGE_STATUS_SAVING;

	/* see if we need any saving at all */
	if (source->exists && !source->modified) {
		return TRUE;
	}

	/* fail if there is no image to save */
	if (priv->image == NULL) {
		g_set_error (error, XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_NOT_LOADED,
			     _("No image loaded."));
		return FALSE;
	}

	/* fail if there is not write rights to save */
	if (!check_if_file_is_writable (priv->file)) {
		g_set_error (error, XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_NOT_SAVED,
			     _("You do not have the permissions necessary to save the file."));
		return FALSE;
	}

	/* generate temporary file */
	tmp_file = tmp_file_get ();

	if (tmp_file == NULL) {
		g_set_error (error, XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_TMP_FILE_FAILED,
			     _("Temporary file creation failed."));
		return FALSE;
	}

	tmp_file_path = g_file_get_path (tmp_file);

#ifdef HAVE_JPEG
	/* determine kind of saving */
	if ((g_ascii_strcasecmp (source->format, XVIEWER_FILE_FORMAT_JPEG) == 0) &&
	    source->exists && source->modified)
	{
		success = xviewer_image_jpeg_save_file (img, tmp_file_path, source, NULL, error);
	}
#endif

	if (!success && (*error == NULL)) {
		success = gdk_pixbuf_save (priv->image, tmp_file_path, source->format, error, NULL);
	}

	if (success) {
		/* try to move result file to target uri */
		success = tmp_file_move_to_uri (img, tmp_file, priv->file, TRUE /*overwrite*/, error);
	}

	if (success) {
		xviewer_image_reset_modifications (img);
	}

	tmp_file_delete (tmp_file);

	g_free (tmp_file_path);
	g_object_unref (tmp_file);

	priv->status = prev_status;

	return success;
}

static gboolean
xviewer_image_copy_file (XviewerImage *image, XviewerImageSaveInfo *source, XviewerImageSaveInfo *target, GError **error)
{
	gboolean result;
	GError *ioerror = NULL;

	g_return_val_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (source), FALSE);
	g_return_val_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (target), FALSE);

	/* copy the image */
	result = g_file_copy (source->file,
			      target->file,
			      (target->overwrite ? G_FILE_COPY_OVERWRITE : 0) |
			      G_FILE_COPY_ALL_METADATA,
			      NULL,
			      XVIEWER_IS_IMAGE (image) ? transfer_progress_cb :NULL,
			      image,
			      &ioerror);

	if (result == FALSE) {
		if (ioerror->code == G_IO_ERROR_EXISTS) {
			g_set_error (error, XVIEWER_IMAGE_ERROR,
				     XVIEWER_IMAGE_ERROR_FILE_EXISTS,
				     "%s", ioerror->message);
		} else {
		g_set_error (error, XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_VFS,
			     "%s", ioerror->message);
		}
		g_error_free (ioerror);
	} else {
		/* reset NAUTILUS-ICON-POSITION metadata attribute */
		g_file_set_attribute (target->file,
				      "metadata::nautilus-icon-position",
				      G_FILE_ATTRIBUTE_TYPE_INVALID,
				      NULL,
				      G_FILE_QUERY_INFO_NONE,
				      NULL,
				      NULL);
	}

	return result;
}

gboolean
xviewer_image_save_as_by_info (XviewerImage *img, XviewerImageSaveInfo *source, XviewerImageSaveInfo *target, GError **error)
{
	XviewerImagePrivate *priv;
	gboolean success = FALSE;
	char *tmp_file_path;
	GFile *tmp_file;
	gboolean direct_copy = FALSE;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (source), FALSE);
	g_return_val_if_fail (XVIEWER_IS_IMAGE_SAVE_INFO (target), FALSE);

	priv = img->priv;

	/* fail if there is no image to save */
	if (priv->image == NULL) {
		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_NOT_LOADED,
			     _("No image loaded."));

		return FALSE;
	}

	/* fail if there is not write rights to save on target */
	if (!check_if_file_is_writable (target->file)) {
		g_set_error (error, XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_NOT_SAVED,
			     _("You do not have the permissions necessary to save the file."));
		return FALSE;
	}

	/* generate temporary file name */
	tmp_file = tmp_file_get ();

	if (tmp_file == NULL) {
		g_set_error (error,
			     XVIEWER_IMAGE_ERROR,
			     XVIEWER_IMAGE_ERROR_TMP_FILE_FAILED,
			     _("Temporary file creation failed."));

		return FALSE;
	}
	tmp_file_path = g_file_get_path (tmp_file);

	/* determine kind of saving */
	if (g_ascii_strcasecmp (source->format, target->format) == 0 && !source->modified) {
		success = xviewer_image_copy_file (img, source, target, error);
		direct_copy = success;
	}

#ifdef HAVE_JPEG
	else if ((g_ascii_strcasecmp (source->format, XVIEWER_FILE_FORMAT_JPEG) == 0 && source->exists) ||
		 (g_ascii_strcasecmp (target->format, XVIEWER_FILE_FORMAT_JPEG) == 0))
	{
		success = xviewer_image_jpeg_save_file (img, tmp_file_path, source, target, error);
	}
#endif

	if (!success && (*error == NULL)) {
		success = gdk_pixbuf_save (priv->image, tmp_file_path, target->format, error, NULL);
	}

	if (success && !direct_copy) { /* not required if we alredy copied the file directly */
		/* try to move result file to target uri */
		success = tmp_file_move_to_uri (img, tmp_file, target->file, target->overwrite, error);
	}

	if (success) {
		/* update image information to new uri */
		xviewer_image_reset_modifications (img);
		xviewer_image_link_with_target (img, target);
	}

	tmp_file_delete (tmp_file);
	g_object_unref (tmp_file);
	g_free (tmp_file_path);

	priv->status = XVIEWER_IMAGE_STATUS_UNKNOWN;

	return success;
}

/*
 * This function is inspired by
 * nautilus/libnautilus-private/nautilus-file.c:nautilus_file_get_display_name_nocopy
 * Revision: 1.309
 * Author: Darin Adler <darin@bentspoon.com>
 */
const gchar*
xviewer_image_get_caption (XviewerImage *img)
{
	XviewerImagePrivate *priv;
	GFileInfo *info;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->file == NULL) return NULL;

	if (priv->caption != NULL)
		/* Use cached caption string */
		return priv->caption;

	info = g_file_query_info (priv->file,
				  G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME,
				  G_FILE_QUERY_INFO_NONE,
				  NULL,
				  NULL);

	if (G_LIKELY (info != NULL))
	{
		priv->caption = g_strdup (g_file_info_get_display_name (info));
		g_object_unref(info);
	}

	if (G_UNLIKELY (priv->caption == NULL)) {
		char *short_str;

		short_str = g_file_get_basename (priv->file);
		if (g_utf8_validate (short_str, -1, NULL)) {
			priv->caption = g_strdup (short_str);
		} else {
			priv->caption = g_filename_to_utf8 (short_str, -1, NULL, NULL, NULL);
		}
		g_free (short_str);
	}

	return priv->caption;
}

const gchar*
xviewer_image_get_collate_key (XviewerImage *img)
{
	XviewerImagePrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->collate_key == NULL) {
		const char *caption;

		caption = xviewer_image_get_caption (img);

		priv->collate_key = g_utf8_collate_key_for_filename (caption, -1);
	}

	return priv->collate_key;
}

void
xviewer_image_cancel_load (XviewerImage *img)
{
	XviewerImagePrivate *priv;

	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	priv = img->priv;

	g_mutex_lock (&priv->status_mutex);

	if (priv->status == XVIEWER_IMAGE_STATUS_LOADING) {
		priv->cancel_loading = TRUE;
	}

	g_mutex_unlock (&priv->status_mutex);
}

#ifdef HAVE_EXIF
ExifData *
xviewer_image_get_exif_info (XviewerImage *img)
{
	XviewerImagePrivate *priv;
	ExifData *data = NULL;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	priv = img->priv;

	g_mutex_lock (&priv->status_mutex);

	exif_data_ref (priv->exif);
	data = priv->exif;

	g_mutex_unlock (&priv->status_mutex);

	return data;
}
#endif

/**
 * xviewer_image_get_xmp_info:
 * @img: a #XviewerImage
 *
 * Gets the XMP info for @img or NULL if compiled without
 * libexempi support.
 *
 * Returns: (transfer full): the xmp data
 **/
gpointer
xviewer_image_get_xmp_info (XviewerImage *img)
{
 	gpointer data = NULL;

 	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

#ifdef HAVE_EXEMPI
	XviewerImagePrivate *priv;
 	priv = img->priv;

	g_mutex_lock (&priv->status_mutex);
 	data = (gpointer) xmp_copy (priv->xmp);
	g_mutex_unlock (&priv->status_mutex);
#endif

 	return data;
}


/**
 * xviewer_image_get_file:
 * @img: a #XviewerImage
 *
 * Gets the #GFile associated with @img
 *
 * Returns: (transfer full): a #GFile
 **/
GFile *
xviewer_image_get_file (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	return g_object_ref (img->priv->file);
}

gboolean
xviewer_image_is_modified (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	return img->priv->modified;
}

goffset
xviewer_image_get_bytes (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), 0);

	return img->priv->bytes;
}

void
xviewer_image_modified (XviewerImage *img)
{
	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	g_signal_emit (G_OBJECT (img), signals[SIGNAL_CHANGED], 0);
}

gchar*
xviewer_image_get_uri_for_display (XviewerImage *img)
{
	XviewerImagePrivate *priv;
	gchar *uri_str = NULL;
	gchar *str = NULL;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	priv = img->priv;

	if (priv->file != NULL) {
		uri_str = g_file_get_uri (priv->file);

		if (uri_str != NULL) {
			str = g_uri_unescape_string (uri_str, NULL);
			g_free (uri_str);
		}
	}

	return str;
}

XviewerImageStatus
xviewer_image_get_status (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), XVIEWER_IMAGE_STATUS_UNKNOWN);

	return img->priv->status;
}

/**
 * xviewer_image_get_metadata_status:
 * @img: a #XviewerImage
 *
 * Returns the current status of the image metadata, that is,
 * whether the metadata has not been read yet, is ready, or not available at all.
 *
 * Returns: one of #XviewerImageMetadataStatus
 **/
XviewerImageMetadataStatus
xviewer_image_get_metadata_status (XviewerImage *img)
{
        g_return_val_if_fail (XVIEWER_IS_IMAGE (img), XVIEWER_IMAGE_METADATA_NOT_AVAILABLE);

        return img->priv->metadata_status;
}

void
xviewer_image_data_ref (XviewerImage *img)
{
	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	g_object_ref (G_OBJECT (img));
	img->priv->data_ref_count++;

	g_assert (img->priv->data_ref_count <= G_OBJECT (img)->ref_count);
}

void
xviewer_image_data_unref (XviewerImage *img)
{
	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	if (img->priv->data_ref_count > 0) {
		img->priv->data_ref_count--;
	} else {
		g_warning ("More image data unrefs than refs.");
	}

	if (img->priv->data_ref_count == 0) {
		xviewer_image_free_mem_private (img);
	}

	g_object_unref (G_OBJECT (img));

	g_assert (img->priv->data_ref_count <= G_OBJECT (img)->ref_count);
}

static gint
compare_quarks (gconstpointer a, gconstpointer b)
{
	GQuark quark;

	quark = g_quark_from_string ((const gchar *) a);

	return quark - GPOINTER_TO_INT (b);
}

/**
 * xviewer_image_get_supported_mime_types:
 *
 * Gets the list of supported mimetypes
 *
 * Returns: (transfer none)(element-type utf8): a #GList of supported mimetypes
 **/
GList *
xviewer_image_get_supported_mime_types (void)
{
	GSList *format_list, *it;
	gchar **mime_types;
	int i;

	if (!supported_mime_types) {
		format_list = gdk_pixbuf_get_formats ();

		for (it = format_list; it != NULL; it = it->next) {
			mime_types =
				gdk_pixbuf_format_get_mime_types ((GdkPixbufFormat *) it->data);

			for (i = 0; mime_types[i] != NULL; i++) {
				supported_mime_types =
					g_list_prepend (supported_mime_types,
							g_strdup (mime_types[i]));
			}

			g_strfreev (mime_types);
		}

		supported_mime_types = g_list_sort (supported_mime_types,
						    (GCompareFunc) compare_quarks);

		g_slist_free (format_list);
	}

	return supported_mime_types;
}

gboolean
xviewer_image_is_supported_mime_type (const char *mime_type)
{
	GList *supported_mime_types, *result;
	GQuark quark;

	if (mime_type == NULL) {
		return FALSE;
	}

	supported_mime_types = xviewer_image_get_supported_mime_types ();

	quark = g_quark_from_string (mime_type);

	result = g_list_find_custom (supported_mime_types,
				     GINT_TO_POINTER (quark),
				     (GCompareFunc) compare_quarks);

	return (result != NULL);
}

static gboolean
xviewer_image_iter_advance (XviewerImage *img)
{
	XviewerImagePrivate *priv;
 	gboolean new_frame;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);
	g_return_val_if_fail (GDK_IS_PIXBUF_ANIMATION_ITER (img->priv->anim_iter), FALSE);

	priv = img->priv;

	if ((new_frame = gdk_pixbuf_animation_iter_advance (img->priv->anim_iter, NULL)) == TRUE)
	  {
		g_mutex_lock (&priv->status_mutex);
		g_object_unref (priv->image);
		priv->image = gdk_pixbuf_animation_iter_get_pixbuf (priv->anim_iter);
	 	g_object_ref (priv->image);
		/* keep the transformation over time */
		if (XVIEWER_IS_TRANSFORM (priv->trans)) {
			GdkPixbuf* transformed = xviewer_transform_apply (priv->trans, priv->image, NULL);
			g_object_unref (priv->image);
			priv->image = transformed;
			priv->width = gdk_pixbuf_get_width (transformed);
			priv->height = gdk_pixbuf_get_height (transformed);
		}
		g_mutex_unlock (&priv->status_mutex);
		/* Emit next frame signal so we can update the display */
		g_signal_emit (img, signals[SIGNAL_NEXT_FRAME], 0,
			       gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter));
	  }

	return new_frame;
}

/**
 * xviewer_image_is_animation:
 * @img: a #XviewerImage
 *
 * Checks whether a given image is animated.
 *
 * Returns: #TRUE if it is an animated image, #FALSE otherwise.
 *
 **/
gboolean
xviewer_image_is_animation (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);
	return img->priv->anim != NULL;
}

static gboolean
private_timeout (gpointer data)
{
	XviewerImage *img = XVIEWER_IMAGE (data);
	XviewerImagePrivate *priv = img->priv;

	if (xviewer_image_is_animation (img) &&
	    !g_source_is_destroyed (g_main_current_source ()) &&
	    priv->is_playing) {
		if (xviewer_image_iter_advance (img) && gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter) != -1) {
			g_timeout_add (gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter), private_timeout, img);
	 		return FALSE;
		}
 	}
	priv->is_playing = FALSE;
	return FALSE; /* stop playing */
}

/**
 * xviewer_image_start_animation:
 * @img: a #XviewerImage
 *
 * Starts playing an animated image.
 *
 * Returns: %TRUE on success, %FALSE if @img is already playing or isn't an animated image.
 **/
gboolean
xviewer_image_start_animation (XviewerImage *img)
{
	XviewerImagePrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);
	priv = img->priv;

	if (!xviewer_image_is_animation (img) || priv->is_playing)
		return FALSE;

	g_mutex_lock (&priv->status_mutex);
	g_object_ref (priv->anim_iter);
	priv->is_playing = TRUE;
	g_mutex_unlock (&priv->status_mutex);

 	g_timeout_add (gdk_pixbuf_animation_iter_get_delay_time (priv->anim_iter), private_timeout, img);

	return TRUE;
}

#ifdef HAVE_RSVG
gboolean
xviewer_image_is_svg (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	return (img->priv->svg != NULL);
}

RsvgHandle *
xviewer_image_get_svg (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	return img->priv->svg;
}
#endif


XviewerTransform *
xviewer_image_get_transform (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	return img->priv->trans;
}

XviewerTransform*
xviewer_image_get_autorotate_transform (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), NULL);

	return img->priv->trans_autorotate;
}

/**
 * xviewer_image_file_changed:
 * @img: a #XviewerImage
 *
 * Marks the image file contents as changed. Also, emits
 * XviewerImage::file-changed signal.
 **/
void
xviewer_image_file_changed (XviewerImage *img)
{
	g_return_if_fail (XVIEWER_IS_IMAGE (img));

	img->priv->file_is_changed = TRUE;
	g_signal_emit (img, signals[SIGNAL_FILE_CHANGED], 0);
}

gboolean
xviewer_image_is_file_changed (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), TRUE);

	return img->priv->file_is_changed;
}

/**
 * xviewer_image_is_file_writable:
 * @img: a #XviewerImage
 *
 * Evaluate if the user has write permission on the image file.
 *
 * Returns: %TRUE on success, %FALSE if the user hasn't write permissions on it,
 * or @img is not an #XviewerImage.
 **/
gboolean
xviewer_image_is_file_writable (XviewerImage *img)
{
	gboolean is_writable;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	is_writable = check_if_file_is_writable (img->priv->file);

	return is_writable;
}

gboolean
xviewer_image_is_jpeg (XviewerImage *img)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (img), FALSE);

	return ((img->priv->file_type != NULL) && (g_ascii_strcasecmp (img->priv->file_type, XVIEWER_FILE_FORMAT_JPEG) == 0));
}

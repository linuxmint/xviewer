/* This code is based on the jpeg saving code from gdk-pixbuf. Full copyright
 * notice is given in the following:
 */
/* GdkPixbuf library - JPEG image loader
 *
 * Copyright (C) 1999 Michael Zucchi
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Progressive loading code Copyright (C) 1999 Red Hat, Inc.
 *
 * Authors: Michael Zucchi <zucchi@zedzone.mmc.com.au>
 *          Federico Mena-Quintero <federico@gimp.org>
 *          Michael Fulbright <drmike@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "xviewer-image-jpeg.h"
#include "xviewer-image-private.h"
#include "xviewer-debug.h"

#if HAVE_JPEG

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <jerror.h>
#include "transupp.h"
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gi18n.h>
#if HAVE_EXIF
#include <libexif/exif-data.h>
#endif

#ifdef G_OS_WIN32
#define sigjmp_buf jmp_buf
#define sigsetjmp(env, savesigs) setjmp (env)
#define siglongjmp longjmp
#endif

typedef enum {
	XVIEWER_SAVE_NONE,
	XVIEWER_SAVE_JPEG_AS_JPEG,
	XVIEWER_SAVE_ANY_AS_JPEG
} XviewerJpegSaveMethod;

/* error handler data */
struct error_handler_data {
	struct jpeg_error_mgr pub;
	sigjmp_buf setjmp_buffer;
        GError **error;
	char *filename;
};


static void
fatal_error_handler (j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
        char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *) cinfo->err;

        /* Create the message */
        (* cinfo->err->format_message) (cinfo, buffer);

        /* broken check for *error == NULL for robustness against
         * crappy JPEG library
         */
        if (errmgr->error && *errmgr->error == NULL) {
                g_set_error (errmgr->error,
			     0,
			     0,
			     "Error interpreting JPEG image file: %s\n\n%s",
			     g_path_get_basename (errmgr->filename),
                             buffer);
        }

	siglongjmp (errmgr->setjmp_buffer, 1);

        g_assert_not_reached ();
}


static void
output_message_handler (j_common_ptr cinfo)
{
	/* This method keeps libjpeg from dumping crap to stderr */
	/* do nothing */
}

static void
init_transform_info (XviewerImage *image, jpeg_transform_info *info)
{
	XviewerImagePrivate *priv;
	XviewerTransform *composition = NULL;
	XviewerTransformType transformation;
	JXFORM_CODE trans_code = JXFORM_NONE;

	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	memset (info, 0x0, sizeof (jpeg_transform_info));

	priv = image->priv;

	if (priv->trans != NULL && priv->trans_autorotate != NULL) {
		composition = xviewer_transform_compose (priv->trans,
						     priv->trans_autorotate);
	} else if (priv->trans != NULL) {
		composition = g_object_ref (priv->trans);
	} else if (priv->trans_autorotate != NULL) {
		composition = g_object_ref (priv->trans_autorotate);
	}

	if (composition != NULL) {
		transformation = xviewer_transform_get_transform_type (composition);

		switch (transformation) {
		case XVIEWER_TRANSFORM_ROT_90:
			trans_code = JXFORM_ROT_90;
			break;
		case XVIEWER_TRANSFORM_ROT_270:
			trans_code = JXFORM_ROT_270;
			break;
		case XVIEWER_TRANSFORM_ROT_180:
			trans_code = JXFORM_ROT_180;
			break;
		case XVIEWER_TRANSFORM_FLIP_HORIZONTAL:
			trans_code = JXFORM_FLIP_H;
			break;
		case XVIEWER_TRANSFORM_FLIP_VERTICAL:
			trans_code = JXFORM_FLIP_V;
			break;
		case XVIEWER_TRANSFORM_TRANSPOSE:
			trans_code = JXFORM_TRANSPOSE;
			break;
		case XVIEWER_TRANSFORM_TRANSVERSE:
			trans_code = JXFORM_TRANSVERSE;
			break;
		default:
			g_warning("XviewerTransformType not supported!");
			/* Fallthrough intended here. */
		case XVIEWER_TRANSFORM_NONE:
			trans_code = JXFORM_NONE;
			break;
		}
	}

	info->transform = trans_code;
	info->trim      = FALSE;
#if JPEG_LIB_VERSION >= 80
	info->crop = FALSE;
#endif
	info->force_grayscale = FALSE;

	g_object_unref (composition);
}

static void
xviewer_image_jpeg_update_exif_data (XviewerImage *image)
{
#ifdef HAVE_EXIF
	XviewerImagePrivate *priv;
	ExifEntry *entry;
	ExifByteOrder bo;

	xviewer_debug (DEBUG_IMAGE_DATA);

	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	priv = image->priv;

	if (priv->exif == NULL) return;

	bo = exif_data_get_byte_order (priv->exif);

	/* Update image width */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_PIXEL_X_DIMENSION);
	if (entry != NULL && (priv->width >= 0)) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, priv->width);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, priv->width);
		else
			g_warning ("Exif entry has unsupported size");
	}

	/* Update image height */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_PIXEL_Y_DIMENSION);
	if (entry != NULL && (priv->height >= 0)) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, priv->height);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, priv->height);
		else
			g_warning ("Exif entry has unsupported size");
	}

	/* Update image orientation */
	entry = exif_data_get_entry (priv->exif, EXIF_TAG_ORIENTATION);
	if (entry != NULL) {
		if (entry->format == EXIF_FORMAT_LONG)
			exif_set_long (entry->data, bo, priv->orientation);
		else if (entry->format == EXIF_FORMAT_SHORT)
			exif_set_short (entry->data, bo, priv->orientation);
		else
			g_warning ("Exif entry has unsupported size");

	}
#endif
}


static gboolean
_save_jpeg_as_jpeg (XviewerImage *image, const char *file, XviewerImageSaveInfo *source,
		    XviewerImageSaveInfo *target, GError **error)
{
	struct jpeg_decompress_struct  srcinfo;
	struct jpeg_compress_struct    dstinfo;
	struct error_handler_data      jsrcerr, jdsterr;
	jpeg_transform_info            transformoption;
	jvirt_barray_ptr              *src_coef_arrays;
	jvirt_barray_ptr              *dst_coef_arrays;
	FILE                          *output_file;
	FILE                          *input_file;
	XviewerImagePrivate           *priv;
	gchar                         *infile_uri;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (image), FALSE);
	g_return_val_if_fail (XVIEWER_IMAGE (image)->priv->file != NULL, FALSE);

	priv = image->priv;

	init_transform_info (image, &transformoption);

	/* Initialize the JPEG decompression object with default error
	 * handling. */
	jsrcerr.filename = g_file_get_path (priv->file);
	srcinfo.err = jpeg_std_error (&(jsrcerr.pub));
	jsrcerr.pub.error_exit = fatal_error_handler;
	jsrcerr.pub.output_message = output_message_handler;
	jsrcerr.error = error;

	jpeg_create_decompress (&srcinfo);

	/* Initialize the JPEG compression object with default error
	 * handling. */
	jdsterr.filename = (char *) file;
	dstinfo.err = jpeg_std_error (&(jdsterr.pub));
	jdsterr.pub.error_exit = fatal_error_handler;
	jdsterr.pub.output_message = output_message_handler;
	jdsterr.error = error;

	jpeg_create_compress (&dstinfo);

	dstinfo.err->trace_level = 0;
	dstinfo.arith_code = FALSE;
	dstinfo.optimize_coding = FALSE;

	jsrcerr.pub.trace_level = jdsterr.pub.trace_level;
	srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;

	/* Open the output file. */
	/* FIXME: Make this a GIO aware input manager */
	infile_uri = g_file_get_path (priv->file);
	input_file = fopen (infile_uri, "rb");
	if (input_file == NULL) {
		g_warning ("Input file not openable: %s\n", infile_uri);
		g_free (jsrcerr.filename);
		g_free (infile_uri);
		return FALSE;
	}
	g_free (infile_uri);

	output_file = fopen (file, "wb");
	if (output_file == NULL) {
		g_warning ("Output file not openable: %s\n", file);
		fclose (input_file);
		g_free (jsrcerr.filename);
		return FALSE;
	}

	if (sigsetjmp (jsrcerr.setjmp_buffer, 1)) {
		fclose (output_file);
		fclose (input_file);
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		g_free (jsrcerr.filename);
		return FALSE;
	}

	if (sigsetjmp (jdsterr.setjmp_buffer, 1)) {
		fclose (output_file);
		fclose (input_file);
		jpeg_destroy_compress (&dstinfo);
		jpeg_destroy_decompress (&srcinfo);
		g_free (jsrcerr.filename);
		return FALSE;
	}

	/* Specify data source for decompression */
	jpeg_stdio_src (&srcinfo, input_file);

	/* Enable saving of extra markers that we want to copy */
	jcopy_markers_setup (&srcinfo, JCOPYOPT_DEFAULT);

	/* Read file header */
	(void) jpeg_read_header (&srcinfo, TRUE);

	/* Any space needed by a transform option must be requested before
	 * jpeg_read_coefficients so that memory allocation will be done right.
	 */
	jtransform_request_workspace (&srcinfo, &transformoption);

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients (&srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters (&srcinfo, &dstinfo);

	/* Adjust destination parameters if required by transform options;
	 * also find out which set of coefficient arrays will hold the output.
	 */
	dst_coef_arrays = jtransform_adjust_parameters (&srcinfo,
							&dstinfo,
							src_coef_arrays,
							&transformoption);

	/* Specify data destination for compression */
	jpeg_stdio_dest (&dstinfo, output_file);

	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients (&dstinfo, dst_coef_arrays);

	/* handle EXIF/IPTC data explicitly */
#if HAVE_EXIF
	/* exif_chunk and exif are mutally exclusive, this is what we assure here */
	g_assert (priv->exif_chunk == NULL);
	if (priv->exif != NULL)
	{
		unsigned char *exif_buf;
		unsigned int   exif_buf_len;

		exif_data_save_data (priv->exif, &exif_buf, &exif_buf_len);
		jpeg_write_marker (&dstinfo, JPEG_APP0+1, exif_buf, exif_buf_len);
		g_free (exif_buf);
	}
#else
	if (priv->exif_chunk != NULL) {
		jpeg_write_marker (&dstinfo, JPEG_APP0+1, priv->exif_chunk, priv->exif_chunk_len);
	}
#endif
	/* FIXME: Consider IPTC data too */

	/* Copy to the output file any extra markers that we want to
	 * preserve */
	jcopy_markers_execute (&srcinfo, &dstinfo, JCOPYOPT_DEFAULT);

	/* Execute image transformation, if any */
	jtransform_execute_transformation (&srcinfo,
					   &dstinfo,
					   src_coef_arrays,
					   &transformoption);

	/* Finish compression and release memory */
	jpeg_finish_compress (&dstinfo);
	jpeg_destroy_compress (&dstinfo);
	(void) jpeg_finish_decompress (&srcinfo);
	jpeg_destroy_decompress (&srcinfo);
	g_free (jsrcerr.filename);

	/* Close files */
	fclose (input_file);
	fclose (output_file);

	return TRUE;
}

static gboolean
_save_any_as_jpeg (XviewerImage *image, const char *file, XviewerImageSaveInfo *source,
		   XviewerImageSaveInfo *target, GError **error)
{
	XviewerImagePrivate *priv;
	GdkPixbuf *pixbuf;
	struct jpeg_compress_struct cinfo;
	guchar *buf = NULL;
	guchar *ptr;
	guchar *pixels = NULL;
	JSAMPROW *jbuf;
	int y = 0;
	volatile int quality = 75; /* default; must be between 0 and 100 */
	int i, j;
	int w, h = 0;
	int rowstride = 0;
	FILE *outfile;
	struct error_handler_data jerr;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (image), FALSE);
	g_return_val_if_fail (XVIEWER_IMAGE (image)->priv->image != NULL, FALSE);

	priv = image->priv;
	pixbuf = priv->image;

	outfile = fopen (file, "wb");
	if (outfile == NULL) {
		g_set_error (error,             /* FIXME: Better error message */
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     _("Couldn't create temporary file for saving: %s"),
			     file);
		return FALSE;
	}

	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	/* no image data? abort */
	pixels = gdk_pixbuf_get_pixels (pixbuf);
	g_return_val_if_fail (pixels != NULL, FALSE);

	/* allocate a small buffer to convert image data */
	buf = g_try_malloc (w * 3 * sizeof (guchar));
	if (!buf) {
		g_set_error (error,
			     GDK_PIXBUF_ERROR,
			     GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
			     _("Couldn't allocate memory for loading JPEG file"));
		fclose (outfile);
		return FALSE;
	}

	/* set up error handling */
	jerr.filename = (char *) file;
	cinfo.err = jpeg_std_error (&(jerr.pub));
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;

	/* setup compress params */
	jpeg_create_compress (&cinfo);
	jpeg_stdio_dest (&cinfo, outfile);
	cinfo.image_width      = w;
	cinfo.image_height     = h;
	cinfo.input_components = 3;
	cinfo.in_color_space   = JCS_RGB;

	/* error exit routine */
	if (sigsetjmp (jerr.setjmp_buffer, 1)) {
		g_free (buf);
		fclose (outfile);
		jpeg_destroy_compress (&cinfo);
		return FALSE;
	}

	/* set desired jpeg quality if available */
	if (target != NULL && target->jpeg_quality >= 0.0) {
		quality = (int) MIN (target->jpeg_quality, 1.0) * 100;
	}

	/* set up jepg compression parameters */
	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, TRUE);
	jpeg_start_compress (&cinfo, TRUE);

	/* write EXIF/IPTC data explicitly */
#if HAVE_EXIF
	/* exif_chunk and exif are mutally exclusvie, this is what we assure here */
	g_assert (priv->exif_chunk == NULL);
	if (priv->exif != NULL)
	{
		unsigned char *exif_buf;
		unsigned int   exif_buf_len;

		exif_data_save_data (priv->exif, &exif_buf, &exif_buf_len);
		jpeg_write_marker (&cinfo, 0xe1, exif_buf, exif_buf_len);
		g_free (exif_buf);
	}
#else
	if (priv->exif_chunk != NULL) {
		jpeg_write_marker (&cinfo, JPEG_APP0+1, priv->exif_chunk, priv->exif_chunk_len);
	}
#endif
	/* FIXME: Consider IPTC data too */

	/* get the start pointer */
	ptr = pixels;
	/* go one scanline at a time... and save */
	i = 0;
	while (cinfo.next_scanline < cinfo.image_height) {
		/* convert scanline from ARGB to RGB packed */
		for (j = 0; j < w; j++)
			memcpy (&(buf[j*3]), &(ptr[i*rowstride + j*(rowstride/w)]), 3);

		/* write scanline */
		jbuf = (JSAMPROW *)(&buf);
		jpeg_write_scanlines (&cinfo, jbuf, 1);
		i++;
		y++;

	}

	/* finish off */
	jpeg_finish_compress (&cinfo);
	jpeg_destroy_compress(&cinfo);
	g_free (buf);

	fclose (outfile);

	return TRUE;
}

gboolean
xviewer_image_jpeg_save_file (XviewerImage *image, const char *file,
			  XviewerImageSaveInfo *source, XviewerImageSaveInfo *target,
			  GError **error)
{
	XviewerJpegSaveMethod method = XVIEWER_SAVE_NONE;
	gboolean source_is_jpeg = FALSE;
	gboolean target_is_jpeg = FALSE;
    gboolean result;

	g_return_val_if_fail (source != NULL, FALSE);

	source_is_jpeg = !g_ascii_strcasecmp (source->format, XVIEWER_FILE_FORMAT_JPEG);

    if (image->priv->modified)
        xviewer_image_jpeg_update_exif_data (image);       /* delay updating the EXIF data to here otherwise any changes
                                                              are lost if the user moves to anther image */

	/* determine which method should be used for saving */
	if (target == NULL) {
		if (source_is_jpeg) {
			method = XVIEWER_SAVE_JPEG_AS_JPEG;
		}
	}
	else {
		target_is_jpeg = !g_ascii_strcasecmp (target->format, XVIEWER_FILE_FORMAT_JPEG);

		if (source_is_jpeg && target_is_jpeg) {
			if (target->jpeg_quality < 0.0) {
				method = XVIEWER_SAVE_JPEG_AS_JPEG;
			}
			else {
				/* reencoding is required, cause quality is set */
				method = XVIEWER_SAVE_ANY_AS_JPEG;
			}
		}
		else if (!source_is_jpeg && target_is_jpeg) {
			method = XVIEWER_SAVE_ANY_AS_JPEG;
		}
	}

	switch (method) {
	case XVIEWER_SAVE_JPEG_AS_JPEG:
		result = _save_jpeg_as_jpeg (image, file, source, target, error);
		break;
	case XVIEWER_SAVE_ANY_AS_JPEG:
		result = _save_any_as_jpeg (image, file, source, target, error);
		break;
	default:
		result = FALSE;
	}

	return result;
}
#endif

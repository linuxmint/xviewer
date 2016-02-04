/* Xviewer - Image
 *
 * Copyright (C) 2007 The Free Software Foundation
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

#ifndef __XVIEWER_IMAGE_H__
#define __XVIEWER_IMAGE_H__

#include "xviewer-jobs.h"
#include "xviewer-window.h"
#include "xviewer-transform.h"
#include "xviewer-image-save-info.h"
#include "xviewer-enums.h"

#include <glib.h>
#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef HAVE_EXIF
#include <libexif/exif-data.h>
#include "xviewer-exif-util.h"
#endif

#ifdef HAVE_LCMS
#include <lcms2.h>
#endif

#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

G_BEGIN_DECLS

#ifndef __XVIEWER_IMAGE_DECLR__
#define __XVIEWER_IMAGE_DECLR__
typedef struct _XviewerImage XviewerImage;
#endif
typedef struct _XviewerImageClass XviewerImageClass;
typedef struct _XviewerImagePrivate XviewerImagePrivate;

#define XVIEWER_TYPE_IMAGE            (xviewer_image_get_type ())
#define XVIEWER_IMAGE(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), XVIEWER_TYPE_IMAGE, XviewerImage))
#define XVIEWER_IMAGE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),  XVIEWER_TYPE_IMAGE, XviewerImageClass))
#define XVIEWER_IS_IMAGE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), XVIEWER_TYPE_IMAGE))
#define XVIEWER_IS_IMAGE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  XVIEWER_TYPE_IMAGE))
#define XVIEWER_IMAGE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  XVIEWER_TYPE_IMAGE, XviewerImageClass))

typedef enum {
	XVIEWER_IMAGE_ERROR_SAVE_NOT_LOCAL,
	XVIEWER_IMAGE_ERROR_NOT_LOADED,
	XVIEWER_IMAGE_ERROR_NOT_SAVED,
	XVIEWER_IMAGE_ERROR_VFS,
	XVIEWER_IMAGE_ERROR_FILE_EXISTS,
	XVIEWER_IMAGE_ERROR_TMP_FILE_FAILED,
	XVIEWER_IMAGE_ERROR_GENERIC,
	XVIEWER_IMAGE_ERROR_UNKNOWN
} XviewerImageError;

#define XVIEWER_IMAGE_ERROR xviewer_image_error_quark ()

typedef enum {
	XVIEWER_IMAGE_STATUS_UNKNOWN,
	XVIEWER_IMAGE_STATUS_LOADING,
	XVIEWER_IMAGE_STATUS_LOADED,
	XVIEWER_IMAGE_STATUS_SAVING,
	XVIEWER_IMAGE_STATUS_FAILED
} XviewerImageStatus;

typedef enum {
  XVIEWER_IMAGE_METADATA_NOT_READ,
  XVIEWER_IMAGE_METADATA_NOT_AVAILABLE,
  XVIEWER_IMAGE_METADATA_READY
} XviewerImageMetadataStatus;

struct _XviewerImage {
	GObject parent;

	XviewerImagePrivate *priv;
};

struct _XviewerImageClass {
	GObjectClass parent_class;

	void (* changed) 	   (XviewerImage *img);

	void (* size_prepared)     (XviewerImage *img,
				    int       width,
				    int       height);

	void (* thumbnail_changed) (XviewerImage *img);

	void (* save_progress)     (XviewerImage *img,
				    gfloat    progress);

	void (* next_frame)        (XviewerImage *img,
				    gint delay);

	void (* file_changed)      (XviewerImage *img);
};

GType	          xviewer_image_get_type	             (void) G_GNUC_CONST;

GQuark            xviewer_image_error_quark              (void);

XviewerImage         *xviewer_image_new                      (const char *txt_uri);

XviewerImage         *xviewer_image_new_file                 (GFile *file);

gboolean          xviewer_image_load                     (XviewerImage   *img,
					              XviewerImageData data2read,
					              XviewerJob     *job,
					              GError    **error);

void              xviewer_image_cancel_load              (XviewerImage   *img);

gboolean          xviewer_image_has_data                 (XviewerImage   *img,
					              XviewerImageData data);

void              xviewer_image_data_ref                 (XviewerImage   *img);

void              xviewer_image_data_unref               (XviewerImage   *img);

void              xviewer_image_set_thumbnail            (XviewerImage   *img,
					              GdkPixbuf  *pixbuf);

gboolean          xviewer_image_save_as_by_info          (XviewerImage   *img,
		      			              XviewerImageSaveInfo *source,
		      			              XviewerImageSaveInfo *target,
		      			              GError    **error);

gboolean          xviewer_image_save_by_info             (XviewerImage   *img,
					              XviewerImageSaveInfo *source,
					              GError    **error);

GdkPixbuf*        xviewer_image_get_pixbuf               (XviewerImage   *img);

GdkPixbuf*        xviewer_image_get_thumbnail            (XviewerImage   *img);

void              xviewer_image_get_size                 (XviewerImage   *img,
					              gint       *width,
					              gint       *height);

goffset           xviewer_image_get_bytes                (XviewerImage   *img);

gboolean          xviewer_image_is_modified              (XviewerImage   *img);

void              xviewer_image_modified                 (XviewerImage   *img);

const gchar*      xviewer_image_get_caption              (XviewerImage   *img);

const gchar      *xviewer_image_get_collate_key          (XviewerImage   *img);

#if HAVE_EXIF
ExifData*      xviewer_image_get_exif_info            (XviewerImage   *img);
#endif

gpointer          xviewer_image_get_xmp_info             (XviewerImage   *img);

GFile*            xviewer_image_get_file                 (XviewerImage   *img);

gchar*            xviewer_image_get_uri_for_display      (XviewerImage   *img);

XviewerImageStatus    xviewer_image_get_status               (XviewerImage   *img);

XviewerImageMetadataStatus xviewer_image_get_metadata_status (XviewerImage   *img);

void              xviewer_image_transform                (XviewerImage   *img,
						      XviewerTransform *trans,
						      XviewerJob     *job);

void              xviewer_image_autorotate               (XviewerImage   *img);

#ifdef HAVE_LCMS
cmsHPROFILE       xviewer_image_get_profile              (XviewerImage    *img);

void              xviewer_image_apply_display_profile    (XviewerImage    *img,
						      cmsHPROFILE  display_profile);
#endif

void              xviewer_image_undo                     (XviewerImage   *img);

GList		 *xviewer_image_get_supported_mime_types (void);

gboolean          xviewer_image_is_supported_mime_type   (const char *mime_type);

gboolean          xviewer_image_is_animation             (XviewerImage *img);

gboolean          xviewer_image_start_animation          (XviewerImage *img);

#ifdef HAVE_RSVG
gboolean          xviewer_image_is_svg                   (XviewerImage *img);
RsvgHandle       *xviewer_image_get_svg                  (XviewerImage *img);
#endif

XviewerTransform     *xviewer_image_get_transform            (XviewerImage *img);
XviewerTransform     *xviewer_image_get_autorotate_transform (XviewerImage *img);

gboolean          xviewer_image_is_jpeg                  (XviewerImage *img);

void              xviewer_image_file_changed             (XviewerImage *img);

gboolean          xviewer_image_is_file_changed          (XviewerImage *img);

gboolean          xviewer_image_is_file_writable         (XviewerImage *img);

G_END_DECLS

#endif /* __XVIEWER_IMAGE_H__ */

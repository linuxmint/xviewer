/* Xviewer - Jobs
 *
 * Copyright (C) 2013 The Free Software Foundation
 *
 * Author: Javier Sánchez <jsanchez@deskblue.com>
 *
 * Based on code (libview/ev-jobs.h) by:
 *      - Carlos Garcia Campos <carlosgc@gnome.org>
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

#ifndef __XVIEWER_JOBS_H__
#define __XVIEWER_JOBS_H__

#include "xviewer-enums.h"
#include "xviewer-image.h"
#include "xviewer-list-store.h"
#include "xviewer-transform.h"
#include "xviewer-uri-converter.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define XVIEWER_TYPE_JOB                      (xviewer_job_get_type ())
#define XVIEWER_JOB(obj)                      (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB, XviewerJob))
#define XVIEWER_JOB_CLASS(klass)              (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB, XviewerJobClass))
#define XVIEWER_IS_JOB(obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB))
#define XVIEWER_IS_JOB_CLASS(klass)           (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB))
#define XVIEWER_JOB_GET_CLASS(obj)            (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB, XviewerJobClass))

#define XVIEWER_TYPE_JOB_COPY                 (xviewer_job_copy_get_type ())
#define XVIEWER_JOB_COPY(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_COPY, XviewerJobCopy))
#define XVIEWER_JOB_COPY_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_COPY, XviewerJobCopyClass))
#define XVIEWER_IS_JOB_COPY(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_COPY))
#define XVIEWER_IS_JOB_COPY_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_COPY))
#define XVIEWER_JOB_COPY_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_COPY, XviewerJobCopyClass))

#define XVIEWER_TYPE_JOB_LOAD                 (xviewer_job_load_get_type ())
#define XVIEWER_JOB_LOAD(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_LOAD, XviewerJobLoad))
#define XVIEWER_JOB_LOAD_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_LOAD, XviewerJobLoadClass))
#define XVIEWER_IS_JOB_LOAD(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_LOAD))
#define XVIEWER_IS_JOB_LOAD_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_LOAD))
#define XVIEWER_JOB_LOAD_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_LOAD, XviewerJobLoadClass))

#define XVIEWER_TYPE_JOB_MODEL                (xviewer_job_model_get_type ())
#define XVIEWER_JOB_MODEL(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_MODEL, XviewerJobModel))
#define XVIEWER_JOB_MODEL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_MODEL, XviewerJobModelClass))
#define XVIEWER_IS_JOB_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_MODEL))
#define XVIEWER_IS_JOB_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_MODEL))
#define XVIEWER_JOB_MODEL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_MODEL, XviewerJobModelClass))

#define XVIEWER_TYPE_JOB_SAVE                 (xviewer_job_save_get_type ())
#define XVIEWER_JOB_SAVE(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_SAVE, XviewerJobSave))
#define XVIEWER_JOB_SAVE_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_SAVE, XviewerJobSaveClass))
#define XVIEWER_IS_JOB_SAVE(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_SAVE))
#define XVIEWER_IS_JOB_SAVE_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_SAVE))
#define XVIEWER_JOB_SAVE_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_SAVE, XviewerJobSaveClass))

#define XVIEWER_TYPE_JOB_SAVE_AS              (xviewer_job_save_as_get_type ())
#define XVIEWER_JOB_SAVE_AS(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_SAVE_AS, XviewerJobSaveAs))
#define XVIEWER_JOB_SAVE_AS_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_SAVE_AS, XviewerJobSaveAsClass))
#define XVIEWER_IS_JOB_SAVE_AS(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_SAVE_AS))
#define XVIEWER_IS_JOB_SAVE_AS_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_SAVE_AS))
#define XVIEWER_JOB_SAVE_AS_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_SAVE_AS, XviewerJobSaveAsClass))

#define XVIEWER_TYPE_JOB_THUMBNAIL            (xviewer_job_thumbnail_get_type ())
#define XVIEWER_JOB_THUMBNAIL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_THUMBNAIL, XviewerJobThumbnail))
#define XVIEWER_JOB_THUMBNAIL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_THUMBNAIL, XviewerJobThumbnailClass))
#define XVIEWER_IS_JOB_THUMBNAIL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_THUMBNAIL))
#define XVIEWER_IS_JOB_THUMBNAIL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_THUMBNAIL))
#define XVIEWER_JOB_THUMBNAIL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_THUMBNAIL, XviewerJobThumbnailClass))

#define XVIEWER_TYPE_JOB_TRANSFORM            (xviewer_job_transform_get_type ())
#define XVIEWER_JOB_TRANSFORM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_JOB_TRANSFORM, XviewerJobTransform))
#define XVIEWER_JOB_TRANSFORM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  XVIEWER_TYPE_JOB_TRANSFORM, XviewerJobTransformClass))
#define XVIEWER_IS_JOB_TRANSFORM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_JOB_TRANSFORM))
#define XVIEWER_IS_JOB_TRANSFORM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  XVIEWER_TYPE_JOB_TRANSFORM))
#define XVIEWER_JOB_TRANSFORM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  XVIEWER_TYPE_JOB_TRANSFORM, XviewerJobTransformClass))


#ifndef __XVIEWER_URI_CONVERTER_DECLR__
#define __XVIEWER_URI_CONVERTER_DECLR__
typedef struct _XviewerURIConverter XviewerURIConverter;
#endif

#ifndef __XVIEWER_JOB_DECLR__
#define __XVIEWER_JOB_DECLR__
typedef struct _XviewerJob               XviewerJob;
#endif

typedef struct _XviewerJobClass          XviewerJobClass;

typedef struct _XviewerJobCopy           XviewerJobCopy;
typedef struct _XviewerJobCopyClass      XviewerJobCopyClass;

typedef struct _XviewerJobLoad           XviewerJobLoad;
typedef struct _XviewerJobLoadClass      XviewerJobLoadClass;

typedef struct _XviewerJobModel          XviewerJobModel;
typedef struct _XviewerJobModelClass     XviewerJobModelClass;

typedef struct _XviewerJobSave           XviewerJobSave;
typedef struct _XviewerJobSaveClass      XviewerJobSaveClass;

typedef struct _XviewerJobSaveAs         XviewerJobSaveAs;
typedef struct _XviewerJobSaveAsClass    XviewerJobSaveAsClass;

typedef struct _XviewerJobThumbnail      XviewerJobThumbnail;
typedef struct _XviewerJobThumbnailClass XviewerJobThumbnailClass;

typedef struct _XviewerJobTransform      XviewerJobTransform;
typedef struct _XviewerJobTransformClass XviewerJobTransformClass;

struct _XviewerJob
{
	GObject       parent;

	GCancellable *cancellable;
	GError       *error;
	GMutex       *mutex;

	gfloat        progress;
	gboolean      cancelled;
	gboolean      finished;
};

struct _XviewerJobClass
{
	GObjectClass parent_class;

	/* vfuncs */
	void    (* run)       (XviewerJob *job);

	/* signals */
	void    (* progress)  (XviewerJob *job,
			       gfloat  progress);
	void    (* cancelled) (XviewerJob *job);
	void    (* finished)  (XviewerJob *job);
};

struct _XviewerJobCopy
{
	XviewerJob           parent;

	GList           *images;
	gchar           *destination;
	guint            current_position;
};

struct _XviewerJobCopyClass
{
	XviewerJobClass      parent_class;
};

struct _XviewerJobLoad
{
	XviewerJob           parent;

	XviewerImage        *image;
	XviewerImageData     data;
};

struct _XviewerJobLoadClass
{
	XviewerJobClass      parent_class;
};

struct _XviewerJobModel
{
	XviewerJob           parent;

	XviewerListStore    *store;
	GSList          *file_list;
};

struct _XviewerJobModelClass
{
        XviewerJobClass      parent_class;
};

struct _XviewerJobSave
{
	XviewerJob           parent;

	GList	        *images;
	XviewerImage        *current_image;
	guint            current_position;
};

struct _XviewerJobSaveClass
{
	XviewerJobClass      parent_class;
};

struct _XviewerJobSaveAs
{
	XviewerJobSave       parent;

	XviewerURIConverter *converter;
	GFile           *file;
};

struct _XviewerJobSaveAsClass
{
	XviewerJobSaveClass  parent;
};

struct _XviewerJobThumbnail
{
	XviewerJob           parent;

	XviewerImage        *image;
	GdkPixbuf       *thumbnail;
};

struct _XviewerJobThumbnailClass
{
	XviewerJobClass      parent_class;
};

struct _XviewerJobTransform
{
	XviewerJob           parent;

	GList           *images;
	XviewerTransform    *transform;
};

struct _XviewerJobTransformClass
{
        XviewerJobClass      parent_class;
};


/* XviewerJob */
GType    xviewer_job_get_type           (void) G_GNUC_CONST;

void     xviewer_job_run                (XviewerJob          *job);
void     xviewer_job_cancel             (XviewerJob          *job);

gfloat   xviewer_job_get_progress       (XviewerJob          *job);
void     xviewer_job_set_progress       (XviewerJob          *job,
				     gfloat           progress);
gboolean xviewer_job_is_cancelled       (XviewerJob          *job);
gboolean xviewer_job_is_finished        (XviewerJob          *job);

/* XviewerJobCopy */
GType    xviewer_job_copy_get_type      (void) G_GNUC_CONST;
XviewerJob  *xviewer_job_copy_new           (GList           *images,
				     const gchar     *destination);

/* XviewerJobLoad */
GType    xviewer_job_load_get_type      (void) G_GNUC_CONST;

XviewerJob  *xviewer_job_load_new           (XviewerImage        *image,
				     XviewerImageData     data);

/* XviewerJobModel */
GType 	 xviewer_job_model_get_type     (void) G_GNUC_CONST;
XviewerJob 	*xviewer_job_model_new          (GSList          *file_list);

/* XviewerJobSave */
GType    xviewer_job_save_get_type      (void) G_GNUC_CONST;
XviewerJob  *xviewer_job_save_new           (GList           *images);

/* XviewerJobSaveAs */
GType    xviewer_job_save_as_get_type   (void) G_GNUC_CONST;
XviewerJob  *xviewer_job_save_as_new        (GList           *images,
				     XviewerURIConverter *converter,
				     GFile           *file);

/* XviewerJobThumbnail */
GType    xviewer_job_thumbnail_get_type (void) G_GNUC_CONST;
XviewerJob  *xviewer_job_thumbnail_new      (XviewerImage        *image);

/* XviewerJobTransform */
GType 	 xviewer_job_transform_get_type (void) G_GNUC_CONST;
XviewerJob  *xviewer_job_transform_new      (GList           *images,
				     XviewerTransform    *transform);

G_END_DECLS

#endif /* __XVIEWER_JOBS_H__ */

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

#include "xviewer-debug.h"
#include "xviewer-jobs.h"
#include "xviewer-thumbnail.h"
#include "xviewer-pixbuf-util.h"

#include <gio/gio.h>

G_DEFINE_ABSTRACT_TYPE (XviewerJob, xviewer_job, G_TYPE_OBJECT);
G_DEFINE_TYPE (XviewerJobCopy,      xviewer_job_copy,      XVIEWER_TYPE_JOB);
G_DEFINE_TYPE (XviewerJobLoad,      xviewer_job_load,      XVIEWER_TYPE_JOB);
G_DEFINE_TYPE (XviewerJobModel,     xviewer_job_model,     XVIEWER_TYPE_JOB);
G_DEFINE_TYPE (XviewerJobSave,      xviewer_job_save,      XVIEWER_TYPE_JOB);
G_DEFINE_TYPE (XviewerJobSaveAs,    xviewer_job_save_as,   XVIEWER_TYPE_JOB_SAVE);
G_DEFINE_TYPE (XviewerJobThumbnail, xviewer_job_thumbnail, XVIEWER_TYPE_JOB);
G_DEFINE_TYPE (XviewerJobTransform, xviewer_job_transform, XVIEWER_TYPE_JOB);

/* signals */
enum {
	PROGRESS,
	CANCELLED,
	FINISHED,
	LAST_SIGNAL
};

static guint job_signals[LAST_SIGNAL];

/* notify signal funcs */
static gboolean notify_progress              (XviewerJob               *job);
static gboolean notify_cancelled             (XviewerJob               *job);
static gboolean notify_finished              (XviewerJob               *job);

/* gobject vfuncs */
static void     xviewer_job_class_init           (XviewerJobClass          *class);
static void     xviewer_job_init                 (XviewerJob               *job);
static void     xviewer_job_dispose              (GObject              *object);

static void     xviewer_job_copy_class_init      (XviewerJobCopyClass      *class);
static void     xviewer_job_copy_init            (XviewerJobCopy           *job);
static void     xviewer_job_copy_dispose         (GObject              *object);

static void     xviewer_job_load_class_init      (XviewerJobLoadClass      *class);
static void     xviewer_job_load_init            (XviewerJobLoad           *job);
static void     xviewer_job_load_dispose         (GObject              *object);

static void     xviewer_job_model_class_init     (XviewerJobModelClass     *class);
static void     xviewer_job_model_init           (XviewerJobModel          *job);
static void     xviewer_job_model_dispose        (GObject              *object);

static void     xviewer_job_save_class_init      (XviewerJobSaveClass      *class);
static void     xviewer_job_save_init            (XviewerJobSave           *job);
static void     xviewer_job_save_dispose         (GObject              *object);

static void     xviewer_job_save_as_class_init   (XviewerJobSaveAsClass    *class);
static void     xviewer_job_save_as_init         (XviewerJobSaveAs         *job);
static void     xviewer_job_save_as_dispose      (GObject              *object);

static void     xviewer_job_thumbnail_class_init (XviewerJobThumbnailClass *class);
static void     xviewer_job_thumbnail_init       (XviewerJobThumbnail      *job);
static void     xviewer_job_thumbnail_dispose    (GObject              *object);

static void     xviewer_job_transform_class_init (XviewerJobTransformClass *class);
static void     xviewer_job_transform_init       (XviewerJobTransform      *job);
static void     xviewer_job_transform_dispose    (GObject              *object);

/* vfuncs */
static void     xviewer_job_run_unimplemented    (XviewerJob               *job);
static void     xviewer_job_copy_run             (XviewerJob               *job);
static void     xviewer_job_load_run             (XviewerJob               *job);
static void     xviewer_job_model_run            (XviewerJob               *job);
static void     xviewer_job_save_run             (XviewerJob               *job);
static void     xviewer_job_save_as_run          (XviewerJob               *job);
static void     xviewer_job_thumbnail_run        (XviewerJob               *job);
static void     xviewer_job_transform_run        (XviewerJob               *job);

/* callbacks */
static void xviewer_job_copy_progress_callback (goffset  current_num_bytes,
					    goffset  total_num_bytes,
					    gpointer user_data);

static void xviewer_job_save_progress_callback (XviewerImage *image,
					    gfloat    progress,
					    gpointer  data);

/* --------------------------- notify signal funcs --------------------------- */
static gboolean
notify_progress (XviewerJob *job)
{
	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
		return FALSE;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job update its progress to -> %1.2f",
			   XVIEWER_GET_TYPE_NAME (job),
			   job,
			   job->progress);

	/* notify progress */
	g_signal_emit (job,
		       job_signals[PROGRESS],
		       0,
		       job->progress);
	return FALSE;
}

static gboolean
notify_cancelled (XviewerJob *job)
{
	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CANCELLED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	/* notify cancelation */
	g_signal_emit (job,
		       job_signals[CANCELLED],
		       0);

	return FALSE;
}

static gboolean
notify_finished (XviewerJob *job)
{
	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was FINISHED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	/* notify job finalization */
	g_signal_emit (job,
		       job_signals[FINISHED],
		       0);
	return FALSE;
}

/* --------------------------------- XviewerJob ---------------------------------- */
static void
xviewer_job_class_init (XviewerJobClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->dispose = xviewer_job_dispose;
	class->run              = xviewer_job_run_unimplemented;

	/* signals */
	job_signals [PROGRESS] =
		g_signal_new ("progress",
			      XVIEWER_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerJobClass, progress),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_FLOAT);

	job_signals [CANCELLED] =
		g_signal_new ("cancelled",
			      XVIEWER_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerJobClass, cancelled),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	job_signals [FINISHED] =
		g_signal_new ("finished",
			      XVIEWER_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerJobClass, finished),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static
void xviewer_job_init (XviewerJob *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->cancellable = g_cancellable_new ();
	job->error       = NULL;

	job->progress    = 0.0;
	job->cancelled   = FALSE;
	job->finished    = FALSE;

	/* NOTE: we need to allocate the mutex here so the ABI stays
	   the same when it used to use g_mutex_new */
	job->mutex = g_malloc (sizeof (GMutex));
	g_mutex_init (job->mutex);
}

static
void xviewer_job_dispose (GObject *object)
{
	XviewerJob *job;

	g_return_if_fail (XVIEWER_IS_JOB (object));

	job = XVIEWER_JOB (object);

	/* free all public and private members */
	if (job->cancellable) {
		g_object_unref (job->cancellable);
		job->cancellable = NULL;
	}

	if (job->error) {
		g_error_free (job->error);
		job->error = NULL;
	}

	if (job->mutex) {
		g_mutex_clear (job->mutex);
		g_free        (job->mutex);
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_parent_class)->dispose (object);
}

static void
xviewer_job_run_unimplemented (XviewerJob *job)
{
	g_critical ("Class \"%s\" does not implement the required run action",
		    G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (job)));
}

void
xviewer_job_run (XviewerJob *job)
{
	XviewerJobClass *class;

	g_return_if_fail (XVIEWER_IS_JOB (job));

	class = XVIEWER_JOB_GET_CLASS (job);
	class->run (job);
}

void
xviewer_job_cancel (XviewerJob *job)
{
	g_return_if_fail (XVIEWER_IS_JOB (job));

	g_object_ref (job);

	/* check if job was cancelled previously */
	if (job->cancelled)
		return;

	/* check if job finished previously */
        if (job->finished)
		return;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "CANCELLING a %s (%p)",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* cancel job */
	job->cancelled = TRUE;
	g_cancellable_cancel (job->cancellable);

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job cancellation */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_cancelled,
			 job,
			 g_object_unref);
}

gfloat
xviewer_job_get_progress (XviewerJob *job)
{
	g_return_val_if_fail (XVIEWER_IS_JOB (job), 0.0);

	return job->progress;
}

void
xviewer_job_set_progress (XviewerJob *job,
		      gfloat  progress)
{
	g_return_if_fail (XVIEWER_IS_JOB (job));
	g_return_if_fail (progress >= 0.0 && progress <= 1.0);

	g_object_ref (job);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	job->progress = progress;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify progress */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_progress,
			 job,
			 g_object_unref);
}

gboolean
xviewer_job_is_cancelled (XviewerJob *job)
{
	g_return_val_if_fail (XVIEWER_IS_JOB (job), TRUE);

	return job->cancelled;
}

gboolean
xviewer_job_is_finished (XviewerJob *job)
{
	g_return_val_if_fail (XVIEWER_IS_JOB (job), TRUE);

	return job->finished;
}

/* ------------------------------- XviewerJobCopy -------------------------------- */
static void
xviewer_job_copy_class_init (XviewerJobCopyClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_copy_dispose;
	xviewer_job_class->run      = xviewer_job_copy_run;
}

static
void xviewer_job_copy_init (XviewerJobCopy *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images           = NULL;
	job->destination      = NULL;
	job->current_position = 0;
}

static
void xviewer_job_copy_dispose (GObject *object)
{
	XviewerJobCopy *job;

	g_return_if_fail (XVIEWER_IS_JOB_COPY (object));

	job = XVIEWER_JOB_COPY (object);

	/* free all public and private members */
	if (job->images) {
		g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
		g_list_free    (job->images);
		job->images = NULL;
	}

	if (job->destination) {
		g_free (job->destination);
		job->destination = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_copy_parent_class)->dispose (object);
}

static void
xviewer_job_copy_progress_callback (goffset  current_num_bytes,
				goffset  total_num_bytes,
				gpointer user_data)
{
	gfloat      progress;
	guint       n_images;
	XviewerJobCopy *job;

	job = XVIEWER_JOB_COPY (user_data);

	n_images = g_list_length (job->images);

	progress = ((current_num_bytes / (gfloat) total_num_bytes) + job->current_position) / n_images;

	xviewer_job_set_progress (XVIEWER_JOB (job), progress);
}

static void
xviewer_job_copy_run (XviewerJob *job)
{
	XviewerJobCopy *copyjob;
	GList *it;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_COPY (job));

	copyjob = XVIEWER_JOB_COPY (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
	{
		g_object_unref (job);
		return;
	}

	copyjob->current_position = 0;

	for (it = copyjob->images; it != NULL; it = g_list_next (it), copyjob->current_position++) {
		GFile *src, *dest;
		gchar *filename, *dest_filename;

		src = (GFile *) it->data;
		filename = g_file_get_basename (src);
		dest_filename = g_build_filename (copyjob->destination, filename, NULL);
		dest = g_file_new_for_path (dest_filename);

		g_file_copy (src, dest,
					 G_FILE_COPY_OVERWRITE, NULL,
					 xviewer_job_copy_progress_callback, job,
					 &job->error);
		g_object_unref (dest);
		g_free (filename);
		g_free (dest_filename);
	}

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_copy_new (GList       *images,
		  const gchar *destination)
{
	XviewerJobCopy *job;

	job = g_object_new (XVIEWER_TYPE_JOB_COPY, NULL);

	if (images)
		job->images = images;

	if (destination)
		job->destination = g_strdup (destination);

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobLoad -------------------------------- */
static void
xviewer_job_load_class_init (XviewerJobLoadClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_load_dispose;
	xviewer_job_class->run      = xviewer_job_load_run;
}

static
void xviewer_job_load_init (XviewerJobLoad *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->image = NULL;
	job->data  = XVIEWER_IMAGE_DATA_ALL;
}

static
void xviewer_job_load_dispose (GObject *object)
{
	XviewerJobLoad *job;

	g_return_if_fail (XVIEWER_IS_JOB_LOAD (object));

	job = XVIEWER_JOB_LOAD (object);

	/* free all public and private members */
	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_load_parent_class)->dispose (object);
}

static void
xviewer_job_load_run (XviewerJob *job)
{
	XviewerJobLoad *job_load;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_LOAD (job));

	job_load = XVIEWER_JOB_LOAD (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* load image from file */
	xviewer_image_load (job_load->image,
			job_load->data,
			job,
			&job->error);

	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
		return;

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_load_new (XviewerImage     *image,
		  XviewerImageData  data)
{
	XviewerJobLoad *job;

	job = g_object_new (XVIEWER_TYPE_JOB_LOAD, NULL);

	if (image)
		job->image = g_object_ref (image);

	job->data = data;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobModel -------------------------------- */
static void
xviewer_job_model_class_init (XviewerJobModelClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_model_dispose;
	xviewer_job_class->run      = xviewer_job_model_run;
}

static
void xviewer_job_model_init (XviewerJobModel *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->store     = NULL;
	job->file_list = NULL;
}

static
void xviewer_job_model_dispose (GObject *object)
{
	XviewerJobModel *job;

	g_return_if_fail (XVIEWER_IS_JOB_MODEL (object));

	job = XVIEWER_JOB_MODEL (object);

	/* free all public and private members */
	if (job->store) {
		g_object_unref (job->store);
		job->store = NULL;
	}

	if (job->file_list) {
		// g_slist_foreach (job->file_list, (GFunc) g_object_unref, NULL);
		// g_slist_free (job->file_list);
		job->file_list = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_model_parent_class)->dispose (object);
}

static void
filter_files (GSList *files, GList **file_list, GList **error_list)
{
	GSList *it;
	GFileInfo *file_info;

	for (it = files; it != NULL; it = it->next) {
		GFile *file;
		GFileType type = G_FILE_TYPE_UNKNOWN;

		file = (GFile *) it->data;

		if (file != NULL) {
			file_info = g_file_query_info (file,
						       G_FILE_ATTRIBUTE_STANDARD_TYPE","G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
						       0, NULL, NULL);
			if (file_info == NULL) {
				type = G_FILE_TYPE_UNKNOWN;
			} else {
				type = g_file_info_get_file_type (file_info);

				/* Workaround for gvfs backends that
				   don't set the GFileType. */
				if (G_UNLIKELY (type == G_FILE_TYPE_UNKNOWN)) {
					const gchar *ctype;

					ctype = g_file_info_get_content_type (file_info);

					/* If the content type is supported
					   adjust the file_type */
					if (xviewer_image_is_supported_mime_type (ctype))
						type = G_FILE_TYPE_REGULAR;
				}

				g_object_unref (file_info);
			}
		}

		switch (type) {
		case G_FILE_TYPE_REGULAR:
		case G_FILE_TYPE_DIRECTORY:
			*file_list = g_list_prepend (*file_list, g_object_ref (file));
			break;
		default:
			*error_list = g_list_prepend (*error_list,
						      g_file_get_uri (file));
			break;
		}
	}

	*file_list  = g_list_reverse (*file_list);
	*error_list = g_list_reverse (*error_list);
}

static void
xviewer_job_model_run (XviewerJob *job)
{
	XviewerJobModel *job_model;
	GList       *filtered_list;
	GList       *error_list;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_MODEL (job));

	job_model     = XVIEWER_JOB_MODEL (g_object_ref (job));
	filtered_list = NULL;
	error_list    = NULL;

	filter_files (job_model->file_list,
		      &filtered_list,
		      &error_list);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* create a list store */
	job_model->store = XVIEWER_LIST_STORE (xviewer_list_store_new ());
	xviewer_list_store_add_files (job_model->store, filtered_list);

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* free lists*/
	g_list_foreach (filtered_list, (GFunc) g_object_unref, NULL);
	g_list_free (filtered_list);

	g_list_foreach (error_list, (GFunc) g_free, NULL);
	g_list_free (error_list);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_model_new (GSList *file_list)
{
	XviewerJobModel *job;

	job = g_object_new (XVIEWER_TYPE_JOB_MODEL, NULL);

	if (file_list != NULL)
		job->file_list = file_list;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobSave -------------------------------- */
static void
xviewer_job_save_class_init (XviewerJobSaveClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_save_dispose;
	xviewer_job_class->run      = xviewer_job_save_run;
}

static
void xviewer_job_save_init (XviewerJobSave *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images           = NULL;
	job->current_position = 0;
	job->current_image    = NULL;
}

static
void xviewer_job_save_dispose (GObject *object)
{
	XviewerJobSave *job;

	g_return_if_fail (XVIEWER_IS_JOB_SAVE (object));

	job = XVIEWER_JOB_SAVE (object);

	/* free all public and private members */
	if (job->images) {
		g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
		g_list_free (job->images);
		job->images = NULL;
	}

	if (job->current_image) {
		g_object_unref (job->current_image);
		job->current_image = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_save_parent_class)->dispose (object);
}

static void
xviewer_job_save_progress_callback (XviewerImage *image,
				gfloat    progress,
				gpointer  data)
{
	XviewerJobSave *job;
	guint       n_images;
	gfloat      job_progress;

	job = XVIEWER_JOB_SAVE (data);

	n_images     = g_list_length (job->images);
	job_progress = (job->current_position / (gfloat) n_images) + (progress / n_images);

	xviewer_job_set_progress (XVIEWER_JOB (job), job_progress);
}

static void
xviewer_job_save_run (XviewerJob *job)
{
	XviewerJobSave *save_job;
	GList *it;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_SAVE (job));

	g_object_ref (job);

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
		return;

	save_job = XVIEWER_JOB_SAVE (job);

	save_job->current_position = 0;

	for (it = save_job->images; it != NULL; it = it->next, save_job->current_position++) {
		XviewerImage *image = XVIEWER_IMAGE (it->data);
		XviewerImageSaveInfo *save_info = NULL;
		gulong handler_id = 0;
		gboolean success = FALSE;

		save_job->current_image = image;

		/* Make sure the image doesn't go away while saving */
		xviewer_image_data_ref (image);

		if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_ALL)) {
			XviewerImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = xviewer_image_get_metadata_status (image);
			if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = XVIEWER_IMAGE_DATA_ALL;
			} else if (m_status == XVIEWER_IMAGE_METADATA_NOT_READ)
			{
				// Load only if we haven't read it yet
				data2load = XVIEWER_IMAGE_DATA_EXIF
						| XVIEWER_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				xviewer_image_load (image,
						data2load,
						NULL,
						&job->error);
			}
		}

		handler_id = g_signal_connect (G_OBJECT (image),
						   "save-progress",
							   G_CALLBACK (xviewer_job_save_progress_callback),
						   job);

		save_info = xviewer_image_save_info_new_from_image (image);

		success = xviewer_image_save_by_info (image,
						  save_info,
						  &job->error);

		if (save_info)
			g_object_unref (save_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		xviewer_image_data_unref (image);

		if (!success)
			break;
	}

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_save_new (GList *images)
{
	XviewerJobSave *job;

	job = g_object_new (XVIEWER_TYPE_JOB_SAVE, NULL);

	if (images)
		job->images = images;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobSaveAs -------------------------------- */
static void
xviewer_job_save_as_class_init (XviewerJobSaveAsClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_save_as_dispose;
	xviewer_job_class->run      = xviewer_job_save_as_run;
}

static
void xviewer_job_save_as_init (XviewerJobSaveAs *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->converter = NULL;
	job->file      = NULL;
}

static
void xviewer_job_save_as_dispose (GObject *object)
{
	XviewerJobSaveAs *job;

	g_return_if_fail (XVIEWER_IS_JOB_SAVE_AS (object));

	job = XVIEWER_JOB_SAVE_AS (object);

	/* free all public and private members */
	if (job->converter != NULL) {
		g_object_unref (job->converter);
		job->converter = NULL;
	}

	if (job->file != NULL) {
		g_object_unref (job->file);
		job->file = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_save_as_parent_class)->dispose (object);
}

static void
xviewer_job_save_as_run (XviewerJob *job)
{
	XviewerJobSave *save_job;
	XviewerJobSaveAs *saveas_job;
	GList *it;
	guint n_images;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_SAVE_AS (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
		return;

	save_job = XVIEWER_JOB_SAVE (g_object_ref (job));
	saveas_job = XVIEWER_JOB_SAVE_AS (job);

	save_job->current_position = 0;
	n_images = g_list_length (save_job->images);

	for (it = save_job->images; it != NULL; it = it->next, save_job->current_position++) {
		GdkPixbufFormat *format;
		XviewerImageSaveInfo *src_info, *dest_info;
		XviewerImage *image = XVIEWER_IMAGE (it->data);
		gboolean success = FALSE;
		gulong handler_id = 0;

		save_job->current_image = image;

		xviewer_image_data_ref (image);

		if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_ALL)) {
			XviewerImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = xviewer_image_get_metadata_status (image);
			if (!xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = XVIEWER_IMAGE_DATA_ALL;
			} else if (m_status == XVIEWER_IMAGE_METADATA_NOT_READ)
			{
				// Load only if we haven't read it yet
				data2load = XVIEWER_IMAGE_DATA_EXIF
						| XVIEWER_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				xviewer_image_load (image,
						data2load,
						NULL,
						&job->error);
			}
		}


		g_assert (job->error == NULL);

		handler_id = g_signal_connect (G_OBJECT (image),
						   "save-progress",
							   G_CALLBACK (xviewer_job_save_progress_callback),
						   job);

		src_info = xviewer_image_save_info_new_from_image (image);

		if (n_images == 1) {
			g_assert (saveas_job->file != NULL);

			format = xviewer_pixbuf_get_format (saveas_job->file);

			dest_info = xviewer_image_save_info_new_from_file (saveas_job->file,
									   format);

		/* SaveAsDialog has already secured permission to overwrite */
			if (dest_info->exists) {
				dest_info->overwrite = TRUE;
			}
		} else {
			GFile *dest_file;
			gboolean result;

			result = xviewer_uri_converter_do (saveas_job->converter,
							   image,
							   &dest_file,
							   &format,
							   NULL);

			g_assert (result);

			dest_info = xviewer_image_save_info_new_from_file (dest_file,
									   format);
		}

		success = xviewer_image_save_as_by_info (image,
							 src_info,
							 dest_info,
							 &job->error);

		if (src_info)
			g_object_unref (src_info);

		if (dest_info)
			g_object_unref (dest_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		xviewer_image_data_unref (image);

		if (!success)
			break;
	}

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_save_as_new (GList           *images,
		     XviewerURIConverter *converter,
		     GFile           *file)
{
	XviewerJobSaveAs *job;

	job = g_object_new (XVIEWER_TYPE_JOB_SAVE_AS, NULL);

	if (images)
		XVIEWER_JOB_SAVE(job)->images = images;

	if (converter)
		job->converter = g_object_ref (converter);

	if (file)
		job->file = g_object_ref (file);

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobThumbnail -------------------------------- */
static void
xviewer_job_thumbnail_class_init (XviewerJobThumbnailClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_thumbnail_dispose;
	xviewer_job_class->run      = xviewer_job_thumbnail_run;
}

static
void xviewer_job_thumbnail_init (XviewerJobThumbnail *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->image     = NULL;
	job->thumbnail = NULL;
}

static
void xviewer_job_thumbnail_dispose (GObject *object)
{
	XviewerJobThumbnail *job;

	g_return_if_fail (XVIEWER_IS_JOB_THUMBNAIL (object));

	job = XVIEWER_JOB_THUMBNAIL (object);

	/* free all public and private members */
	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	if (job->thumbnail) {
		g_object_unref (job->thumbnail);
		job->thumbnail = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_thumbnail_parent_class)->dispose (object);
}

static void
xviewer_job_thumbnail_run (XviewerJob *job)
{
	XviewerJobThumbnail *job_thumbnail;
	gchar           *original_width;
	gchar           *original_height;
	gint             width;
	gint             height;
	GdkPixbuf       *pixbuf;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_THUMBNAIL (job));

	job_thumbnail = XVIEWER_JOB_THUMBNAIL (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* try to load the image thumbnail from cache */
	job_thumbnail->thumbnail = xviewer_thumbnail_load (job_thumbnail->image,
						       &job->error);

	if (!job_thumbnail->thumbnail) {
		job->finished = TRUE;
		return;
	}

	/* create the image thumbnail */
	original_width  = g_strdup (gdk_pixbuf_get_option (job_thumbnail->thumbnail, "tEXt::Thumb::Image::Width"));
	original_height = g_strdup (gdk_pixbuf_get_option (job_thumbnail->thumbnail, "tEXt::Thumb::Image::Height"));

	pixbuf = xviewer_thumbnail_fit_to_size (job_thumbnail->thumbnail,
					    XVIEWER_LIST_STORE_THUMB_SIZE);

	g_object_unref (job_thumbnail->thumbnail);
	job_thumbnail->thumbnail = xviewer_thumbnail_add_frame (pixbuf);
	g_object_unref (pixbuf);

	if (original_width) {
		sscanf (original_width, "%i", &width);
		g_object_set_data (G_OBJECT (job_thumbnail->thumbnail),
				   XVIEWER_THUMBNAIL_ORIGINAL_WIDTH,
				   GINT_TO_POINTER (width));
		g_free (original_width);
	}

	if (original_height) {
		sscanf (original_height, "%i", &height);
		g_object_set_data (G_OBJECT (job_thumbnail->thumbnail),
				   XVIEWER_THUMBNAIL_ORIGINAL_HEIGHT,
				   GINT_TO_POINTER (height));
		g_free (original_height);
	}

	/* show info for debugging */
	if (job->error)
		g_warning ("%s", job->error->message);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_thumbnail_new (XviewerImage *image)
{
	XviewerJobThumbnail *job;

	job = g_object_new (XVIEWER_TYPE_JOB_THUMBNAIL, NULL);

	if (image)
		job->image = g_object_ref (image);

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

/* ------------------------------- XviewerJobTransform -------------------------------- */
static void
xviewer_job_transform_class_init (XviewerJobTransformClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	XviewerJobClass  *xviewer_job_class  = (XviewerJobClass *)  class;

	g_object_class->dispose = xviewer_job_transform_dispose;
	xviewer_job_class->run      = xviewer_job_transform_run;
}

static
void xviewer_job_transform_init (XviewerJobTransform *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images    = NULL;
	job->transform = NULL;
}

static
void xviewer_job_transform_dispose (GObject *object)
{
	XviewerJobTransform *job;

	g_return_if_fail (XVIEWER_IS_JOB_TRANSFORM (object));

	job = XVIEWER_JOB_TRANSFORM (object);

	/* free all public and private members */
	if (job->transform) {
		g_object_unref (job->transform);
		job->transform = NULL;
	}

	if (job->images) {
		g_list_foreach (job->images, (GFunc) g_object_unref, NULL);
		g_list_free (job->images);
	}

	/* call parent dispose */
	G_OBJECT_CLASS (xviewer_job_transform_parent_class)->dispose (object);
}

static gboolean
xviewer_job_transform_image_modified (gpointer data)
{
	g_return_val_if_fail (XVIEWER_IS_IMAGE (data), FALSE);

	xviewer_image_modified (XVIEWER_IMAGE (data));
	g_object_unref (G_OBJECT (data));

	return FALSE;
}

static void
xviewer_job_transform_run (XviewerJob *job)
{
	XviewerJobTransform *transjob;
	GList *it;

	/* initialization */
	g_return_if_fail (XVIEWER_IS_JOB_TRANSFORM (job));

	transjob = XVIEWER_JOB_TRANSFORM (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (xviewer_job_is_cancelled (job))
	{
		g_object_unref (transjob);
		return;
	}

	for (it = transjob->images; it != NULL; it = it->next) {
		XviewerImage *image = XVIEWER_IMAGE (it->data);

		if (transjob->transform == NULL) {
			xviewer_image_undo (image);
		} else {
			xviewer_image_transform (image, transjob->transform, job);
		}

		if (xviewer_image_is_modified (image) || transjob->transform == NULL) {
			g_object_ref (image);
			g_idle_add (xviewer_job_transform_image_modified, image);
		}

		if (G_UNLIKELY (xviewer_job_is_cancelled (job)))
		{
			g_object_unref (transjob);
			return;
		}
	}

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* job finished */
	job->finished = TRUE;

	/* --- leave critical section --- */
	g_mutex_unlock (job->mutex);

	/* notify job finalization */
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc) notify_finished,
			 job,
			 g_object_unref);
}

XviewerJob *
xviewer_job_transform_new (GList        *images,
		       XviewerTransform *transform)
{
	XviewerJobTransform *job;

	job = g_object_new (XVIEWER_TYPE_JOB_TRANSFORM, NULL);

	if (images)
		job->images = images;

	if (transform)
		job->transform = g_object_ref (transform);

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	return XVIEWER_JOB (job);
}

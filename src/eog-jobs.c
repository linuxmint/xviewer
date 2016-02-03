/* Eye Of Gnome - Jobs
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

#include "eog-debug.h"
#include "eog-jobs.h"
#include "eog-thumbnail.h"
#include "eog-pixbuf-util.h"

#include <gio/gio.h>

G_DEFINE_ABSTRACT_TYPE (EogJob, eog_job, G_TYPE_OBJECT);
G_DEFINE_TYPE (EogJobCopy,      eog_job_copy,      EOG_TYPE_JOB);
G_DEFINE_TYPE (EogJobLoad,      eog_job_load,      EOG_TYPE_JOB);
G_DEFINE_TYPE (EogJobModel,     eog_job_model,     EOG_TYPE_JOB);
G_DEFINE_TYPE (EogJobSave,      eog_job_save,      EOG_TYPE_JOB);
G_DEFINE_TYPE (EogJobSaveAs,    eog_job_save_as,   EOG_TYPE_JOB_SAVE);
G_DEFINE_TYPE (EogJobThumbnail, eog_job_thumbnail, EOG_TYPE_JOB);
G_DEFINE_TYPE (EogJobTransform, eog_job_transform, EOG_TYPE_JOB);

/* signals */
enum {
	PROGRESS,
	CANCELLED,
	FINISHED,
	LAST_SIGNAL
};

static guint job_signals[LAST_SIGNAL];

/* notify signal funcs */
static gboolean notify_progress              (EogJob               *job);
static gboolean notify_cancelled             (EogJob               *job);
static gboolean notify_finished              (EogJob               *job);

/* gobject vfuncs */
static void     eog_job_class_init           (EogJobClass          *class);
static void     eog_job_init                 (EogJob               *job);
static void     eog_job_dispose              (GObject              *object);

static void     eog_job_copy_class_init      (EogJobCopyClass      *class);
static void     eog_job_copy_init            (EogJobCopy           *job);
static void     eog_job_copy_dispose         (GObject              *object);

static void     eog_job_load_class_init      (EogJobLoadClass      *class);
static void     eog_job_load_init            (EogJobLoad           *job);
static void     eog_job_load_dispose         (GObject              *object);

static void     eog_job_model_class_init     (EogJobModelClass     *class);
static void     eog_job_model_init           (EogJobModel          *job);
static void     eog_job_model_dispose        (GObject              *object);

static void     eog_job_save_class_init      (EogJobSaveClass      *class);
static void     eog_job_save_init            (EogJobSave           *job);
static void     eog_job_save_dispose         (GObject              *object);

static void     eog_job_save_as_class_init   (EogJobSaveAsClass    *class);
static void     eog_job_save_as_init         (EogJobSaveAs         *job);
static void     eog_job_save_as_dispose      (GObject              *object);

static void     eog_job_thumbnail_class_init (EogJobThumbnailClass *class);
static void     eog_job_thumbnail_init       (EogJobThumbnail      *job);
static void     eog_job_thumbnail_dispose    (GObject              *object);

static void     eog_job_transform_class_init (EogJobTransformClass *class);
static void     eog_job_transform_init       (EogJobTransform      *job);
static void     eog_job_transform_dispose    (GObject              *object);

/* vfuncs */
static void     eog_job_run_unimplemented    (EogJob               *job);
static void     eog_job_copy_run             (EogJob               *job);
static void     eog_job_load_run             (EogJob               *job);
static void     eog_job_model_run            (EogJob               *job);
static void     eog_job_save_run             (EogJob               *job);
static void     eog_job_save_as_run          (EogJob               *job);
static void     eog_job_thumbnail_run        (EogJob               *job);
static void     eog_job_transform_run        (EogJob               *job);

/* callbacks */
static void eog_job_copy_progress_callback (goffset  current_num_bytes,
					    goffset  total_num_bytes,
					    gpointer user_data);

static void eog_job_save_progress_callback (EogImage *image,
					    gfloat    progress,
					    gpointer  data);

/* --------------------------- notify signal funcs --------------------------- */
static gboolean
notify_progress (EogJob *job)
{
	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
		return FALSE;

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job update its progress to -> %1.2f",
			   EOG_GET_TYPE_NAME (job),
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
notify_cancelled (EogJob *job)
{
	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CANCELLED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	/* notify cancelation */
	g_signal_emit (job,
		       job_signals[CANCELLED],
		       0);

	return FALSE;
}

static gboolean
notify_finished (EogJob *job)
{
	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was FINISHED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	/* notify job finalization */
	g_signal_emit (job,
		       job_signals[FINISHED],
		       0);
	return FALSE;
}

/* --------------------------------- EogJob ---------------------------------- */
static void
eog_job_class_init (EogJobClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;

	g_object_class->dispose = eog_job_dispose;
	class->run              = eog_job_run_unimplemented;

	/* signals */
	job_signals [PROGRESS] =
		g_signal_new ("progress",
			      EOG_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EogJobClass, progress),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__FLOAT,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_FLOAT);

	job_signals [CANCELLED] =
		g_signal_new ("cancelled",
			      EOG_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EogJobClass, cancelled),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	job_signals [FINISHED] =
		g_signal_new ("finished",
			      EOG_TYPE_JOB,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (EogJobClass, finished),
			      NULL,
			      NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static
void eog_job_init (EogJob *job)
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
void eog_job_dispose (GObject *object)
{
	EogJob *job;

	g_return_if_fail (EOG_IS_JOB (object));

	job = EOG_JOB (object);

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
	G_OBJECT_CLASS (eog_job_parent_class)->dispose (object);
}

static void
eog_job_run_unimplemented (EogJob *job)
{
	g_critical ("Class \"%s\" does not implement the required run action",
		    G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (job)));
}

void
eog_job_run (EogJob *job)
{
	EogJobClass *class;

	g_return_if_fail (EOG_IS_JOB (job));

	class = EOG_JOB_GET_CLASS (job);
	class->run (job);
}

void
eog_job_cancel (EogJob *job)
{
	g_return_if_fail (EOG_IS_JOB (job));

	g_object_ref (job);

	/* check if job was cancelled previously */
	if (job->cancelled)
		return;

	/* check if job finished previously */
        if (job->finished)
		return;

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "CANCELLING a %s (%p)",
			   EOG_GET_TYPE_NAME (job),
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
eog_job_get_progress (EogJob *job)
{
	g_return_val_if_fail (EOG_IS_JOB (job), 0.0);

	return job->progress;
}

void
eog_job_set_progress (EogJob *job,
		      gfloat  progress)
{
	g_return_if_fail (EOG_IS_JOB (job));
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
eog_job_is_cancelled (EogJob *job)
{
	g_return_val_if_fail (EOG_IS_JOB (job), TRUE);

	return job->cancelled;
}

gboolean
eog_job_is_finished (EogJob *job)
{
	g_return_val_if_fail (EOG_IS_JOB (job), TRUE);

	return job->finished;
}

/* ------------------------------- EogJobCopy -------------------------------- */
static void
eog_job_copy_class_init (EogJobCopyClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_copy_dispose;
	eog_job_class->run      = eog_job_copy_run;
}

static
void eog_job_copy_init (EogJobCopy *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images           = NULL;
	job->destination      = NULL;
	job->current_position = 0;
}

static
void eog_job_copy_dispose (GObject *object)
{
	EogJobCopy *job;

	g_return_if_fail (EOG_IS_JOB_COPY (object));

	job = EOG_JOB_COPY (object);

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
	G_OBJECT_CLASS (eog_job_copy_parent_class)->dispose (object);
}

static void
eog_job_copy_progress_callback (goffset  current_num_bytes,
				goffset  total_num_bytes,
				gpointer user_data)
{
	gfloat      progress;
	guint       n_images;
	EogJobCopy *job;

	job = EOG_JOB_COPY (user_data);

	n_images = g_list_length (job->images);

	progress = ((current_num_bytes / (gfloat) total_num_bytes) + job->current_position) / n_images;

	eog_job_set_progress (EOG_JOB (job), progress);
}

static void
eog_job_copy_run (EogJob *job)
{
	EogJobCopy *copyjob;
	GList *it;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_COPY (job));

	copyjob = EOG_JOB_COPY (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
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
					 eog_job_copy_progress_callback, job,
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

EogJob *
eog_job_copy_new (GList       *images,
		  const gchar *destination)
{
	EogJobCopy *job;

	job = g_object_new (EOG_TYPE_JOB_COPY, NULL);

	if (images)
		job->images = images;

	if (destination)
		job->destination = g_strdup (destination);

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobLoad -------------------------------- */
static void
eog_job_load_class_init (EogJobLoadClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_load_dispose;
	eog_job_class->run      = eog_job_load_run;
}

static
void eog_job_load_init (EogJobLoad *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->image = NULL;
	job->data  = EOG_IMAGE_DATA_ALL;
}

static
void eog_job_load_dispose (GObject *object)
{
	EogJobLoad *job;

	g_return_if_fail (EOG_IS_JOB_LOAD (object));

	job = EOG_JOB_LOAD (object);

	/* free all public and private members */
	if (job->image) {
		g_object_unref (job->image);
		job->image = NULL;
	}

	/* call parent dispose */
	G_OBJECT_CLASS (eog_job_load_parent_class)->dispose (object);
}

static void
eog_job_load_run (EogJob *job)
{
	EogJobLoad *job_load;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_LOAD (job));

	job_load = EOG_JOB_LOAD (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* load image from file */
	eog_image_load (job_load->image,
			job_load->data,
			job,
			&job->error);

	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
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

EogJob *
eog_job_load_new (EogImage     *image,
		  EogImageData  data)
{
	EogJobLoad *job;

	job = g_object_new (EOG_TYPE_JOB_LOAD, NULL);

	if (image)
		job->image = g_object_ref (image);

	job->data = data;

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobModel -------------------------------- */
static void
eog_job_model_class_init (EogJobModelClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_model_dispose;
	eog_job_class->run      = eog_job_model_run;
}

static
void eog_job_model_init (EogJobModel *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->store     = NULL;
	job->file_list = NULL;
}

static
void eog_job_model_dispose (GObject *object)
{
	EogJobModel *job;

	g_return_if_fail (EOG_IS_JOB_MODEL (object));

	job = EOG_JOB_MODEL (object);

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
	G_OBJECT_CLASS (eog_job_model_parent_class)->dispose (object);
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
					if (eog_image_is_supported_mime_type (ctype))
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
eog_job_model_run (EogJob *job)
{
	EogJobModel *job_model;
	GList       *filtered_list;
	GList       *error_list;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_MODEL (job));

	job_model     = EOG_JOB_MODEL (g_object_ref (job));
	filtered_list = NULL;
	error_list    = NULL;

	filter_files (job_model->file_list,
		      &filtered_list,
		      &error_list);

	/* --- enter critical section --- */
	g_mutex_lock (job->mutex);

	/* create a list store */
	job_model->store = EOG_LIST_STORE (eog_list_store_new ());
	eog_list_store_add_files (job_model->store, filtered_list);

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

EogJob *
eog_job_model_new (GSList *file_list)
{
	EogJobModel *job;

	job = g_object_new (EOG_TYPE_JOB_MODEL, NULL);

	if (file_list != NULL)
		job->file_list = file_list;

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobSave -------------------------------- */
static void
eog_job_save_class_init (EogJobSaveClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_save_dispose;
	eog_job_class->run      = eog_job_save_run;
}

static
void eog_job_save_init (EogJobSave *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images           = NULL;
	job->current_position = 0;
	job->current_image    = NULL;
}

static
void eog_job_save_dispose (GObject *object)
{
	EogJobSave *job;

	g_return_if_fail (EOG_IS_JOB_SAVE (object));

	job = EOG_JOB_SAVE (object);

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
	G_OBJECT_CLASS (eog_job_save_parent_class)->dispose (object);
}

static void
eog_job_save_progress_callback (EogImage *image,
				gfloat    progress,
				gpointer  data)
{
	EogJobSave *job;
	guint       n_images;
	gfloat      job_progress;

	job = EOG_JOB_SAVE (data);

	n_images     = g_list_length (job->images);
	job_progress = (job->current_position / (gfloat) n_images) + (progress / n_images);

	eog_job_set_progress (EOG_JOB (job), job_progress);
}

static void
eog_job_save_run (EogJob *job)
{
	EogJobSave *save_job;
	GList *it;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_SAVE (job));

	g_object_ref (job);

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
		return;

	save_job = EOG_JOB_SAVE (job);

	save_job->current_position = 0;

	for (it = save_job->images; it != NULL; it = it->next, save_job->current_position++) {
		EogImage *image = EOG_IMAGE (it->data);
		EogImageSaveInfo *save_info = NULL;
		gulong handler_id = 0;
		gboolean success = FALSE;

		save_job->current_image = image;

		/* Make sure the image doesn't go away while saving */
		eog_image_data_ref (image);

		if (!eog_image_has_data (image, EOG_IMAGE_DATA_ALL)) {
			EogImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = eog_image_get_metadata_status (image);
			if (!eog_image_has_data (image, EOG_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = EOG_IMAGE_DATA_ALL;
			} else if (m_status == EOG_IMAGE_METADATA_NOT_READ)
			{
				// Load only if we haven't read it yet
				data2load = EOG_IMAGE_DATA_EXIF
						| EOG_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				eog_image_load (image,
						data2load,
						NULL,
						&job->error);
			}
		}

		handler_id = g_signal_connect (G_OBJECT (image),
						   "save-progress",
							   G_CALLBACK (eog_job_save_progress_callback),
						   job);

		save_info = eog_image_save_info_new_from_image (image);

		success = eog_image_save_by_info (image,
						  save_info,
						  &job->error);

		if (save_info)
			g_object_unref (save_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		eog_image_data_unref (image);

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

EogJob *
eog_job_save_new (GList *images)
{
	EogJobSave *job;

	job = g_object_new (EOG_TYPE_JOB_SAVE, NULL);

	if (images)
		job->images = images;

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobSaveAs -------------------------------- */
static void
eog_job_save_as_class_init (EogJobSaveAsClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_save_as_dispose;
	eog_job_class->run      = eog_job_save_as_run;
}

static
void eog_job_save_as_init (EogJobSaveAs *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->converter = NULL;
	job->file      = NULL;
}

static
void eog_job_save_as_dispose (GObject *object)
{
	EogJobSaveAs *job;

	g_return_if_fail (EOG_IS_JOB_SAVE_AS (object));

	job = EOG_JOB_SAVE_AS (object);

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
	G_OBJECT_CLASS (eog_job_save_as_parent_class)->dispose (object);
}

static void
eog_job_save_as_run (EogJob *job)
{
	EogJobSave *save_job;
	EogJobSaveAs *saveas_job;
	GList *it;
	guint n_images;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_SAVE_AS (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
		return;

	save_job = EOG_JOB_SAVE (g_object_ref (job));
	saveas_job = EOG_JOB_SAVE_AS (job);

	save_job->current_position = 0;
	n_images = g_list_length (save_job->images);

	for (it = save_job->images; it != NULL; it = it->next, save_job->current_position++) {
		GdkPixbufFormat *format;
		EogImageSaveInfo *src_info, *dest_info;
		EogImage *image = EOG_IMAGE (it->data);
		gboolean success = FALSE;
		gulong handler_id = 0;

		save_job->current_image = image;

		eog_image_data_ref (image);

		if (!eog_image_has_data (image, EOG_IMAGE_DATA_ALL)) {
			EogImageMetadataStatus m_status;
			gint data2load = 0;

			m_status = eog_image_get_metadata_status (image);
			if (!eog_image_has_data (image, EOG_IMAGE_DATA_IMAGE)) {
				// Queue full read in this case
				data2load = EOG_IMAGE_DATA_ALL;
			} else if (m_status == EOG_IMAGE_METADATA_NOT_READ)
			{
				// Load only if we haven't read it yet
				data2load = EOG_IMAGE_DATA_EXIF
						| EOG_IMAGE_DATA_XMP;
			}

			if (data2load != 0) {
				eog_image_load (image,
						data2load,
						NULL,
						&job->error);
			}
		}


		g_assert (job->error == NULL);

		handler_id = g_signal_connect (G_OBJECT (image),
						   "save-progress",
							   G_CALLBACK (eog_job_save_progress_callback),
						   job);

		src_info = eog_image_save_info_new_from_image (image);

		if (n_images == 1) {
			g_assert (saveas_job->file != NULL);

			format = eog_pixbuf_get_format (saveas_job->file);

			dest_info = eog_image_save_info_new_from_file (saveas_job->file,
									   format);

		/* SaveAsDialog has already secured permission to overwrite */
			if (dest_info->exists) {
				dest_info->overwrite = TRUE;
			}
		} else {
			GFile *dest_file;
			gboolean result;

			result = eog_uri_converter_do (saveas_job->converter,
							   image,
							   &dest_file,
							   &format,
							   NULL);

			g_assert (result);

			dest_info = eog_image_save_info_new_from_file (dest_file,
									   format);
		}

		success = eog_image_save_as_by_info (image,
							 src_info,
							 dest_info,
							 &job->error);

		if (src_info)
			g_object_unref (src_info);

		if (dest_info)
			g_object_unref (dest_info);

		if (handler_id != 0)
			g_signal_handler_disconnect (G_OBJECT (image), handler_id);

		eog_image_data_unref (image);

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

EogJob *
eog_job_save_as_new (GList           *images,
		     EogURIConverter *converter,
		     GFile           *file)
{
	EogJobSaveAs *job;

	job = g_object_new (EOG_TYPE_JOB_SAVE_AS, NULL);

	if (images)
		EOG_JOB_SAVE(job)->images = images;

	if (converter)
		job->converter = g_object_ref (converter);

	if (file)
		job->file = g_object_ref (file);

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobThumbnail -------------------------------- */
static void
eog_job_thumbnail_class_init (EogJobThumbnailClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_thumbnail_dispose;
	eog_job_class->run      = eog_job_thumbnail_run;
}

static
void eog_job_thumbnail_init (EogJobThumbnail *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->image     = NULL;
	job->thumbnail = NULL;
}

static
void eog_job_thumbnail_dispose (GObject *object)
{
	EogJobThumbnail *job;

	g_return_if_fail (EOG_IS_JOB_THUMBNAIL (object));

	job = EOG_JOB_THUMBNAIL (object);

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
	G_OBJECT_CLASS (eog_job_thumbnail_parent_class)->dispose (object);
}

static void
eog_job_thumbnail_run (EogJob *job)
{
	EogJobThumbnail *job_thumbnail;
	gchar           *original_width;
	gchar           *original_height;
	gint             width;
	gint             height;
	GdkPixbuf       *pixbuf;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_THUMBNAIL (job));

	job_thumbnail = EOG_JOB_THUMBNAIL (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* try to load the image thumbnail from cache */
	job_thumbnail->thumbnail = eog_thumbnail_load (job_thumbnail->image,
						       &job->error);

	if (!job_thumbnail->thumbnail) {
		job->finished = TRUE;
		return;
	}

	/* create the image thumbnail */
	original_width  = g_strdup (gdk_pixbuf_get_option (job_thumbnail->thumbnail, "tEXt::Thumb::Image::Width"));
	original_height = g_strdup (gdk_pixbuf_get_option (job_thumbnail->thumbnail, "tEXt::Thumb::Image::Height"));

	pixbuf = eog_thumbnail_fit_to_size (job_thumbnail->thumbnail,
					    EOG_LIST_STORE_THUMB_SIZE);

	g_object_unref (job_thumbnail->thumbnail);
	job_thumbnail->thumbnail = eog_thumbnail_add_frame (pixbuf);
	g_object_unref (pixbuf);

	if (original_width) {
		sscanf (original_width, "%i", &width);
		g_object_set_data (G_OBJECT (job_thumbnail->thumbnail),
				   EOG_THUMBNAIL_ORIGINAL_WIDTH,
				   GINT_TO_POINTER (width));
		g_free (original_width);
	}

	if (original_height) {
		sscanf (original_height, "%i", &height);
		g_object_set_data (G_OBJECT (job_thumbnail->thumbnail),
				   EOG_THUMBNAIL_ORIGINAL_HEIGHT,
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

EogJob *
eog_job_thumbnail_new (EogImage *image)
{
	EogJobThumbnail *job;

	job = g_object_new (EOG_TYPE_JOB_THUMBNAIL, NULL);

	if (image)
		job->image = g_object_ref (image);

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* ------------------------------- EogJobTransform -------------------------------- */
static void
eog_job_transform_class_init (EogJobTransformClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	EogJobClass  *eog_job_class  = (EogJobClass *)  class;

	g_object_class->dispose = eog_job_transform_dispose;
	eog_job_class->run      = eog_job_transform_run;
}

static
void eog_job_transform_init (EogJobTransform *job)
{
	/* initialize all public and private members to reasonable
	   default values. */
	job->images    = NULL;
	job->transform = NULL;
}

static
void eog_job_transform_dispose (GObject *object)
{
	EogJobTransform *job;

	g_return_if_fail (EOG_IS_JOB_TRANSFORM (object));

	job = EOG_JOB_TRANSFORM (object);

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
	G_OBJECT_CLASS (eog_job_transform_parent_class)->dispose (object);
}

static gboolean
eog_job_transform_image_modified (gpointer data)
{
	g_return_val_if_fail (EOG_IS_IMAGE (data), FALSE);

	eog_image_modified (EOG_IMAGE (data));
	g_object_unref (G_OBJECT (data));

	return FALSE;
}

static void
eog_job_transform_run (EogJob *job)
{
	EogJobTransform *transjob;
	GList *it;

	/* initialization */
	g_return_if_fail (EOG_IS_JOB_TRANSFORM (job));

	transjob = EOG_JOB_TRANSFORM (g_object_ref (job));

	/* clean previous errors */
	if (job->error) {
	        g_error_free (job->error);
		job->error = NULL;
	}

	/* check if the current job was previously cancelled */
	if (eog_job_is_cancelled (job))
	{
		g_object_unref (transjob);
		return;
	}

	for (it = transjob->images; it != NULL; it = it->next) {
		EogImage *image = EOG_IMAGE (it->data);

		if (transjob->transform == NULL) {
			eog_image_undo (image);
		} else {
			eog_image_transform (image, transjob->transform, job);
		}

		if (eog_image_is_modified (image) || transjob->transform == NULL) {
			g_object_ref (image);
			g_idle_add (eog_job_transform_image_modified, image);
		}

		if (G_UNLIKELY (eog_job_is_cancelled (job)))
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

EogJob *
eog_job_transform_new (GList        *images,
		       EogTransform *transform)
{
	EogJobTransform *job;

	job = g_object_new (EOG_TYPE_JOB_TRANSFORM, NULL);

	if (images)
		job->images = images;

	if (transform)
		job->transform = g_object_ref (transform);

	/* show info for debugging */
	eog_debug_message (DEBUG_JOBS,
			   "%s (%p) job was CREATED",
			   EOG_GET_TYPE_NAME (job),
			   job);

	return EOG_JOB (job);
}

/* Xviewer - Jobs scheduler
 *
 * Copyright (C) 2013 The Free Software Foundation
 *
 * Author: Javier Sánchez <jsanchez@deskblue.com>
 *
 * Based on code (ev-job-scheduler.h) by:
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

#include "xviewer-job-scheduler.h"
#include "xviewer-jobs.h"

#include "xviewer-debug.h"

/* sync thread tools */
static GCond  job_queue_cond;
static GMutex job_queue_mutex;

/* priority queues */
static GQueue queue_high   = G_QUEUE_INIT;
static GQueue queue_medium = G_QUEUE_INIT;
static GQueue queue_low    = G_QUEUE_INIT;

static GQueue *job_queue[XVIEWER_JOB_N_PRIORITIES] = {
	&queue_high,
	&queue_medium,
	&queue_low
};

static void      xviewer_job_scheduler_enqueue_job (XviewerJob         *job,
						XviewerJobPriority  priority);
static XviewerJob   *xviewer_job_scheduler_dequeue_job (void);
static gpointer  xviewer_job_scheduler             (gpointer        data);
static void      xviewer_job_process               (XviewerJob         *job);


static void
xviewer_job_scheduler_enqueue_job (XviewerJob         *job,
			       XviewerJobPriority  priority)
{
	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "ENQUEUED %s (%p) with priority %d",
			   XVIEWER_GET_TYPE_NAME (job),
			   job,
			   priority);

	/* --- enter critical section --- */
	g_mutex_lock (&job_queue_mutex);

	g_queue_push_tail (job_queue[priority], job);
	g_cond_broadcast  (&job_queue_cond);

	/* --- leave critical section --- */
	g_mutex_unlock (&job_queue_mutex);
}

static XviewerJob *
xviewer_job_scheduler_dequeue_job (void)
{
	XviewerJob *job;
	gint    priority;

	/* initialization */
	job = NULL;

	while (!job) {
		/* --- enter critical section --- */
		g_mutex_lock (&job_queue_mutex);

		/* try to retrieve the next job from priority queue */
		for (priority = XVIEWER_JOB_PRIORITY_HIGH; priority < XVIEWER_JOB_N_PRIORITIES; priority++) {
			job = (XviewerJob *) g_queue_pop_head (job_queue[priority]);

			if (job)
				break;
		}

		/* show info for debugging */
		xviewer_debug_message (DEBUG_JOBS,
				   (job ? "DEQUEUED %s (%p)" : "No jobs in queue"),
				   XVIEWER_GET_TYPE_NAME (job),
				   job);

		/* if there is no job, wait for it */
		if (!job) {
			xviewer_debug_message (DEBUG_JOBS,
					   "Wating for jobs ...");

			g_cond_wait    (&job_queue_cond,
					&job_queue_mutex);
			g_mutex_unlock (&job_queue_mutex);
			continue;
		}

		/* --- leave critical section --- */
		g_mutex_unlock (&job_queue_mutex);
	}

	return job;
}

static gpointer
xviewer_job_scheduler (gpointer data)
{
	XviewerJob *job;

	while (TRUE) {
		/* retrieve the next job */
		job = xviewer_job_scheduler_dequeue_job ();

		/* execute the job */
		xviewer_job_process (job);

		/* free executed job */
		g_object_unref  (job);
	}

	return NULL;
}

static void
xviewer_job_process (XviewerJob *job)
{
	g_return_if_fail (XVIEWER_IS_JOB (job));

	/* nothing to do if job was cancelled */
	if (xviewer_job_is_cancelled (job))
		return;

	/* show info for debugging */
	xviewer_debug_message (DEBUG_JOBS,
			   "PROCESSING a %s (%p)",
			   XVIEWER_GET_TYPE_NAME (job),
			   job);

	/* process the current job */
	xviewer_job_run (job);
}

void
xviewer_job_scheduler_init ()
{
	g_thread_new ("XviewerJobScheduler",
		      xviewer_job_scheduler,
		      NULL);
}

void
xviewer_job_scheduler_add_job (XviewerJob *job)
{
	g_return_if_fail (XVIEWER_IS_JOB (job));

	/* make sure the job isn't destroyed */
	g_object_ref (job);

	/* enqueue the job */
	xviewer_job_scheduler_enqueue_job (job, XVIEWER_JOB_PRIORITY_LOW);
}

void
xviewer_job_scheduler_add_job_with_priority (XviewerJob         *job,
					 XviewerJobPriority  priority)
{
	g_return_if_fail (XVIEWER_IS_JOB (job));

	/* make sure the job isn't destroyed */
	g_object_ref (job);

	/* enqueue the job */
	xviewer_job_scheduler_enqueue_job (job, priority);
}

/* Eye Of Gnome - Jobs scheduler
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

#ifndef __EOG_JOB_SCHEDULER_H__
#define __EOG_JOB_SCHEDULER_H__

#include "eog-jobs.h"

G_BEGIN_DECLS

typedef enum {
	EOG_JOB_PRIORITY_HIGH,
	EOG_JOB_PRIORITY_MEDIUM,
	EOG_JOB_PRIORITY_LOW,
	EOG_JOB_N_PRIORITIES
} EogJobPriority;

/* initialization */
void eog_job_scheduler_init                  (void);

/* jobs management */
void eog_job_scheduler_add_job               (EogJob         *job);
void eog_job_scheduler_add_job_with_priority (EogJob         *job,
					      EogJobPriority  priority);

G_END_DECLS

#endif /* __EOG_JOB_SCHEDULER_H__ */

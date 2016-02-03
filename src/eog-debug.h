/* Eye Of Gnome - Debugging
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-debug.h) by:
 * 	- Alex Roberts <bse@error.fsnet.co.uk>
 *	- Evan Lawrence <evan@worldpath.net>
 *	- Chema Celorio <chema@celorio.com>
 *	- Paolo Maggi <paolo@gnome.org>
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

#ifndef __EOG_DEBUG_H__
#define __EOG_DEBUG_H__

#include <glib.h>

#define EOG_GET_TYPE_NAME(instance) g_type_name_from_instance ((gpointer)instance)

typedef enum {
	EOG_DEBUG_NO_DEBUG     = 0,
	EOG_DEBUG_WINDOW       = 1 << 0,
	EOG_DEBUG_VIEW         = 1 << 1,
	EOG_DEBUG_JOBS         = 1 << 2,
	EOG_DEBUG_THUMBNAIL    = 1 << 3,
	EOG_DEBUG_IMAGE_DATA   = 1 << 4,
	EOG_DEBUG_IMAGE_LOAD   = 1 << 5,
	EOG_DEBUG_IMAGE_SAVE   = 1 << 6,
	EOG_DEBUG_LIST_STORE   = 1 << 7,
	EOG_DEBUG_PREFERENCES  = 1 << 8,
	EOG_DEBUG_PRINTING     = 1 << 9,
	EOG_DEBUG_LCMS         = 1 << 10,
	EOG_DEBUG_PLUGINS      = 1 << 11
} EogDebug;

#define	DEBUG_WINDOW		EOG_DEBUG_WINDOW,      __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_VIEW		EOG_DEBUG_VIEW,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_JOBS		EOG_DEBUG_JOBS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_THUMBNAIL		EOG_DEBUG_THUMBNAIL,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_DATA	EOG_DEBUG_IMAGE_DATA,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_LOAD	EOG_DEBUG_IMAGE_LOAD,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_SAVE	EOG_DEBUG_IMAGE_SAVE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LIST_STORE	EOG_DEBUG_LIST_STORE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFERENCES	EOG_DEBUG_PREFERENCES, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PRINTING		EOG_DEBUG_PRINTING,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LCMS 		EOG_DEBUG_LCMS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS 		EOG_DEBUG_PLUGINS,     __FILE__, __LINE__, G_STRFUNC

void   eog_debug_init        (void);

void   eog_debug             (EogDebug    section,
          	              const gchar       *file,
          	              gint               line,
          	              const gchar       *function);

void   eog_debug_message     (EogDebug    section,
			      const gchar       *file,
			      gint               line,
			      const gchar       *function,
			      const gchar       *format, ...) G_GNUC_PRINTF(5, 6);

#endif /* __EOG_DEBUG_H__ */

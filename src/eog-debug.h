/* Xviewer - Debugging
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

#ifndef __XVIEWER_DEBUG_H__
#define __XVIEWER_DEBUG_H__

#include <glib.h>

#define XVIEWER_GET_TYPE_NAME(instance) g_type_name_from_instance ((gpointer)instance)

typedef enum {
	XVIEWER_DEBUG_NO_DEBUG     = 0,
	XVIEWER_DEBUG_WINDOW       = 1 << 0,
	XVIEWER_DEBUG_VIEW         = 1 << 1,
	XVIEWER_DEBUG_JOBS         = 1 << 2,
	XVIEWER_DEBUG_THUMBNAIL    = 1 << 3,
	XVIEWER_DEBUG_IMAGE_DATA   = 1 << 4,
	XVIEWER_DEBUG_IMAGE_LOAD   = 1 << 5,
	XVIEWER_DEBUG_IMAGE_SAVE   = 1 << 6,
	XVIEWER_DEBUG_LIST_STORE   = 1 << 7,
	XVIEWER_DEBUG_PREFERENCES  = 1 << 8,
	XVIEWER_DEBUG_PRINTING     = 1 << 9,
	XVIEWER_DEBUG_LCMS         = 1 << 10,
	XVIEWER_DEBUG_PLUGINS      = 1 << 11
} XviewerDebug;

#define	DEBUG_WINDOW		XVIEWER_DEBUG_WINDOW,      __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_VIEW		XVIEWER_DEBUG_VIEW,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_JOBS		XVIEWER_DEBUG_JOBS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_THUMBNAIL		XVIEWER_DEBUG_THUMBNAIL,   __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_DATA	XVIEWER_DEBUG_IMAGE_DATA,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_LOAD	XVIEWER_DEBUG_IMAGE_LOAD,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_IMAGE_SAVE	XVIEWER_DEBUG_IMAGE_SAVE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LIST_STORE	XVIEWER_DEBUG_LIST_STORE,  __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PREFERENCES	XVIEWER_DEBUG_PREFERENCES, __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PRINTING		XVIEWER_DEBUG_PRINTING,    __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_LCMS 		XVIEWER_DEBUG_LCMS,        __FILE__, __LINE__, G_STRFUNC
#define	DEBUG_PLUGINS 		XVIEWER_DEBUG_PLUGINS,     __FILE__, __LINE__, G_STRFUNC

void   xviewer_debug_init        (void);

void   xviewer_debug             (XviewerDebug    section,
          	              const gchar       *file,
          	              gint               line,
          	              const gchar       *function);

void   xviewer_debug_message     (XviewerDebug    section,
			      const gchar       *file,
			      gint               line,
			      const gchar       *function,
			      const gchar       *format, ...) G_GNUC_PRINTF(5, 6);

#endif /* __XVIEWER_DEBUG_H__ */

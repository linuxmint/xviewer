/* Xviewer - General Utilities
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 *	- Jens Finke <jens@gnome.org>
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

#ifndef __XVIEWER_UTIL_H__
#define __XVIEWER_UTIL_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

G_GNUC_INTERNAL
void     xviewer_util_show_help                  (const gchar *section,
					      GtkWindow   *parent);

G_GNUC_INTERNAL
gchar   *xviewer_util_make_valid_utf8            (const gchar *name);

G_GNUC_INTERNAL
GSList  *xviewer_util_parse_uri_string_list_to_file_list (const gchar *uri_list);

G_GNUC_INTERNAL
GSList  *xviewer_util_string_list_to_file_list    (GSList *string_list);

G_GNUC_INTERNAL
GSList  *xviewer_util_strings_to_file_list        (gchar **strings);

G_GNUC_INTERNAL
GSList  *xviewer_util_string_array_to_list       (const gchar **files,
	 				      gboolean create_uri);

G_GNUC_INTERNAL
gchar  **xviewer_util_string_array_make_absolute (gchar **files);

G_GNUC_INTERNAL
gboolean xviewer_util_launch_desktop_file        (const gchar *filename,
					      guint32      user_time);

G_GNUC_INTERNAL
const    gchar *xviewer_util_dot_dir             (void);

G_GNUC_INTERNAL
char *  xviewer_util_filename_get_extension      (const char * filename);

G_GNUC_INTERNAL
gboolean xviewer_util_file_is_persistent (GFile *file);

G_GNUC_INTERNAL
void     xviewer_util_show_file_in_filemanager   (GFile *file,
					      GdkScreen *screen);

G_END_DECLS

#endif /* __XVIEWER_UTIL_H__ */

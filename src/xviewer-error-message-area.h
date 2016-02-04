/* Xviewer - Error Message Area
 *
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on gedit code (gedit/gedit-message-area.h) by:
 * 	- Paolo Maggi <paolo@gnome.org>
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

#ifndef __XVIEWER_ERROR_MESSAGE_AREA__
#define __XVIEWER_ERROR_MESSAGE_AREA__

#include <glib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

/**
 * XviewerErrorMessageAreaResponseType:
 * @XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_NONE: Returned if the message area has no response id,
 * or if the message area gets programmatically hidden or destroyed
 * @XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_CANCEL: Returned by CANCEL button in the message area
 * @XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_RELOAD: Returned by RELOAD button in the message area
 * @XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_SAVEAS: Returned by SAVE AS button in the message area
 *
 */
typedef enum
{
	XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_NONE   = 0,
	XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_CANCEL = 1,
	XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_RELOAD = 2,
	XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_SAVEAS = 3
} XviewerErrorMessageAreaResponseType;

G_GNUC_INTERNAL
GtkWidget   *xviewer_image_load_error_message_area_new   (const gchar  *caption,
						      const GError *error);

G_GNUC_INTERNAL
GtkWidget   *xviewer_image_save_error_message_area_new   (const gchar  *caption,
						      const GError *error);

G_GNUC_INTERNAL
GtkWidget   *xviewer_no_images_error_message_area_new    (GFile *file);

#endif /* __XVIEWER_ERROR_MESSAGE_AREA__ */

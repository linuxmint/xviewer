/* Xviewer - GSettings Keys Macros
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnome.org>
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

#ifndef __XVIEWER_CONFIG_KEYS_H__
#define __XVIEWER_CONFIG_KEYS_H__

#define XVIEWER_CONF_DOMAIN				"org.x.viewer"
#define XVIEWER_CONF_FULLSCREEN			XVIEWER_CONF_DOMAIN".fullscreen"
#define XVIEWER_CONF_PLUGINS			XVIEWER_CONF_DOMAIN".plugins"
#define XVIEWER_CONF_UI				XVIEWER_CONF_DOMAIN".ui"
#define XVIEWER_CONF_VIEW				XVIEWER_CONF_DOMAIN".view"

#define XVIEWER_CONF_DESKTOP_LOCKDOWN_SCHEMA	"org.gnome.desktop.lockdown"
#define XVIEWER_CONF_DESKTOP_CAN_PRINT		"disable-printing"
#define XVIEWER_CONF_DESKTOP_CAN_SAVE		"disable-save-to-disk"
#define XVIEWER_CONF_DESKTOP_CAN_SETUP_PAGE 	"disable-print-setup"

#define XVIEWER_CONF_VIEW_BACKGROUND_COLOR		"background-color"
#define XVIEWER_CONF_VIEW_INTERPOLATE		"interpolate"
#define XVIEWER_CONF_VIEW_EXTRAPOLATE		"extrapolate"
#define XVIEWER_CONF_VIEW_SCROLL_WHEEL_ZOOM		"scroll-wheel-zoom"
#define XVIEWER_CONF_VIEW_ZOOM_MULTIPLIER		"zoom-multiplier"
#define XVIEWER_CONF_VIEW_AUTOROTATE                "autorotate"
#define XVIEWER_CONF_VIEW_TRANSPARENCY		"transparency"
#define XVIEWER_CONF_VIEW_TRANS_COLOR		"trans-color"
#define XVIEWER_CONF_VIEW_USE_BG_COLOR		"use-background-color"

#define XVIEWER_CONF_FULLSCREEN_LOOP		"loop"
#define XVIEWER_CONF_FULLSCREEN_UPSCALE		"upscale"
#define XVIEWER_CONF_FULLSCREEN_SECONDS		"seconds"

#define XVIEWER_CONF_UI_TOOLBAR			"toolbar"
#define XVIEWER_CONF_UI_STATUSBAR			"statusbar"
#define XVIEWER_CONF_UI_IMAGE_GALLERY		"image-gallery"
#define XVIEWER_CONF_UI_IMAGE_GALLERY_POSITION	"image-gallery-position"
#define XVIEWER_CONF_UI_IMAGE_GALLERY_RESIZABLE	"image-gallery-resizable"
#define XVIEWER_CONF_UI_SIDEBAR			"sidebar"
#define XVIEWER_CONF_UI_SCROLL_BUTTONS		"scroll-buttons"
#define XVIEWER_CONF_UI_DISABLE_CLOSE_CONFIRMATION  "disable-close-confirmation"
#define XVIEWER_CONF_UI_DISABLE_TRASH_CONFIRMATION	"disable-trash-confirmation"
#define XVIEWER_CONF_UI_FILECHOOSER_XDG_FALLBACK	"filechooser-xdg-fallback"
#define XVIEWER_CONF_UI_PROPSDIALOG_NETBOOK_MODE	"propsdialog-netbook-mode"
#define XVIEWER_CONF_UI_EXTERNAL_EDITOR		"external-editor"

#define XVIEWER_CONF_PLUGINS_ACTIVE_PLUGINS         "active-plugins"

#endif /* __XVIEWER_CONFIG_KEYS_H__ */

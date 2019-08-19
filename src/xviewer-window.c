/* Xviewer - Main Window
 *
 * Copyright (C) 2000-2008 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on code by:
 * 	- Federico Mena-Quintero <federico@gnome.org>
 *	- Jens Finke <jens@gnome.org>
 * Based on evince code (shell/ev-window.c) by:
 * 	- Martin Kretzschmar <martink@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "xviewer-window.h"
#include "xviewer-scroll-view.h"
#include "xviewer-debug.h"
#include "xviewer-file-chooser.h"
#include "xviewer-thumb-view.h"
#include "xviewer-list-store.h"
#include "xviewer-sidebar.h"
#include "xviewer-statusbar.h"
#include "xviewer-preferences-dialog.h"
#include "xviewer-properties-dialog.h"
#include "xviewer-print.h"
#include "xviewer-error-message-area.h"
#include "xviewer-application.h"
#include "xviewer-application-internal.h"
#include "xviewer-thumb-nav.h"
#include "xviewer-config-keys.h"
#include "xviewer-job-scheduler.h"
#include "xviewer-jobs.h"
#include "xviewer-util.h"
#include "xviewer-save-as-dialog-helper.h"
#include "xviewer-plugin-engine.h"
#include "xviewer-close-confirmation-dialog.h"
#include "xviewer-clipboard-handler.h"
#include "xviewer-window-activatable.h"
#include "xviewer-metadata-sidebar.h"

#include "xviewer-enum-types.h"

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gdk/gdkkeysyms.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#include <libpeas/peas-extension-set.h>
#include <libpeas/peas-activatable.h>

#if defined(HAVE_LCMS) && defined(GDK_WINDOWING_X11)
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <lcms2.h>
#endif

#include <stdlib.h>

#define XVIEWER_WINDOW_MIN_WIDTH  440
#define XVIEWER_WINDOW_MIN_HEIGHT 350

#define XVIEWER_WINDOW_DEFAULT_WIDTH  540
#define XVIEWER_WINDOW_DEFAULT_HEIGHT 450

#define XVIEWER_WINDOW_FULLSCREEN_TIMEOUT 2 * 1000
#define XVIEWER_WINDOW_FULLSCREEN_POPUP_THRESHOLD 5

#define XVIEWER_RECENT_FILES_GROUP  "Graphics"
#define XVIEWER_RECENT_FILES_APP_NAME "Image Viewer"
#define XVIEWER_RECENT_FILES_LIMIT  5

#define XVIEWER_WALLPAPER_FILENAME "xviewer-wallpaper"

#define is_rtl (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL)

typedef enum {
	XVIEWER_WINDOW_STATUS_UNKNOWN,
	XVIEWER_WINDOW_STATUS_INIT,
	XVIEWER_WINDOW_STATUS_NORMAL
} XviewerWindowStatus;

enum {
	PROP_0,
	PROP_GALLERY_POS,
	PROP_GALLERY_RESIZABLE,
	PROP_STARTUP_FLAGS
};

enum {
	SIGNAL_PREPARED,
	SIGNAL_LAST
};

static gint signals[SIGNAL_LAST];

struct _XviewerWindowPrivate {
	GSettings           *fullscreen_settings;
	GSettings           *ui_settings;
	GSettings           *view_settings;
	GSettings           *lockdown_settings;

        XviewerListStore        *store;
        XviewerImage            *image;
	XviewerWindowMode        mode;
	XviewerWindowStatus      status;

        GtkUIManager        *ui_mgr;
        GtkWidget           *overlay;
        GtkWidget           *box;
        GtkWidget           *layout;
        GtkWidget           *cbox;
        GtkWidget           *view;
        GtkWidget           *sidebar;
        GtkWidget           *thumbview;
        GtkWidget           *statusbar;
        GtkWidget           *nav;
	GtkWidget           *message_area;
	GtkWidget           *toolbar;
	GtkWidget 			*toolbar_revealer;
	GtkWidget           *properties_dlg;

        GtkActionGroup      *actions_window;
        GtkActionGroup      *actions_image;
        GtkActionGroup      *actions_gallery;
        GtkActionGroup      *actions_recent;

	GtkWidget           *fullscreen_popup;
	GSource             *fullscreen_timeout_source;

	gboolean             slideshow_loop;
	gint                 slideshow_switch_timeout;
	GSource             *slideshow_switch_source;

	guint                fullscreen_idle_inhibit_cookie;

        guint		     recent_menu_id;

        XviewerJob              *load_job;
        XviewerJob              *transform_job;
	XviewerJob              *save_job;
	GFile               *last_save_as_folder;
	XviewerJob              *copy_job;

        guint                image_info_message_cid;
        guint                tip_message_cid;
	guint                copy_file_cid;

        XviewerStartupFlags      flags;
	GSList              *file_list;

	XviewerWindowGalleryPos  gallery_position;
	gboolean             gallery_resizable;

        GtkActionGroup      *actions_open_with;
	guint                open_with_menu_id;

	gboolean	     save_disabled;
	gboolean             needs_reload_confirmation;

	GtkPageSetup        *page_setup;

	PeasExtensionSet    *extensions;

#ifdef HAVE_LCMS
        cmsHPROFILE         *display_profile;
#endif
};

G_DEFINE_TYPE_WITH_PRIVATE (XviewerWindow, xviewer_window, GTK_TYPE_APPLICATION_WINDOW);

static void xviewer_window_cmd_fullscreen (GtkAction *action, gpointer user_data);
static void xviewer_window_run_fullscreen (XviewerWindow *window, gboolean slideshow);
static void xviewer_window_cmd_save (GtkAction *action, gpointer user_data);
static void xviewer_window_cmd_save_as (GtkAction *action, gpointer user_data);
static void xviewer_window_cmd_slideshow (GtkAction *action, gpointer user_data);
static void xviewer_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data);
static void xviewer_window_stop_fullscreen (XviewerWindow *window, gboolean slideshow);
static void xviewer_job_load_cb (XviewerJobLoad *job, gpointer data);
static void xviewer_job_save_progress_cb (XviewerJobSave *job, float progress, gpointer data);
static void xviewer_job_progress_cb (XviewerJobLoad *job, float progress, gpointer data);
static void xviewer_job_transform_cb (XviewerJobTransform *job, gpointer data);
static void fullscreen_set_timeout (XviewerWindow *window);
static void fullscreen_clear_timeout (XviewerWindow *window);
static void update_action_groups_state (XviewerWindow *window);
static void open_with_launch_application_cb (GtkAction *action, gpointer callback_data);
static void xviewer_window_update_openwith_menu (XviewerWindow *window, XviewerImage *image);
static void xviewer_window_list_store_image_added (GtkTreeModel *tree_model,
					       GtkTreePath  *path,
					       GtkTreeIter  *iter,
					       gpointer      user_data);
static void xviewer_window_list_store_image_removed (GtkTreeModel *tree_model,
                 				 GtkTreePath  *path,
						 gpointer      user_data);
static void xviewer_window_set_wallpaper (XviewerWindow *window, const gchar *filename, const gchar *visible_filename);
static gboolean xviewer_window_save_images (XviewerWindow *window, GList *images);
static void xviewer_window_finish_saving (XviewerWindow *window);
static GAppInfo *get_appinfo_for_editor (XviewerWindow *window);

static GQuark
xviewer_window_error_quark (void)
{
	static GQuark q = 0;

	if (q == 0)
		q = g_quark_from_static_string ("xviewer-window-error-quark");

	return q;
}

static gboolean
_xviewer_zoom_shrink_to_boolean (GBinding *binding, const GValue *source,
			     GValue *target, gpointer user_data)
{
	XviewerZoomMode mode = g_value_get_enum (source);
	gboolean is_fit;

	is_fit = (mode == XVIEWER_ZOOM_MODE_SHRINK_TO_FIT);
	g_value_set_boolean (target, is_fit);

	return TRUE;
}

static void
xviewer_window_set_gallery_mode (XviewerWindow           *window,
			     XviewerWindowGalleryPos  position,
			     gboolean             resizable)
{
	XviewerWindowPrivate *priv;
	GtkWidget *hpaned;
	XviewerThumbNavMode mode = XVIEWER_THUMB_NAV_MODE_ONE_ROW;

	xviewer_debug (DEBUG_PREFERENCES);

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	priv = window->priv;

	if (priv->gallery_position == position &&
	    priv->gallery_resizable == resizable)
		return;

	priv->gallery_position = position;
	priv->gallery_resizable = resizable;

	hpaned = gtk_widget_get_parent (priv->sidebar);

	g_object_ref (hpaned);
	g_object_ref (priv->nav);

	gtk_container_remove (GTK_CONTAINER (priv->layout), hpaned);
	gtk_container_remove (GTK_CONTAINER (priv->layout), priv->nav);

	gtk_widget_destroy (priv->layout);

	switch (position) {
	case XVIEWER_WINDOW_GALLERY_POS_BOTTOM:
	case XVIEWER_WINDOW_GALLERY_POS_TOP:
		if (resizable) {
			mode = XVIEWER_THUMB_NAV_MODE_MULTIPLE_ROWS;

			priv->layout = gtk_paned_new (GTK_ORIENTATION_VERTICAL);

			if (position == XVIEWER_WINDOW_GALLERY_POS_BOTTOM) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			}
		} else {
			mode = XVIEWER_THUMB_NAV_MODE_ONE_ROW;

			priv->layout = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

			if (position == XVIEWER_WINDOW_GALLERY_POS_BOTTOM) {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			}
		}
		break;

	case XVIEWER_WINDOW_GALLERY_POS_LEFT:
	case XVIEWER_WINDOW_GALLERY_POS_RIGHT:
		if (resizable) {
			mode = XVIEWER_THUMB_NAV_MODE_MULTIPLE_COLUMNS;

			priv->layout = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

			if (position == XVIEWER_WINDOW_GALLERY_POS_LEFT) {
				gtk_paned_pack1 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
			} else {
				gtk_paned_pack1 (GTK_PANED (priv->layout), hpaned, TRUE, FALSE);
				gtk_paned_pack2 (GTK_PANED (priv->layout), priv->nav, FALSE, TRUE);
			}
		} else {
			mode = XVIEWER_THUMB_NAV_MODE_ONE_COLUMN;

			priv->layout = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,
						    2);

			if (position == XVIEWER_WINDOW_GALLERY_POS_LEFT) {
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
			} else {
				gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);
				gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);
			}
		}

		break;
	}

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	xviewer_thumb_nav_set_mode (XVIEWER_THUMB_NAV (priv->nav), mode);

	if (priv->mode != XVIEWER_WINDOW_MODE_UNKNOWN) {
		update_action_groups_state (window);
	}
}

static void
xviewer_window_can_save_changed_cb (GSettings   *settings,
				const gchar *key,
				gpointer     user_data)
{
	XviewerWindowPrivate *priv;
	XviewerWindow *window;
	gboolean save_disabled = FALSE;
	GtkAction *action_save, *action_save_as;

	xviewer_debug (DEBUG_PREFERENCES);

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = XVIEWER_WINDOW (user_data)->priv;

	save_disabled = g_settings_get_boolean (settings, key);

	priv->save_disabled = save_disabled;

	action_save =
		gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_save_as =
		gtk_action_group_get_action (priv->actions_image, "ImageSaveAs");

	if (priv->save_disabled) {
		gtk_action_set_sensitive (action_save, FALSE);
		gtk_action_set_sensitive (action_save_as, FALSE);
	} else {
		XviewerImage *image = xviewer_window_get_image (window);

		if (XVIEWER_IS_IMAGE (image)) {
			gtk_action_set_sensitive (action_save,
						  xviewer_image_is_modified (image));

			gtk_action_set_sensitive (action_save_as, TRUE);
		}
	}
}

#if defined(HAVE_LCMS) && defined(GDK_WINDOWING_X11)
static cmsHPROFILE *
xviewer_window_get_display_profile (GtkWidget *window)
{
	GdkScreen *screen;
	Display *dpy;
	Atom icc_atom, type;
	int format;
	gulong nitems;
	gulong bytes_after;
	gulong length;
	guchar *str;
	int result;
	cmsHPROFILE *profile = NULL;
	char *atom_name;

	screen = gtk_widget_get_screen (window);

        if (!GDK_IS_X11_SCREEN (screen))
                return NULL;

	dpy = GDK_DISPLAY_XDISPLAY (gdk_screen_get_display (screen));

	if (gdk_screen_get_number (screen) > 0)
		atom_name = g_strdup_printf ("_ICC_PROFILE_%d", gdk_screen_get_number (screen));
	else
		atom_name = g_strdup ("_ICC_PROFILE");

	icc_atom = gdk_x11_get_xatom_by_name_for_display (gdk_screen_get_display (screen), atom_name);

	g_free (atom_name);

	result = XGetWindowProperty (dpy,
				     GDK_WINDOW_XID (gdk_screen_get_root_window (screen)),
				     icc_atom,
				     0,
				     G_MAXLONG,
				     False,
				     XA_CARDINAL,
				     &type,
				     &format,
				     &nitems,
				     &bytes_after,
                                     (guchar **)&str);

	/* TODO: handle bytes_after != 0 */

	if ((result == Success) && (type == XA_CARDINAL) && (nitems > 0)) {
		switch (format)
		{
			case 8:
				length = nitems;
				break;
			case 16:
				length = sizeof(short) * nitems;
				break;
			case 32:
				length = sizeof(long) * nitems;
				break;
			default:
				xviewer_debug_message (DEBUG_LCMS, "Unable to read profile, not correcting");

				XFree (str);
				return NULL;
		}

		profile = cmsOpenProfileFromMem (str, length);

		if (G_UNLIKELY (profile == NULL)) {
			xviewer_debug_message (DEBUG_LCMS,
					   "Invalid display profile set, "
					   "not using it");
		}

		XFree (str);
	}

	if (profile == NULL) {
		profile = cmsCreate_sRGBProfile ();
		xviewer_debug_message (DEBUG_LCMS,
				 "No valid display profile set, assuming sRGB");
	}

	return profile;
}
#endif

static void
update_image_pos (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	GAction* action;
	gint pos = -1, n_images = 0;

	priv = window->priv;

	n_images = xviewer_list_store_length (XVIEWER_LIST_STORE (priv->store));

	if (n_images > 0) {
		pos = xviewer_list_store_get_pos_by_image (XVIEWER_LIST_STORE (priv->store),
						       priv->image);
	}
	/* Images: (image pos) / (n_total_images) */
	xviewer_statusbar_set_image_number (XVIEWER_STATUSBAR (priv->statusbar),
					pos + 1,
					n_images);

	action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                     "current-image");

	g_return_if_fail (action != NULL);
	g_simple_action_set_state (G_SIMPLE_ACTION (action),
	                           g_variant_new ("(ii)", pos + 1, n_images));

}

static void
update_status_bar (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	char *str = NULL;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->image != NULL &&
	    xviewer_image_has_data (priv->image, XVIEWER_IMAGE_DATA_DIMENSION)) {
		int zoom, width, height;
		goffset bytes = 0;

		zoom = floor (100 * xviewer_scroll_view_get_zoom (XVIEWER_SCROLL_VIEW (priv->view)) + 0.5);

		xviewer_image_get_size (priv->image, &width, &height);

		bytes = xviewer_image_get_bytes (priv->image);

		if ((width > 0) && (height > 0)) {
			gchar *size_string;

			size_string = g_format_size (bytes);

			/* Translators: This is the string displayed in the statusbar
			 * The tokens are from left to right:
			 * - image width
			 * - image height
			 * - image size in bytes
			 * - zoom in percent */
			str = g_strdup_printf (ngettext("%i × %i pixel  %s    %i%%",
							"%i × %i pixels  %s    %i%%", height),
						width,
						height,
						size_string,
						zoom);

			g_free (size_string);
		}

		update_image_pos (window);
	}

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, str ? str : "");

	g_free (str);
}

static void
xviewer_window_set_message_area (XviewerWindow *window,
		             GtkWidget *message_area)
{
	if (window->priv->message_area == message_area)
		return;

	if (window->priv->message_area != NULL)
		gtk_widget_destroy (window->priv->message_area);

	window->priv->message_area = message_area;

	if (message_area == NULL) return;

	gtk_box_pack_start (GTK_BOX (window->priv->cbox),
			    window->priv->message_area,
			    FALSE,
			    FALSE,
			    0);

	g_object_add_weak_pointer (G_OBJECT (window->priv->message_area),
				   (void *) &window->priv->message_area);
}

static void
update_action_groups_state (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	GtkAction *action_gallery;
	GtkAction *action_sidebar;
	GtkAction *action_fscreen;
	GtkAction *action_sshow;
	GtkAction *action_print;
	gboolean print_disabled = FALSE;
	gboolean show_image_gallery = FALSE;
	gint n_images = 0;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	action_gallery =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewImageGallery");

	action_sidebar =
		gtk_action_group_get_action (priv->actions_window,
					     "ViewSidebar");

	action_fscreen =
		gtk_action_group_get_action (priv->actions_image,
					     "ViewFullscreen");

	action_sshow =
		gtk_action_group_get_action (priv->actions_gallery,
					     "ViewSlideshow");

	action_print =
		gtk_action_group_get_action (priv->actions_image,
					     "ImagePrint");

	g_assert (action_gallery != NULL);
	g_assert (action_sidebar != NULL);
	g_assert (action_fscreen != NULL);
	g_assert (action_sshow != NULL);
	g_assert (action_print != NULL);

	if (priv->store != NULL) {
		n_images = xviewer_list_store_length (XVIEWER_LIST_STORE (priv->store));
	}

	if (n_images == 0) {
		gtk_widget_hide (priv->layout);

		gtk_action_group_set_sensitive (priv->actions_window,      TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,       FALSE);
		gtk_action_group_set_sensitive (priv->actions_gallery,  FALSE);

		gtk_action_set_sensitive (action_fscreen, FALSE);
		gtk_action_set_sensitive (action_sshow,   FALSE);

		/* If there are no images on model, initialization
 		   stops here. */
		if (priv->status == XVIEWER_WINDOW_STATUS_INIT) {
			priv->status = XVIEWER_WINDOW_STATUS_NORMAL;
		}
	} else {
		if (priv->flags & XVIEWER_STARTUP_DISABLE_GALLERY) {
			g_settings_set_boolean (priv->ui_settings,
						XVIEWER_CONF_UI_IMAGE_GALLERY,
						FALSE);

			show_image_gallery = FALSE;
		} else {
			show_image_gallery =
				g_settings_get_boolean (priv->ui_settings,
						XVIEWER_CONF_UI_IMAGE_GALLERY);
		}

		show_image_gallery = show_image_gallery &&
				     n_images > 1 &&
				     priv->mode != XVIEWER_WINDOW_MODE_SLIDESHOW;

		gtk_widget_show (priv->layout);

		if (show_image_gallery)
			gtk_widget_show (priv->nav);

		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_gallery),
					      show_image_gallery);

		gtk_action_group_set_sensitive (priv->actions_window, TRUE);
		gtk_action_group_set_sensitive (priv->actions_image,  TRUE);

		gtk_action_set_sensitive (action_fscreen, TRUE);

		if (n_images == 1) {
			gtk_action_group_set_sensitive (priv->actions_gallery,
							FALSE);
			gtk_action_set_sensitive (action_gallery, FALSE);
			gtk_action_set_sensitive (action_sshow, FALSE);
		} else {
			gtk_action_group_set_sensitive (priv->actions_gallery,
							TRUE);
			gtk_action_set_sensitive (action_sshow, TRUE);
		}

		if (show_image_gallery)
			gtk_widget_grab_focus (priv->thumbview);
		else
			gtk_widget_grab_focus (priv->view);
	}

	print_disabled = g_settings_get_boolean (priv->lockdown_settings,
						XVIEWER_CONF_DESKTOP_CAN_PRINT);

	if (print_disabled) {
		gtk_action_set_sensitive (action_print, FALSE);
	}

	if (xviewer_sidebar_is_empty (XVIEWER_SIDEBAR (priv->sidebar))) {
		gtk_action_set_sensitive (action_sidebar, FALSE);
		gtk_widget_hide (priv->sidebar);
	}
}

static void
update_selection_ui_visibility (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	GtkAction *wallpaper_action;
	gint n_selected;

	priv = window->priv;

	n_selected = xviewer_thumb_view_get_n_selected (XVIEWER_THUMB_VIEW (priv->thumbview));

	wallpaper_action =
		gtk_action_group_get_action (priv->actions_image,
					     "ImageSetAsWallpaper");

	if (n_selected == 1) {
		gtk_action_set_sensitive (wallpaper_action, TRUE);
	} else {
		gtk_action_set_sensitive (wallpaper_action, FALSE);
	}
}

static gboolean
add_file_to_recent_files (GFile *file)
{
	gchar *text_uri;
	GFileInfo *file_info;
	GtkRecentData *recent_data;
	static gchar *groups[2] = { XVIEWER_RECENT_FILES_GROUP , NULL };

	if (file == NULL) return FALSE;

	/* The password gets stripped here because ~/.recently-used.xbel is
	 * readable by everyone (chmod 644). It also makes the workaround
	 * for the bug with gtk_recent_info_get_uri_display() easier
	 * (see the comment in xviewer_window_update_recent_files_menu()). */
	text_uri = g_file_get_uri (file);

	if (text_uri == NULL)
		return FALSE;

	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);
	if (file_info == NULL)
		return FALSE;

	recent_data = g_slice_new (GtkRecentData);
	recent_data->display_name = NULL;
	recent_data->description = NULL;
	recent_data->mime_type = (gchar *) g_file_info_get_content_type (file_info);
	recent_data->app_name = XVIEWER_RECENT_FILES_APP_NAME;
	recent_data->app_exec = g_strjoin(" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;
	recent_data->is_private = FALSE;

	gtk_recent_manager_add_full (gtk_recent_manager_get_default (),
				     text_uri,
				     recent_data);

	g_free (recent_data->app_exec);
	g_free (text_uri);
	g_object_unref (file_info);

	g_slice_free (GtkRecentData, recent_data);

	return FALSE;
}

static void
image_thumb_changed_cb (XviewerImage *image, gpointer data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	GdkPixbuf *thumb;

	g_return_if_fail (XVIEWER_IS_WINDOW (data));

	window = XVIEWER_WINDOW (data);
	priv = window->priv;

	thumb = xviewer_image_get_thumbnail (image);

	if (thumb != NULL) {
		gtk_window_set_icon (GTK_WINDOW (window), thumb);

		if (window->priv->properties_dlg != NULL) {
			xviewer_properties_dialog_update (XVIEWER_PROPERTIES_DIALOG (priv->properties_dlg),
						      image);
		}

		g_object_unref (thumb);
	} else if (!gtk_widget_get_visible (window->priv->nav)) {
		gint img_pos = xviewer_list_store_get_pos_by_image (window->priv->store, image);
		GtkTreePath *path = gtk_tree_path_new_from_indices (img_pos,-1);
		GtkTreeIter iter;

		gtk_tree_model_get_iter (GTK_TREE_MODEL (window->priv->store), &iter, path);
		xviewer_list_store_thumbnail_set (window->priv->store, &iter);
		gtk_tree_path_free (path);
	}
}

static gboolean
on_button_pressed (GtkWidget *widget, GdkEventButton *event, XviewerWindow *window)
{
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		XviewerWindowMode mode = xviewer_window_get_mode (window);
		GdkEvent *ev = (GdkEvent*) event;

		if (!gtk_widget_get_realized (GTK_WIDGET (window->priv->view))) {
			return FALSE;
		}

		if (!xviewer_scroll_view_event_is_over_image (window->priv->view, ev)) {
			return FALSE;
		}

		if (mode == XVIEWER_WINDOW_MODE_SLIDESHOW || mode == XVIEWER_WINDOW_MODE_FULLSCREEN) {
			xviewer_window_set_mode (window, XVIEWER_WINDOW_MODE_NORMAL);
		}
		else if (mode == XVIEWER_WINDOW_MODE_NORMAL) {
			xviewer_window_set_mode (window, XVIEWER_WINDOW_MODE_FULLSCREEN);
		}

		return TRUE;
	}

	return FALSE;
}

static void
file_changed_info_bar_response (GtkInfoBar *info_bar,
				gint response,
				XviewerWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		xviewer_window_reload_image (window);
	}

	window->priv->needs_reload_confirmation = TRUE;

	xviewer_window_set_message_area (window, NULL);
}

static void
image_file_changed_cb (XviewerImage *img, XviewerWindow *window)
{
	GtkWidget *info_bar;
	gchar *text, *markup;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;

	if (window->priv->needs_reload_confirmation == FALSE)
		return;

	if (!xviewer_image_is_modified (img)) {
		/* Auto-reload when image is unmodified (bug #555370) */
		xviewer_window_reload_image (window);
		return;
	}

	window->priv->needs_reload_confirmation = FALSE;

	info_bar = gtk_info_bar_new_with_buttons (_("_Reload"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea", "Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);
	image = gtk_image_new_from_icon_name ("dialog-question",
					      GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been modified by an external application."
				  "\nWould you like to reload it?"), xviewer_image_get_caption (img));
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (text);
	g_free (markup);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_widget_set_valign (image, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);

	xviewer_window_set_message_area (window, info_bar);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (file_changed_info_bar_response), window);
}

static void
xviewer_window_display_image (XviewerWindow *window, XviewerImage *image)
{
	XviewerWindowPrivate *priv;
	GFile *file;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));
	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	xviewer_debug (DEBUG_WINDOW);

	g_assert (xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_IMAGE));

	priv = window->priv;

	if (image != NULL) {
		g_signal_connect (image,
				  "thumbnail_changed",
				  G_CALLBACK (image_thumb_changed_cb),
				  window);
		g_signal_connect (image, "file-changed",
				  G_CALLBACK (image_file_changed_cb),
				  window);

		image_thumb_changed_cb (image, window);
	}

	priv->needs_reload_confirmation = TRUE;

	xviewer_scroll_view_set_image (XVIEWER_SCROLL_VIEW (priv->view), image);

	gtk_window_set_title (GTK_WINDOW (window), xviewer_image_get_caption (image));

	update_status_bar (window);

	file = xviewer_image_get_file (image);
	g_idle_add_full (G_PRIORITY_LOW,
			 (GSourceFunc) add_file_to_recent_files,
			 file,
			 (GDestroyNotify) g_object_unref);

	xviewer_window_update_openwith_menu (window, image);
}

static void
open_with_launch_application_cb (GtkAction *action, gpointer data) {
	XviewerImage *image;
	GAppInfo *app;
	GFile *file;
	GList *files = NULL;

	image = XVIEWER_IMAGE (data);
	file = xviewer_image_get_file (image);

	app = g_object_get_data (G_OBJECT (action), "app");
	files = g_list_append (files, file);
	g_app_info_launch (app,
			   files,
			   NULL, NULL);

	g_object_unref (file);
	g_list_free (files);
}

static void
xviewer_window_update_openwith_menu (XviewerWindow *window, XviewerImage *image)
{
	gboolean edit_button_active;
	GAppInfo *editor_app;
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	gchar *label, *tip;
	const gchar *mime_type;
	GtkAction *action;
	XviewerWindowPrivate *priv;
        GList *apps;
        guint action_id = 0;
        GIcon *app_icon;
        char *path;
        GtkWidget *menuitem;

	priv = window->priv;

	edit_button_active = FALSE;
	editor_app = get_appinfo_for_editor (window);

	file = xviewer_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
				       0, NULL, NULL);

	if (file_info == NULL)
		return;
	else {
		mime_type = g_file_info_get_content_type (file_info);
	}

        if (priv->open_with_menu_id != 0) {
               gtk_ui_manager_remove_ui (priv->ui_mgr, priv->open_with_menu_id);
               priv->open_with_menu_id = 0;
        }

        if (priv->actions_open_with != NULL) {
              gtk_ui_manager_remove_action_group (priv->ui_mgr, priv->actions_open_with);
              priv->actions_open_with = NULL;
        }

        if (mime_type == NULL) {
                g_object_unref (file_info);
                return;
	}

        apps = g_app_info_get_all_for_type (mime_type);

	g_object_unref (file_info);

        if (!apps)
                return;

        priv->actions_open_with = gtk_action_group_new ("OpenWithActions");
        gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_open_with, -1);

        priv->open_with_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);

        for (iter = apps; iter; iter = iter->next) {
                GAppInfo *app = iter->data;
                gchar name[64];

                if (editor_app != NULL && g_app_info_equal (editor_app, app)) {
                        edit_button_active = TRUE;
                }

                /* Do not include xviewer itself */
                if (g_ascii_strcasecmp (g_app_info_get_executable (app),
                                        g_get_prgname ()) == 0) {
                        g_object_unref (app);
                        continue;
                }

                g_snprintf (name, sizeof (name), "OpenWith%u", action_id++);

                label = g_strdup (g_app_info_get_name (app));
                tip = g_strdup_printf (_("Use \"%s\" to open the selected image"), g_app_info_get_name (app));

                action = gtk_action_new (name, label, tip, NULL);

		app_icon = g_app_info_get_icon (app);
		if (G_LIKELY (app_icon != NULL)) {
			g_object_ref (app_icon);
                	gtk_action_set_gicon (action, app_icon);
                	g_object_unref (app_icon);
		}

                g_free (label);
                g_free (tip);

                g_object_set_data_full (G_OBJECT (action), "app", app,
                                        (GDestroyNotify) g_object_unref);

                g_signal_connect (action,
                                  "activate",
                                  G_CALLBACK (open_with_launch_application_cb),
                                  image);

                gtk_action_group_add_action (priv->actions_open_with, action);
                g_object_unref (action);

                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/MainMenu/Image/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);

                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/ThumbnailPopup/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);
                gtk_ui_manager_add_ui (priv->ui_mgr,
                                priv->open_with_menu_id,
                                "/ViewPopup/ImageOpenWith/Applications Placeholder",
                                name,
                                name,
                                GTK_UI_MANAGER_MENUITEM,
                                FALSE);

                path = g_strdup_printf ("/MainMenu/Image/ImageOpenWith/Applications Placeholder/%s", name);

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);

                path = g_strdup_printf ("/ThumbnailPopup/ImageOpenWith/Applications Placeholder/%s", name);

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);

                path = g_strdup_printf ("/ViewPopup/ImageOpenWith/Applications Placeholder/%s", name);

                menuitem = gtk_ui_manager_get_widget (priv->ui_mgr, path);

                /* Only force displaying the icon if it is an application icon */
                gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (menuitem), app_icon != NULL);

                g_free (path);
        }

        g_list_free (apps);

        action = gtk_action_group_get_action (window->priv->actions_image,
                                              "OpenEditor");
        if (action != NULL) {
                gtk_action_set_sensitive (action, edit_button_active);
        }
}

static void
xviewer_window_clear_load_job (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;

	if (priv->load_job != NULL) {
		if (!priv->load_job->finished)
			xviewer_job_cancel (priv->load_job);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      xviewer_job_progress_cb,
						      window);

		g_signal_handlers_disconnect_by_func (priv->load_job,
						      xviewer_job_load_cb,
						      window);

		xviewer_image_cancel_load (XVIEWER_JOB_LOAD (priv->load_job)->image);

		g_object_unref (priv->load_job);
		priv->load_job = NULL;

		/* Hide statusbar */
		xviewer_statusbar_set_progress (XVIEWER_STATUSBAR (priv->statusbar), 0);
	}
}

static void
xviewer_job_progress_cb (XviewerJobLoad *job, float progress, gpointer user_data)
{
	XviewerWindow *window;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);

	xviewer_statusbar_set_progress (XVIEWER_STATUSBAR (window->priv->statusbar),
				    progress);
}

static void
xviewer_job_save_progress_cb (XviewerJobSave *job, float progress, gpointer user_data)
{
	XviewerWindowPrivate *priv;
	XviewerWindow *window;

	static XviewerImage *image = NULL;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	xviewer_statusbar_set_progress (XVIEWER_STATUSBAR (priv->statusbar),
				    progress);

	if (image != job->current_image) {
		gchar *str_image, *status_message;
		guint n_images;

		image = job->current_image;

		n_images = g_list_length (job->images);

		str_image = xviewer_image_get_uri_for_display (image);

		/* Translators: This string is displayed in the statusbar
		 * while saving images. The tokens are from left to right:
		 * - the original filename
		 * - the current image's position in the queue
		 * - the total number of images queued for saving */
		status_message = g_strdup_printf (_("Saving image \"%s\" (%u/%u)"),
					          str_image,
						  job->current_position + 1,
						  n_images);
		g_free (str_image);

		gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
				   priv->image_info_message_cid);

		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->image_info_message_cid,
				    status_message);

		g_free (status_message);
	}

	if (progress == 1.0)
		image = NULL;
}

static void
xviewer_window_obtain_desired_size (XviewerImage  *image,
				gint       width,
				gint       height,
				XviewerWindow *window)
{
	GdkScreen *screen;
	GdkRectangle monitor;
	GtkAllocation allocation;
	gint final_width, final_height;
	gint screen_width, screen_height;
	gint window_width, window_height;
	gint img_width, img_height;
	gint view_width, view_height;
	gint deco_width, deco_height;

	update_action_groups_state (window);

	img_width = width;
	img_height = height;

	if (!gtk_widget_get_realized (window->priv->view)) {
		gtk_widget_realize (window->priv->view);
	}

	xviewer_debug_message (DEBUG_WINDOW, "Initial Image Size: %d x %d", img_width, img_height);

	gtk_widget_get_allocation (window->priv->view, &allocation);
	view_width  = allocation.width;
	view_height = allocation.height;

	xviewer_debug_message (DEBUG_WINDOW, "Initial View Size: %d x %d", view_width, view_height);


	if (!gtk_widget_get_realized (GTK_WIDGET (window))) {
		gtk_widget_realize (GTK_WIDGET (window));
	}

	gtk_widget_get_allocation (GTK_WIDGET (window), &allocation);
	window_width  = allocation.width;
	window_height = allocation.height;

	xviewer_debug_message (DEBUG_WINDOW, "Initial Window Size: %d x %d", window_width, window_height);


	screen = gtk_window_get_screen (GTK_WINDOW (window));

	gdk_screen_get_monitor_geometry (screen,
			gdk_screen_get_monitor_at_window (screen,
				gtk_widget_get_window (GTK_WIDGET (window))),
			&monitor);

	screen_width  = monitor.width;
	screen_height = monitor.height;

	xviewer_debug_message (DEBUG_WINDOW, "Screen Size: %d x %d", screen_width, screen_height);

	deco_width = window_width - view_width;
	deco_height = window_height - view_height;

	xviewer_debug_message (DEBUG_WINDOW, "Decoration Size: %d x %d", deco_width, deco_height);

	if (img_width > 0 && img_height > 0) {
		if ((img_width + deco_width > screen_width) ||
		    (img_height + deco_height > screen_height))
		{
			double width_factor, height_factor, factor;

			width_factor = (screen_width * 0.85 - deco_width) / (double) img_width;
			height_factor = (screen_height * 0.85 - deco_height) / (double) img_height;
			factor = MIN (width_factor, height_factor);

			xviewer_debug_message (DEBUG_WINDOW, "Scaling Factor: %.2lf", factor);


			img_width = img_width * factor;
			img_height = img_height * factor;
		}
	}

	final_width = MAX (XVIEWER_WINDOW_MIN_WIDTH, img_width + deco_width);
	final_height = MAX (XVIEWER_WINDOW_MIN_HEIGHT, img_height + deco_height);

	xviewer_debug_message (DEBUG_WINDOW, "Setting window size: %d x %d", final_width, final_height);

	gtk_window_set_default_size (GTK_WINDOW (window), final_width, final_height);

	g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
}

static void
xviewer_window_error_message_area_response (GtkInfoBar       *message_area,
					gint              response_id,
					XviewerWindow        *window)
{
	GtkAction *action_save_as;

	g_return_if_fail (GTK_IS_INFO_BAR (message_area));
	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	/* remove message area */
	xviewer_window_set_message_area (window, NULL);

	/* evaluate message area response */
	switch (response_id) {
	case XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_NONE:
	case XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_CANCEL:
		/* nothing to do in this case */
		break;
	case XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_RELOAD:
		/* TODO: trigger loading for current image again */
		break;
	case XVIEWER_ERROR_MESSAGE_AREA_RESPONSE_SAVEAS:
		/* trigger save as command for current image */
		action_save_as = gtk_action_group_get_action (window->priv->actions_image,
							      "ImageSaveAs");
		xviewer_window_cmd_save_as (action_save_as, window);
		break;
	}
}

static void
xviewer_job_load_cb (XviewerJobLoad *job, gpointer data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	GtkAction *action_undo, *action_save;

        g_return_if_fail (XVIEWER_IS_WINDOW (data));

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (data);
	priv = window->priv;

	xviewer_statusbar_set_progress (XVIEWER_STATUSBAR (priv->statusbar), 0.0);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   priv->image_info_message_cid);

	if (priv->image != NULL) {
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);

		g_object_unref (priv->image);
	}

	priv->image = g_object_ref (job->image);

	if (XVIEWER_JOB (job)->error == NULL) {
#ifdef HAVE_LCMS
		xviewer_image_apply_display_profile (job->image,
						 priv->display_profile);
#endif

		gtk_action_group_set_sensitive (priv->actions_image, TRUE);

		/* Make sure the window is really realized
		 *  before displaying the image. The ScrollView needs that.  */
        	if (!gtk_widget_get_realized (GTK_WIDGET (window))) {
			gint width = -1, height = -1;

			xviewer_image_get_size (job->image, &width, &height);
			xviewer_window_obtain_desired_size (job->image, width,
			                                height, window);

		}

		xviewer_window_display_image (window, job->image);
	} else {
		GtkWidget *message_area;

		message_area = xviewer_image_load_error_message_area_new (
					xviewer_image_get_caption (job->image),
					XVIEWER_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (xviewer_window_error_message_area_response),
				  window);

		gtk_window_set_icon (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW (window),
				      xviewer_image_get_caption (job->image));

		xviewer_window_set_message_area (window, message_area);

		gtk_info_bar_set_default_response (GTK_INFO_BAR (message_area),
						   GTK_RESPONSE_CANCEL);

		gtk_widget_show (message_area);

		update_status_bar (window);

		xviewer_scroll_view_set_image (XVIEWER_SCROLL_VIEW (priv->view), NULL);

        	if (window->priv->status == XVIEWER_WINDOW_STATUS_INIT) {
			update_action_groups_state (window);

			g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
		}

		gtk_action_group_set_sensitive (priv->actions_image, FALSE);
	}

	xviewer_window_clear_load_job (window);

        if (window->priv->status == XVIEWER_WINDOW_STATUS_INIT) {
		window->priv->status = XVIEWER_WINDOW_STATUS_NORMAL;

		g_signal_handlers_disconnect_by_func
			(job->image,
			 G_CALLBACK (xviewer_window_obtain_desired_size),
			 window);
	}

	action_save = gtk_action_group_get_action (priv->actions_image, "ImageSave");
	action_undo = gtk_action_group_get_action (priv->actions_image, "EditUndo");

	/* Set Save and Undo sensitive according to image state.
	 * Respect lockdown in case of Save.*/
	gtk_action_set_sensitive (action_save, (!priv->save_disabled && xviewer_image_is_modified (job->image)));
	gtk_action_set_sensitive (action_undo, xviewer_image_is_modified (job->image));

	g_object_unref (job->image);
}

static void
xviewer_window_clear_transform_job (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;

	if (priv->transform_job != NULL) {
		if (!priv->transform_job->finished)
			xviewer_job_cancel (priv->transform_job);

		g_signal_handlers_disconnect_by_func (priv->transform_job,
						      xviewer_job_transform_cb,
						      window);
		g_object_unref (priv->transform_job);
		priv->transform_job = NULL;
	}
}

static void
xviewer_job_transform_cb (XviewerJobTransform *job, gpointer data)
{
	XviewerWindow *window;
	GtkAction *action_undo, *action_save;
	XviewerImage *image;

        g_return_if_fail (XVIEWER_IS_WINDOW (data));

	window = XVIEWER_WINDOW (data);

	xviewer_window_clear_transform_job (window);

	action_undo =
		gtk_action_group_get_action (window->priv->actions_image, "EditUndo");
	action_save =
		gtk_action_group_get_action (window->priv->actions_image, "ImageSave");

	image = xviewer_window_get_image (window);

	gtk_action_set_sensitive (action_undo, xviewer_image_is_modified (image));

	if (!window->priv->save_disabled)
	{
		gtk_action_set_sensitive (action_save, xviewer_image_is_modified (image));
	}
}

static void
apply_transformation (XviewerWindow *window, XviewerTransform *trans)
{
	XviewerWindowPrivate *priv;
	GList *images;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	priv = window->priv;

	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (priv->thumbview));

	xviewer_window_clear_transform_job (window);

	priv->transform_job = xviewer_job_transform_new (images, trans);

	g_signal_connect (priv->transform_job,
			  "finished",
			  G_CALLBACK (xviewer_job_transform_cb),
			  window);

	g_signal_connect (priv->transform_job,
			  "progress",
			  G_CALLBACK (xviewer_job_progress_cb),
			  window);

	xviewer_job_scheduler_add_job (priv->transform_job);
}

static void
handle_image_selection_changed_cb (XviewerThumbView *thumbview, XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	XviewerImage *image;
	gchar *status_message;
	gchar *str_image;

	priv = window->priv;

	if (xviewer_list_store_length (XVIEWER_LIST_STORE (priv->store)) == 0) {
		gtk_window_set_title (GTK_WINDOW (window),
				      g_get_application_name());
		gtk_statusbar_remove_all (GTK_STATUSBAR (priv->statusbar),
					  priv->image_info_message_cid);
		xviewer_scroll_view_set_image (XVIEWER_SCROLL_VIEW (priv->view),
					   NULL);
	}
	if (xviewer_thumb_view_get_n_selected (XVIEWER_THUMB_VIEW (priv->thumbview)) == 0)
		return;

	update_selection_ui_visibility (window);

	image = xviewer_thumb_view_get_first_selected_image (XVIEWER_THUMB_VIEW (priv->thumbview));

	g_assert (XVIEWER_IS_IMAGE (image));

	xviewer_window_clear_load_job (window);

	xviewer_window_set_message_area (window, NULL);

	gtk_statusbar_pop (GTK_STATUSBAR (priv->statusbar),
			   priv->image_info_message_cid);

	if (image == priv->image) {
		update_status_bar (window);
		return;
	}

	if (xviewer_image_has_data (image, XVIEWER_IMAGE_DATA_IMAGE)) {
		if (priv->image != NULL)
			g_object_unref (priv->image);
		priv->image = image;
		xviewer_window_display_image (window, image);
		return;
	}

	if (priv->status == XVIEWER_WINDOW_STATUS_INIT) {
		g_signal_connect (image,
				  "size-prepared",
				  G_CALLBACK (xviewer_window_obtain_desired_size),
				  window);
	}

	priv->load_job = xviewer_job_load_new (image, XVIEWER_IMAGE_DATA_ALL);

	g_signal_connect (priv->load_job,
			  "finished",
			  G_CALLBACK (xviewer_job_load_cb),
			  window);

	g_signal_connect (priv->load_job,
			  "progress",
			  G_CALLBACK (xviewer_job_progress_cb),
			  window);

	xviewer_job_scheduler_add_job (priv->load_job);

	str_image = xviewer_image_get_uri_for_display (image);

	status_message = g_strdup_printf (_("Opening image \"%s\""),
				          str_image);

	g_free (str_image);

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->image_info_message_cid, status_message);

	g_free (status_message);
}

static void
view_zoom_changed_cb (GtkWidget *widget, double zoom, gpointer user_data)
{
	XviewerWindow *window;
	GtkAction *action_zoom_in;
	GtkAction *action_zoom_out;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);

	update_status_bar (window);

	action_zoom_in =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomIn");

	action_zoom_out =
		gtk_action_group_get_action (window->priv->actions_image,
					     "ViewZoomOut");

	gtk_action_set_sensitive (action_zoom_in,
			!xviewer_scroll_view_get_zoom_is_max (XVIEWER_SCROLL_VIEW (window->priv->view)));
	gtk_action_set_sensitive (action_zoom_out,
			!xviewer_scroll_view_get_zoom_is_min (XVIEWER_SCROLL_VIEW (window->priv->view)));
}

static void
xviewer_window_open_recent_cb (GtkAction *action, XviewerWindow *window)
{
	GtkRecentInfo *info;
	const gchar *uri;
	GSList *list = NULL;

	info = g_object_get_data (G_OBJECT (action), "gtk-recent-info");
	g_return_if_fail (info != NULL);

	uri = gtk_recent_info_get_uri (info);
	list = g_slist_prepend (list, g_strdup (uri));

	xviewer_application_open_uri_list (XVIEWER_APP,
				       list,
				       GDK_CURRENT_TIME,
				       0,
				       NULL);

	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}

static void
file_open_dialog_response_cb (GtkWidget *chooser,
			      gint       response_id,
			      XviewerWindow  *ev_window)
{
	if (response_id == GTK_RESPONSE_OK) {
		GSList *uris;

		uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (chooser));

		xviewer_application_open_uri_list (XVIEWER_APP,
					       uris,
					       GDK_CURRENT_TIME,
					       0,
					       NULL);

		g_slist_foreach (uris, (GFunc) g_free, NULL);
		g_slist_free (uris);
	}

	gtk_widget_destroy (chooser);
}

static void
xviewer_window_update_fullscreen_action (XviewerWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ViewFullscreen");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (xviewer_window_cmd_fullscreen), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == XVIEWER_WINDOW_MODE_FULLSCREEN);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (xviewer_window_cmd_fullscreen), window);
}

static void
xviewer_window_update_slideshow_action (XviewerWindow *window)
{
	GtkAction *action;

	action = gtk_action_group_get_action (window->priv->actions_gallery,
					      "ViewSlideshow");

	g_signal_handlers_block_by_func
		(action, G_CALLBACK (xviewer_window_cmd_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action),
				      window->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW);

	g_signal_handlers_unblock_by_func
		(action, G_CALLBACK (xviewer_window_cmd_slideshow), window);
}

static void
xviewer_window_update_pause_slideshow_action (XviewerWindow *window)
{
	GtkAction *action;
    gboolean active;

    active = (window->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW);

	action = gtk_action_group_get_action (window->priv->actions_image, "PauseSlideshow");

	g_signal_handlers_block_by_func (action, G_CALLBACK (xviewer_window_cmd_pause_slideshow), window);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

    if (active)
    {
        gtk_action_set_icon_name (action, "media-playback-pause-symbolic");
    }
    else
    {
        gtk_action_set_icon_name (action, "media-playback-start-symbolic");
    }

	g_signal_handlers_unblock_by_func (action, G_CALLBACK (xviewer_window_cmd_pause_slideshow), window);
}

static gboolean
fullscreen_timeout_cb (gpointer data)
{
	XviewerWindow *window = XVIEWER_WINDOW (data);

	xviewer_debug (DEBUG_WINDOW);

	gtk_revealer_set_reveal_child (
		    GTK_REVEALER (window->priv->fullscreen_popup), FALSE);
	xviewer_scroll_view_hide_cursor (XVIEWER_SCROLL_VIEW (window->priv->view));

	fullscreen_clear_timeout (window);

	return FALSE;
}

static gboolean
slideshow_is_loop_end (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;
	XviewerImage *image = NULL;
	gint pos;

	image = xviewer_thumb_view_get_first_selected_image (XVIEWER_THUMB_VIEW (priv->thumbview));

	pos = xviewer_list_store_get_pos_by_image (priv->store, image);

	return (pos == (xviewer_list_store_length (priv->store) - 1));
}

static gboolean
slideshow_switch_cb (gpointer data)
{
	XviewerWindow *window = XVIEWER_WINDOW (data);
	XviewerWindowPrivate *priv = window->priv;

	xviewer_debug (DEBUG_WINDOW);

	if (!priv->slideshow_loop && slideshow_is_loop_end (window)) {
		xviewer_window_stop_fullscreen (window, TRUE);
		return FALSE;
	}

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_RIGHT);

	return TRUE;
}

static void
fullscreen_clear_timeout (XviewerWindow *window)
{
	xviewer_debug (DEBUG_WINDOW);

	if (window->priv->fullscreen_timeout_source != NULL) {
		g_source_unref (window->priv->fullscreen_timeout_source);
		g_source_destroy (window->priv->fullscreen_timeout_source);
	}

	window->priv->fullscreen_timeout_source = NULL;
}

static void
fullscreen_set_timeout (XviewerWindow *window)
{
	GSource *source;

	xviewer_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	source = g_timeout_source_new (XVIEWER_WINDOW_FULLSCREEN_TIMEOUT);
	g_source_set_callback (source, fullscreen_timeout_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->fullscreen_timeout_source = source;

	xviewer_scroll_view_show_cursor (XVIEWER_SCROLL_VIEW (window->priv->view));
}

static void
slideshow_clear_timeout (XviewerWindow *window)
{
	xviewer_debug (DEBUG_WINDOW);

	if (window->priv->slideshow_switch_source != NULL) {
		g_source_unref (window->priv->slideshow_switch_source);
		g_source_destroy (window->priv->slideshow_switch_source);
	}

	window->priv->slideshow_switch_source = NULL;
}

static void
slideshow_set_timeout (XviewerWindow *window)
{
	GSource *source;

	xviewer_debug (DEBUG_WINDOW);

	slideshow_clear_timeout (window);

	if (window->priv->slideshow_switch_timeout <= 0)
		return;

	source = g_timeout_source_new (window->priv->slideshow_switch_timeout * 1000);
	g_source_set_callback (source, slideshow_switch_cb, window, NULL);

	g_source_attach (source, NULL);

	window->priv->slideshow_switch_source = source;
}

static void
show_fullscreen_popup (XviewerWindow *window)
{
	xviewer_debug (DEBUG_WINDOW);

	if (!gtk_widget_get_visible (window->priv->fullscreen_popup)) {
		gtk_widget_show_all (GTK_WIDGET (window->priv->fullscreen_popup));
	}

	gtk_revealer_set_reveal_child (
		    GTK_REVEALER (window->priv->fullscreen_popup), TRUE);

	fullscreen_set_timeout (window);
}

static gboolean
fullscreen_motion_notify_cb (GtkWidget      *widget,
			     GdkEventMotion *event,
			     gpointer       user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);

	xviewer_debug (DEBUG_WINDOW);

	if (event->y < XVIEWER_WINDOW_FULLSCREEN_POPUP_THRESHOLD) {
		show_fullscreen_popup (window);
	} else {
		fullscreen_set_timeout (window);
	}

	return FALSE;
}

static gboolean
fullscreen_leave_notify_cb (GtkWidget *widget,
			    GdkEventCrossing *event,
			    gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);

	xviewer_debug (DEBUG_WINDOW);

	fullscreen_clear_timeout (window);

	return FALSE;
}

static void
exit_fullscreen_button_clicked_cb (GtkWidget *button, XviewerWindow *window)
{
	GtkAction *action;

	xviewer_debug (DEBUG_WINDOW);

	if (window->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW) {
		action = gtk_action_group_get_action (window->priv->actions_gallery,
						      "ViewSlideshow");
	} else {
		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ViewFullscreen");
	}
	g_return_if_fail (action != NULL);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), FALSE);
}

static GtkWidget *
xviewer_window_get_exit_fullscreen_button (XviewerWindow *window)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_button_new ();
	image = gtk_image_new_from_icon_name ("view-restore-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "flat");
	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_button_set_label (GTK_BUTTON (button), NULL);

	g_signal_connect (button, "clicked",
			  G_CALLBACK (exit_fullscreen_button_clicked_cb),
			  window);

	return button;
}

static GtkWidget *
create_toolbar_button (GtkAction *action)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_button_new ();
	image = gtk_image_new ();

	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "flat");
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
	gtk_button_set_label (GTK_BUTTON (button), NULL);
	gtk_widget_set_tooltip_text (button, gtk_action_get_tooltip (action));

	return button;
}

static GtkWidget *
create_toolbar_toggle_button (GtkAction *action)
{
	GtkWidget *button;
	GtkWidget *image;

	button = gtk_toggle_button_new ();
	image = gtk_image_new ();

	gtk_button_set_image (GTK_BUTTON (button), image);
	gtk_style_context_add_class (gtk_widget_get_style_context (button), "flat");
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (button), action);
	gtk_button_set_label (GTK_BUTTON (button), NULL);
	gtk_widget_set_tooltip_text (button, gtk_action_get_tooltip (action));

	return button;
}

static GtkWidget *
xviewer_window_create_fullscreen_popup (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	GtkWidget *revealer;
	GtkWidget *main_box;
	GtkWidget *box;
	GtkWidget *button;
	GtkWidget *separator;
	GtkWidget *toolbar;
	GtkWidget *tool_item;
	GtkAction *action;
	GtkWidget *frame;

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	revealer = gtk_revealer_new();
	gtk_widget_add_events (revealer, GDK_ENTER_NOTIFY_MASK);

	frame = gtk_frame_new (NULL);
	gtk_container_add (GTK_CONTAINER (revealer), frame);

	toolbar = gtk_toolbar_new ();
	gtk_container_add (GTK_CONTAINER (frame), toolbar);
	tool_item = gtk_tool_item_new ();
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar), GTK_TOOL_ITEM (tool_item), 0);

	main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_hexpand (main_box, TRUE);
	gtk_widget_set_valign (revealer, GTK_ALIGN_START);
	gtk_widget_set_halign (revealer, GTK_ALIGN_FILL);
	gtk_container_add (GTK_CONTAINER (tool_item), main_box);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoFirst");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoPrevious");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoNext");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoLast");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomOut");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomIn");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomNormal");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomFit");
	button = create_toolbar_toggle_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "EditRotate270");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "EditRotate90");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_window, "ViewImageGallery");
	button = create_toolbar_toggle_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (main_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (main_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "PauseSlideshow");
	button = create_toolbar_toggle_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	button = xviewer_window_get_exit_fullscreen_button (window);
	gtk_box_pack_end (GTK_BOX (main_box), button, FALSE, FALSE, 0);

	/* Disable timer when the pointer enters the toolbar window. */
	g_signal_connect (revealer,
			  "enter-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	return revealer;
}

static void
update_ui_visibility (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;

	GtkAction *action;
	GtkWidget *menubar;

	gboolean fullscreen_mode, visible;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	fullscreen_mode = priv->mode == XVIEWER_WINDOW_MODE_FULLSCREEN ||
			  priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW;

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));

	visible = g_settings_get_boolean (priv->ui_settings,
					  XVIEWER_CONF_UI_TOOLBAR);
	visible = visible && !fullscreen_mode;
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ToolbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->toolbar_revealer), "reveal-child", visible, NULL);

	visible = g_settings_get_boolean (priv->ui_settings,
					  XVIEWER_CONF_UI_STATUSBAR);
	visible = visible && !fullscreen_mode;
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/StatusbarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

	if (priv->status != XVIEWER_WINDOW_STATUS_INIT) {
		visible = g_settings_get_boolean (priv->ui_settings,
						  XVIEWER_CONF_UI_IMAGE_GALLERY);
		visible = visible && priv->mode != XVIEWER_WINDOW_MODE_SLIDESHOW;
		action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/ImageGalleryToggle");
		g_assert (action != NULL);
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
		if (visible) {
			gtk_widget_show (priv->nav);
		} else {
			gtk_widget_hide (priv->nav);
		}
	}

	visible = g_settings_get_boolean (priv->ui_settings,
					  XVIEWER_CONF_UI_SIDEBAR);
    visible = visible && priv->mode != XVIEWER_WINDOW_MODE_SLIDESHOW;
	action = gtk_ui_manager_get_action (priv->ui_mgr, "/MainMenu/View/SidebarToggle");
	g_assert (action != NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
	if (visible) {
		gtk_widget_show (priv->sidebar);
	} else {
		gtk_widget_hide (priv->sidebar);
	}

	if (priv->fullscreen_popup != NULL) {
		gtk_widget_hide (priv->fullscreen_popup);
	}
}

static void
xviewer_window_inhibit_screensaver (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;

	g_return_if_fail (priv->fullscreen_idle_inhibit_cookie == 0);

	xviewer_debug (DEBUG_WINDOW);

	window->priv->fullscreen_idle_inhibit_cookie =
		gtk_application_inhibit (GTK_APPLICATION (XVIEWER_APP),
		                         GTK_WINDOW (window),
		                         GTK_APPLICATION_INHIBIT_IDLE,
	/* L10N: This the reason why the screensaver is inhibited. */
		                         _("Viewing a slideshow"));
}

static void
xviewer_window_uninhibit_screensaver (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;

	if (G_UNLIKELY (priv->fullscreen_idle_inhibit_cookie == 0))
		return;

	xviewer_debug (DEBUG_WINDOW);

	gtk_application_uninhibit (GTK_APPLICATION (XVIEWER_APP),
	                           priv->fullscreen_idle_inhibit_cookie);
	priv->fullscreen_idle_inhibit_cookie = 0;
}

static void
xviewer_window_run_fullscreen (XviewerWindow *window, gboolean slideshow)
{
	static const GdkRGBA black = { 0., 0., 0., 1.};
	XviewerWindowPrivate *priv;
	GtkWidget *menubar;
	gboolean upscale;

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (slideshow) {
		priv->mode = XVIEWER_WINDOW_MODE_SLIDESHOW;
	} else {
		/* Stop the timer if we come from slideshowing */
		if (priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW)
			slideshow_clear_timeout (window);

		priv->mode = XVIEWER_WINDOW_MODE_FULLSCREEN;
	}

	if (window->priv->fullscreen_popup == NULL)
	{
		priv->fullscreen_popup
			= xviewer_window_create_fullscreen_popup (window);
		gtk_overlay_add_overlay (GTK_OVERLAY(priv->overlay),
					 priv->fullscreen_popup);
	}

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_hide (menubar);

	g_signal_connect (priv->view,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->view,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "motion-notify-event",
			  G_CALLBACK (fullscreen_motion_notify_cb),
			  window);

	g_signal_connect (priv->thumbview,
			  "leave-notify-event",
			  G_CALLBACK (fullscreen_leave_notify_cb),
			  window);

	fullscreen_set_timeout (window);

	if (slideshow) {
		priv->slideshow_loop =
			g_settings_get_boolean (priv->fullscreen_settings,
						XVIEWER_CONF_FULLSCREEN_LOOP);

		priv->slideshow_switch_timeout =
			g_settings_get_int (priv->fullscreen_settings,
					    XVIEWER_CONF_FULLSCREEN_SECONDS);

		slideshow_set_timeout (window);
	}

	upscale = g_settings_get_boolean (priv->fullscreen_settings,
					  XVIEWER_CONF_FULLSCREEN_UPSCALE);

	xviewer_scroll_view_set_zoom_upscale (XVIEWER_SCROLL_VIEW (priv->view),
					  upscale);

	gtk_widget_grab_focus (priv->view);

	gtk_window_fullscreen (GTK_WINDOW (window));

	xviewer_window_inhibit_screensaver (window);

	/* Update both actions as we could've already been in one those modes */
	xviewer_window_update_slideshow_action (window);
	xviewer_window_update_fullscreen_action (window);
	xviewer_window_update_pause_slideshow_action (window);
}

static void
xviewer_window_stop_fullscreen (XviewerWindow *window, gboolean slideshow)
{
	XviewerWindowPrivate *priv;
	GtkWidget *menubar;

	xviewer_debug (DEBUG_WINDOW);

	priv = window->priv;

	if (priv->mode != XVIEWER_WINDOW_MODE_SLIDESHOW &&
	    priv->mode != XVIEWER_WINDOW_MODE_FULLSCREEN) return;

	priv->mode = XVIEWER_WINDOW_MODE_NORMAL;

	fullscreen_clear_timeout (window);
	gtk_revealer_set_reveal_child (GTK_REVEALER(window->priv->fullscreen_popup), FALSE);

	if (slideshow) {
		slideshow_clear_timeout (window);
	}

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->view,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_motion_notify_cb,
					      window);

	g_signal_handlers_disconnect_by_func (priv->thumbview,
					      (gpointer) fullscreen_leave_notify_cb,
					      window);

	update_ui_visibility (window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_widget_show (menubar);

	xviewer_scroll_view_set_zoom_upscale (XVIEWER_SCROLL_VIEW (priv->view), FALSE);

	xviewer_scroll_view_override_bg_color (XVIEWER_SCROLL_VIEW (window->priv->view),
					   NULL);
	gtk_window_unfullscreen (GTK_WINDOW (window));

	if (slideshow) {
		xviewer_window_update_slideshow_action (window);
	} else {
		xviewer_window_update_fullscreen_action (window);
	}

	xviewer_scroll_view_show_cursor (XVIEWER_SCROLL_VIEW (priv->view));

	xviewer_window_uninhibit_screensaver (window);
}

static void
set_basename_for_print_settings (GtkPrintSettings *print_settings, XviewerWindow *window)
{
	const char *basename = NULL;

	if(G_LIKELY (window->priv->image != NULL))
		basename = xviewer_image_get_caption (window->priv->image);

	if (G_LIKELY(basename))
		gtk_print_settings_set (print_settings,
		                        GTK_PRINT_SETTINGS_OUTPUT_BASENAME,
		                        basename);
}

static void
xviewer_window_print (XviewerWindow *window)
{
	GtkWidget *dialog;
	GError *error = NULL;
	GtkPrintOperation *print;
	GtkPrintOperationResult res;
	GtkPageSetup *page_setup;
	GtkPrintSettings *print_settings;
	gboolean page_setup_disabled = FALSE;

	xviewer_debug (DEBUG_PRINTING);

	print_settings = xviewer_print_get_print_settings ();
	set_basename_for_print_settings (print_settings, window);

	/* Make sure the window stays valid while printing */
	g_object_ref (window);

	if (window->priv->page_setup != NULL)
		page_setup = g_object_ref (window->priv->page_setup);
	else
		page_setup = NULL;

	print = xviewer_print_operation_new (window->priv->image,
					 print_settings,
					 page_setup);


	// Disable page setup options if they are locked down
	page_setup_disabled = g_settings_get_boolean (window->priv->lockdown_settings,
						      XVIEWER_CONF_DESKTOP_CAN_SETUP_PAGE);
	if (page_setup_disabled)
		gtk_print_operation_set_embed_page_setup (print, FALSE);


	res = gtk_print_operation_run (print,
				       GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
				       GTK_WINDOW (window), &error);

	if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Error printing file:\n%s"),
						 error->message);
		g_signal_connect (dialog, "response",
				  G_CALLBACK (gtk_widget_destroy), NULL);
		gtk_widget_show (dialog);
		g_error_free (error);
	} else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
		GtkPageSetup *new_page_setup;
		xviewer_print_set_print_settings (gtk_print_operation_get_print_settings (print));
		new_page_setup = gtk_print_operation_get_default_page_setup (print);
		if (window->priv->page_setup != NULL)
			g_object_unref (window->priv->page_setup);
		window->priv->page_setup = g_object_ref (new_page_setup);
	}

	if (page_setup != NULL)
		g_object_unref (page_setup);
	g_object_unref (print_settings);
	g_object_unref (window);
}

static void
xviewer_window_cmd_file_open (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
        XviewerImage *current;
	GtkWidget *dlg;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);

        priv = window->priv;

	dlg = xviewer_file_chooser_new (GTK_FILE_CHOOSER_ACTION_OPEN);

	current = xviewer_thumb_view_get_first_selected_image (XVIEWER_THUMB_VIEW (priv->thumbview));

	if (current != NULL) {
		gchar *dir_uri, *file_uri;

		file_uri = xviewer_image_get_uri_for_display (current);
		dir_uri = g_path_get_dirname (file_uri);

	        gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dlg),
                                                         dir_uri);
		g_free (file_uri);
		g_free (dir_uri);
		g_object_unref (current);
	} else {
		/* If desired by the user,
		   fallback to the XDG_PICTURES_DIR (if available) */
		const gchar *pics_dir;
		gboolean use_fallback;

		use_fallback = g_settings_get_boolean (priv->ui_settings,
					XVIEWER_CONF_UI_FILECHOOSER_XDG_FALLBACK);
		pics_dir = g_get_user_special_dir (G_USER_DIRECTORY_PICTURES);
		if (use_fallback && pics_dir) {
			gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dlg),
							     pics_dir);
		}
	}

	g_signal_connect (dlg, "response",
			  G_CALLBACK (file_open_dialog_response_cb),
			  window);

	gtk_widget_show_all (dlg);
}

static void
xviewer_job_close_save_cb (XviewerJobSave *job, gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);
	GtkAction *action_save;

	g_signal_handlers_disconnect_by_func (job,
					      xviewer_job_close_save_cb,
					      window);

	/* clean the last save job */
	g_object_unref (window->priv->save_job);
	window->priv->save_job = NULL;

	/* recover save action from actions group */
	action_save = gtk_action_group_get_action (window->priv->actions_image,
						   "ImageSave");

	/* check if job contains any error */
	if (XVIEWER_JOB (job)->error == NULL) {
		gtk_widget_destroy (GTK_WIDGET (window));
	} else {
		GtkWidget *message_area;

		xviewer_thumb_view_set_current_image (XVIEWER_THUMB_VIEW (window->priv->thumbview),
						  job->current_image,
						  TRUE);

		message_area = xviewer_image_save_error_message_area_new (
					xviewer_image_get_caption (job->current_image),
					XVIEWER_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (xviewer_window_error_message_area_response),
				  window);

		gtk_window_set_icon (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW (window),
				      xviewer_image_get_caption (job->current_image));

		xviewer_window_set_message_area (window, message_area);

		gtk_info_bar_set_default_response (GTK_INFO_BAR (message_area),
						   GTK_RESPONSE_CANCEL);

		gtk_widget_show (message_area);

		update_status_bar (window);

		gtk_action_set_sensitive (action_save, TRUE);
	}
}

static void
close_confirmation_dialog_response_handler (XviewerCloseConfirmationDialog *dlg,
					    gint                        response_id,
					    XviewerWindow                  *window)
{
	GList            *selected_images;
	XviewerWindowPrivate *priv;
	GtkAction        *action_save_as;

	priv = window->priv;

	switch (response_id) {
	case XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVE:
		selected_images = xviewer_close_confirmation_dialog_get_selected_images (dlg);
		gtk_widget_destroy (GTK_WIDGET (dlg));

		if (xviewer_window_save_images (window, selected_images)) {
			g_signal_connect (priv->save_job,
					  "finished",
					  G_CALLBACK (xviewer_job_close_save_cb),
					  window);

			xviewer_job_scheduler_add_job (priv->save_job);
		}

		break;

	case XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_SAVEAS:
		selected_images = xviewer_close_confirmation_dialog_get_selected_images (dlg);
		gtk_widget_destroy (GTK_WIDGET (dlg));

		xviewer_thumb_view_set_current_image (XVIEWER_THUMB_VIEW (priv->thumbview),
						  g_list_first (selected_images)->data,
						  TRUE);

		action_save_as = gtk_action_group_get_action (priv->actions_image,
							      "ImageSaveAs");
		xviewer_window_cmd_save_as (action_save_as, window);
		break;

	case XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CLOSE:
		gtk_widget_destroy (GTK_WIDGET (window));
		break;

	case XVIEWER_CLOSE_CONFIRMATION_DIALOG_RESPONSE_CANCEL:
		gtk_widget_destroy (GTK_WIDGET (dlg));
		break;
	}
}

static gboolean
xviewer_window_unsaved_images_confirm (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	gboolean disabled;
	GtkWidget *dialog;
	GList *list;
	XviewerImage *image;
	GtkTreeIter iter;

	priv = window->priv;

	disabled = g_settings_get_boolean(priv->ui_settings,
					XVIEWER_CONF_UI_DISABLE_CLOSE_CONFIRMATION);
	disabled |= window->priv->save_disabled;

	if (disabled | !priv->store) {
		return FALSE;
	}

	list = NULL;
	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->store), &iter)) {
		do {
			gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
					    XVIEWER_LIST_STORE_XVIEWER_IMAGE, &image,
					    -1);
			if (!image)
				continue;

			if (xviewer_image_is_modified (image)) {
				list = g_list_prepend (list, image);
			}
		} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->store), &iter));
	}

	if (list) {
		list = g_list_reverse (list);
		dialog = xviewer_close_confirmation_dialog_new (GTK_WINDOW (window),
							    list);
		g_list_free (list);
		g_signal_connect (dialog,
				  "response",
				  G_CALLBACK (close_confirmation_dialog_response_handler),
				  window);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);

		gtk_widget_show (dialog);
		return TRUE;

	}
	return FALSE;
}

static void
xviewer_window_cmd_close_window (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_window_close (XVIEWER_WINDOW (user_data));
}

static void
xviewer_window_cmd_preferences (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_window_show_preferences_dialog (XVIEWER_WINDOW (user_data));
}

static void
xviewer_window_cmd_help (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);

	xviewer_util_show_help (NULL, GTK_WINDOW (window));
}

static void
xviewer_window_cmd_about (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_window_show_about_dialog (XVIEWER_WINDOW (user_data));

}

static void
xviewer_window_cmd_show_hide_bar (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	gboolean visible;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	if (priv->mode != XVIEWER_WINDOW_MODE_NORMAL &&
            priv->mode != XVIEWER_WINDOW_MODE_FULLSCREEN) return;

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewToolbar") == 0) {
		g_object_set (G_OBJECT (priv->toolbar_revealer), "reveal-child", visible, NULL);

		if (priv->mode == XVIEWER_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings,
						XVIEWER_CONF_UI_TOOLBAR, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewStatusbar") == 0) {
		g_object_set (G_OBJECT (priv->statusbar), "visible", visible, NULL);

		if (priv->mode == XVIEWER_WINDOW_MODE_NORMAL)
			g_settings_set_boolean (priv->ui_settings,
						XVIEWER_CONF_UI_STATUSBAR, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewImageGallery") == 0) {
		if (visible) {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events */
			if (!gtk_widget_get_realized (window->priv->thumbview))
				gtk_widget_realize (window->priv->thumbview);

			gtk_widget_show (priv->nav);
			gtk_widget_grab_focus (priv->thumbview);
		} else {
			/* Make sure the focus widget is realized to
			 * avoid warnings on keypress events.
			 * Don't do it during init phase or the view
			 * will get a bogus allocation. */
			if (!gtk_widget_get_realized (priv->view)
			    && priv->status == XVIEWER_WINDOW_STATUS_NORMAL)
				gtk_widget_realize (priv->view);

			gtk_widget_hide (priv->nav);

			if (gtk_widget_get_realized (priv->view))
				gtk_widget_grab_focus (priv->view);
		}
		g_settings_set_boolean (priv->ui_settings,
					XVIEWER_CONF_UI_IMAGE_GALLERY, visible);

	} else if (g_ascii_strcasecmp (gtk_action_get_name (action), "ViewSidebar") == 0) {
		if (visible) {
			gtk_widget_show (priv->sidebar);
		} else {
			gtk_widget_hide (priv->sidebar);
		}
		g_settings_set_boolean (priv->ui_settings, XVIEWER_CONF_UI_SIDEBAR,
					visible);
	}
}

static void
wallpaper_info_bar_response (GtkInfoBar *bar, gint response, XviewerWindow *window)
{
	if (response == GTK_RESPONSE_YES) {
		GAppInfo *app_info;
		GError *error = NULL;

		if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "Cinnamon") == 0 || g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "X-Cinnamon") == 0)
			app_info = g_app_info_create_from_commandline ("cinnamon-settings backgrounds", "System Settings", G_APP_INFO_CREATE_NONE, &error);
		else if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "MATE") == 0)
			app_info = g_app_info_create_from_commandline ("mate-appearance-properties --show-page=background", "System Settings", G_APP_INFO_CREATE_NONE, &error);
		else if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "XFCE") == 0)
			app_info = g_app_info_create_from_commandline ("xfdesktop-settings", "Desktop", G_APP_INFO_CREATE_NONE, &error);
		else if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "Unity") == 0)
			app_info = g_app_info_create_from_commandline ("unity-control-center appearance", "System Settings", G_APP_INFO_CREATE_NONE, &error);
		else
			app_info = g_app_info_create_from_commandline ("gnome-XDG_CURRENT_DESKTOPcontrol-center background", "System Settings", G_APP_INFO_CREATE_NONE, &error);

		if (error != NULL) {
			g_warning ("%s%s", _("Error launching System Settings: "),
				   error->message);
			g_error_free (error);
			error = NULL;
		}

		if (app_info != NULL) {
			GdkAppLaunchContext *context;
			GdkDisplay *display;

			display = gtk_widget_get_display (GTK_WIDGET (window));
			context = gdk_display_get_app_launch_context (display);
			g_app_info_launch (app_info, NULL, G_APP_LAUNCH_CONTEXT (context), &error);

			if (error != NULL) {
				g_warning ("%s%s", _("Error launching System Settings: "),
					   error->message);
				g_error_free (error);
				error = NULL;
			}

			g_object_unref (context);
			g_object_unref (app_info);
		}
	}

	/* Close message area on every response */
	xviewer_window_set_message_area (window, NULL);
}

static void
xviewer_window_set_wallpaper (XviewerWindow *window, const gchar *filename, const gchar *visible_filename)
{
	GSettings *settings;
	GtkWidget *info_bar;
	GtkWidget *image;
	GtkWidget *label;
	GtkWidget *hbox;
	gchar *markup;
	gchar *text;
	gchar *basename;
	gchar *uri;

	uri = g_filename_to_uri (filename, NULL, NULL);

	if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "Cinnamon") == 0 || g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "X-Cinnamon") == 0) {
		settings = g_settings_new ("org.cinnamon.desktop.background");
		g_settings_set_string (settings, "picture-uri", uri);
		g_object_unref (settings);
	}
	else if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "MATE") == 0) {
		settings = g_settings_new ("org.mate.background");
		g_settings_set_string (settings, "picture-filename", filename);
		g_object_unref (settings);
	}
	else if (g_strcmp0 (g_getenv ("XDG_CURRENT_DESKTOP"), "XFCE") == 0) {
		gchar *command = g_strdup_printf("xfce4-set-wallpaper '%s'", filename);
		system(command);
		g_free(command);
	}
	else {
		settings = g_settings_new ("org.gnome.desktop.background");
		g_settings_set_string (settings, "picture-uri", uri);
		g_object_unref (settings);
	}

	g_free (uri);

	/* I18N: When setting mnemonics for these strings, watch out to not
	   clash with mnemonics from xviewer's menubar */
	info_bar = gtk_info_bar_new_with_buttons (_("_Open Background Preferences"),
						  GTK_RESPONSE_YES,
						  C_("MessageArea","Hi_de"),
						  GTK_RESPONSE_NO, NULL);
	gtk_info_bar_set_message_type (GTK_INFO_BAR (info_bar),
				       GTK_MESSAGE_QUESTION);

	image = gtk_image_new_from_icon_name ("dialog-question",
					      GTK_ICON_SIZE_DIALOG);
	label = gtk_label_new (NULL);

	if (!visible_filename)
		basename = g_path_get_basename (filename);

	/* The newline character is currently necessary due to a problem
	 * with the automatic line break. */
	text = g_strdup_printf (_("The image \"%s\" has been set as Desktop Background."
				  "\nWould you like to modify its appearance?"),
				visible_filename ? visible_filename : basename);
	markup = g_markup_printf_escaped ("<b>%s</b>", text);
	gtk_label_set_markup (GTK_LABEL (label), markup);
	g_free (markup);
	g_free (text);
	if (!visible_filename)
		g_free (basename);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_widget_set_valign (image, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (info_bar))), hbox, TRUE, TRUE, 0);
	gtk_widget_show_all (hbox);
	gtk_widget_show (info_bar);


	xviewer_window_set_message_area (window, info_bar);
	gtk_info_bar_set_default_response (GTK_INFO_BAR (info_bar),
					   GTK_RESPONSE_YES);
	g_signal_connect (info_bar, "response",
			  G_CALLBACK (wallpaper_info_bar_response), window);
}

static void
xviewer_job_save_cb (XviewerJobSave *job, gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);
	GtkAction *action_save;

	g_signal_handlers_disconnect_by_func (job,
					      xviewer_job_save_cb,
					      window);

	g_signal_handlers_disconnect_by_func (job,
					      xviewer_job_save_progress_cb,
					      window);

	/* clean the last save job */
	g_object_unref (window->priv->save_job);
	window->priv->save_job = NULL;

	/* recover save action from actions group */
	action_save = gtk_action_group_get_action (window->priv->actions_image,
						   "ImageSave");

	/* check if job contains any error */
	if (XVIEWER_JOB (job)->error == NULL) {
		update_status_bar (window);
		gtk_window_set_title (GTK_WINDOW (window),
				      xviewer_image_get_caption (job->current_image));

		gtk_action_set_sensitive (action_save, FALSE);
	} else {
		GtkWidget *message_area;

		message_area = xviewer_image_save_error_message_area_new (
					xviewer_image_get_caption (job->current_image),
					XVIEWER_JOB (job)->error);

		g_signal_connect (message_area,
				  "response",
				  G_CALLBACK (xviewer_window_error_message_area_response),
				  window);

		gtk_window_set_icon (GTK_WINDOW (window), NULL);
		gtk_window_set_title (GTK_WINDOW (window),
				      xviewer_image_get_caption (job->current_image));

		xviewer_window_set_message_area (window, message_area);

		gtk_info_bar_set_default_response (GTK_INFO_BAR (message_area),
						   GTK_RESPONSE_CANCEL);

		gtk_widget_show (message_area);

		update_status_bar (window);

		gtk_action_set_sensitive (action_save, TRUE);
	}
}

static void
xviewer_job_copy_cb (XviewerJobCopy *job, gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);
	gchar *filepath, *basename, *filename, *extension;
	GtkAction *action;
	GFile *source_file, *dest_file;
	GTimeVal mtime;

	/* Create source GFile */
	basename = g_file_get_basename (job->images->data);
	filepath = g_build_filename (job->destination, basename, NULL);
	source_file = g_file_new_for_path (filepath);
	g_free (filepath);

	/* Create destination GFile */
	extension = xviewer_util_filename_get_extension (basename);
	filename = g_strdup_printf  ("%s.%s", XVIEWER_WALLPAPER_FILENAME, extension);
	filepath = g_build_filename (job->destination, filename, NULL);
	dest_file = g_file_new_for_path (filepath);
	g_free (filename);
	g_free (extension);

	/* Move the file */
	g_file_move (source_file, dest_file, G_FILE_COPY_OVERWRITE,
		     NULL, NULL, NULL, NULL);

	/* Update mtime, see bug 664747 */
	g_get_current_time (&mtime);
	g_file_set_attribute_uint64 (dest_file, G_FILE_ATTRIBUTE_TIME_MODIFIED,
	                             mtime.tv_sec, G_FILE_QUERY_INFO_NONE,
				     NULL, NULL);
	g_file_set_attribute_uint32 (dest_file,
				     G_FILE_ATTRIBUTE_TIME_MODIFIED_USEC,
				     mtime.tv_usec, G_FILE_QUERY_INFO_NONE,
				     NULL, NULL);

	/* Set the wallpaper */
	xviewer_window_set_wallpaper (window, filepath, basename);
	g_free (basename);
	g_free (filepath);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->copy_file_cid);
	action = gtk_action_group_get_action (window->priv->actions_image,
					      "ImageSetAsWallpaper");
	gtk_action_set_sensitive (action, TRUE);

	window->priv->copy_job = NULL;

	g_object_unref (source_file);
	g_object_unref (dest_file);
	g_object_unref (job);
}

static gboolean
xviewer_window_save_images (XviewerWindow *window, GList *images)
{
	XviewerWindowPrivate *priv;

	priv = window->priv;

	if (window->priv->save_job != NULL)
		return FALSE;

	priv->save_job = xviewer_job_save_new (images);

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (xviewer_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (xviewer_job_save_progress_cb),
			  window);

	return TRUE;
}

static void
xviewer_window_cmd_save (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;
	XviewerWindow *window;
	GList *images;

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (priv->thumbview));

	if (xviewer_window_save_images (window, images)) {
		xviewer_job_scheduler_add_job (priv->save_job);
	}
}

static GFile*
xviewer_window_retrieve_save_as_file (XviewerWindow *window, XviewerImage *image)
{
	GtkWidget *dialog;
	GFile *save_file = NULL;
	GFile *last_dest_folder;
	gint response;

	g_assert (image != NULL);

	dialog = xviewer_file_chooser_new (GTK_FILE_CHOOSER_ACTION_SAVE);

	last_dest_folder = window->priv->last_save_as_folder;

	if (last_dest_folder && g_file_query_exists (last_dest_folder, NULL)) {
		gtk_file_chooser_set_current_folder_file (GTK_FILE_CHOOSER (dialog), last_dest_folder, NULL);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog),
						 xviewer_image_get_caption (image));
	} else {
		GFile *image_file;

		image_file = xviewer_image_get_file (image);
		/* Setting the file will also navigate to its parent folder */
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (dialog),
					   image_file, NULL);
		g_object_unref (image_file);
	}

	response = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);

	if (response == GTK_RESPONSE_OK) {
		save_file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (dialog));
		if (window->priv->last_save_as_folder)
			g_object_unref (window->priv->last_save_as_folder);
		window->priv->last_save_as_folder = g_file_get_parent (save_file);
	}
	gtk_widget_destroy (dialog);

	return save_file;
}

static void
xviewer_window_cmd_save_as (GtkAction *action, gpointer user_data)
{
        XviewerWindowPrivate *priv;
        XviewerWindow *window;
	GList *images;
	guint n_images;

        window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	if (window->priv->save_job != NULL)
		return;

	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (priv->thumbview));
	n_images = g_list_length (images);

	if (n_images == 1) {
		GFile *file;

		file = xviewer_window_retrieve_save_as_file (window, images->data);

		if (!file) {
			g_list_free (images);
			return;
		}

		priv->save_job = xviewer_job_save_as_new (images, NULL, file);

		g_object_unref (file);
	} else if (n_images > 1) {
		GFile *base_file;
		GtkWidget *dialog;
		gchar *basedir;
		XviewerURIConverter *converter;

		basedir = g_get_current_dir ();
		base_file = g_file_new_for_path (basedir);
		g_free (basedir);

		dialog = xviewer_save_as_dialog_new (GTK_WINDOW (window),
						 images,
						 base_file);

		gtk_widget_show_all (dialog);

		if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_OK) {
			g_object_unref (base_file);
			g_list_free (images);
			gtk_widget_destroy (dialog);

			return;
		}

		converter = xviewer_save_as_dialog_get_converter (dialog);

		g_assert (converter != NULL);

		priv->save_job = xviewer_job_save_as_new (images, converter, NULL);

		gtk_widget_destroy (dialog);

		g_object_unref (converter);
		g_object_unref (base_file);
	} else {
		/* n_images = 0 -- No Image selected */
		return;
	}

	g_signal_connect (priv->save_job,
			  "finished",
			  G_CALLBACK (xviewer_job_save_cb),
			  window);

	g_signal_connect (priv->save_job,
			  "progress",
			  G_CALLBACK (xviewer_job_save_progress_cb),
			  window);

	xviewer_job_scheduler_add_job (priv->save_job);
}

static void
xviewer_window_cmd_open_containing_folder (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;
	GFile *file;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	priv = XVIEWER_WINDOW (user_data)->priv;

	g_return_if_fail (priv->image != NULL);

	file = xviewer_image_get_file (priv->image);

	g_return_if_fail (file != NULL);

	xviewer_util_show_file_in_filemanager (file,
				gtk_widget_get_screen (GTK_WIDGET (user_data)));
}

static void
xviewer_window_cmd_print (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);

	xviewer_window_print (window);
}

GtkWidget*
xviewer_window_get_properties_dialog (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	priv = window->priv;

	if (priv->properties_dlg == NULL) {
		GtkAction *next_image_action, *previous_image_action;

		next_image_action =
			gtk_action_group_get_action (priv->actions_gallery,
						     "GoNext");

		previous_image_action =
			gtk_action_group_get_action (priv->actions_gallery,
						     "GoPrevious");
		priv->properties_dlg =
			xviewer_properties_dialog_new (GTK_WINDOW (window),
						   XVIEWER_THUMB_VIEW (priv->thumbview),
						   next_image_action,
						   previous_image_action);

		xviewer_properties_dialog_update (XVIEWER_PROPERTIES_DIALOG (priv->properties_dlg),
					      priv->image);
		g_settings_bind (priv->ui_settings,
				 XVIEWER_CONF_UI_PROPSDIALOG_NETBOOK_MODE,
				 priv->properties_dlg, "netbook-mode",
				 G_SETTINGS_BIND_GET);
	}

	return priv->properties_dlg;
}

static void
xviewer_window_cmd_properties (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);
	GtkWidget *dialog;

	dialog = xviewer_window_get_properties_dialog (window);
	gtk_widget_show (dialog);
}

static void
xviewer_window_cmd_undo (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	apply_transformation (XVIEWER_WINDOW (user_data), NULL);
}

static void
xviewer_window_cmd_flip_horizontal (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	apply_transformation (XVIEWER_WINDOW (user_data),
			      xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_HORIZONTAL));
}

static void
xviewer_window_cmd_flip_vertical (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	apply_transformation (XVIEWER_WINDOW (user_data),
			      xviewer_transform_flip_new (XVIEWER_TRANSFORM_FLIP_VERTICAL));
}

static void
xviewer_window_cmd_rotate_90 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	apply_transformation (XVIEWER_WINDOW (user_data),
			      xviewer_transform_rotate_new (90));
}

static void
xviewer_window_cmd_rotate_270 (GtkAction *action, gpointer user_data)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	apply_transformation (XVIEWER_WINDOW (user_data),
			      xviewer_transform_rotate_new (270));
}

static void
xviewer_window_cmd_wallpaper (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	XviewerImage *image;
	GFile *file;
	char *filename = NULL;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	/* If currently copying an image to set it as wallpaper, return. */
	if (priv->copy_job != NULL)
		return;

	image = xviewer_thumb_view_get_first_selected_image (XVIEWER_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	file = xviewer_image_get_file (image);

	filename = g_file_get_path (file);

	/* Currently only local files can be set as wallpaper */
	if (filename == NULL || !xviewer_util_file_is_persistent (file))
	{
		GList *files = NULL;
		GtkAction *action;

		action = gtk_action_group_get_action (window->priv->actions_image,
						      "ImageSetAsWallpaper");
		gtk_action_set_sensitive (action, FALSE);

		priv->copy_file_cid = gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
								    "copy_file_cid");
		gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
				    priv->copy_file_cid,
				    _("Saving image locally…"));

		files = g_list_append (files, xviewer_image_get_file (image));
		priv->copy_job = xviewer_job_copy_new (files, g_get_user_data_dir ());
		g_signal_connect (priv->copy_job,
				  "finished",
				  G_CALLBACK (xviewer_job_copy_cb),
				  window);
		g_signal_connect (priv->copy_job,
				  "progress",
				  G_CALLBACK (xviewer_job_progress_cb),
				  window);
		xviewer_job_scheduler_add_job (priv->copy_job);

		g_object_unref (file);
		g_free (filename);
		return;
	}

	g_object_unref (file);

	xviewer_window_set_wallpaper (window, filename, NULL);

	g_free (filename);
}

static gboolean
xviewer_window_all_images_trasheable (GList *images)
{
	GFile *file;
	GFileInfo *file_info;
	GList *iter;
	XviewerImage *image;
	gboolean can_trash = TRUE;

	for (iter = images; iter != NULL; iter = g_list_next (iter)) {
		image = (XviewerImage *) iter->data;
		file = xviewer_image_get_file (image);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
					       0, NULL, NULL);
		can_trash = g_file_info_get_attribute_boolean (file_info,
							       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);

		g_object_unref (file_info);
		g_object_unref (file);

		if (can_trash == FALSE)
			break;
	}

	return can_trash;
}

static gint
show_force_image_delete_confirm_dialog (XviewerWindow *window,
					GList     *images)
{
	static gboolean dont_ask_again_force_delete = FALSE;

	GtkWidget *dialog;
	GtkWidget *dont_ask_again_button;
	XviewerImage  *image;
	gchar     *prompt;
	guint      n_images;
	gint       response;

	/* assume agreement, if the user doesn't want to be asked and deletion is available */
	if (dont_ask_again_force_delete)
		return GTK_RESPONSE_OK;

	/* retrieve the selected images count */
	n_images = g_list_length (images);

	/* make the dialog prompt message */
	if (n_images == 1) {
		image = XVIEWER_IMAGE (images->data);

		prompt = g_strdup_printf (_("Are you sure you want to remove\n\"%s\" permanently?"),
					  xviewer_image_get_caption (image));
	} else {
		prompt = g_strdup_printf (ngettext ("Are you sure you want to remove\n"
						    "the selected image permanently?",
						    "Are you sure you want to remove\n"
						    "the %d selected images permanently?",
						    n_images),
					  n_images);
	}

	/* create the dialog */
	dialog = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						     GTK_MESSAGE_WARNING,
						     GTK_BUTTONS_NONE,
						     "<span weight=\"bold\" size=\"larger\">%s</span>",
						     prompt);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog),
					 GTK_RESPONSE_OK);

	/* add buttons to the dialog */
	if (n_images == 1) {
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Delete"), GTK_RESPONSE_OK);
	} else {
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Cancel"), GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (GTK_DIALOG (dialog), _("_Yes")   , GTK_RESPONSE_OK);
	}

	/* add 'dont ask again' button */
	dont_ask_again_button = gtk_check_button_new_with_mnemonic (_("Do _not ask again during this session"));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dont_ask_again_button),
				      FALSE);

	gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
			  dont_ask_again_button,
			  TRUE,
			  TRUE,
			  0);

	/* show dialog and get user response */
	gtk_widget_show_all (dialog);
	response = gtk_dialog_run (GTK_DIALOG (dialog));

	/* only update the 'dont ask again' property if the user has accepted */
	if (response == GTK_RESPONSE_OK)
		dont_ask_again_force_delete = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dont_ask_again_button));

	/* free resources */
	g_free (prompt);
	gtk_widget_destroy (dialog);

	return response;
}

static gboolean
force_image_delete_real (XviewerImage  *image,
			 GError   **error)
{
	GFile     *file;
	GFileInfo *file_info;
	gboolean   can_delete;
	gboolean   result;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (image), FALSE);

	/* retrieve image file */
	file = xviewer_image_get_file (image);

	if (file == NULL) {
		g_set_error (error,
			     XVIEWER_WINDOW_ERROR,
			     XVIEWER_WINDOW_ERROR_IO,
			     _("Couldn't retrieve image file"));

		return FALSE;
	}

	/* retrieve some image file information */
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE,
				       0,
				       NULL,
				       NULL);

	if (file_info == NULL) {
		g_set_error (error,
			     XVIEWER_WINDOW_ERROR,
			     XVIEWER_WINDOW_ERROR_IO,
			     _("Couldn't retrieve image file information"));

		/* free resources */
		g_object_unref (file);

		return FALSE;
	}

	/* check that image file can be deleted */
	can_delete = g_file_info_get_attribute_boolean (file_info,
							G_FILE_ATTRIBUTE_ACCESS_CAN_DELETE);

	if (!can_delete) {
		g_set_error (error,
			     XVIEWER_WINDOW_ERROR,
			     XVIEWER_WINDOW_ERROR_IO,
			     _("Couldn't delete file"));

		/* free resources */
		g_object_unref (file_info);
		g_object_unref (file);

		return FALSE;
	}

	/* delete image file */
	result = g_file_delete (file,
				NULL,
				error);

	/* free resources */
	g_object_unref (file_info);
        g_object_unref (file);

	return result;
}

static void
xviewer_window_force_image_delete (XviewerWindow *window,
			       GList     *images)
{
	GList    *item;
	gint      current_position;
	XviewerImage *current_image;
	gboolean  success;

	g_return_if_fail (XVIEWER_WINDOW (window));

	current_position = xviewer_list_store_get_pos_by_image (window->priv->store,
							    XVIEWER_IMAGE (images->data));

	/* force delete of each image of the list */
	for (item = images; item != NULL; item = item->next) {
		GError   *error;
		XviewerImage *image;

		error = NULL;
		image = XVIEWER_IMAGE (item->data);

		success = force_image_delete_real (image, &error);

		if (!success) {
			GtkWidget *dialog;
			gchar     *header;

			/* set dialog error message */
			header = g_strdup_printf (_("Error on deleting image %s"),
						  xviewer_image_get_caption (image));

			/* create dialog */
			dialog = gtk_message_dialog_new (GTK_WINDOW (window),
							 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
							 GTK_MESSAGE_ERROR,
							 GTK_BUTTONS_OK,
							 "%s", header);

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
								  "%s",
								  error->message);

			/* show dialog */
			gtk_dialog_run (GTK_DIALOG (dialog));

			/* free resources */
			gtk_widget_destroy (dialog);
			g_free (header);

			return;
		}

		/* remove image from store */
		xviewer_list_store_remove_image (window->priv->store, image);
	}

	/* free list */
	g_list_foreach (images, (GFunc) g_object_unref, NULL);
	g_list_free    (images);

	/* select image at previously saved position */
	current_position = MIN (current_position,
				xviewer_list_store_length (window->priv->store) - 1);

	if (current_position >= 0) {
		current_image = xviewer_list_store_get_image_by_pos (window->priv->store,
								 current_position);

		xviewer_thumb_view_set_current_image (XVIEWER_THUMB_VIEW (window->priv->thumbview),
						  current_image,
						  TRUE);

		if (current_image != NULL)
			g_object_unref (current_image);
	}
}

static void
xviewer_window_cmd_reload (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	GList     *images;

	window = XVIEWER_WINDOW (user_data);
	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (window->priv->thumbview));
	if (G_LIKELY (g_list_length (images) > 0))
	{
		xviewer_window_reload_image (window);
	}
}

static void
xviewer_window_cmd_delete (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	GList     *images;
	gint       result;

	window = XVIEWER_WINDOW (user_data);
	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (window->priv->thumbview));
	if (G_LIKELY (g_list_length (images) > 0))
	{
		result = show_force_image_delete_confirm_dialog (window, images);

		if (result == GTK_RESPONSE_OK)
			xviewer_window_force_image_delete (window, images);
	}
}

static int
show_move_to_trash_confirm_dialog (XviewerWindow *window, GList *images, gboolean can_trash)
{
	GtkWidget *dlg;
	char *prompt;
	int response;
	int n_images;
	XviewerImage *image;
	static gboolean dontaskagain = FALSE;
	gboolean neverask = FALSE;
	GtkWidget* dontask_cbutton = NULL;

	/* Check if the user never wants to be bugged. */
	neverask = g_settings_get_boolean (window->priv->ui_settings,
					   XVIEWER_CONF_UI_DISABLE_TRASH_CONFIRMATION);

	/* Assume agreement, if the user doesn't want to be
	 * asked and the trash is available */
	if (can_trash && (dontaskagain || neverask))
		return GTK_RESPONSE_OK;

	n_images = g_list_length (images);

	if (n_images == 1) {
		image = XVIEWER_IMAGE (images->data);
		if (can_trash) {
			prompt = g_strdup_printf (_("Are you sure you want to move\n\"%s\" to the trash?"),
						  xviewer_image_get_caption (image));
		} else {
			prompt = g_strdup_printf (_("A trash for \"%s\" couldn't be found. Do you want to remove "
						    "this image permanently?"), xviewer_image_get_caption (image));
		}
	} else {
		if (can_trash) {
			prompt = g_strdup_printf (ngettext("Are you sure you want to move\n"
							   "the selected image to the trash?",
							   "Are you sure you want to move\n"
							   "the %d selected images to the trash?", n_images), n_images);
		} else {
			prompt = g_strdup (_("Some of the selected images can't be moved to the trash "
					     "and will be removed permanently. Are you sure you want "
					     "to proceed?"));
		}
	}

	dlg = gtk_message_dialog_new_with_markup (GTK_WINDOW (window),
						  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_WARNING,
						  GTK_BUTTONS_NONE,
						  "<span weight=\"bold\" size=\"larger\">%s</span>",
						  prompt);
	g_free (prompt);

	gtk_dialog_add_button (GTK_DIALOG (dlg), _("_Cancel"), GTK_RESPONSE_CANCEL);

	if (can_trash) {
		gtk_dialog_add_button (GTK_DIALOG (dlg), _("Move to _Trash"), GTK_RESPONSE_OK);

		dontask_cbutton = gtk_check_button_new_with_mnemonic (_("Do _not ask again during this session"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dontask_cbutton), FALSE);

		gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dlg))), dontask_cbutton, TRUE, TRUE, 0);
	} else {
		if (n_images == 1) {
			gtk_dialog_add_button (GTK_DIALOG (dlg), _("_Delete"), GTK_RESPONSE_OK);
		} else {
			gtk_dialog_add_button (GTK_DIALOG (dlg), _("_Yes"), GTK_RESPONSE_OK);
		}
	}

	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_OK);
	gtk_window_set_title (GTK_WINDOW (dlg), "");
	gtk_widget_show_all (dlg);

	response = gtk_dialog_run (GTK_DIALOG (dlg));

	/* Only update the property if the user has accepted */
	if (can_trash && response == GTK_RESPONSE_OK)
		dontaskagain = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dontask_cbutton));

	/* The checkbutton is destroyed together with the dialog */
	gtk_widget_destroy (dlg);

	return response;
}

static gboolean
move_to_trash_real (XviewerImage *image, GError **error)
{
	GFile *file;
	GFileInfo *file_info;
	gboolean can_trash, result;

	g_return_val_if_fail (XVIEWER_IS_IMAGE (image), FALSE);

	file = xviewer_image_get_file (image);
	file_info = g_file_query_info (file,
				       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH,
				       0, NULL, NULL);
	if (file_info == NULL) {
		g_set_error (error,
			     XVIEWER_WINDOW_ERROR,
			     XVIEWER_WINDOW_ERROR_TRASH_NOT_FOUND,
			     _("Couldn't access trash."));
		return FALSE;
	}

	can_trash = g_file_info_get_attribute_boolean (file_info,
						       G_FILE_ATTRIBUTE_ACCESS_CAN_TRASH);
	g_object_unref (file_info);
	if (can_trash)
	{
		result = g_file_trash (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     XVIEWER_WINDOW_ERROR,
				     XVIEWER_WINDOW_ERROR_TRASH_NOT_FOUND,
				     _("Couldn't access trash."));
		}
	} else {
		result = g_file_delete (file, NULL, NULL);
		if (result == FALSE) {
			g_set_error (error,
				     XVIEWER_WINDOW_ERROR,
				     XVIEWER_WINDOW_ERROR_IO,
				     _("Couldn't delete file"));
		}
	}

        g_object_unref (file);

	return result;
}

static void
xviewer_window_cmd_copy_image (GtkAction *action, gpointer user_data)
{
	GtkClipboard *clipboard;
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	XviewerImage *image;
	XviewerClipboardHandler *cbhandler;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;

	image = xviewer_thumb_view_get_first_selected_image (XVIEWER_THUMB_VIEW (priv->thumbview));

	g_return_if_fail (XVIEWER_IS_IMAGE (image));

	clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);

	cbhandler = xviewer_clipboard_handler_new (image);
	// cbhandler will self-destruct when it's not needed anymore
	xviewer_clipboard_handler_copy_to_clipboard (cbhandler, clipboard);
}

static void
xviewer_window_cmd_move_to_trash (GtkAction *action, gpointer user_data)
{
	GList *images;
	GList *it;
	XviewerWindowPrivate *priv;
	XviewerListStore *list;
	int pos;
	XviewerImage *img;
	XviewerWindow *window;
	int response;
	int n_images;
	gboolean success;
	gboolean can_trash;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	window = XVIEWER_WINDOW (user_data);
	priv = window->priv;
	list = priv->store;

	n_images = xviewer_thumb_view_get_n_selected (XVIEWER_THUMB_VIEW (priv->thumbview));

	if (n_images < 1) return;

	/* save position of selected image after the deletion */
	images = xviewer_thumb_view_get_selected_images (XVIEWER_THUMB_VIEW (priv->thumbview));

	g_assert (images != NULL);

	/* HACK: xviewer_list_store_get_n_selected return list in reverse order */
	images = g_list_reverse (images);

	can_trash = xviewer_window_all_images_trasheable (images);

	if (g_ascii_strcasecmp (gtk_action_get_name (action), "Delete") == 0 ||
	    can_trash == FALSE) {
		response = show_move_to_trash_confirm_dialog (window, images, can_trash);

		if (response != GTK_RESPONSE_OK) return;
	}

	pos = xviewer_list_store_get_pos_by_image (list, XVIEWER_IMAGE (images->data));

	/* FIXME: make a nice progress dialog */
	/* Do the work actually. First try to delete the image from the disk. If this
	 * is successful, remove it from the screen. Otherwise show error dialog.
	 */
	for (it = images; it != NULL; it = it->next) {
		GError *error = NULL;
		XviewerImage *image;

		image = XVIEWER_IMAGE (it->data);

		success = move_to_trash_real (image, &error);

		if (success) {
			xviewer_list_store_remove_image (list, image);
		} else {
			char *header;
			GtkWidget *dlg;

			header = g_strdup_printf (_("Error on deleting image %s"),
						  xviewer_image_get_caption (image));

			dlg = gtk_message_dialog_new (GTK_WINDOW (window),
						      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_ERROR,
						      GTK_BUTTONS_OK,
						      "%s", header);

			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dlg),
								  "%s", error->message);

			gtk_dialog_run (GTK_DIALOG (dlg));

			gtk_widget_destroy (dlg);

			g_free (header);
		}
	}

	/* free list */
	g_list_foreach (images, (GFunc) g_object_unref, NULL);
	g_list_free (images);

	/* select image at previously saved position */
	pos = MIN (pos, xviewer_list_store_length (list) - 1);

	if (pos >= 0) {
		img = xviewer_list_store_get_image_by_pos (list, pos);

		xviewer_thumb_view_set_current_image (XVIEWER_THUMB_VIEW (priv->thumbview),
						  img,
						  TRUE);

		if (img != NULL) {
			g_object_unref (img);
		}
	}
}

static void
xviewer_window_cmd_fullscreen (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	gboolean fullscreen;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (user_data);

	fullscreen = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (fullscreen) {
		xviewer_window_run_fullscreen (window, FALSE);
	} else {
		xviewer_window_stop_fullscreen (window, FALSE);
	}
}

static void
xviewer_window_cmd_slideshow (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	gboolean slideshow;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (user_data);

	slideshow = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	if (slideshow) {
		xviewer_window_run_fullscreen (window, TRUE);
	} else {
		xviewer_window_stop_fullscreen (window, TRUE);
	}
}

static void
xviewer_window_cmd_pause_slideshow (GtkAction *action, gpointer user_data)
{
	XviewerWindow *window;
	gboolean slideshow;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (user_data);

	slideshow = window->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW;

	if (!slideshow && window->priv->mode != XVIEWER_WINDOW_MODE_FULLSCREEN)
		return;

	xviewer_window_run_fullscreen (window, !slideshow);
}

static void
xviewer_window_cmd_zoom_in (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	if (priv->view) {
		xviewer_scroll_view_zoom_in (XVIEWER_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
xviewer_window_cmd_zoom_out (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	if (priv->view) {
		xviewer_scroll_view_zoom_out (XVIEWER_SCROLL_VIEW (priv->view), FALSE);
	}
}

static void
xviewer_window_cmd_zoom_normal (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	if (priv->view) {
		xviewer_scroll_view_set_zoom (XVIEWER_SCROLL_VIEW (priv->view), 1.0);
	}
}

static void
xviewer_window_cmd_zoom_fit (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;
	XviewerZoomMode mode;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	mode = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action))
	       ? XVIEWER_ZOOM_MODE_SHRINK_TO_FIT : XVIEWER_ZOOM_MODE_FREE;

	if (priv->view) {
		xviewer_scroll_view_set_zoom_mode (XVIEWER_SCROLL_VIEW (priv->view),
					       mode);
	}
}

static void
xviewer_window_cmd_go_prev (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_LEFT);
}

static void
xviewer_window_cmd_go_next (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_RIGHT);
}

static void
xviewer_window_cmd_go_first (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_FIRST);
}

static void
xviewer_window_cmd_go_last (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_LAST);
}

static void
xviewer_window_cmd_go_random (GtkAction *action, gpointer user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_RANDOM);
}

static const GtkActionEntry action_entries_window[] = {
	{ "Image", NULL, N_("_File") },
	{ "Edit",  NULL, N_("_Edit") },
	{ "View",  NULL, N_("_View") },
	{ "Go",    NULL, N_("_Go") },
	{ "Tools", NULL, N_("_Tools") },
	{ "Help",  NULL, N_("_Help") },

	{ "ImageOpen", "document-open-symbolic",  N_("_Open…"), "<control>O",
	  N_("Open a file"),
	  G_CALLBACK (xviewer_window_cmd_file_open) },
	{ "ImageClose", "window-close-symbolic", N_("_Close"), "<control>W",
	  N_("Close window"),
	  G_CALLBACK (xviewer_window_cmd_close_window) },
	{ "EditPreferences", "preferences-system-symbolic", N_("Prefere_nces"), NULL,
	  N_("Preferences for Image Viewer"),
	  G_CALLBACK (xviewer_window_cmd_preferences) },
	{ "HelpManual", "help-contents-symbolic", N_("_Contents"), "F1",
	  N_("Help on this application"),
	  G_CALLBACK (xviewer_window_cmd_help) },
	{ "HelpAbout", "help-about-symbolic", N_("_About"), NULL,
	  N_("About this application"),
	  G_CALLBACK (xviewer_window_cmd_about) }
};

static const GtkToggleActionEntry toggle_entries_window[] = {
	{ "ViewToolbar", NULL, N_("_Toolbar"), NULL,
	  N_("Changes the visibility of the toolbar in the current window"),
	  G_CALLBACK (xviewer_window_cmd_show_hide_bar), TRUE },
	{ "ViewStatusbar", NULL, N_("_Statusbar"), NULL,
	  N_("Changes the visibility of the statusbar in the current window"),
	  G_CALLBACK (xviewer_window_cmd_show_hide_bar), TRUE },
	{ "ViewImageGallery", "xviewer-image-gallery", N_("_Image Gallery"), "F9",
	  N_("Changes the visibility of the image gallery pane in the current window"),
	  G_CALLBACK (xviewer_window_cmd_show_hide_bar), TRUE },
	{ "ViewSidebar", NULL, N_("Side _Pane"), "<control>F9",
	  N_("Changes the visibility of the side pane in the current window"),
	  G_CALLBACK (xviewer_window_cmd_show_hide_bar), TRUE },
};

static const GtkActionEntry action_entries_image[] = {
	{ "ImageSave", "document-save-symbolic", N_("_Save"), "<control>s",
	  N_("Save changes in currently selected images"),
	  G_CALLBACK (xviewer_window_cmd_save) },
	{ "ImageOpenWith", NULL, N_("Open _with"), NULL,
	  N_("Open the selected image with a different application"),
	  NULL},
	{ "ImageSaveAs", "document-save-as-symbolic", N_("Save _As…"), "<control><shift>s",
	  N_("Save the selected images with a different name"),
	  G_CALLBACK (xviewer_window_cmd_save_as) },
	{ "ImageOpenContainingFolder", "folder-symbolic", N_("Show Containing _Folder"), NULL,
	  N_("Show the folder which contains this file in the file manager"),
	  G_CALLBACK (xviewer_window_cmd_open_containing_folder) },
	{ "ImagePrint", "document-print-symbolic", N_("_Print…"), "<control>p",
	  N_("Print the selected image"),
	  G_CALLBACK (xviewer_window_cmd_print) },
	{ "ImageProperties", "document-properties-symbolic", N_("Prope_rties"), "<alt>Return",
	  N_("Show the properties and metadata of the selected image"),
	  G_CALLBACK (xviewer_window_cmd_properties) },
	{ "EditUndo", "edit-undo-symbolic", N_("_Undo"), "<control>z",
	  N_("Undo the last change in the image"),
	  G_CALLBACK (xviewer_window_cmd_undo) },
	{ "EditFlipHorizontal", "object-flip-horizontal-symbolic", N_("Flip _Horizontal"), NULL,
	  N_("Mirror the image horizontally"),
	  G_CALLBACK (xviewer_window_cmd_flip_horizontal) },
	{ "EditFlipVertical", "object-flip-vertical-symbolic", N_("Flip _Vertical"), NULL,
	  N_("Mirror the image vertically"),
	  G_CALLBACK (xviewer_window_cmd_flip_vertical) },
	{ "EditRotate90",  "object-rotate-right-symbolic",  N_("_Rotate Clockwise"), "<control>r",
	  N_("Rotate the image 90 degrees to the right"),
	  G_CALLBACK (xviewer_window_cmd_rotate_90) },
	{ "EditRotate270", "object-rotate-left-symbolic", N_("Rotate Counterc_lockwise"), "<control><shift>r",
	  N_("Rotate the image 90 degrees to the left"),
	  G_CALLBACK (xviewer_window_cmd_rotate_270) },
	{ "ImageSetAsWallpaper", NULL, N_("Set as Wa_llpaper"),
	  "<control>F8", N_("Set the selected image as the wallpaper"),
	  G_CALLBACK (xviewer_window_cmd_wallpaper) },
	{ "EditMoveToTrash", "user-trash-symbolic", N_("Move to _Trash"), NULL,
	  N_("Move the selected image to the trash folder"),
	  G_CALLBACK (xviewer_window_cmd_move_to_trash) },
	{ "EditDelete", "edit-delete-symbolic", N_("_Delete Image"), "<shift>Delete",
	  N_("Delete the selected image"),
	  G_CALLBACK (xviewer_window_cmd_delete) },
	{ "EditCopyImage", "edit-copy-symbolic", N_("_Copy"), "<control>C",
	  N_("Copy the selected image to the clipboard"),
	  G_CALLBACK (xviewer_window_cmd_copy_image) },
	{ "ViewZoomIn", "zoom-in-symbolic", N_("_Zoom In"), "<control>plus",
	  N_("Enlarge the image"),
	  G_CALLBACK (xviewer_window_cmd_zoom_in) },
	{ "ViewZoomOut", "zoom-out-symbolic", N_("Zoom _Out"), "<control>minus",
	  N_("Shrink the image"),
	  G_CALLBACK (xviewer_window_cmd_zoom_out) },
	{ "ViewZoomNormal", "zoom-original-symbolic", N_("_Normal Size"), "<control>0",
	  N_("Show the image at its normal size"),
	  G_CALLBACK (xviewer_window_cmd_zoom_normal) },
	{ "ViewReload", "view-refresh-symbolic", N_("_Reload"), "R",
	  N_("Reload the image"),
	  G_CALLBACK (xviewer_window_cmd_reload) },
	{ "ControlEqual", "zoom-in-symbolic", N_("_Zoom In"), "<control>equal",
	  N_("Enlarge the image"),
	  G_CALLBACK (xviewer_window_cmd_zoom_in) },
	{ "ControlKpAdd", "zoom-in-symbolic", N_("_Zoom In"), "<control>KP_Add",
	  N_("Shrink the image"),
	  G_CALLBACK (xviewer_window_cmd_zoom_in) },
	{ "ControlKpSub", "zoom-in-symbolic", N_("Zoom _Out"), "<control>KP_Subtract",
	  N_("Shrink the image"),
	  G_CALLBACK (xviewer_window_cmd_zoom_out) },
	{ "ControlKpZero", "zoom-original-symbolic", N_("_Normal Size"), "<control>KP_0",
	  N_("Show the image at its normal size"),
	  G_CALLBACK (xviewer_window_cmd_zoom_normal) },
	{ "Delete", NULL, N_("Move to _Trash"), "Delete",
	  NULL,
	  G_CALLBACK (xviewer_window_cmd_move_to_trash) },
};

static const GtkToggleActionEntry toggle_entries_image[] = {
	{ "ViewFullscreen", "view-fullscreen-symbolic", N_("_Fullscreen"), "F11",
	  N_("Show the current image in fullscreen mode"),
	  G_CALLBACK (xviewer_window_cmd_fullscreen), FALSE },
	{ "PauseSlideshow", "media-playback-pause-symbolic", N_("Pause Slideshow"),
	  NULL, N_("Pause or resume the slideshow"),
	  G_CALLBACK (xviewer_window_cmd_pause_slideshow), FALSE },
	{ "ViewZoomFit", "zoom-fit-best-symbolic", N_("_Best Fit"), "F",
	  N_("Fit the image to the window"),
	  G_CALLBACK (xviewer_window_cmd_zoom_fit) },
};

static const GtkActionEntry action_entries_gallery[] = {
	{ "GoPrevious", "go-previous-symbolic", N_("_Previous Image"), "Left",
	  N_("Go to the previous image of the gallery"),
	  G_CALLBACK (xviewer_window_cmd_go_prev) },
	{ "GoNext", "go-next-symbolic", N_("_Next Image"), "Right",
	  N_("Go to the next image of the gallery"),
	  G_CALLBACK (xviewer_window_cmd_go_next) },
	{ "GoFirst", "go-first-symbolic", N_("_First Image"), "<Alt>Home",
	  N_("Go to the first image of the gallery"),
	  G_CALLBACK (xviewer_window_cmd_go_first) },
	{ "GoLast", "go-last-symbolic", N_("_Last Image"), "<Alt>End",
	  N_("Go to the last image of the gallery"),
	  G_CALLBACK (xviewer_window_cmd_go_last) },
	{ "GoRandom", NULL, N_("_Random Image"), "<control>M",
	  N_("Go to a random image of the gallery"),
	  G_CALLBACK (xviewer_window_cmd_go_random) },
	{ "BackSpace", NULL, N_("_Previous Image"), "BackSpace",
	  NULL,
	  G_CALLBACK (xviewer_window_cmd_go_prev) },
	{ "Home", NULL, N_("_First Image"), "Home",
	  NULL,
	  G_CALLBACK (xviewer_window_cmd_go_first) },
	{ "End", NULL, N_("_Last Image"), "End",
	  NULL,
	  G_CALLBACK (xviewer_window_cmd_go_last) },
};

static void
xviewer_window_action_go_prev (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	xviewer_window_cmd_go_prev (NULL, user_data);
}

static void
xviewer_window_action_go_next (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	xviewer_window_cmd_go_next (NULL, user_data);
}

static void
xviewer_window_action_go_first (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	xviewer_window_cmd_go_first (NULL, user_data);
}

static void
xviewer_window_action_go_last (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	xviewer_window_cmd_go_last (NULL, user_data);
}

static void
xviewer_window_action_go_random (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data)
{
	xviewer_window_cmd_go_random (NULL, user_data);
}

static void
xviewer_window_action_rotate_90 (GSimpleAction *action,
                             GVariant      *parameter,
                             gpointer       user_data)
{
	xviewer_debug (DEBUG_WINDOW);

	xviewer_window_cmd_rotate_90 (NULL, user_data);
}

static void
xviewer_window_action_rotate_270 (GSimpleAction *action,
                              GVariant      *parameter,
                              gpointer       user_data)
{
	xviewer_debug (DEBUG_WINDOW);

	xviewer_window_cmd_rotate_270 (NULL, user_data);
}

static void
xviewer_window_action_zoom_in (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	xviewer_debug (DEBUG_WINDOW);
	xviewer_window_cmd_zoom_in (NULL, user_data);
}

static void
xviewer_window_action_zoom_out (GSimpleAction *action,
                           GVariant      *parameter,
                           gpointer       user_data)
{
	xviewer_debug (DEBUG_WINDOW);
	xviewer_window_cmd_zoom_out (NULL, user_data);
}

static void
xviewer_window_action_zoom_best_fit (GSimpleAction *action,
                                 GVariant      *parameter,
                                 gpointer       user_data)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));

	xviewer_debug (DEBUG_WINDOW);

	priv = XVIEWER_WINDOW (user_data)->priv;

	if (priv->view) {
		xviewer_scroll_view_set_zoom_mode (XVIEWER_SCROLL_VIEW (priv->view),
		                               XVIEWER_ZOOM_MODE_SHRINK_TO_FIT);
	}
}

static void
xviewer_window_action_set_zoom (GSimpleAction *action,
                            GVariant      *parameter,
                            gpointer       user_data)
{
	XviewerWindow *window;
	double zoom;

	g_return_if_fail (XVIEWER_IS_WINDOW (user_data));
	g_return_if_fail (g_variant_is_of_type (parameter, G_VARIANT_TYPE_DOUBLE));

	window = XVIEWER_WINDOW (user_data);

	zoom = g_variant_get_double (parameter);

	xviewer_debug_message (DEBUG_WINDOW, "Set zoom factor to %.4lf", zoom);

	if (window->priv->view) {
		xviewer_scroll_view_set_zoom (XVIEWER_SCROLL_VIEW (window->priv->view),
		                          zoom);
	}
}

static void
readonly_state_handler (GSimpleAction *action,
                        GVariant      *value,
                        gpointer       user_data)
{
	g_warning ("The state of action \"%s\" is read-only! Ignoring request!",
	           g_action_get_name (G_ACTION (action)));
}

static const GActionEntry window_actions[] = {
	{ "go-previous", xviewer_window_action_go_prev },
	{ "go-next",     xviewer_window_action_go_next },
	{ "go-first",    xviewer_window_action_go_first },
	{ "go-last",     xviewer_window_action_go_last },
	{ "go-random",   xviewer_window_action_go_random },
	{ "rotate-right", xviewer_window_action_rotate_90 },
	{ "rotate-left", xviewer_window_action_rotate_270 },
	{ "zoom-in",     xviewer_window_action_zoom_in },
	{ "zoom-out",    xviewer_window_action_zoom_out },
	{ "zoom-fit",    xviewer_window_action_zoom_best_fit },
	{ "zoom-set",    xviewer_window_action_set_zoom, "d" },
	{ "current-image", NULL, NULL, "@(ii) (0, 0)", readonly_state_handler }
};

static const GtkToggleActionEntry toggle_entries_gallery[] = {
	{ "ViewSlideshow", "slideshow-play", N_("S_lideshow"), "F5",
	  N_("Start a slideshow view of the images"),
	  G_CALLBACK (xviewer_window_cmd_slideshow), FALSE },
};

static void
menu_item_select_cb (GtkMenuItem *proxy, XviewerWindow *window)
{
	GtkAction *action;
	char *message;

	action = gtk_activatable_get_related_action (GTK_ACTIVATABLE (proxy));

	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy, XviewerWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  XviewerWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     XviewerWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
set_action_properties (XviewerWindow      *window,
                       GtkActionGroup *window_group,
                       GtkActionGroup *image_group,
                       GtkActionGroup *gallery_group)
{
	GtkAction *action;
	XviewerWindowPrivate *priv = window->priv;
	gboolean rtl;

	rtl = gtk_widget_get_direction (GTK_WIDGET (window)) == GTK_TEXT_DIR_RTL;

	action = gtk_action_group_get_action (gallery_group, "GoPrevious");
	g_object_set (action, "icon-name", rtl ? "go-previous-symbolic-rtl" : "go-previous-symbolic", NULL);
	g_object_set (action, "short_label", _("Previous"), NULL);
	g_object_set (action, "is-important", TRUE, NULL);

	action = gtk_action_group_get_action (gallery_group, "GoNext");
	g_object_set (action, "icon-name", rtl ? "go-next-symbolic-rtl" : "go-next-symbolic", NULL);
	g_object_set (action, "short_label", _("Next"), NULL);
	g_object_set (action, "is-important", TRUE, NULL);

	action = gtk_action_group_get_action (image_group, "EditUndo");
	g_object_set (action, "icon-name", rtl ? "edit-undo-symbolic-rtl" : "edit-undo-symbolic", NULL);

	action = gtk_action_group_get_action (image_group, "EditRotate90");
	g_object_set (action, "short_label", _("Right"), NULL);

	action = gtk_action_group_get_action (image_group, "EditRotate270");
	g_object_set (action, "short_label", _("Left"), NULL);

	action = gtk_action_group_get_action (image_group, "ImageOpenContainingFolder");
	g_object_set (action, "short_label", _("Show Folder"), NULL);

	action = gtk_action_group_get_action (image_group, "ViewZoomOut");
	g_object_set (action, "short_label", _("Out"), NULL);

	action = gtk_action_group_get_action (image_group, "ViewZoomIn");
	g_object_set (action, "short_label", _("In"), NULL);

	action = gtk_action_group_get_action (image_group, "ViewZoomNormal");
	g_object_set (action, "short_label", _("Normal"), NULL);

	action = gtk_action_group_get_action (image_group, "ViewZoomFit");
	g_object_set (action, "short_label", _("Fit"), NULL);

	action = gtk_action_group_get_action (window_group, "ViewImageGallery");
	g_object_set (action, "short_label", _("Gallery"), NULL);
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_IMAGE_GALLERY, action,
	                 "active", G_SETTINGS_BIND_GET);

	action = gtk_action_group_get_action (window_group, "ViewSidebar");
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_SIDEBAR, action,
	                 "active", G_SETTINGS_BIND_GET);

	action = gtk_action_group_get_action (window_group, "ViewStatusbar");
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_STATUSBAR, action,
	                 "active", G_SETTINGS_BIND_GET);

	action = gtk_action_group_get_action (window_group, "ViewToolbar");
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_TOOLBAR, action,
	                 "active", G_SETTINGS_BIND_GET);

	action = gtk_action_group_get_action (image_group, "EditMoveToTrash");
	g_object_set (action, "short_label", C_("action (to trash)", "Trash"), NULL);
}

static gint
sort_recents_mru (GtkRecentInfo *a, GtkRecentInfo *b)
{
	gboolean has_xviewer_a, has_xviewer_b;

	/* We need to check this first as gtk_recent_info_get_application_info
	 * will treat it as a non-fatal error when the GtkRecentInfo doesn't
	 * have the application registered. */
	has_xviewer_a = gtk_recent_info_has_application (a,
						     XVIEWER_RECENT_FILES_APP_NAME);
	has_xviewer_b = gtk_recent_info_has_application (b,
						     XVIEWER_RECENT_FILES_APP_NAME);
	if (has_xviewer_a && has_xviewer_b) {
		time_t time_a, time_b;

		/* These should not fail as we already checked that
		 * the application is registered with the info objects */
		gtk_recent_info_get_application_info (a,
						      XVIEWER_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_a);
		gtk_recent_info_get_application_info (b,
						      XVIEWER_RECENT_FILES_APP_NAME,
						      NULL,
						      NULL,
						      &time_b);

		return (time_b - time_a);
	} else if (has_xviewer_a) {
		return -1;
	} else if (has_xviewer_b) {
		return 1;
	}

	return 0;
}

static void
xviewer_window_update_recent_files_menu (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;
	GList *actions = NULL, *li = NULL, *items = NULL;
	guint count_recent = 0;

	priv = window->priv;

	if (priv->recent_menu_id != 0)
		gtk_ui_manager_remove_ui (priv->ui_mgr, priv->recent_menu_id);

	actions = gtk_action_group_list_actions (priv->actions_recent);

	for (li = actions; li != NULL; li = li->next) {
		g_signal_handlers_disconnect_by_func (GTK_ACTION (li->data),
						      G_CALLBACK(xviewer_window_open_recent_cb),
						      window);

		gtk_action_group_remove_action (priv->actions_recent,
						GTK_ACTION (li->data));
	}

	g_list_free (actions);

	priv->recent_menu_id = gtk_ui_manager_new_merge_id (priv->ui_mgr);
	items = gtk_recent_manager_get_items (gtk_recent_manager_get_default());
	items = g_list_sort (items, (GCompareFunc) sort_recents_mru);

	for (li = items; li != NULL && count_recent < XVIEWER_RECENT_FILES_LIMIT; li = li->next) {
		gchar *action_name;
		gchar *label;
		gchar *tip;
		gchar **display_name;
		gchar *label_filename;
		GtkAction *action;
		GtkRecentInfo *info = li->data;

		/* Sorting moves non-XVIEWER files to the end of the list.
		 * So no file of interest will follow if this test fails */
		if (!gtk_recent_info_has_application (info, XVIEWER_RECENT_FILES_APP_NAME))
			break;

		count_recent++;

		action_name = g_strdup_printf ("recent-info-%d", count_recent);
		display_name = g_strsplit (gtk_recent_info_get_display_name (info), "_", -1);
		label_filename = g_strjoinv ("__", display_name);
		label = g_strdup_printf ("%s_%d. %s",
				(is_rtl ? "\xE2\x80\x8F" : ""), count_recent, label_filename);
		g_free (label_filename);
		g_strfreev (display_name);

		tip = gtk_recent_info_get_uri_display (info);

		/* This is a workaround for a bug (#351945) regarding
		 * gtk_recent_info_get_uri_display() and remote URIs.
		 * gnome_vfs_format_uri_for_display is sufficient here
		 * since the password gets stripped when adding the
		 * file to the recently used list. */
		if (tip == NULL)
			tip = g_uri_unescape_string (gtk_recent_info_get_uri (info), NULL);

		action = gtk_action_new (action_name, label, tip, NULL);
		gtk_action_set_always_show_image (action, TRUE);

		g_object_set_data_full (G_OBJECT (action), "gtk-recent-info",
					gtk_recent_info_ref (info),
					(GDestroyNotify) gtk_recent_info_unref);

		g_object_set (G_OBJECT (action), "icon-name", "image-x-generic", NULL);

		g_signal_connect (action, "activate",
				  G_CALLBACK (xviewer_window_open_recent_cb),
				  window);

		gtk_action_group_add_action (priv->actions_recent, action);

		g_object_unref (action);

		gtk_ui_manager_add_ui (priv->ui_mgr, priv->recent_menu_id,
				       "/MainMenu/Image/RecentDocuments",
				       action_name, action_name,
				       GTK_UI_MANAGER_AUTO, FALSE);

		g_free (action_name);
		g_free (label);
		g_free (tip);
	}

	g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (items);
}

static void
xviewer_window_recent_manager_changed_cb (GtkRecentManager *manager, XviewerWindow *window)
{
	xviewer_window_update_recent_files_menu (window);
}

static void
xviewer_window_drag_data_received (GtkWidget *widget,
                               GdkDragContext *context,
                               gint x, gint y,
                               GtkSelectionData *selection_data,
                               guint info, guint time)
{
        GSList *file_list;
        XviewerWindow *window;
	GdkAtom target;
	GtkWidget *src;

	target = gtk_selection_data_get_target (selection_data);

        if (!gtk_targets_include_uri (&target, 1))
                return;

	/* if the request is from another process this will return NULL */
	src = gtk_drag_get_source_widget (context);

	/* if the drag request originates from the current xviewer instance, ignore
	   the request if the source window is the same as the dest window */
	if (src &&
	    gtk_widget_get_toplevel (src) == gtk_widget_get_toplevel (widget))
	{
		gdk_drag_status (context, 0, time);
		return;
	}

        if (gdk_drag_context_get_suggested_action (context) == GDK_ACTION_COPY)
        {
                window = XVIEWER_WINDOW (widget);

                file_list = xviewer_util_parse_uri_string_list_to_file_list ((const gchar *) gtk_selection_data_get_data (selection_data));

		xviewer_window_open_file_list (window, file_list);
        }
}

static void
xviewer_window_set_drag_dest (XviewerWindow *window)
{
        gtk_drag_dest_set (GTK_WIDGET (window),
                           GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_DROP,
                           NULL, 0,
                           GDK_ACTION_COPY | GDK_ACTION_ASK);
	gtk_drag_dest_add_uri_targets (GTK_WIDGET (window));
}

static void
xviewer_window_sidebar_visibility_changed (GtkWidget *widget, XviewerWindow *window)
{
	GtkAction *action;
	gboolean visible;

	visible = gtk_widget_get_visible (window->priv->sidebar);

	action = gtk_action_group_get_action (window->priv->actions_window,
					      "ViewSidebar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* Focus the image */
	if (!visible && window->priv->image != NULL)
		gtk_widget_grab_focus (window->priv->view);
}

static void
xviewer_window_sidebar_page_added (XviewerSidebar  *sidebar,
			       GtkWidget   *main_widget,
			       XviewerWindow   *window)
{
	if (xviewer_sidebar_get_n_pages (sidebar) == 1) {
		GtkAction *action;
		gboolean show;

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, TRUE);

		show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

		if (show)
			gtk_widget_show (GTK_WIDGET (sidebar));
	}
}
static void
xviewer_window_sidebar_page_removed (XviewerSidebar  *sidebar,
			         GtkWidget   *main_widget,
			         XviewerWindow   *window)
{
	if (xviewer_sidebar_is_empty (sidebar)) {
		GtkAction *action;

		gtk_widget_hide (GTK_WIDGET (sidebar));

		action = gtk_action_group_get_action (window->priv->actions_window,
						      "ViewSidebar");

		gtk_action_set_sensitive (action, FALSE);
	}
}

static void
xviewer_window_finish_saving (XviewerWindow *window)
{
	XviewerWindowPrivate *priv = window->priv;

	gtk_widget_set_sensitive (GTK_WIDGET (window), FALSE);

	do {
		gtk_main_iteration ();
	} while (priv->save_job != NULL);
}

static GAppInfo *
get_appinfo_for_editor (XviewerWindow *window)
{
	/* We want this function to always return the same thing, not
	 * just for performance reasons, but because if someone edits
	 * GConf while xviewer is running, the application could get into an
	 * inconsistent state.
	 */
	static GDesktopAppInfo *app_info = NULL;
	static gboolean initialised;

	if (!initialised) {
		gchar *editor;

		editor = g_settings_get_string (window->priv->ui_settings,
		                                XVIEWER_CONF_UI_EXTERNAL_EDITOR);

		if (editor != NULL) {
			app_info = g_desktop_app_info_new (editor);
		}

		initialised = TRUE;
		g_free (editor);
	}

	return (GAppInfo *) app_info;
}

static void
xviewer_window_view_rotation_changed_cb (XviewerScrollView *view,
				     gdouble        degrees,
				     XviewerWindow     *window)
{
	apply_transformation (window, xviewer_transform_rotate_new (degrees));
}

static void
xviewer_window_view_next_image_cb (XviewerScrollView *view,
			       XviewerWindow     *window)
{
	xviewer_window_cmd_go_next (NULL, window);
}

static void
xviewer_window_view_previous_image_cb (XviewerScrollView *view,
				   XviewerWindow     *window)
{
	xviewer_window_cmd_go_prev (NULL, window);
}

static void
xviewer_window_construct_ui (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;

	GError *error = NULL;

	GtkWidget *menubar;
	GtkWidget *thumb_popup;
	GtkWidget *view_popup;
	GtkWidget *hpaned;
	GtkWidget *menuitem;
	GtkWidget *tool_item;
	GtkWidget *tool_box;
	GtkWidget *box;
	GtkWidget *separator;
	GtkWidget *button;
	GtkAction *action = NULL;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	priv = window->priv;

	priv->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (window), priv->box);
	gtk_widget_show (priv->box);
	priv->ui_mgr = gtk_ui_manager_new ();

	priv->actions_window = gtk_action_group_new ("MenuActionsWindow");

	gtk_action_group_set_translation_domain (priv->actions_window,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_window,
				      action_entries_window,
				      G_N_ELEMENTS (action_entries_window),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_window,
					     toggle_entries_window,
					     G_N_ELEMENTS (toggle_entries_window),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_window, 0);

	priv->actions_image = gtk_action_group_new ("MenuActionsImage");
	gtk_action_group_set_translation_domain (priv->actions_image,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_image,
				      action_entries_image,
				      G_N_ELEMENTS (action_entries_image),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_image,
					     toggle_entries_image,
					     G_N_ELEMENTS (toggle_entries_image),
					     window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_image, 0);

	priv->actions_gallery = gtk_action_group_new ("MenuActionsGallery");
	gtk_action_group_set_translation_domain (priv->actions_gallery,
						 GETTEXT_PACKAGE);

	gtk_action_group_add_actions (priv->actions_gallery,
				      action_entries_gallery,
				      G_N_ELEMENTS (action_entries_gallery),
				      window);

	gtk_action_group_add_toggle_actions (priv->actions_gallery,
					     toggle_entries_gallery,
					     G_N_ELEMENTS (toggle_entries_gallery),
					     window);

	set_action_properties (window, priv->actions_window,
			       priv->actions_image,
			       priv->actions_gallery);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_gallery, 0);

	if (!gtk_ui_manager_add_ui_from_resource (priv->ui_mgr,
						  "/org/x/viewer/ui/xviewer-ui.xml",
						  &error)) {
                g_warning ("building menus failed: %s", error->message);
                g_error_free (error);
        }

	g_signal_connect (priv->ui_mgr, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_mgr, "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb), window);

	menubar = gtk_ui_manager_get_widget (priv->ui_mgr, "/MainMenu");
	g_assert (GTK_IS_WIDGET (menubar));
	gtk_box_pack_start (GTK_BOX (priv->box), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipHorizontal");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditFlipVertical");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate90");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	menuitem = gtk_ui_manager_get_widget (priv->ui_mgr,
			"/MainMenu/Edit/EditRotate270");
	gtk_image_menu_item_set_always_show_image (
			GTK_IMAGE_MENU_ITEM (menuitem), TRUE);

	priv->toolbar_revealer = gtk_revealer_new ();
	gtk_revealer_set_transition_duration (GTK_REVEALER (priv->toolbar_revealer), 175);

	priv->toolbar = gtk_toolbar_new ();
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (priv->toolbar)),
				     			 GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
	gtk_container_add (GTK_CONTAINER (priv->toolbar_revealer), priv->toolbar);

	tool_item = gtk_tool_item_new ();
	gtk_tool_item_set_expand (GTK_TOOL_ITEM (tool_item), TRUE);
	gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), GTK_TOOL_ITEM (tool_item), 0);

	tool_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_container_add (GTK_CONTAINER (tool_item), tool_box);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tool_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoPrevious");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_gallery, "GoNext");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (tool_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tool_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomOut");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomIn");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomNormal");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "ViewZoomFit");
	button = create_toolbar_toggle_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	separator = gtk_separator_new (GTK_ORIENTATION_VERTICAL);
	gtk_box_pack_start (GTK_BOX (tool_box), separator, FALSE, FALSE, 0);

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tool_box), box, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "EditRotate270");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	action = gtk_action_group_get_action (priv->actions_image, "EditRotate90");
	button = create_toolbar_button (action);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (priv->box), priv->toolbar_revealer, FALSE, FALSE, 0);

	gtk_widget_show_all (priv->toolbar_revealer);

	gtk_window_add_accel_group (GTK_WINDOW (window), gtk_ui_manager_get_accel_group (priv->ui_mgr));

	priv->actions_recent = gtk_action_group_new ("RecentFilesActions");
	gtk_action_group_set_translation_domain (priv->actions_recent,
						 GETTEXT_PACKAGE);

	g_signal_connect (gtk_recent_manager_get_default (), "changed",
			  G_CALLBACK (xviewer_window_recent_manager_changed_cb),
			  window);

	xviewer_window_update_recent_files_menu (window);

	gtk_ui_manager_insert_action_group (priv->ui_mgr, priv->actions_recent, 0);

	priv->cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (priv->box), priv->cbox, TRUE, TRUE, 0);
	gtk_widget_show (priv->cbox);

	priv->statusbar = xviewer_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (priv->box),
			  GTK_WIDGET (priv->statusbar),
			  FALSE, FALSE, 0);
	gtk_widget_show (priv->statusbar);

	priv->image_info_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "image_info_message");
	priv->tip_message_cid =
		gtk_statusbar_get_context_id (GTK_STATUSBAR (priv->statusbar),
					      "tip_message");

	priv->layout = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);

	hpaned = gtk_paned_new (GTK_ORIENTATION_HORIZONTAL);

	priv->sidebar = xviewer_sidebar_new ();
	/* The sidebar shouldn't be shown automatically on show_all(),
	   but only when the user actually wants it. */
	gtk_widget_set_no_show_all (priv->sidebar, TRUE);

	gtk_widget_set_size_request (priv->sidebar, 210, -1);

	g_signal_connect_after (priv->sidebar,
				"show",
				G_CALLBACK (xviewer_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"hide",
				G_CALLBACK (xviewer_window_sidebar_visibility_changed),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-added",
				G_CALLBACK (xviewer_window_sidebar_page_added),
				window);

	g_signal_connect_after (priv->sidebar,
				"page-removed",
				G_CALLBACK (xviewer_window_sidebar_page_removed),
				window);
	priv->overlay = gtk_overlay_new();

 	priv->view = xviewer_scroll_view_new ();
	g_signal_connect (priv->view,
			  "rotation-changed",
			  G_CALLBACK (xviewer_window_view_rotation_changed_cb),
			  window);
	g_signal_connect (priv->view,
			  "next-image",
			  G_CALLBACK (xviewer_window_view_next_image_cb),
			  window);
	g_signal_connect (priv->view,
			  "previous-image",
			  G_CALLBACK (xviewer_window_view_previous_image_cb),
			  window);

	gtk_container_add (GTK_CONTAINER(priv->overlay), priv->view);

	xviewer_sidebar_add_page (XVIEWER_SIDEBAR (priv->sidebar),
			      _("Image Properties"),
			      GTK_WIDGET (xviewer_metadata_sidebar_new (window)));

	gtk_widget_set_size_request (GTK_WIDGET (priv->view), 100, 100);
	g_signal_connect (G_OBJECT (priv->view),
			  "zoom_changed",
			  G_CALLBACK (view_zoom_changed_cb),
			  window);
	action = gtk_action_group_get_action (priv->actions_image,
					      "ViewZoomFit");
	if (action != NULL) {
		/* Binding will be destroyed when the objects finalize */
		g_object_bind_property_full (priv->view, "zoom-mode",
					     action, "active",
					     G_BINDING_SYNC_CREATE,
					     _xviewer_zoom_shrink_to_boolean,
					     NULL, NULL, NULL);
	}
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_SCROLL_WHEEL_ZOOM,
			 priv->view, "scrollwheel-zoom", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_ZOOM_MULTIPLIER,
			 priv->view, "zoom-multiplier", G_SETTINGS_BIND_GET);

	view_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ViewPopup");
	xviewer_scroll_view_set_popup (XVIEWER_SCROLL_VIEW (priv->view),
				   GTK_MENU (view_popup));

	gtk_paned_pack1 (GTK_PANED (hpaned),
			 priv->sidebar,
			 FALSE,
			 FALSE);

	gtk_paned_pack2 (GTK_PANED (hpaned),
			 priv->overlay,
			 TRUE,
			 FALSE);

	gtk_widget_show_all (hpaned);

	gtk_box_pack_start (GTK_BOX (priv->layout), hpaned, TRUE, TRUE, 0);

	priv->thumbview = g_object_ref (xviewer_thumb_view_new ());

	/* giving shape to the view */
	gtk_icon_view_set_margin (GTK_ICON_VIEW (priv->thumbview), 4);
	gtk_icon_view_set_row_spacing (GTK_ICON_VIEW (priv->thumbview), 0);

	g_signal_connect (G_OBJECT (priv->thumbview), "selection_changed",
			  G_CALLBACK (handle_image_selection_changed_cb), window);

	priv->nav = xviewer_thumb_nav_new (priv->thumbview,
				       XVIEWER_THUMB_NAV_MODE_ONE_ROW,
				       g_settings_get_boolean (priv->ui_settings
				       	, XVIEWER_CONF_UI_SCROLL_BUTTONS));

	// Bind the scroll buttons to their GSettings key
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_SCROLL_BUTTONS,
			 priv->nav, "show-buttons", G_SETTINGS_BIND_GET);

	thumb_popup = gtk_ui_manager_get_widget (priv->ui_mgr, "/ThumbnailPopup");
	xviewer_thumb_view_set_thumbnail_popup (XVIEWER_THUMB_VIEW (priv->thumbview),
					    GTK_MENU (thumb_popup));

	gtk_box_pack_start (GTK_BOX (priv->layout), priv->nav, FALSE, FALSE, 0);

	gtk_box_pack_end (GTK_BOX (priv->cbox), priv->layout, TRUE, TRUE, 0);

	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_IMAGE_GALLERY_POSITION,
			 window, "gallery-position", G_SETTINGS_BIND_GET);
	g_settings_bind (priv->ui_settings, XVIEWER_CONF_UI_IMAGE_GALLERY_RESIZABLE,
			 window, "gallery-resizable", G_SETTINGS_BIND_GET);

	g_signal_connect (priv->lockdown_settings,
			  "changed::" XVIEWER_CONF_DESKTOP_CAN_SAVE,
			  G_CALLBACK (xviewer_window_can_save_changed_cb), window);
	// Call callback once to have the value set
	xviewer_window_can_save_changed_cb (priv->lockdown_settings,
					XVIEWER_CONF_DESKTOP_CAN_SAVE, window);

	update_action_groups_state (window);

	if ((priv->flags & XVIEWER_STARTUP_FULLSCREEN) ||
	    (priv->flags & XVIEWER_STARTUP_SLIDE_SHOW)) {
		xviewer_window_run_fullscreen (window, (priv->flags & XVIEWER_STARTUP_SLIDE_SHOW));
	} else {
		priv->mode = XVIEWER_WINDOW_MODE_NORMAL;
		update_ui_visibility (window);
	}

	xviewer_window_set_drag_dest (window);
}

static void
xviewer_window_init (XviewerWindow *window)
{
	GdkGeometry hints;
	XviewerWindowPrivate *priv;
	GAction* action;

	xviewer_debug (DEBUG_WINDOW);

	hints.min_width  = XVIEWER_WINDOW_MIN_WIDTH;
	hints.min_height = XVIEWER_WINDOW_MIN_HEIGHT;

	priv = window->priv = xviewer_window_get_instance_private (window);

	priv->fullscreen_settings = g_settings_new (XVIEWER_CONF_FULLSCREEN);
	priv->ui_settings = g_settings_new (XVIEWER_CONF_UI);
	priv->view_settings = g_settings_new (XVIEWER_CONF_VIEW);
	priv->lockdown_settings = g_settings_new (XVIEWER_CONF_DESKTOP_LOCKDOWN_SCHEMA);

	window->priv->store = NULL;
	window->priv->image = NULL;

	window->priv->fullscreen_popup = NULL;
	window->priv->fullscreen_timeout_source = NULL;
	window->priv->slideshow_loop = FALSE;
	window->priv->slideshow_switch_timeout = 0;
	window->priv->slideshow_switch_source = NULL;
	window->priv->fullscreen_idle_inhibit_cookie = 0;

	gtk_window_set_geometry_hints (GTK_WINDOW (window),
				       GTK_WIDGET (window),
				       &hints,
				       GDK_HINT_MIN_SIZE);

	gtk_window_set_default_size (GTK_WINDOW (window),
				     XVIEWER_WINDOW_DEFAULT_WIDTH,
				     XVIEWER_WINDOW_DEFAULT_HEIGHT);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	window->priv->mode = XVIEWER_WINDOW_MODE_UNKNOWN;
	window->priv->status = XVIEWER_WINDOW_STATUS_UNKNOWN;

#if defined(HAVE_LCMS) && defined(GDK_WINDOWING_X11)
	window->priv->display_profile =
		xviewer_window_get_display_profile (GTK_WIDGET (window));
#endif

	window->priv->recent_menu_id = 0;

	window->priv->gallery_position = 0;
	window->priv->gallery_resizable = FALSE;

	window->priv->save_disabled = FALSE;

	window->priv->page_setup = NULL;

	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (XVIEWER_APP));

	g_action_map_add_action_entries (G_ACTION_MAP (window),
	                                 window_actions, G_N_ELEMENTS (window_actions),
	                                 window);

	action = g_action_map_lookup_action (G_ACTION_MAP (window),
	                                     "current-image");
	if (G_LIKELY (action != NULL))
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), FALSE);

	g_signal_connect (GTK_WINDOW (window), "button-press-event",
			  G_CALLBACK (on_button_pressed), window);
}

static void
xviewer_window_dispose (GObject *object)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;

	g_return_if_fail (object != NULL);
	g_return_if_fail (XVIEWER_IS_WINDOW (object));

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (object);
	priv = window->priv;

	peas_engine_garbage_collect (PEAS_ENGINE (XVIEWER_APP->priv->plugin_engine));

	if (priv->extensions != NULL) {
		g_object_unref (priv->extensions);
		priv->extensions = NULL;
		peas_engine_garbage_collect (PEAS_ENGINE (XVIEWER_APP->priv->plugin_engine));
	}

	if (priv->store != NULL) {
		g_signal_handlers_disconnect_by_func (priv->store,
					      xviewer_window_list_store_image_added,
					      window);
		g_signal_handlers_disconnect_by_func (priv->store,
					    xviewer_window_list_store_image_removed,
					    window);
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	if (priv->image != NULL) {
	  	g_signal_handlers_disconnect_by_func (priv->image,
						      image_thumb_changed_cb,
						      window);
		g_signal_handlers_disconnect_by_func (priv->image,
						      image_file_changed_cb,
						      window);
		g_object_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->actions_window != NULL) {
		g_object_unref (priv->actions_window);
		priv->actions_window = NULL;
	}

	if (priv->actions_image != NULL) {
		g_object_unref (priv->actions_image);
		priv->actions_image = NULL;
	}

	if (priv->actions_gallery != NULL) {
		g_object_unref (priv->actions_gallery);
		priv->actions_gallery = NULL;
	}

	if (priv->actions_recent != NULL) {
		g_object_unref (priv->actions_recent);
		priv->actions_recent = NULL;
	}

        if (priv->actions_open_with != NULL) {
                g_object_unref (priv->actions_open_with);
                priv->actions_open_with = NULL;
        }

	fullscreen_clear_timeout (window);

	if (window->priv->fullscreen_popup != NULL) {
		gtk_widget_destroy (priv->fullscreen_popup);
		priv->fullscreen_popup = NULL;
	}

	slideshow_clear_timeout (window);
	xviewer_window_uninhibit_screensaver (window);

	g_signal_handlers_disconnect_by_func (gtk_recent_manager_get_default (),
					      G_CALLBACK (xviewer_window_recent_manager_changed_cb),
					      window);

	priv->recent_menu_id = 0;

	xviewer_window_clear_load_job (window);

	xviewer_window_clear_transform_job (window);

	if (priv->view_settings) {
		g_object_unref (priv->view_settings);
		priv->view_settings = NULL;
	}

	if (priv->ui_settings) {
		g_object_unref (priv->ui_settings);
		priv->ui_settings = NULL;
	}

	if (priv->fullscreen_settings) {
		g_object_unref (priv->fullscreen_settings);
		priv->fullscreen_settings = NULL;
	}

	if (priv->lockdown_settings) {
		g_object_unref (priv->lockdown_settings);
		priv->lockdown_settings = NULL;
	}

	if (priv->file_list != NULL) {
		g_slist_foreach (priv->file_list, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->file_list);
		priv->file_list = NULL;
	}

#ifdef HAVE_LCMS
	if (priv->display_profile != NULL) {
		cmsCloseProfile (priv->display_profile);
		priv->display_profile = NULL;
	}
#endif

	if (priv->last_save_as_folder != NULL) {
		g_object_unref (priv->last_save_as_folder);
		priv->last_save_as_folder = NULL;
	}

	if (priv->page_setup != NULL) {
		g_object_unref (priv->page_setup);
		priv->page_setup = NULL;
	}

	if (priv->thumbview)
	{
		/* Disconnect so we don't get any unwanted callbacks
		 * when the thumb view is disposed. */
		g_signal_handlers_disconnect_by_func (priv->thumbview,
		                 G_CALLBACK (handle_image_selection_changed_cb),
		                 window);
		g_clear_object (&priv->thumbview);
	}

	peas_engine_garbage_collect (PEAS_ENGINE (XVIEWER_APP->priv->plugin_engine));

	G_OBJECT_CLASS (xviewer_window_parent_class)->dispose (object);
}

static gint
xviewer_window_delete (GtkWidget *widget, GdkEventAny *event)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;

	g_return_val_if_fail (XVIEWER_IS_WINDOW (widget), FALSE);

	window = XVIEWER_WINDOW (widget);
	priv = window->priv;

	if (priv->save_job != NULL) {
		xviewer_window_finish_saving (window);
	}

	if (xviewer_window_unsaved_images_confirm (window)) {
		return TRUE;
	}

	gtk_widget_destroy (widget);

	return TRUE;
}

static gint
xviewer_window_key_press (GtkWidget *widget, GdkEventKey *event)
{
	GtkContainer *tbcontainer = GTK_CONTAINER ((XVIEWER_WINDOW (widget)->priv->toolbar));
	gint result = FALSE;
	gboolean handle_selection = FALSE;
	GdkModifierType modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();

	switch (event->keyval) {
	case GDK_KEY_space:
		if ((event->state & modifiers) == GDK_CONTROL_MASK) {
			handle_selection = TRUE;
			break;
		}
	case GDK_KEY_Return:
		if (gtk_container_get_focus_child (tbcontainer) == NULL) {
			/* Image properties dialog case */
			if ((event->state & modifiers) == GDK_MOD1_MASK) {
				result = FALSE;
				break;
			}

			if ((event->state & modifiers) == GDK_SHIFT_MASK) {
				xviewer_window_cmd_go_prev (NULL, XVIEWER_WINDOW (widget));
			} else {
				xviewer_window_cmd_go_next (NULL, XVIEWER_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case GDK_KEY_p:
	case GDK_KEY_P:
		if (XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_FULLSCREEN || XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW) {
			gboolean slideshow;

			slideshow = XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW;
			xviewer_window_run_fullscreen (XVIEWER_WINDOW (widget), !slideshow);
		}
		break;
	case GDK_KEY_Q:
	case GDK_KEY_q:
		if ((event->state & modifiers) == GDK_CONTROL_MASK) {
			xviewer_window_cmd_close_window (NULL, XVIEWER_WINDOW (widget));
			return TRUE;
		}
		break;
	case GDK_KEY_Escape:
		if (XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_FULLSCREEN) {
			xviewer_window_stop_fullscreen (XVIEWER_WINDOW (widget), FALSE);
		} else if (XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW) {
			xviewer_window_stop_fullscreen (XVIEWER_WINDOW (widget), TRUE);
		} else {
			xviewer_window_cmd_close_window (NULL, XVIEWER_WINDOW (widget));
			return TRUE;
		}
		break;
	case GDK_KEY_Left:
	case GDK_KEY_Up:
		if ((event->state & modifiers) == 0) {
			/* Left and Up move to previous image */
			if (is_rtl) { /* move to next in RTL mode */
				xviewer_window_cmd_go_next (NULL, XVIEWER_WINDOW (widget));
			} else {
				xviewer_window_cmd_go_prev (NULL, XVIEWER_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case GDK_KEY_Right:
	case GDK_KEY_Down:
		if ((event->state & modifiers) == 0) {
			/* Right and Down move to next image */
			if (is_rtl) { /* move to previous in RTL mode */
				xviewer_window_cmd_go_prev (NULL, XVIEWER_WINDOW (widget));
			} else {
				xviewer_window_cmd_go_next (NULL, XVIEWER_WINDOW (widget));
			}
			result = TRUE;
		}
		break;
	case GDK_KEY_Page_Up:
		if ((event->state & modifiers) == 0) {
			if (!xviewer_scroll_view_scrollbars_visible (XVIEWER_SCROLL_VIEW (XVIEWER_WINDOW (widget)->priv->view))) {
				if (!gtk_widget_get_visible (XVIEWER_WINDOW (widget)->priv->nav)) {
					/* If the iconview is not visible skip to the
					 * previous image manually as it won't handle
					 * the keypress then. */
					xviewer_window_cmd_go_prev (NULL,
								XVIEWER_WINDOW (widget));
					result = TRUE;
				} else
					handle_selection = TRUE;
			}
		}
		break;
	case GDK_KEY_Page_Down:
		if ((event->state & modifiers) == 0) {
			if (!xviewer_scroll_view_scrollbars_visible (XVIEWER_SCROLL_VIEW (XVIEWER_WINDOW (widget)->priv->view))) {
				if (!gtk_widget_get_visible (XVIEWER_WINDOW (widget)->priv->nav)) {
					/* If the iconview is not visible skip to the
					 * next image manually as it won't handle
					 * the keypress then. */
					xviewer_window_cmd_go_next (NULL,
								XVIEWER_WINDOW (widget));
					result = TRUE;
				} else
					handle_selection = TRUE;
			}
		}
		break;
	}

	/* Update slideshow timeout */
	if (result && (XVIEWER_WINDOW (widget)->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW)) {
		slideshow_set_timeout (XVIEWER_WINDOW (widget));
	}

	if (handle_selection == TRUE && result == FALSE) {
		gtk_widget_grab_focus (GTK_WIDGET (XVIEWER_WINDOW (widget)->priv->thumbview));

		result = gtk_widget_event (GTK_WIDGET (XVIEWER_WINDOW (widget)->priv->thumbview),
					   (GdkEvent *) event);
	}

	/* If the focus is not in the toolbar and we still haven't handled the
	   event, give the scrollview a chance to do it.  */
	if (!gtk_container_get_focus_child (tbcontainer) && result == FALSE &&
		gtk_widget_get_realized (GTK_WIDGET (XVIEWER_WINDOW (widget)->priv->view))) {
			result = gtk_widget_event (GTK_WIDGET (XVIEWER_WINDOW (widget)->priv->view),
						   (GdkEvent *) event);
	}

	if (result == FALSE && GTK_WIDGET_CLASS (xviewer_window_parent_class)->key_press_event) {
		result = (* GTK_WIDGET_CLASS (xviewer_window_parent_class)->key_press_event) (widget, event);
	}

	return result;
}

static gint
xviewer_window_button_press (GtkWidget *widget, GdkEventButton *event)
{
	XviewerWindow *window = XVIEWER_WINDOW (widget);
	gint result = FALSE;

	/* We currently can't tell whether the old button codes (6, 7) are
	 * still in use. So we keep them in addition to the new ones (8, 9)
	 */
	if (event->type == GDK_BUTTON_PRESS) {
		switch (event->button) {
		case 6:
		case 8:
			xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (window->priv->thumbview),
						      XVIEWER_THUMB_VIEW_SELECT_LEFT);
			result = TRUE;
		       	break;
		case 7:
		case 9:
			xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (window->priv->thumbview),
						      XVIEWER_THUMB_VIEW_SELECT_RIGHT);
			result = TRUE;
		       	break;
		}
	}

	if (result == FALSE && GTK_WIDGET_CLASS (xviewer_window_parent_class)->button_press_event) {
		result = (* GTK_WIDGET_CLASS (xviewer_window_parent_class)->button_press_event) (widget, event);
	}

	return result;
}

static gboolean
xviewer_window_focus_out_event (GtkWidget *widget, GdkEventFocus *event)
{
	XviewerWindow *window = XVIEWER_WINDOW (widget);
	XviewerWindowPrivate *priv = window->priv;
	gboolean fullscreen;

	xviewer_debug (DEBUG_WINDOW);

	fullscreen = priv->mode == XVIEWER_WINDOW_MODE_FULLSCREEN ||
		     priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW;

	if (fullscreen) {
		gtk_widget_hide (priv->fullscreen_popup);
	}

	return GTK_WIDGET_CLASS (xviewer_window_parent_class)->focus_out_event (widget, event);
}

static void
xviewer_window_set_property (GObject      *object,
			 guint         property_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;

        g_return_if_fail (XVIEWER_IS_WINDOW (object));

        window = XVIEWER_WINDOW (object);
	priv = window->priv;

        switch (property_id) {
	case PROP_GALLERY_POS:
		xviewer_window_set_gallery_mode (window, g_value_get_enum (value),
					     priv->gallery_resizable);
		break;
	case PROP_GALLERY_RESIZABLE:
		xviewer_window_set_gallery_mode (window, priv->gallery_position,
					     g_value_get_boolean (value));
		break;
	case PROP_STARTUP_FLAGS:
		priv->flags = g_value_get_flags (value);
		break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        }
}

static void
xviewer_window_get_property (GObject    *object,
			 guint       property_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;

        g_return_if_fail (XVIEWER_IS_WINDOW (object));

        window = XVIEWER_WINDOW (object);
	priv = window->priv;

        switch (property_id) {
	case PROP_GALLERY_POS:
		g_value_set_enum (value, priv->gallery_position);
		break;
	case PROP_GALLERY_RESIZABLE:
		g_value_set_boolean (value, priv->gallery_resizable);
		break;
	case PROP_STARTUP_FLAGS:
		g_value_set_flags (value, priv->flags);
		break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
on_extension_added (PeasExtensionSet *set,
		    PeasPluginInfo   *info,
		    PeasExtension    *exten,
		    GtkWindow        *window)
{
	peas_extension_call (exten, "activate", window);
}

static void
on_extension_removed (PeasExtensionSet *set,
		      PeasPluginInfo   *info,
		      PeasExtension    *exten,
		      GtkWindow        *window)
{
	peas_extension_call (exten, "deactivate", window);
}

static GObject *
xviewer_window_constructor (GType type,
			guint n_construct_properties,
			GObjectConstructParam *construct_params)
{
	GObject *object;
	XviewerWindowPrivate *priv;

	object = G_OBJECT_CLASS (xviewer_window_parent_class)->constructor
			(type, n_construct_properties, construct_params);

	priv = XVIEWER_WINDOW (object)->priv;

	xviewer_window_construct_ui (XVIEWER_WINDOW (object));

	priv->extensions = peas_extension_set_new (PEAS_ENGINE (XVIEWER_APP->priv->plugin_engine),
						   XVIEWER_TYPE_WINDOW_ACTIVATABLE,
						   "window",
						   XVIEWER_WINDOW (object), NULL);
	peas_extension_set_call (priv->extensions, "activate");
	g_signal_connect (priv->extensions, "extension-added",
			  G_CALLBACK (on_extension_added), object);
	g_signal_connect (priv->extensions, "extension-removed",
			  G_CALLBACK (on_extension_removed), object);

	return object;
}

static void
xviewer_window_class_init (XviewerWindowClass *class)
{
	GObjectClass *g_object_class = (GObjectClass *) class;
	GtkWidgetClass *widget_class = (GtkWidgetClass *) class;

	g_object_class->constructor = xviewer_window_constructor;
	g_object_class->dispose = xviewer_window_dispose;
	g_object_class->set_property = xviewer_window_set_property;
	g_object_class->get_property = xviewer_window_get_property;

	widget_class->delete_event = xviewer_window_delete;
	widget_class->key_press_event = xviewer_window_key_press;
	widget_class->button_press_event = xviewer_window_button_press;
	widget_class->drag_data_received = xviewer_window_drag_data_received;
	widget_class->focus_out_event = xviewer_window_focus_out_event;

/**
 * XviewerWindow:gallery-position:
 *
 * Determines the position of the image gallery in the window
 * relative to the image.
 */
	g_object_class_install_property (
		g_object_class, PROP_GALLERY_POS,
		g_param_spec_enum ("gallery-position", NULL, NULL,
				   XVIEWER_TYPE_WINDOW_GALLERY_POS,
				   XVIEWER_WINDOW_GALLERY_POS_BOTTOM,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * XviewerWindow:gallery-resizable:
 *
 * If %TRUE the gallery will be resizable by the user otherwise it will be
 * in single column/row mode.
 */
	g_object_class_install_property (
		g_object_class, PROP_GALLERY_RESIZABLE,
		g_param_spec_boolean ("gallery-resizable", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

/**
 * XviewerWindow:startup-flags:
 *
 * A bitwise OR of #XviewerStartupFlags elements, indicating how the window
 * should behave upon creation.
 */
	g_object_class_install_property (g_object_class,
					 PROP_STARTUP_FLAGS,
					 g_param_spec_flags ("startup-flags",
							     NULL,
							     NULL,
							     XVIEWER_TYPE_STARTUP_FLAGS,
					 		     0,
					 		     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));

/**
 * XviewerWindow::prepared:
 * @window: the object which received the signal.
 *
 * The #XviewerWindow::prepared signal is emitted when the @window is ready
 * to be shown.
 */
	signals [SIGNAL_PREPARED] =
		g_signal_new ("prepared",
			      XVIEWER_TYPE_WINDOW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerWindowClass, prepared),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
}

/**
 * xviewer_window_new:
 * @flags: the initialization parameters for the new window.
 *
 *
 * Creates a new and empty #XviewerWindow. Use @flags to indicate
 * if the window should be initialized fullscreen, in slideshow mode,
 * and/or without the thumbnails gallery visible. See #XviewerStartupFlags.
 *
 * Returns: a newly created #XviewerWindow.
 **/
GtkWidget*
xviewer_window_new (XviewerStartupFlags flags)
{
	XviewerWindow *window;

	xviewer_debug (DEBUG_WINDOW);

	window = XVIEWER_WINDOW (g_object_new (XVIEWER_TYPE_WINDOW,
					   "type", GTK_WINDOW_TOPLEVEL,
	                                   "application", XVIEWER_APP,
	                                   "show-menubar", FALSE,
					   "startup-flags", flags,
					   NULL));

	return GTK_WIDGET (window);
}

static void
xviewer_window_list_store_image_added (GtkTreeModel *tree_model,
                                   GtkTreePath  *path,
                                   GtkTreeIter  *iter,
                                   gpointer      user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
xviewer_window_list_store_image_removed (GtkTreeModel *tree_model,
                                     GtkTreePath  *path,
                                     gpointer      user_data)
{
	XviewerWindow *window = XVIEWER_WINDOW (user_data);

	update_image_pos (window);
	update_action_groups_state (window);
}

static void
xviewer_job_model_cb (XviewerJobModel *job, gpointer data)
{
	XviewerWindow *window;
	XviewerWindowPrivate *priv;
	gint n_images;

	xviewer_debug (DEBUG_WINDOW);

#ifdef HAVE_EXIF
        int i;
	XviewerImage *image;
#endif

        g_return_if_fail (XVIEWER_IS_WINDOW (data));

	window = XVIEWER_WINDOW (data);
	priv = window->priv;

	if (priv->store != NULL) {
		g_object_unref (priv->store);
		priv->store = NULL;
	}

	priv->store = g_object_ref (job->store);

	n_images = xviewer_list_store_length (XVIEWER_LIST_STORE (priv->store));

#ifdef HAVE_EXIF
	if (g_settings_get_boolean (priv->view_settings, XVIEWER_CONF_VIEW_AUTOROTATE)) {
		for (i = 0; i < n_images; i++) {
			image = xviewer_list_store_get_image_by_pos (priv->store, i);
			xviewer_image_autorotate (image);
			g_object_unref (image);
		}
	}
#endif

	xviewer_thumb_view_set_model (XVIEWER_THUMB_VIEW (priv->thumbview), priv->store);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-inserted",
			  G_CALLBACK (xviewer_window_list_store_image_added),
			  window);

	g_signal_connect (G_OBJECT (priv->store),
			  "row-deleted",
			  G_CALLBACK (xviewer_window_list_store_image_removed),
			  window);

	if (n_images == 0) {
		gint n_files;

		priv->status = XVIEWER_WINDOW_STATUS_NORMAL;
		update_action_groups_state (window);

		n_files = g_slist_length (priv->file_list);

		if (n_files > 0) {
			GtkWidget *message_area;
			GFile *file = NULL;

			if (n_files == 1) {
				file = (GFile *) priv->file_list->data;
			}

			message_area = xviewer_no_images_error_message_area_new (file);

			xviewer_window_set_message_area (window, message_area);

			gtk_widget_show (message_area);
		}

		g_signal_emit (window, signals[SIGNAL_PREPARED], 0);
	}
}

/**
 * xviewer_window_open_file_list:
 * @window: An #XviewerWindow.
 * @file_list: (element-type GFile): A %NULL-terminated list of #GFile's.
 *
 * Opens a list of files, adding them to the gallery in @window.
 * Files will be checked to be readable and later filtered according
 * with xviewer_list_store_add_files().
 **/
void
xviewer_window_open_file_list (XviewerWindow *window, GSList *file_list)
{
	XviewerJob *job;

	xviewer_debug (DEBUG_WINDOW);

	window->priv->status = XVIEWER_WINDOW_STATUS_INIT;

	g_slist_foreach (file_list, (GFunc) g_object_ref, NULL);
	window->priv->file_list = file_list;

	job = xviewer_job_model_new (file_list);

	g_signal_connect (job,
			  "finished",
			  G_CALLBACK (xviewer_job_model_cb),
			  window);

	xviewer_job_scheduler_add_job (job);
	g_object_unref (job);
}

/**
 * xviewer_window_get_ui_manager:
 * @window: An #XviewerWindow.
 *
 * Gets the #GtkUIManager that describes the UI of @window.
 *
 * Returns: (transfer none): A #GtkUIManager.
 **/
GtkUIManager *
xviewer_window_get_ui_manager (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->ui_mgr;
}

/**
 * xviewer_window_get_mode:
 * @window: An #XviewerWindow.
 *
 * Gets the mode of @window. See #XviewerWindowMode for details.
 *
 * Returns: An #XviewerWindowMode.
 **/
XviewerWindowMode
xviewer_window_get_mode (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), XVIEWER_WINDOW_MODE_UNKNOWN);

	return window->priv->mode;
}

/**
 * xviewer_window_set_mode:
 * @window: an #XviewerWindow.
 * @mode: an #XviewerWindowMode value.
 *
 * Changes the mode of @window to normal, fullscreen, or slideshow.
 * See #XviewerWindowMode for details.
 **/
void
xviewer_window_set_mode (XviewerWindow *window, XviewerWindowMode mode)
{
        g_return_if_fail (XVIEWER_IS_WINDOW (window));

	if (window->priv->mode == mode)
		return;

	switch (mode) {
	case XVIEWER_WINDOW_MODE_NORMAL:
		xviewer_window_stop_fullscreen (window,
					    window->priv->mode == XVIEWER_WINDOW_MODE_SLIDESHOW);
		break;
	case XVIEWER_WINDOW_MODE_FULLSCREEN:
		xviewer_window_run_fullscreen (window, FALSE);
		break;
	case XVIEWER_WINDOW_MODE_SLIDESHOW:
		xviewer_window_run_fullscreen (window, TRUE);
		break;
	case XVIEWER_WINDOW_MODE_UNKNOWN:
		break;
	}
}

/**
 * xviewer_window_get_store:
 * @window: An #XviewerWindow.
 *
 * Gets the #XviewerListStore that contains the images in the gallery
 * of @window.
 *
 * Returns: (transfer none): an #XviewerListStore.
 **/
XviewerListStore *
xviewer_window_get_store (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return XVIEWER_LIST_STORE (window->priv->store);
}

/**
 * xviewer_window_get_view:
 * @window: An #XviewerWindow.
 *
 * Gets the #XviewerScrollView in the window.
 *
 * Returns: (transfer none): the #XviewerScrollView.
 **/
GtkWidget *
xviewer_window_get_view (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

       return window->priv->view;
}

/**
 * xviewer_window_get_sidebar:
 * @window: An #XviewerWindow.
 *
 * Gets the sidebar widget of @window.
 *
 * Returns: (transfer none): the #XviewerSidebar.
 **/
GtkWidget *
xviewer_window_get_sidebar (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->sidebar;
}

/**
 * xviewer_window_get_thumb_view:
 * @window: an #XviewerWindow.
 *
 * Gets the thumbnails view in @window.
 *
 * Returns: (transfer none): an #XviewerThumbView.
 **/
GtkWidget *
xviewer_window_get_thumb_view (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->thumbview;
}

/**
 * xviewer_window_get_thumb_nav:
 * @window: an #XviewerWindow.
 *
 * Gets the thumbnails navigation pane in @window.
 *
 * Returns: (transfer none): an #XviewerThumbNav.
 **/
GtkWidget *
xviewer_window_get_thumb_nav (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->nav;
}

/**
 * xviewer_window_get_statusbar:
 * @window: an #XviewerWindow.
 *
 * Gets the statusbar in @window.
 *
 * Returns: (transfer none): a #XviewerStatusbar.
 **/
GtkWidget *
xviewer_window_get_statusbar (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->statusbar;
}

/**
 * xviewer_window_get_image:
 * @window: an #XviewerWindow.
 *
 * Gets the image currently displayed in @window or %NULL if
 * no image is being displayed.
 *
 * Returns: (transfer none): an #XviewerImage.
 **/
XviewerImage *
xviewer_window_get_image (XviewerWindow *window)
{
        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), NULL);

	return window->priv->image;
}

/**
 * xviewer_window_is_empty:
 * @window: an #XviewerWindow.
 *
 * Tells whether @window is currently empty or not.
 *
 * Returns: %TRUE if @window has no images, %FALSE otherwise.
 **/
gboolean
xviewer_window_is_empty (XviewerWindow *window)
{
        XviewerWindowPrivate *priv;
        gboolean empty = TRUE;

	xviewer_debug (DEBUG_WINDOW);

        g_return_val_if_fail (XVIEWER_IS_WINDOW (window), FALSE);

        priv = window->priv;

        if (priv->store != NULL) {
                empty = (xviewer_list_store_length (XVIEWER_LIST_STORE (priv->store)) == 0);
        }

        return empty;
}

void
xviewer_window_reload_image (XviewerWindow *window)
{
	GtkWidget *view;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	if (window->priv->image == NULL)
		return;

	g_object_unref (window->priv->image);
	window->priv->image = NULL;

	view = xviewer_window_get_view (window);
	xviewer_scroll_view_set_image (XVIEWER_SCROLL_VIEW (view), NULL);

	xviewer_thumb_view_select_single (XVIEWER_THUMB_VIEW (window->priv->thumbview),
				      XVIEWER_THUMB_VIEW_SELECT_CURRENT);
}

gboolean
xviewer_window_is_not_initializing (const XviewerWindow *window)
{
	g_return_val_if_fail (XVIEWER_IS_WINDOW (window), FALSE);

	return window->priv->status != XVIEWER_WINDOW_STATUS_INIT;
}

void
xviewer_window_show_about_dialog (XviewerWindow *window)
{
	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "program-name", "Xviewer",
			       "version", VERSION,
			       "website", "https://github.com/linuxmint/xviewer",
			       "logo-icon-name", "xviewer",
			       "wrap-license", TRUE,
			       "license-type", GTK_LICENSE_GPL_2_0,
			       NULL);
}

void
xviewer_window_show_preferences_dialog (XviewerWindow *window)
{
	GtkWidget *pref_dlg;

	g_return_if_fail (window != NULL);

	pref_dlg = xviewer_preferences_dialog_get_instance (GTK_WINDOW (window));

	gtk_widget_show (pref_dlg);
}

void
xviewer_window_close (XviewerWindow *window)
{
	XviewerWindowPrivate *priv;

	g_return_if_fail (XVIEWER_IS_WINDOW (window));

	priv = window->priv;

	if (priv->save_job != NULL) {
		xviewer_window_finish_saving (window);
	}

	if (!xviewer_window_unsaved_images_confirm (window)) {
		gtk_widget_destroy (GTK_WIDGET (window));
	}
}

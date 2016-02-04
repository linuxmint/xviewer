/* Xviewer - Application Facade
 *
 * Copyright (C) 2006 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-application.h) by:
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

#include "xviewer-config-keys.h"
#include "xviewer-debug.h"
#include "xviewer-image.h"
#include "xviewer-job-scheduler.h"
#include "xviewer-session.h"
#include "xviewer-thumbnail.h"
#include "xviewer-window.h"
#include "xviewer-application.h"
#include "xviewer-application-activatable.h"
#include "xviewer-application-internal.h"
#include "xviewer-util.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#define APPLICATION_SERVICE_NAME "org.gnome.xviewer.ApplicationService"

#define XVIEWER_CSS_FILE_PATH XVIEWER_DATA_DIR G_DIR_SEPARATOR_S "xviewer.css"

static void xviewer_application_load_accelerators (void);
static void xviewer_application_save_accelerators (void);

G_DEFINE_TYPE_WITH_PRIVATE (XviewerApplication, xviewer_application, GTK_TYPE_APPLICATION);

static XviewerWindow*
get_focus_window (GtkApplication *application)
{
	GList *windows;
	GtkWindow *window = NULL;

	/* the windows are ordered with the last focused first */
	windows = gtk_application_get_windows (application);

	if (windows != NULL) {
		window = g_list_nth_data (windows, 0);
	}

	return XVIEWER_WINDOW (window);
}

static void
action_about (GSimpleAction *action,
	      GVariant      *parameter,
	      gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	xviewer_window_show_about_dialog (get_focus_window (application));
}

static void
action_help (GSimpleAction *action,
	     GVariant      *parameter,
	     gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	xviewer_util_show_help (NULL,
	                    GTK_WINDOW (get_focus_window (application)));
}

static void
action_preferences (GSimpleAction *action,
	            GVariant      *parameter,
	            gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	xviewer_window_show_preferences_dialog (get_focus_window (application));
}

static void
action_toggle_state (GSimpleAction *action,
                     GVariant *parameter,
                     gpointer user_data)
{
	GVariant *state;
	gboolean new_state;

	state = g_action_get_state (G_ACTION (action));
	new_state = !g_variant_get_boolean (state);
	g_action_change_state (G_ACTION (action),
	                       g_variant_new_boolean (new_state));
	g_variant_unref (state);
}

static void
action_quit (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
	GList *windows;

	windows = gtk_application_get_windows (GTK_APPLICATION (user_data));

	g_list_foreach (windows, (GFunc) xviewer_window_close, NULL);
}

static GActionEntry app_entries[] = {
	{ "toolbar", action_toggle_state, NULL, "true", NULL },
	{ "view-statusbar", action_toggle_state, NULL, "true", NULL },
	{ "view-gallery", action_toggle_state, NULL, "true",  NULL },
	{ "view-sidebar", action_toggle_state, NULL, "true",  NULL },
	{ "preferences", action_preferences, NULL, NULL, NULL },
	{ "about", action_about, NULL, NULL, NULL },
	{ "help", action_help, NULL, NULL, NULL },
	{ "quit", action_quit, NULL, NULL, NULL },
};

static gboolean
_settings_map_get_bool_variant (GValue   *value,
                               GVariant *variant,
                               gpointer  user_data)
{
	g_return_val_if_fail (g_variant_is_of_type (variant,
	                                            G_VARIANT_TYPE_BOOLEAN),
	                      FALSE);

	g_value_set_variant (value, variant);

	return TRUE;
}

static GVariant*
_settings_map_set_variant (const GValue       *value,
                           const GVariantType *expected_type,
                           gpointer            user_data)
{
	g_return_val_if_fail (g_variant_is_of_type (g_value_get_variant (value), expected_type), NULL);

	return g_value_dup_variant (value);
}

static void
xviewer_application_init_app_menu (XviewerApplication *application)
{
	XviewerApplicationPrivate *priv = application->priv;
	GtkBuilder *builder;
	GError *error = NULL;
	GAction *action;

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 app_entries, G_N_ELEMENTS (app_entries),
					 application);

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder,
				       "/org/gnome/xviewer/ui/xviewer-app-menu.xml",
				       &error);

	if (error == NULL) {
		gtk_application_set_app_menu (GTK_APPLICATION (application),
					      G_MENU_MODEL (gtk_builder_get_object (builder,
		                                                                    "app-menu")));
	} else {
		g_critical ("Unable to add the application menu: %s\n", error->message);
		g_error_free (error);
	}

	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "view-gallery");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              XVIEWER_CONF_UI_IMAGE_GALLERY, action,
	                              "state", G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "toolbar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              XVIEWER_CONF_UI_TOOLBAR, action, "state",
                                      G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);
	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "view-sidebar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              XVIEWER_CONF_UI_SIDEBAR, action, "state",
                                      G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);
	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "view-statusbar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              XVIEWER_CONF_UI_STATUSBAR, action, "state",
                                      G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);

	g_object_unref (builder);
}

static void
on_extension_added (PeasExtensionSet *set,
		    PeasPluginInfo   *info,
		    PeasExtension    *exten,
		    XviewerApplication   *app)
{
	xviewer_application_activatable_activate (XVIEWER_APPLICATION_ACTIVATABLE (exten));
}

static void
on_extension_removed (PeasExtensionSet *set,
		      PeasPluginInfo   *info,
		      PeasExtension    *exten,
		      XviewerApplication   *app)
{
	xviewer_application_activatable_deactivate (XVIEWER_APPLICATION_ACTIVATABLE (exten));
}

static void
xviewer_application_startup (GApplication *application)
{
	XviewerApplication *app = XVIEWER_APPLICATION (application);
	GError *error = NULL;
	GFile *css_file;
	GtkSettings *settings;
	GtkCssProvider *provider;
	gboolean shows_app_menu;
	gboolean shows_menubar;

	G_APPLICATION_CLASS (xviewer_application_parent_class)->startup (application);

#ifdef HAVE_EXEMPI
	xmp_init();
#endif
	xviewer_debug_init ();
	xviewer_job_scheduler_init ();
	xviewer_thumbnail_init ();

	/* Load special style properties for XviewerThumbView's scrollbar */
	css_file = g_file_new_for_uri ("resource:///org/gnome/xviewer/ui/xviewer.css");
	provider = gtk_css_provider_new ();
	if (G_UNLIKELY (!gtk_css_provider_load_from_file(provider,
							 css_file,
							 &error)))
	{
		g_critical ("Could not load CSS data: %s", error->message);
		g_clear_error (&error);
	} else {
		gtk_style_context_add_provider_for_screen (
				gdk_screen_get_default(),
				GTK_STYLE_PROVIDER (provider),
				GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
	g_object_unref (provider);

	/* Add application specific icons to search path */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                           XVIEWER_DATA_DIR G_DIR_SEPARATOR_S "icons");

	gtk_window_set_default_icon_name ("xviewer");
	g_set_application_name (_("Image Viewer"));

	settings = gtk_settings_get_default ();
	g_object_set (G_OBJECT (settings),
	              "gtk-application-prefer-dark-theme", TRUE,
	              NULL);

	g_object_get (gtk_settings_get_default (),
		      "gtk-shell-shows-app-menu", &shows_app_menu,
		      "gtk-shell-shows-menubar", &shows_menubar,
		      NULL);

	if (shows_app_menu && !shows_menubar)
		xviewer_application_init_app_menu (XVIEWER_APPLICATION (application));

	app->priv->extensions = peas_extension_set_new (
				   PEAS_ENGINE (app->priv->plugin_engine),
				   XVIEWER_TYPE_APPLICATION_ACTIVATABLE,
				   "app", app, NULL);
	g_signal_connect (app->priv->extensions, "extension-added",
			  G_CALLBACK (on_extension_added), app);
	g_signal_connect (app->priv->extensions, "extension-removed",
			  G_CALLBACK (on_extension_removed), app);

	peas_extension_set_call (app->priv->extensions, "activate");
}

static void
xviewer_application_shutdown (GApplication *application)
{
#ifdef HAVE_EXEMPI
	xmp_terminate();
#endif

	G_APPLICATION_CLASS (xviewer_application_parent_class)->shutdown (application);
}

static void
xviewer_application_activate (GApplication *application)
{
	xviewer_application_open_window (XVIEWER_APPLICATION (application),
				     GDK_CURRENT_TIME,
				     XVIEWER_APPLICATION (application)->priv->flags,
				     NULL);
}

static void
xviewer_application_open (GApplication *application,
		      GFile       **files,
		      gint          n_files,
		      const gchar  *hint)
{
	GSList *list = NULL;

	while (n_files--)
		list = g_slist_prepend (list, files[n_files]);

	xviewer_application_open_file_list (XVIEWER_APPLICATION (application),
					list, GDK_CURRENT_TIME,
					XVIEWER_APPLICATION (application)->priv->flags,
					NULL);
}

static void
xviewer_application_finalize (GObject *object)
{
	XviewerApplication *application = XVIEWER_APPLICATION (object);
	XviewerApplicationPrivate *priv = application->priv;

	if (priv->toolbars_model) {
		g_object_unref (priv->toolbars_model);
		priv->toolbars_model = NULL;
		g_free (priv->toolbars_file);
		priv->toolbars_file = NULL;
	}

	g_clear_object (&priv->extensions);

	if (priv->plugin_engine) {
		g_object_unref (priv->plugin_engine);
		priv->plugin_engine = NULL;
	}

	g_clear_object (&priv->ui_settings);

	xviewer_application_save_accelerators ();
}

static void
xviewer_application_add_platform_data (GApplication *application,
				   GVariantBuilder *builder)
{
	XviewerApplication *app = XVIEWER_APPLICATION (application);

	G_APPLICATION_CLASS (xviewer_application_parent_class)->add_platform_data (application,
									       builder);

	if (app->priv->flags) {
		g_variant_builder_add (builder, "{sv}",
				       "xviewer-application-startup-flags",
		                       g_variant_new_byte (app->priv->flags));
	}
}

static void
xviewer_application_before_emit (GApplication *application,
			     GVariant *platform_data)
{
	GVariantIter iter;
	const gchar *key;
	GVariant *value;

	XVIEWER_APPLICATION (application)->priv->flags = 0;
	g_variant_iter_init (&iter, platform_data);
	while (g_variant_iter_loop (&iter, "{&sv}", &key, &value)) {
		if (strcmp (key, "xviewer-application-startup-flags") == 0) {
			XVIEWER_APPLICATION (application)->priv->flags = g_variant_get_byte (value);
		}
	}

	G_APPLICATION_CLASS (xviewer_application_parent_class)->before_emit (application,
									 platform_data);
}

static void
xviewer_application_class_init (XviewerApplicationClass *xviewer_application_class)
{
	GApplicationClass *application_class;
	GObjectClass *object_class;

	application_class = (GApplicationClass *) xviewer_application_class;
	object_class = (GObjectClass *) xviewer_application_class;

	object_class->finalize = xviewer_application_finalize;

	application_class->startup = xviewer_application_startup;
	application_class->shutdown = xviewer_application_shutdown;
	application_class->activate = xviewer_application_activate;
	application_class->open = xviewer_application_open;
	application_class->add_platform_data = xviewer_application_add_platform_data;
	application_class->before_emit = xviewer_application_before_emit;
}

static void
xviewer_application_init (XviewerApplication *xviewer_application)
{
	XviewerApplicationPrivate *priv;
	const gchar *dot_dir = xviewer_util_dot_dir ();

	xviewer_session_init (xviewer_application);

	xviewer_application->priv = xviewer_application_get_instance_private (xviewer_application);
	priv = xviewer_application->priv;

	priv->toolbars_model = egg_toolbars_model_new ();
	priv->plugin_engine = xviewer_plugin_engine_new ();
	priv->flags = 0;

	priv->ui_settings = g_settings_new (XVIEWER_CONF_UI);

	egg_toolbars_model_load_names (priv->toolbars_model,
				       XVIEWER_DATA_DIR "/xviewer-toolbar.xml");

	if (G_LIKELY (dot_dir != NULL))
		priv->toolbars_file = g_build_filename
			(dot_dir, "xviewer_toolbar.xml", NULL);

	if (!dot_dir || !egg_toolbars_model_load_toolbars (priv->toolbars_model,
							priv->toolbars_file)) {

		egg_toolbars_model_load_toolbars (priv->toolbars_model,
						  XVIEWER_DATA_DIR "/xviewer-toolbar.xml");
	}

	egg_toolbars_model_set_flags (priv->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);

	xviewer_application_load_accelerators ();
}

/**
 * xviewer_application_get_instance:
 *
 * Returns a singleton instance of #XviewerApplication currently running.
 * If not running yet, it will create one.
 *
 * Returns: (transfer none): a running #XviewerApplication.
 **/
XviewerApplication *
xviewer_application_get_instance (void)
{
	static XviewerApplication *instance;

	if (!instance) {
		instance = XVIEWER_APPLICATION (g_object_new (XVIEWER_TYPE_APPLICATION,
							  "application-id", APPLICATION_SERVICE_NAME,
							  "flags", G_APPLICATION_HANDLES_OPEN,
							  NULL));
	}

	return instance;
}

static XviewerWindow *
xviewer_application_get_empty_window (XviewerApplication *application)
{
	XviewerWindow *empty_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), NULL);

	windows = gtk_application_get_windows (GTK_APPLICATION (application));

	for (l = windows; l != NULL; l = l->next) {
		XviewerWindow *window = XVIEWER_WINDOW (l->data);

		/* Make sure the window is empty and not initializing */
		if (xviewer_window_is_empty (window) &&
		    xviewer_window_is_not_initializing (window)) {
			empty_window = window;
			break;
		}
	}

	return empty_window;
}

/**
 * xviewer_application_open_window:
 * @application: An #XviewerApplication.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #XviewerStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens and presents an empty #XviewerWindow to the user. If there is
 * an empty window already open, this will be used. Otherwise, a
 * new one will be instantiated.
 *
 * Returns: %FALSE if @application is invalid, %TRUE otherwise
 **/
gboolean
xviewer_application_open_window (XviewerApplication  *application,
			     guint32         timestamp,
			     XviewerStartupFlags flags,
			     GError        **error)
{
	GtkWidget *new_window = NULL;

	new_window = GTK_WIDGET (xviewer_application_get_empty_window (application));

	if (new_window == NULL) {
		new_window = xviewer_window_new (flags);
	}

	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), FALSE);

	gtk_window_present_with_time (GTK_WINDOW (new_window),
				      timestamp);

	return TRUE;
}

static XviewerWindow *
xviewer_application_get_file_window (XviewerApplication *application, GFile *file)
{
	XviewerWindow *file_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), NULL);

	windows = gtk_window_list_toplevels ();

	for (l = windows; l != NULL; l = l->next) {
		if (XVIEWER_IS_WINDOW (l->data)) {
			XviewerWindow *window = XVIEWER_WINDOW (l->data);

			if (!xviewer_window_is_empty (window)) {
				XviewerImage *image = xviewer_window_get_image (window);
				GFile *window_file;

				window_file = xviewer_image_get_file (image);
				if (g_file_equal (window_file, file)) {
					file_window = window;
					break;
				}
			}
		}
	}

	g_list_free (windows);

	return file_window;
}

static XviewerWindow *
xviewer_application_get_first_window (XviewerApplication *application)
{
	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), NULL);

	GList *windows;
	GList *l;
	XviewerWindow *window = NULL;
	windows = gtk_window_list_toplevels ();
	for (l = windows; l != NULL; l = l->next) {
		if (XVIEWER_IS_WINDOW (l->data)) {
			window = XVIEWER_WINDOW (l->data);
			break;
		}
	}
	g_list_free (windows);

	return window;
}


static void
xviewer_application_show_window (XviewerWindow *window, gpointer user_data)
{
	gtk_window_present_with_time (GTK_WINDOW (window),
				      GPOINTER_TO_UINT (user_data));
}

/**
 * xviewer_application_open_file_list:
 * @application: An #XviewerApplication.
 * @file_list: (element-type GFile): A list of #GFile<!-- -->s.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #XviewerStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of files in a #XviewerWindow. If an #XviewerWindow displaying the first
 * image in the list is already open, this will be used. Otherwise, an empty
 * #XviewerWindow is used, either already existing or newly created.
 * If the XVIEWER_STARTUP_SINGLE_WINDOW flag is set, the files are opened in the
 * first #XviewerWindow and no new one is opened.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
xviewer_application_open_file_list (XviewerApplication  *application,
				GSList          *file_list,
				guint           timestamp,
				XviewerStartupFlags flags,
				GError         **error)
{
	XviewerWindow *new_window = NULL;

	if (file_list != NULL) {
		if(flags & XVIEWER_STARTUP_SINGLE_WINDOW)
			new_window = xviewer_application_get_first_window (application);
		else
			new_window = xviewer_application_get_file_window (application,
								      (GFile *) file_list->data);
	}

	if (new_window != NULL) {
		if(flags & XVIEWER_STARTUP_SINGLE_WINDOW)
		        xviewer_window_open_file_list (new_window, file_list);
		else
			gtk_window_present_with_time (GTK_WINDOW (new_window),
						      timestamp);
		return TRUE;
	}

	new_window = xviewer_application_get_empty_window (application);

	if (new_window == NULL) {
		new_window = XVIEWER_WINDOW (xviewer_window_new (flags));
	}

	g_signal_connect (new_window,
			  "prepared",
			  G_CALLBACK (xviewer_application_show_window),
			  GUINT_TO_POINTER (timestamp));

	xviewer_window_open_file_list (new_window, file_list);

	return TRUE;
}

/**
 * xviewer_application_open_uri_list:
 * @application: An #XviewerApplication.
 * @uri_list: (element-type utf8): A list of URIs.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #XviewerStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URIs. See
 * xviewer_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
xviewer_application_open_uri_list (XviewerApplication  *application,
 			       GSList          *uri_list,
 			       guint           timestamp,
 			       XviewerStartupFlags flags,
 			       GError         **error)
{
 	GSList *file_list = NULL;

 	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), FALSE);

 	file_list = xviewer_util_string_list_to_file_list (uri_list);

 	return xviewer_application_open_file_list (application,
					       file_list,
					       timestamp,
					       flags,
					       error);
}

/**
 * xviewer_application_open_uris:
 * @application: an #XviewerApplication
 * @uris:  A #GList of URI strings.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #XviewerStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URI strings. See
 * xviewer_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
xviewer_application_open_uris (XviewerApplication  *application,
 			   gchar          **uris,
 			   guint           timestamp,
 			   XviewerStartupFlags flags,
 			   GError        **error)
{
 	GSList *file_list = NULL;

 	file_list = xviewer_util_strings_to_file_list (uris);

 	return xviewer_application_open_file_list (application, file_list, timestamp,
						    flags, error);
}


/**
 * xviewer_application_get_toolbars_model:
 * @application: An #XviewerApplication.
 *
 * Retrieves the #EggToolbarsModel for the toolbar in #XviewerApplication.
 *
 * Returns: (transfer none): An #EggToolbarsModel.
 **/
EggToolbarsModel *
xviewer_application_get_toolbars_model (XviewerApplication *application)
{
	g_return_val_if_fail (XVIEWER_IS_APPLICATION (application), NULL);

	return application->priv->toolbars_model;
}

/**
 * xviewer_application_save_toolbars_model:
 * @application: An #XviewerApplication.
 *
 * Causes the saving of the model of the toolbar in #XviewerApplication to a file.
 **/
void
xviewer_application_save_toolbars_model (XviewerApplication *application)
{
	if (G_LIKELY(application->priv->toolbars_file != NULL))
		egg_toolbars_model_save_toolbars (application->priv->toolbars_model,
		                                  application->priv->toolbars_file,
						  "1.0");
}

/**
 * xviewer_application_reset_toolbars_model:
 * @app: an #XviewerApplication
 *
 * Restores the toolbars model to the defaults.
 **/
void
xviewer_application_reset_toolbars_model (XviewerApplication *app)
{
	XviewerApplicationPrivate *priv;
	g_return_if_fail (XVIEWER_IS_APPLICATION (app));

	priv = app->priv;

	g_object_unref (app->priv->toolbars_model);

	priv->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (priv->toolbars_model,
				       XVIEWER_DATA_DIR "/xviewer-toolbar.xml");
	egg_toolbars_model_load_toolbars (priv->toolbars_model,
					  XVIEWER_DATA_DIR "/xviewer-toolbar.xml");
	egg_toolbars_model_set_flags (priv->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);
}

static void
xviewer_application_load_accelerators (void)
{
	gchar *accelfile = g_build_filename (xviewer_util_dot_dir (), "accels", NULL);

	/* gtk_accel_map_load does nothing if the file does not exist */
	gtk_accel_map_load (accelfile);

	g_free (accelfile);
}

static void
xviewer_application_save_accelerators (void)
{
	/* save to XDG_CONFIG_HOME/xviewer/accels */
	gchar *accelfile = g_build_filename (xviewer_util_dot_dir (), "accels", NULL);

	gtk_accel_map_save (accelfile);
	g_free (accelfile);
}

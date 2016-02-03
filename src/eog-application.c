/* Eye Of Gnome - Application Facade
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

#include "eog-config-keys.h"
#include "eog-debug.h"
#include "eog-image.h"
#include "eog-job-scheduler.h"
#include "eog-session.h"
#include "eog-thumbnail.h"
#include "eog-window.h"
#include "eog-application.h"
#include "eog-application-activatable.h"
#include "eog-application-internal.h"
#include "eog-util.h"

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#if HAVE_EXEMPI
#include <exempi/xmp.h>
#endif

#define APPLICATION_SERVICE_NAME "org.gnome.eog.ApplicationService"

#define EOG_CSS_FILE_PATH EOG_DATA_DIR G_DIR_SEPARATOR_S "eog.css"

static void eog_application_load_accelerators (void);
static void eog_application_save_accelerators (void);

G_DEFINE_TYPE_WITH_PRIVATE (EogApplication, eog_application, GTK_TYPE_APPLICATION);

static EogWindow*
get_focus_window (GtkApplication *application)
{
	GList *windows;
	GtkWindow *window = NULL;

	/* the windows are ordered with the last focused first */
	windows = gtk_application_get_windows (application);

	if (windows != NULL) {
		window = g_list_nth_data (windows, 0);
	}

	return EOG_WINDOW (window);
}

static void
action_about (GSimpleAction *action,
	      GVariant      *parameter,
	      gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	eog_window_show_about_dialog (get_focus_window (application));
}

static void
action_help (GSimpleAction *action,
	     GVariant      *parameter,
	     gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	eog_util_show_help (NULL,
	                    GTK_WINDOW (get_focus_window (application)));
}

static void
action_preferences (GSimpleAction *action,
	            GVariant      *parameter,
	            gpointer       user_data)
{
	GtkApplication *application = GTK_APPLICATION (user_data);

	eog_window_show_preferences_dialog (get_focus_window (application));
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

	g_list_foreach (windows, (GFunc) eog_window_close, NULL);
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
eog_application_init_app_menu (EogApplication *application)
{
	EogApplicationPrivate *priv = application->priv;
	GtkBuilder *builder;
	GError *error = NULL;
	GAction *action;

	g_action_map_add_action_entries (G_ACTION_MAP (application),
					 app_entries, G_N_ELEMENTS (app_entries),
					 application);

	builder = gtk_builder_new ();
	gtk_builder_add_from_resource (builder,
				       "/org/gnome/eog/ui/eog-app-menu.xml",
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
	                              EOG_CONF_UI_IMAGE_GALLERY, action,
	                              "state", G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);

	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "toolbar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              EOG_CONF_UI_TOOLBAR, action, "state",
                                      G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);
	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "view-sidebar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              EOG_CONF_UI_SIDEBAR, action, "state",
                                      G_SETTINGS_BIND_DEFAULT,
	                              _settings_map_get_bool_variant,
	                              _settings_map_set_variant,
	                              NULL, NULL);
	action = g_action_map_lookup_action (G_ACTION_MAP (application),
	                                     "view-statusbar");
	g_settings_bind_with_mapping (priv->ui_settings,
	                              EOG_CONF_UI_STATUSBAR, action, "state",
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
		    EogApplication   *app)
{
	eog_application_activatable_activate (EOG_APPLICATION_ACTIVATABLE (exten));
}

static void
on_extension_removed (PeasExtensionSet *set,
		      PeasPluginInfo   *info,
		      PeasExtension    *exten,
		      EogApplication   *app)
{
	eog_application_activatable_deactivate (EOG_APPLICATION_ACTIVATABLE (exten));
}

static void
eog_application_startup (GApplication *application)
{
	EogApplication *app = EOG_APPLICATION (application);
	GError *error = NULL;
	GFile *css_file;
	GtkSettings *settings;
	GtkCssProvider *provider;
	gboolean shows_app_menu;
	gboolean shows_menubar;

	G_APPLICATION_CLASS (eog_application_parent_class)->startup (application);

#ifdef HAVE_EXEMPI
	xmp_init();
#endif
	eog_debug_init ();
	eog_job_scheduler_init ();
	eog_thumbnail_init ();

	/* Load special style properties for EogThumbView's scrollbar */
	css_file = g_file_new_for_uri ("resource:///org/gnome/eog/ui/eog.css");
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
                                           EOG_DATA_DIR G_DIR_SEPARATOR_S "icons");

	gtk_window_set_default_icon_name ("eog");
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
		eog_application_init_app_menu (EOG_APPLICATION (application));

	app->priv->extensions = peas_extension_set_new (
				   PEAS_ENGINE (app->priv->plugin_engine),
				   EOG_TYPE_APPLICATION_ACTIVATABLE,
				   "app", app, NULL);
	g_signal_connect (app->priv->extensions, "extension-added",
			  G_CALLBACK (on_extension_added), app);
	g_signal_connect (app->priv->extensions, "extension-removed",
			  G_CALLBACK (on_extension_removed), app);

	peas_extension_set_call (app->priv->extensions, "activate");
}

static void
eog_application_shutdown (GApplication *application)
{
#ifdef HAVE_EXEMPI
	xmp_terminate();
#endif

	G_APPLICATION_CLASS (eog_application_parent_class)->shutdown (application);
}

static void
eog_application_activate (GApplication *application)
{
	eog_application_open_window (EOG_APPLICATION (application),
				     GDK_CURRENT_TIME,
				     EOG_APPLICATION (application)->priv->flags,
				     NULL);
}

static void
eog_application_open (GApplication *application,
		      GFile       **files,
		      gint          n_files,
		      const gchar  *hint)
{
	GSList *list = NULL;

	while (n_files--)
		list = g_slist_prepend (list, files[n_files]);

	eog_application_open_file_list (EOG_APPLICATION (application),
					list, GDK_CURRENT_TIME,
					EOG_APPLICATION (application)->priv->flags,
					NULL);
}

static void
eog_application_finalize (GObject *object)
{
	EogApplication *application = EOG_APPLICATION (object);
	EogApplicationPrivate *priv = application->priv;

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

	eog_application_save_accelerators ();
}

static void
eog_application_add_platform_data (GApplication *application,
				   GVariantBuilder *builder)
{
	EogApplication *app = EOG_APPLICATION (application);

	G_APPLICATION_CLASS (eog_application_parent_class)->add_platform_data (application,
									       builder);

	if (app->priv->flags) {
		g_variant_builder_add (builder, "{sv}",
				       "eog-application-startup-flags",
		                       g_variant_new_byte (app->priv->flags));
	}
}

static void
eog_application_before_emit (GApplication *application,
			     GVariant *platform_data)
{
	GVariantIter iter;
	const gchar *key;
	GVariant *value;

	EOG_APPLICATION (application)->priv->flags = 0;
	g_variant_iter_init (&iter, platform_data);
	while (g_variant_iter_loop (&iter, "{&sv}", &key, &value)) {
		if (strcmp (key, "eog-application-startup-flags") == 0) {
			EOG_APPLICATION (application)->priv->flags = g_variant_get_byte (value);
		}
	}

	G_APPLICATION_CLASS (eog_application_parent_class)->before_emit (application,
									 platform_data);
}

static void
eog_application_class_init (EogApplicationClass *eog_application_class)
{
	GApplicationClass *application_class;
	GObjectClass *object_class;

	application_class = (GApplicationClass *) eog_application_class;
	object_class = (GObjectClass *) eog_application_class;

	object_class->finalize = eog_application_finalize;

	application_class->startup = eog_application_startup;
	application_class->shutdown = eog_application_shutdown;
	application_class->activate = eog_application_activate;
	application_class->open = eog_application_open;
	application_class->add_platform_data = eog_application_add_platform_data;
	application_class->before_emit = eog_application_before_emit;
}

static void
eog_application_init (EogApplication *eog_application)
{
	EogApplicationPrivate *priv;
	const gchar *dot_dir = eog_util_dot_dir ();

	eog_session_init (eog_application);

	eog_application->priv = eog_application_get_instance_private (eog_application);
	priv = eog_application->priv;

	priv->toolbars_model = egg_toolbars_model_new ();
	priv->plugin_engine = eog_plugin_engine_new ();
	priv->flags = 0;

	priv->ui_settings = g_settings_new (EOG_CONF_UI);

	egg_toolbars_model_load_names (priv->toolbars_model,
				       EOG_DATA_DIR "/eog-toolbar.xml");

	if (G_LIKELY (dot_dir != NULL))
		priv->toolbars_file = g_build_filename
			(dot_dir, "eog_toolbar.xml", NULL);

	if (!dot_dir || !egg_toolbars_model_load_toolbars (priv->toolbars_model,
							priv->toolbars_file)) {

		egg_toolbars_model_load_toolbars (priv->toolbars_model,
						  EOG_DATA_DIR "/eog-toolbar.xml");
	}

	egg_toolbars_model_set_flags (priv->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);

	eog_application_load_accelerators ();
}

/**
 * eog_application_get_instance:
 *
 * Returns a singleton instance of #EogApplication currently running.
 * If not running yet, it will create one.
 *
 * Returns: (transfer none): a running #EogApplication.
 **/
EogApplication *
eog_application_get_instance (void)
{
	static EogApplication *instance;

	if (!instance) {
		instance = EOG_APPLICATION (g_object_new (EOG_TYPE_APPLICATION,
							  "application-id", APPLICATION_SERVICE_NAME,
							  "flags", G_APPLICATION_HANDLES_OPEN,
							  NULL));
	}

	return instance;
}

static EogWindow *
eog_application_get_empty_window (EogApplication *application)
{
	EogWindow *empty_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	windows = gtk_application_get_windows (GTK_APPLICATION (application));

	for (l = windows; l != NULL; l = l->next) {
		EogWindow *window = EOG_WINDOW (l->data);

		/* Make sure the window is empty and not initializing */
		if (eog_window_is_empty (window) &&
		    eog_window_is_not_initializing (window)) {
			empty_window = window;
			break;
		}
	}

	return empty_window;
}

/**
 * eog_application_open_window:
 * @application: An #EogApplication.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens and presents an empty #EogWindow to the user. If there is
 * an empty window already open, this will be used. Otherwise, a
 * new one will be instantiated.
 *
 * Returns: %FALSE if @application is invalid, %TRUE otherwise
 **/
gboolean
eog_application_open_window (EogApplication  *application,
			     guint32         timestamp,
			     EogStartupFlags flags,
			     GError        **error)
{
	GtkWidget *new_window = NULL;

	new_window = GTK_WIDGET (eog_application_get_empty_window (application));

	if (new_window == NULL) {
		new_window = eog_window_new (flags);
	}

	g_return_val_if_fail (EOG_IS_APPLICATION (application), FALSE);

	gtk_window_present_with_time (GTK_WINDOW (new_window),
				      timestamp);

	return TRUE;
}

static EogWindow *
eog_application_get_file_window (EogApplication *application, GFile *file)
{
	EogWindow *file_window = NULL;
	GList *windows;
	GList *l;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	windows = gtk_window_list_toplevels ();

	for (l = windows; l != NULL; l = l->next) {
		if (EOG_IS_WINDOW (l->data)) {
			EogWindow *window = EOG_WINDOW (l->data);

			if (!eog_window_is_empty (window)) {
				EogImage *image = eog_window_get_image (window);
				GFile *window_file;

				window_file = eog_image_get_file (image);
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

static EogWindow *
eog_application_get_first_window (EogApplication *application)
{
	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	GList *windows;
	GList *l;
	EogWindow *window = NULL;
	windows = gtk_window_list_toplevels ();
	for (l = windows; l != NULL; l = l->next) {
		if (EOG_IS_WINDOW (l->data)) {
			window = EOG_WINDOW (l->data);
			break;
		}
	}
	g_list_free (windows);

	return window;
}


static void
eog_application_show_window (EogWindow *window, gpointer user_data)
{
	gtk_window_present_with_time (GTK_WINDOW (window),
				      GPOINTER_TO_UINT (user_data));
}

/**
 * eog_application_open_file_list:
 * @application: An #EogApplication.
 * @file_list: (element-type GFile): A list of #GFile<!-- -->s.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of files in a #EogWindow. If an #EogWindow displaying the first
 * image in the list is already open, this will be used. Otherwise, an empty
 * #EogWindow is used, either already existing or newly created.
 * If the EOG_STARTUP_SINGLE_WINDOW flag is set, the files are opened in the
 * first #EogWindow and no new one is opened.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_file_list (EogApplication  *application,
				GSList          *file_list,
				guint           timestamp,
				EogStartupFlags flags,
				GError         **error)
{
	EogWindow *new_window = NULL;

	if (file_list != NULL) {
		if(flags & EOG_STARTUP_SINGLE_WINDOW)
			new_window = eog_application_get_first_window (application);
		else
			new_window = eog_application_get_file_window (application,
								      (GFile *) file_list->data);
	}

	if (new_window != NULL) {
		if(flags & EOG_STARTUP_SINGLE_WINDOW)
		        eog_window_open_file_list (new_window, file_list);
		else
			gtk_window_present_with_time (GTK_WINDOW (new_window),
						      timestamp);
		return TRUE;
	}

	new_window = eog_application_get_empty_window (application);

	if (new_window == NULL) {
		new_window = EOG_WINDOW (eog_window_new (flags));
	}

	g_signal_connect (new_window,
			  "prepared",
			  G_CALLBACK (eog_application_show_window),
			  GUINT_TO_POINTER (timestamp));

	eog_window_open_file_list (new_window, file_list);

	return TRUE;
}

/**
 * eog_application_open_uri_list:
 * @application: An #EogApplication.
 * @uri_list: (element-type utf8): A list of URIs.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URIs. See
 * eog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_uri_list (EogApplication  *application,
 			       GSList          *uri_list,
 			       guint           timestamp,
 			       EogStartupFlags flags,
 			       GError         **error)
{
 	GSList *file_list = NULL;

 	g_return_val_if_fail (EOG_IS_APPLICATION (application), FALSE);

 	file_list = eog_util_string_list_to_file_list (uri_list);

 	return eog_application_open_file_list (application,
					       file_list,
					       timestamp,
					       flags,
					       error);
}

/**
 * eog_application_open_uris:
 * @application: an #EogApplication
 * @uris:  A #GList of URI strings.
 * @timestamp: The timestamp of the user interaction which triggered this call
 * (see gtk_window_present_with_time()).
 * @flags: A set of #EogStartupFlags influencing a new windows' state.
 * @error: Return location for a #GError, or NULL to ignore errors.
 *
 * Opens a list of images, from a list of URI strings. See
 * eog_application_open_file_list() for details.
 *
 * Returns: Currently always %TRUE.
 **/
gboolean
eog_application_open_uris (EogApplication  *application,
 			   gchar          **uris,
 			   guint           timestamp,
 			   EogStartupFlags flags,
 			   GError        **error)
{
 	GSList *file_list = NULL;

 	file_list = eog_util_strings_to_file_list (uris);

 	return eog_application_open_file_list (application, file_list, timestamp,
						    flags, error);
}


/**
 * eog_application_get_toolbars_model:
 * @application: An #EogApplication.
 *
 * Retrieves the #EggToolbarsModel for the toolbar in #EogApplication.
 *
 * Returns: (transfer none): An #EggToolbarsModel.
 **/
EggToolbarsModel *
eog_application_get_toolbars_model (EogApplication *application)
{
	g_return_val_if_fail (EOG_IS_APPLICATION (application), NULL);

	return application->priv->toolbars_model;
}

/**
 * eog_application_save_toolbars_model:
 * @application: An #EogApplication.
 *
 * Causes the saving of the model of the toolbar in #EogApplication to a file.
 **/
void
eog_application_save_toolbars_model (EogApplication *application)
{
	if (G_LIKELY(application->priv->toolbars_file != NULL))
		egg_toolbars_model_save_toolbars (application->priv->toolbars_model,
		                                  application->priv->toolbars_file,
						  "1.0");
}

/**
 * eog_application_reset_toolbars_model:
 * @app: an #EogApplication
 *
 * Restores the toolbars model to the defaults.
 **/
void
eog_application_reset_toolbars_model (EogApplication *app)
{
	EogApplicationPrivate *priv;
	g_return_if_fail (EOG_IS_APPLICATION (app));

	priv = app->priv;

	g_object_unref (app->priv->toolbars_model);

	priv->toolbars_model = egg_toolbars_model_new ();

	egg_toolbars_model_load_names (priv->toolbars_model,
				       EOG_DATA_DIR "/eog-toolbar.xml");
	egg_toolbars_model_load_toolbars (priv->toolbars_model,
					  EOG_DATA_DIR "/eog-toolbar.xml");
	egg_toolbars_model_set_flags (priv->toolbars_model, 0,
				      EGG_TB_MODEL_NOT_REMOVABLE);
}

static void
eog_application_load_accelerators (void)
{
	gchar *accelfile = g_build_filename (eog_util_dot_dir (), "accels", NULL);

	/* gtk_accel_map_load does nothing if the file does not exist */
	gtk_accel_map_load (accelfile);

	g_free (accelfile);
}

static void
eog_application_save_accelerators (void)
{
	/* save to XDG_CONFIG_HOME/eog/accels */
	gchar *accelfile = g_build_filename (eog_util_dot_dir (), "accels", NULL);

	gtk_accel_map_save (accelfile);
	g_free (accelfile);
}

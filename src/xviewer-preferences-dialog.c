/* Xviewer - XVIEWER Preferences Dialog
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "xviewer-preferences-dialog.h"
#include "xviewer-scroll-view.h"
#include "xviewer-util.h"
#include "xviewer-config-keys.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <libpeas-gtk/peas-gtk-plugin-manager.h>

#define RADIO_VALUE "radio-value"

struct _XviewerPreferencesDialogPrivate {
	GSettings     *view_settings;
	GSettings     *fullscreen_settings;

	GtkWidget     *interpolate_check;
	GtkWidget     *extrapolate_check;
	GtkWidget     *autorotate_check;
	GtkWidget     *bg_color_check;
	GtkWidget     *bg_color_button;
	GtkWidget     *color_radio;
	GtkWidget     *checkpattern_radio;
	GtkWidget     *background_radio;
	GtkWidget     *transp_color_button;

	GtkWidget     *upscale_check;
	GtkWidget     *loop_check;
	GtkWidget     *seconds_scale;
    GtkWidget     *pause_resume_check;

	GtkWidget     *plugin_manager;

    GtkWidget     *ScrollTextComboBox;
    GtkWidget     *ScrollShiftTextComboBox;
    GtkWidget     *ScrollCtrlTextComboBox;
    GtkWidget     *ScrollShiftCtrlTextComboBox;
    GtkWidget     *TiltTextComboBox;
    GtkWidget     *TiltShiftTextComboBox;
    GtkWidget     *TiltCtrlTextComboBox;
    GtkWidget     *TiltShiftCtrlTextComboBox;
};

static GObject *instance = NULL;

G_DEFINE_TYPE_WITH_PRIVATE (XviewerPreferencesDialog, xviewer_preferences_dialog, GTK_TYPE_DIALOG);

static gboolean
pd_string_to_rgba_mapping (GValue   *value,
			   GVariant *variant,
			   gpointer  user_data)
{
	GdkRGBA color;

	g_return_val_if_fail (g_variant_is_of_type (variant, G_VARIANT_TYPE_STRING), FALSE);

	if (gdk_rgba_parse (&color, g_variant_get_string (variant, NULL))) {
		g_value_set_boxed (value, &color);
		return TRUE;
	}

	return FALSE;
}

static GVariant*
pd_rgba_to_string_mapping (const GValue       *value,
			   const GVariantType *expected_type,
			   gpointer            user_data)
{
	GVariant *variant = NULL;
	GdkRGBA *color;
	gchar *hex_val;

	g_return_val_if_fail (G_VALUE_TYPE (value) == GDK_TYPE_RGBA, NULL);
	g_return_val_if_fail (g_variant_type_equal (expected_type, G_VARIANT_TYPE_STRING), NULL);

	color = g_value_get_boxed (value);
	hex_val = gdk_rgba_to_string(color);

	variant = g_variant_new_string (hex_val);
	g_free (hex_val);

	return variant;
}

static void
pd_transp_radio_toggle_cb (GtkWidget *widget, gpointer data)
{
	gpointer value = NULL;

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
	    return;

	value = g_object_get_data (G_OBJECT (widget), RADIO_VALUE);

	g_settings_set_enum (G_SETTINGS (data), XVIEWER_CONF_VIEW_TRANSPARENCY,
			     GPOINTER_TO_INT (value));
}

static gchar*
pd_seconds_scale_format_value_cb (GtkScale *scale, gdouble value, gpointer ptr)
{
	gulong int_val = (gulong) value;

	return g_strdup_printf (ngettext("%lu second", "%lu seconds", int_val),
	                        int_val);
}

static void
xviewer_preferences_response_cb (GtkDialog *dlg, gint res_id, gpointer data)
{
	switch (res_id) {
		case GTK_RESPONSE_HELP:
			xviewer_util_show_help ("preferences", NULL);
			break;
		default:
			gtk_widget_destroy (GTK_WIDGET (dlg));
			instance = NULL;
	}
}

static void
xviewer_preferences_dialog_class_init (XviewerPreferencesDialogClass *klass)
{
	GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;

	/* This should make sure the libpeas-gtk dependency isn't
	 * dropped by aggressive linkers (#739618) */
	g_type_ensure (PEAS_GTK_TYPE_PLUGIN_MANAGER);

	gtk_widget_class_set_template_from_resource (widget_class,
						     "/org/x/viewer/ui/xviewer-preferences-dialog.ui");
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      interpolate_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      extrapolate_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      autorotate_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      bg_color_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      bg_color_button);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      color_radio);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      checkpattern_radio);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      background_radio);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      transp_color_button);

	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      upscale_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      loop_check);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      seconds_scale);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      pause_resume_check);

	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      plugin_manager);

	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      ScrollTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      ScrollShiftTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      ScrollCtrlTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      ScrollShiftCtrlTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      TiltTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      TiltShiftTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      TiltCtrlTextComboBox);
	gtk_widget_class_bind_template_child_private (widget_class,
						      XviewerPreferencesDialog,
						      TiltShiftCtrlTextComboBox);
}

static void
xviewer_preferences_dialog_init (XviewerPreferencesDialog *pref_dlg)
{
	XviewerPreferencesDialogPrivate *priv;
	GtkAdjustment *scale_adjustment;

	pref_dlg->priv = xviewer_preferences_dialog_get_instance_private (pref_dlg);
	priv = pref_dlg->priv;

	gtk_widget_init_template (GTK_WIDGET (pref_dlg));

	priv->view_settings = g_settings_new (XVIEWER_CONF_VIEW);
	priv->fullscreen_settings = g_settings_new (XVIEWER_CONF_FULLSCREEN);

	g_signal_connect (G_OBJECT (pref_dlg),
			  "response",
			  G_CALLBACK (xviewer_preferences_response_cb),
			  pref_dlg);

	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_INTERPOLATE,
			 priv->interpolate_check, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_EXTRAPOLATE,
			 priv->extrapolate_check, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_AUTOROTATE,
			 priv->autorotate_check, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_USE_BG_COLOR,
			 priv->bg_color_check, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping (priv->view_settings,
				      XVIEWER_CONF_VIEW_BACKGROUND_COLOR,
				      priv->bg_color_button, "rgba",
				      G_SETTINGS_BIND_DEFAULT,
				      pd_string_to_rgba_mapping,
				      pd_rgba_to_string_mapping,
				      NULL, NULL);

	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_SCROLL_ACTION,
			 priv->ScrollTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_SCROLL_SHIFT_ACTION,
			 priv->ScrollShiftTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_SCROLL_CTRL_ACTION,
			 priv->ScrollCtrlTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_SCROLL_SHIFT_CTRL_ACTION,
			 priv->ScrollShiftCtrlTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_TILT_ACTION,
			 priv->TiltTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_TILT_SHIFT_ACTION,
			 priv->TiltShiftTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_TILT_CTRL_ACTION,
			 priv->TiltCtrlTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);
	g_settings_bind (priv->view_settings, XVIEWER_CONF_VIEW_TILT_SHIFT_CTRL_ACTION,
			 priv->TiltShiftCtrlTextComboBox, "active",
			 G_SETTINGS_BIND_DEFAULT);

	g_object_set_data (G_OBJECT (priv->color_radio),
			   RADIO_VALUE,
			   GINT_TO_POINTER (XVIEWER_TRANSP_COLOR));

	g_signal_connect (G_OBJECT (priv->color_radio),
			  "toggled",
			  G_CALLBACK (pd_transp_radio_toggle_cb),
			  priv->view_settings);

	g_object_set_data (G_OBJECT (priv->checkpattern_radio),
			   RADIO_VALUE,
			   GINT_TO_POINTER (XVIEWER_TRANSP_CHECKED));

	g_signal_connect (G_OBJECT (priv->checkpattern_radio),
			  "toggled",
			  G_CALLBACK (pd_transp_radio_toggle_cb),
			  priv->view_settings);

	g_object_set_data (G_OBJECT (priv->background_radio),
			   RADIO_VALUE,
			   GINT_TO_POINTER (XVIEWER_TRANSP_BACKGROUND));

	g_signal_connect (G_OBJECT (priv->background_radio),
			  "toggled",
			  G_CALLBACK (pd_transp_radio_toggle_cb),
			  priv->view_settings);

	g_signal_connect (G_OBJECT (priv->seconds_scale), "format-value",
			  G_CALLBACK (pd_seconds_scale_format_value_cb),
			  NULL);

	switch (g_settings_get_enum (priv->view_settings,
				     XVIEWER_CONF_VIEW_TRANSPARENCY))
	{
	case XVIEWER_TRANSP_COLOR:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->color_radio), TRUE);
		break;
	case XVIEWER_TRANSP_CHECKED:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->checkpattern_radio), TRUE);
		break;
	default:
		// Log a warning and use XVIEWER_TRANSP_BACKGROUND as fallback
		g_warn_if_reached();
	case XVIEWER_TRANSP_BACKGROUND:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->background_radio), TRUE);
		break;
	}

	g_settings_bind_with_mapping (priv->view_settings,
				      XVIEWER_CONF_VIEW_TRANS_COLOR,
				      priv->transp_color_button, "rgba",
				      G_SETTINGS_BIND_DEFAULT,
				      pd_string_to_rgba_mapping,
				      pd_rgba_to_string_mapping,
				      NULL, NULL);

	g_settings_bind (priv->fullscreen_settings, XVIEWER_CONF_FULLSCREEN_UPSCALE,
			 priv->upscale_check, "active",
			 G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (priv->fullscreen_settings, XVIEWER_CONF_FULLSCREEN_LOOP,
			 priv->loop_check, "active",
			 G_SETTINGS_BIND_DEFAULT);

	g_settings_bind (priv->fullscreen_settings, XVIEWER_CONF_FULLSCREEN_SPACEBAR_PAUSE,
			 priv->pause_resume_check, "active",
			 G_SETTINGS_BIND_DEFAULT);

	scale_adjustment = gtk_range_get_adjustment (GTK_RANGE (priv->seconds_scale));

	g_settings_bind (priv->fullscreen_settings, XVIEWER_CONF_FULLSCREEN_SECONDS,
			 scale_adjustment, "value",
			 G_SETTINGS_BIND_DEFAULT);

	gtk_widget_show_all (priv->plugin_manager);

}

GtkWidget *xviewer_preferences_dialog_get_instance(GtkWindow *parent)
{
	if (instance == NULL) {
		instance = g_object_new (XVIEWER_TYPE_PREFERENCES_DIALOG,
				 	 NULL);
	}

	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (instance), parent);

	return GTK_WIDGET(instance);
}

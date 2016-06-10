/* Xviewer - Side bar
 *
 * Copyright (C) 2004 Red Hat, Inc.
 * Copyright (C) 2007 The Free Software Foundation
 *
 * Author: Lucas Rocha <lucasr@gnome.org>
 *
 * Based on evince code (shell/ev-sidebar.c) by:
 * 	- Jonathan Blandford <jrb@alum.mit.edu>
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

#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "xviewer-sidebar.h"

enum {
	PROP_0,
	PROP_CURRENT_PAGE
};

enum {
	PAGE_COLUMN_TITLE,
	PAGE_COLUMN_MENU_ITEM,
	PAGE_COLUMN_MAIN_WIDGET,
	PAGE_COLUMN_NOTEBOOK_INDEX,
	PAGE_COLUMN_NUM_COLS
};

enum {
	SIGNAL_PAGE_ADDED,
	SIGNAL_PAGE_REMOVED,
	SIGNAL_LAST
};

static gint signals[SIGNAL_LAST];

struct _XviewerSidebarPrivate {
	GtkWidget *notebook;
	GtkWidget *select_button;
	GtkWidget *menu;
	GtkWidget *hbox;
	GtkWidget *label;

	GtkTreeModel *page_model;
};

G_DEFINE_TYPE_WITH_PRIVATE (XviewerSidebar, xviewer_sidebar, GTK_TYPE_BOX)

static void
xviewer_sidebar_destroy (GtkWidget *object)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (object);

	if (xviewer_sidebar->priv->menu) {
		gtk_menu_detach (GTK_MENU (xviewer_sidebar->priv->menu));
		xviewer_sidebar->priv->menu = NULL;
	}

	if (xviewer_sidebar->priv->page_model) {
		g_object_unref (xviewer_sidebar->priv->page_model);
		xviewer_sidebar->priv->page_model = NULL;
	}

	(* GTK_WIDGET_CLASS (xviewer_sidebar_parent_class)->destroy) (object);
}

static void
xviewer_sidebar_select_page (XviewerSidebar *xviewer_sidebar, GtkTreeIter *iter)
{
	gchar *title;
	gint index;

	gtk_tree_model_get (xviewer_sidebar->priv->page_model, iter,
			    PAGE_COLUMN_TITLE, &title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook), index);
	gtk_label_set_text (GTK_LABEL (xviewer_sidebar->priv->label), title);

	g_free (title);
}

void
xviewer_sidebar_set_page (XviewerSidebar   *xviewer_sidebar,
		     GtkWidget   *main_widget)
{
	GtkTreeIter iter;
	gboolean valid;

	valid = gtk_tree_model_get_iter_first (xviewer_sidebar->priv->page_model, &iter);

	while (valid) {
		GtkWidget *widget;

		gtk_tree_model_get (xviewer_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			xviewer_sidebar_select_page (xviewer_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (xviewer_sidebar->priv->page_model, &iter);
		}

		g_object_unref (widget);
	}

	g_object_notify (G_OBJECT (xviewer_sidebar), "current-page");
}

static GtkWidget *
xviewer_sidebar_get_current_page (XviewerSidebar *sidebar)
{
	GtkNotebook *notebook = GTK_NOTEBOOK (sidebar->priv->notebook);

	return gtk_notebook_get_nth_page
		(notebook, gtk_notebook_get_current_page (notebook));
}

static void
xviewer_sidebar_set_property (GObject     *object,
		         guint         prop_id,
		         const GValue *value,
		         GParamSpec   *pspec)
{
	XviewerSidebar *sidebar = XVIEWER_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		xviewer_sidebar_set_page (sidebar, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
xviewer_sidebar_get_property (GObject    *object,
		          guint       prop_id,
		          GValue     *value,
		          GParamSpec *pspec)
{
	XviewerSidebar *sidebar = XVIEWER_SIDEBAR (object);

	switch (prop_id) {
	case PROP_CURRENT_PAGE:
		g_value_set_object (value, xviewer_sidebar_get_current_page (sidebar));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
xviewer_sidebar_class_init (XviewerSidebarClass *xviewer_sidebar_class)
{
	GObjectClass *g_object_class;
	GtkWidgetClass *widget_class;

	g_object_class = G_OBJECT_CLASS (xviewer_sidebar_class);
	widget_class = GTK_WIDGET_CLASS (xviewer_sidebar_class);

	widget_class->destroy = xviewer_sidebar_destroy;
	g_object_class->get_property = xviewer_sidebar_get_property;
	g_object_class->set_property = xviewer_sidebar_set_property;

	g_object_class_install_property (g_object_class,
					 PROP_CURRENT_PAGE,
					 g_param_spec_object ("current-page",
							      "Current page",
							      "The currently visible page",
							      GTK_TYPE_WIDGET,
							      G_PARAM_READWRITE));

	signals[SIGNAL_PAGE_ADDED] =
		g_signal_new ("page-added",
			      XVIEWER_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (XviewerSidebarClass, page_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);

	signals[SIGNAL_PAGE_REMOVED] =
		g_signal_new ("page-removed",
			      XVIEWER_TYPE_SIDEBAR,
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (XviewerSidebarClass, page_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_WIDGET);
}

static void
xviewer_sidebar_menu_position_under (GtkMenu  *menu,
				 gint     *x,
				 gint     *y,
				 gboolean *push_in,
				 gpointer  user_data)
{
	GtkWidget *widget;
	GtkAllocation allocation;

	g_return_if_fail (GTK_IS_BUTTON (user_data));
	g_return_if_fail (!gtk_widget_get_has_window (user_data));

	widget = GTK_WIDGET (user_data);
	gtk_widget_get_allocation (widget, &allocation);

	gdk_window_get_origin (gtk_widget_get_window (widget), x, y);

	*x += allocation.x;
	*y += allocation.y + allocation.height;

	*push_in = FALSE;
}

static gboolean
xviewer_sidebar_select_button_press_cb (GtkWidget      *widget,
				    GdkEventButton *event,
				    gpointer        user_data)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (user_data);

	if (event->button == 1) {
		GtkRequisition requisition;
		GtkAllocation allocation;

		gtk_widget_get_allocation (widget, &allocation);

		gtk_widget_set_size_request (xviewer_sidebar->priv->menu, -1, -1);
		gtk_widget_get_preferred_size (xviewer_sidebar->priv->menu, &requisition, NULL);
		gtk_widget_set_size_request (xviewer_sidebar->priv->menu,
					     MAX (allocation.width,
						  requisition.width), -1);

		gtk_widget_grab_focus (widget);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (xviewer_sidebar->priv->menu),
				NULL, NULL, xviewer_sidebar_menu_position_under, widget,
				event->button, event->time);

		return TRUE;
	}

	return FALSE;
}

static gboolean
xviewer_sidebar_select_button_key_press_cb (GtkWidget   *widget,
				        GdkEventKey *event,
				        gpointer     user_data)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (user_data);

	if (event->keyval == GDK_KEY_space ||
	    event->keyval == GDK_KEY_KP_Space ||
	    event->keyval == GDK_KEY_Return ||
	    event->keyval == GDK_KEY_KP_Enter) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

		gtk_menu_popup (GTK_MENU (xviewer_sidebar->priv->menu),
			        NULL, NULL, xviewer_sidebar_menu_position_under, widget,
				1, event->time);

		return TRUE;
	}

	return FALSE;
}

static void
xviewer_sidebar_close_clicked_cb (GtkWidget *widget,
 			      gpointer   user_data)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (user_data);

	gtk_widget_hide (GTK_WIDGET (xviewer_sidebar));
}

static void
xviewer_sidebar_menu_deactivate_cb (GtkWidget *widget,
			       gpointer   user_data)
{
	GtkWidget *menu_button;

	menu_button = GTK_WIDGET (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (menu_button), FALSE);
}

static void
xviewer_sidebar_menu_detach_cb (GtkWidget *widget,
			   GtkMenu   *menu)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (widget);

	xviewer_sidebar->priv->menu = NULL;
}

static void
xviewer_sidebar_menu_item_activate_cb (GtkWidget *widget,
				   gpointer   user_data)
{
	XviewerSidebar *xviewer_sidebar = XVIEWER_SIDEBAR (user_data);
	GtkTreeIter iter;
	GtkWidget *menu_item, *item;
	gboolean valid;

	menu_item = gtk_menu_get_active (GTK_MENU (xviewer_sidebar->priv->menu));
	valid = gtk_tree_model_get_iter_first (xviewer_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (xviewer_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_MENU_ITEM, &item,
				    -1);

		if (item == menu_item) {
			xviewer_sidebar_select_page (xviewer_sidebar, &iter);
			valid = FALSE;
		} else {
			valid = gtk_tree_model_iter_next (xviewer_sidebar->priv->page_model, &iter);
		}

		g_object_unref (item);
	}

	g_object_notify (G_OBJECT (xviewer_sidebar), "current-page");
}

static void
xviewer_sidebar_init (XviewerSidebar *xviewer_sidebar)
{
	GtkWidget *hbox;
	GtkWidget *close_button;
	GtkWidget *select_hbox;
	GtkWidget *arrow;
	GtkWidget *image;

	gtk_orientable_set_orientation (GTK_ORIENTABLE (xviewer_sidebar),
					GTK_ORIENTATION_VERTICAL);

	xviewer_sidebar->priv = xviewer_sidebar_get_instance_private (xviewer_sidebar);

	/* data model */
	xviewer_sidebar->priv->page_model = (GtkTreeModel *)
			gtk_list_store_new (PAGE_COLUMN_NUM_COLS,
					    G_TYPE_STRING,
					    GTK_TYPE_WIDGET,
					    GTK_TYPE_WIDGET,
					    G_TYPE_INT);

	/* top option menu */
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	xviewer_sidebar->priv->hbox = hbox;
	gtk_box_pack_start (GTK_BOX (xviewer_sidebar), hbox, FALSE, FALSE, 0);
	gtk_widget_show (hbox);

	xviewer_sidebar->priv->select_button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (xviewer_sidebar->priv->select_button),
			       GTK_RELIEF_NONE);

	g_signal_connect (xviewer_sidebar->priv->select_button, "button_press_event",
			  G_CALLBACK (xviewer_sidebar_select_button_press_cb),
			  xviewer_sidebar);

	g_signal_connect (xviewer_sidebar->priv->select_button, "key_press_event",
			  G_CALLBACK (xviewer_sidebar_select_button_key_press_cb),
			  xviewer_sidebar);

	select_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	xviewer_sidebar->priv->label = gtk_label_new ("");

	gtk_box_pack_start (GTK_BOX (select_hbox),
			    xviewer_sidebar->priv->label,
			    FALSE, FALSE, 0);

	gtk_widget_show (xviewer_sidebar->priv->label);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_box_pack_end (GTK_BOX (select_hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_container_add (GTK_CONTAINER (xviewer_sidebar->priv->select_button), select_hbox);
	gtk_widget_show (select_hbox);

	gtk_box_pack_start (GTK_BOX (hbox), xviewer_sidebar->priv->select_button, TRUE, TRUE, 0);
	gtk_widget_show (xviewer_sidebar->priv->select_button);

	close_button = gtk_button_new ();

	gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);

	g_signal_connect (close_button, "clicked",
			  G_CALLBACK (xviewer_sidebar_close_clicked_cb),
			  xviewer_sidebar);

	image = gtk_image_new_from_icon_name ("window-close-symbolic",
					      GTK_ICON_SIZE_MENU);
	gtk_container_add (GTK_CONTAINER (close_button), image);
	gtk_widget_show (image);

	gtk_box_pack_end (GTK_BOX (hbox), close_button, FALSE, FALSE, 0);
	gtk_widget_show (close_button);

	xviewer_sidebar->priv->menu = gtk_menu_new ();

	g_signal_connect (xviewer_sidebar->priv->menu, "deactivate",
			  G_CALLBACK (xviewer_sidebar_menu_deactivate_cb),
			  xviewer_sidebar->priv->select_button);

	gtk_menu_attach_to_widget (GTK_MENU (xviewer_sidebar->priv->menu),
				   GTK_WIDGET (xviewer_sidebar),
				   xviewer_sidebar_menu_detach_cb);

	gtk_widget_show (xviewer_sidebar->priv->menu);

	xviewer_sidebar->priv->notebook = gtk_notebook_new ();

	gtk_notebook_set_show_border (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook), FALSE);

	gtk_box_pack_start (GTK_BOX (xviewer_sidebar), xviewer_sidebar->priv->notebook,
			    TRUE, TRUE, 0);

	gtk_widget_show (xviewer_sidebar->priv->notebook);
}

GtkWidget *
xviewer_sidebar_new (void)
{
	GtkWidget *xviewer_sidebar;

	xviewer_sidebar = g_object_new (XVIEWER_TYPE_SIDEBAR, NULL);

	return xviewer_sidebar;
}

void
xviewer_sidebar_add_page (XviewerSidebar   *xviewer_sidebar,
		      const gchar  *title,
		      GtkWidget    *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *menu_item;
	gchar *label_title;
	gint index;

	g_return_if_fail (XVIEWER_IS_SIDEBAR (xviewer_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	index = gtk_notebook_append_page (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook),
					  main_widget, NULL);

	menu_item = gtk_menu_item_new_with_label (title);

	g_signal_connect (menu_item, "activate",
			  G_CALLBACK (xviewer_sidebar_menu_item_activate_cb),
			  xviewer_sidebar);

	gtk_widget_show (menu_item);

	gtk_menu_shell_append (GTK_MENU_SHELL (xviewer_sidebar->priv->menu),
			       menu_item);

	/* Insert and move to end */
	gtk_list_store_insert_with_values (GTK_LIST_STORE (xviewer_sidebar->priv->page_model),
					   &iter, 0,
					   PAGE_COLUMN_TITLE, title,
					   PAGE_COLUMN_MENU_ITEM, menu_item,
					   PAGE_COLUMN_MAIN_WIDGET, main_widget,
					   PAGE_COLUMN_NOTEBOOK_INDEX, index,
					   -1);

	gtk_list_store_move_before (GTK_LIST_STORE(xviewer_sidebar->priv->page_model),
				    &iter,
				    NULL);

	/* Set the first item added as active */
	gtk_tree_model_get_iter_first (xviewer_sidebar->priv->page_model, &iter);
	gtk_tree_model_get (xviewer_sidebar->priv->page_model,
			    &iter,
			    PAGE_COLUMN_TITLE, &label_title,
			    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
			    -1);

	gtk_menu_set_active (GTK_MENU (xviewer_sidebar->priv->menu), index);

	gtk_label_set_text (GTK_LABEL (xviewer_sidebar->priv->label), label_title);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook),
				       index);

	g_free (label_title);

	g_signal_emit (G_OBJECT (xviewer_sidebar),
		       signals[SIGNAL_PAGE_ADDED], 0, main_widget);
}

void
xviewer_sidebar_remove_page (XviewerSidebar *xviewer_sidebar, GtkWidget *main_widget)
{
	GtkTreeIter iter;
	GtkWidget *widget, *menu_item;
	gboolean valid;
	gint index;

	g_return_if_fail (XVIEWER_IS_SIDEBAR (xviewer_sidebar));
	g_return_if_fail (GTK_IS_WIDGET (main_widget));

	valid = gtk_tree_model_get_iter_first (xviewer_sidebar->priv->page_model, &iter);

	while (valid) {
		gtk_tree_model_get (xviewer_sidebar->priv->page_model, &iter,
				    PAGE_COLUMN_NOTEBOOK_INDEX, &index,
				    PAGE_COLUMN_MENU_ITEM, &menu_item,
				    PAGE_COLUMN_MAIN_WIDGET, &widget,
				    -1);

		if (widget == main_widget) {
			break;
		} else {
			valid = gtk_tree_model_iter_next (xviewer_sidebar->priv->page_model,
							  &iter);
		}

		g_object_unref (menu_item);
		g_object_unref (widget);
	}

	if (valid) {
		gtk_notebook_remove_page (GTK_NOTEBOOK (xviewer_sidebar->priv->notebook),
					  index);

		gtk_container_remove (GTK_CONTAINER (xviewer_sidebar->priv->menu), menu_item);

		gtk_list_store_remove (GTK_LIST_STORE (xviewer_sidebar->priv->page_model),
				       &iter);

		g_signal_emit (G_OBJECT (xviewer_sidebar),
			       signals[SIGNAL_PAGE_REMOVED], 0, main_widget);
	}
}

gint
xviewer_sidebar_get_n_pages (XviewerSidebar *xviewer_sidebar)
{
	g_return_val_if_fail (XVIEWER_IS_SIDEBAR (xviewer_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (xviewer_sidebar->priv->page_model), NULL);
}

gboolean
xviewer_sidebar_is_empty (XviewerSidebar *xviewer_sidebar)
{
	g_return_val_if_fail (XVIEWER_IS_SIDEBAR (xviewer_sidebar), TRUE);

	return gtk_tree_model_iter_n_children (
		GTK_TREE_MODEL (xviewer_sidebar->priv->page_model), NULL) == 0;
}

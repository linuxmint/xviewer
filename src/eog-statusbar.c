/* Eye of Gnome - Statusbar
 *
 * Copyright (C) 2000-2006 The Free Software Foundation
 *
 * Author: Federico Mena-Quintero <federico@gnome.org>
 *	   Jens Finke <jens@gnome.org>
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

#include "eog-statusbar.h"

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

struct _EogStatusbarPrivate
{
	GtkWidget *progressbar;
	GtkWidget *img_num_label;
};

G_DEFINE_TYPE_WITH_PRIVATE (EogStatusbar, eog_statusbar, GTK_TYPE_STATUSBAR)

static void
eog_statusbar_class_init (EogStatusbarClass *klass)
{
    /* empty */
}

static void
eog_statusbar_init (EogStatusbar *statusbar)
{
	EogStatusbarPrivate *priv;
	GtkWidget *vbox;

	statusbar->priv = eog_statusbar_get_instance_private (statusbar);
	priv = statusbar->priv;

	priv->img_num_label = gtk_label_new (NULL);
	gtk_widget_set_size_request (priv->img_num_label, 100, 10);
	gtk_widget_show (priv->img_num_label);

	gtk_box_pack_end (GTK_BOX (statusbar),
			  priv->img_num_label,
			  FALSE,
			  TRUE,
			  0);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

	gtk_box_pack_end (GTK_BOX (statusbar),
			  vbox,
			  FALSE,
			  FALSE,
			  2);

	statusbar->priv->progressbar = gtk_progress_bar_new ();

	gtk_box_pack_end (GTK_BOX (vbox),
			  priv->progressbar,
			  TRUE,
			  TRUE,
			  0);

	/* Set margins by hand to avoid causing redraws due to the statusbar
	 * becoming too small for the progressbar */
	gtk_widget_set_margin_left (priv->progressbar, 2);
	gtk_widget_set_margin_right (priv->progressbar, 2);
	gtk_widget_set_margin_top (priv->progressbar, 1);
	gtk_widget_set_margin_bottom (priv->progressbar, 0);

	gtk_widget_set_size_request (priv->progressbar, -1, 10);

	gtk_widget_show (vbox);

	gtk_widget_hide (statusbar->priv->progressbar);

}

GtkWidget *
eog_statusbar_new (void)
{
	return GTK_WIDGET (g_object_new (EOG_TYPE_STATUSBAR, NULL));
}

void
eog_statusbar_set_image_number (EogStatusbar *statusbar,
                                gint          num,
				gint          tot)
{
	gchar *msg;

	g_return_if_fail (EOG_IS_STATUSBAR (statusbar));

	/* Hide number display if values don't make sense */
	if (G_UNLIKELY (num <= 0 || tot <= 0))
		return;

	/* Translators: This string is displayed in the statusbar.
	 * The first token is the image number, the second is total image
	 * count.
	 *
	 * Translate to "%Id" if you want to use localized digits, or
	 * translate to "%d" otherwise.
	 *
	 * Note that translating this doesn't guarantee that you get localized
	 * digits. That needs support from your system and locale definition
	 * too.*/
	msg = g_strdup_printf (_("%d / %d"), num, tot);

	gtk_label_set_text (GTK_LABEL (statusbar->priv->img_num_label), msg);

      	g_free (msg);
}

void
eog_statusbar_set_progress (EogStatusbar *statusbar,
			    gdouble       progress)
{
	g_return_if_fail (EOG_IS_STATUSBAR (statusbar));

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (statusbar->priv->progressbar),
				       progress);

	if (progress > 0 && progress < 1) {
		gtk_widget_show (statusbar->priv->progressbar);
		gtk_widget_hide (statusbar->priv->img_num_label);
	} else {
		gtk_widget_hide (statusbar->priv->progressbar);
		gtk_widget_show (statusbar->priv->img_num_label);
	}
}

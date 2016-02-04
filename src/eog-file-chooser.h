/*
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

#ifndef _XVIEWER_FILE_CHOOSER_H_
#define _XVIEWER_FILE_CHOOSER_H_

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#define XVIEWER_TYPE_FILE_CHOOSER          (xviewer_file_chooser_get_type ())
#define XVIEWER_FILE_CHOOSER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_FILE_CHOOSER, XviewerFileChooser))
#define XVIEWER_FILE_CHOOSER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_FILE_CHOOSER, XviewerFileChooserClass))

#define XVIEWER_IS_FILE_CHOOSER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_FILE_CHOOSER))
#define XVIEWER_IS_FILE_CHOOSER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_FILE_CHOOSER))
#define XVIEWER_FILE_CHOOSER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_FILE_CHOOSER, XviewerFileChooserClass))

typedef struct _XviewerFileChooser         XviewerFileChooser;
typedef struct _XviewerFileChooserClass    XviewerFileChooserClass;
typedef struct _XviewerFileChooserPrivate  XviewerFileChooserPrivate;

struct _XviewerFileChooser
{
	GtkFileChooserDialog  parent;

	XviewerFileChooserPrivate *priv;
};

struct _XviewerFileChooserClass
{
	GtkFileChooserDialogClass  parent_class;
};


GType		 xviewer_file_chooser_get_type	(void) G_GNUC_CONST;

GtkWidget	*xviewer_file_chooser_new		(GtkFileChooserAction action);

GdkPixbufFormat	*xviewer_file_chooser_get_format	(XviewerFileChooser *chooser);


G_END_DECLS

#endif /* _XVIEWER_FILE_CHOOSER_H_ */

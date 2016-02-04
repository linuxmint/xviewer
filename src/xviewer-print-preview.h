/* Xviewer -- Print Preview Widget
 *
 * Copyright (C) 2006-2007 The Free Software Foundation
 *
 * Author: Claudio Saavedra <csaavedra@alumnos.utalca.cl>
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

#ifndef _XVIEWER_PRINT_PREVIEW_H_
#define _XVIEWER_PRINT_PREVIEW_H_

G_BEGIN_DECLS

typedef struct _XviewerPrintPreview XviewerPrintPreview;
typedef struct _XviewerPrintPreviewClass XviewerPrintPreviewClass;
typedef struct _XviewerPrintPreviewPrivate XviewerPrintPreviewPrivate;

#define XVIEWER_TYPE_PRINT_PREVIEW            (xviewer_print_preview_get_type ())
#define XVIEWER_PRINT_PREVIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_PRINT_PREVIEW, XviewerPrintPreview))
#define XVIEWER_PRINT_PREVIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), XVIEWER_TYPE_PRINT_PREVIEW, XviewerPrintPreviewClass))
#define XVIEWER_IS_PRINT_PREVIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_PRINT_PREVIEW))
#define XVIEWER_IS_PRINT_PREVIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), XVIEWER_TYPE_PRINT_PREVIEW))

struct _XviewerPrintPreview {
	GtkAspectFrame aspect_frame;

	XviewerPrintPreviewPrivate *priv;
};

struct _XviewerPrintPreviewClass {
	GtkAspectFrameClass parent_class;

};

G_GNUC_INTERNAL
GType        xviewer_print_preview_get_type            (void) G_GNUC_CONST;

G_GNUC_INTERNAL
GtkWidget   *xviewer_print_preview_new                 (void);

G_GNUC_INTERNAL
GtkWidget   *xviewer_print_preview_new_with_pixbuf     (GdkPixbuf       *pixbuf);

G_GNUC_INTERNAL
void         xviewer_print_preview_set_page_margins    (XviewerPrintPreview *preview,
						    gfloat          l_margin,
						    gfloat          r_margin,
						    gfloat          t_margin,
						    gfloat          b_margin);

G_GNUC_INTERNAL
void         xviewer_print_preview_set_from_page_setup (XviewerPrintPreview *preview,
						    GtkPageSetup    *setup);

G_GNUC_INTERNAL
void         xviewer_print_preview_get_image_position  (XviewerPrintPreview *preview,
						    gdouble         *x,
						    gdouble         *y);

G_GNUC_INTERNAL
void         xviewer_print_preview_set_image_position  (XviewerPrintPreview *preview,
						    gdouble          x,
						    gdouble          y);

G_GNUC_INTERNAL
gboolean     xviewer_print_preview_point_in_image_area (XviewerPrintPreview *preview,
						    guint            x,
						    guint            y);
G_GNUC_INTERNAL
void         xviewer_print_preview_set_scale           (XviewerPrintPreview *preview,
						    gfloat           scale);
G_GNUC_INTERNAL
gfloat       xviewer_print_preview_get_scale           (XviewerPrintPreview *preview);

G_END_DECLS

#endif /* _XVIEWER_PRINT_PREVIEW_H_ */

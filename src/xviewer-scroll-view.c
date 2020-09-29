#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#ifdef HAVE_RSVG
#include <librsvg/rsvg.h>
#endif

#include "xviewer-config-keys.h"
#include "xviewer-enum-types.h"
#include "xviewer-scroll-view.h"
#include "xviewer-debug.h"

#if 0
#include "uta.h"
#endif
#include "zoom.h"

/* Maximum size of delayed repaint rectangles */
#define PAINT_RECT_WIDTH 128
#define PAINT_RECT_HEIGHT 128

/* Scroll step increment */
#define SCROLL_STEP_SIZE 32

/* Maximum zoom factor */
#define MAX_ZOOM_FACTOR 20
#define MIN_ZOOM_FACTOR 0.02

#define CHECK_MEDIUM 8
#define CHECK_BLACK "#000000"
#define CHECK_DARK "#555555"
#define CHECK_GRAY "#808080"
#define CHECK_LIGHT "#cccccc"
#define CHECK_WHITE "#ffffff"

/* Default increment for zooming.  The current zoom factor is multiplied or
 * divided by this amount on every zooming step.  For consistency, you should
 * use the same value elsewhere in the program.
 */
#define IMAGE_VIEW_ZOOM_MULTIPLIER 1.05

/* from cairo-utils.h */
#define _CAIRO_MAX_IMAGE_SIZE 32767


#if 0
/* Progressive loading state */
typedef enum {
	PROGRESSIVE_NONE,	/* We are not loading an image or it is already loaded */
	PROGRESSIVE_LOADING,	/* An image is being loaded */
	PROGRESSIVE_POLISHING	/* We have finished loading an image but have not scaled it with interpolation */
} ProgressiveState;
#endif

/* Signal IDs */
enum {
	SIGNAL_ZOOM_CHANGED,
	SIGNAL_ROTATION_CHANGED,
	SIGNAL_NEXT_IMAGE,
	SIGNAL_PREVIOUS_IMAGE,
	SIGNAL_LAST
};
static gint view_signals [SIGNAL_LAST];

typedef enum {
	XVIEWER_SCROLL_VIEW_CURSOR_NORMAL,
	XVIEWER_SCROLL_VIEW_CURSOR_HIDDEN,
	XVIEWER_SCROLL_VIEW_CURSOR_DRAG
} XviewerScrollViewCursor;

typedef enum {
	XVIEWER_ROTATION_0,
	XVIEWER_ROTATION_90,
	XVIEWER_ROTATION_180,
	XVIEWER_ROTATION_270,
	N_XVIEWER_ROTATIONS
} XviewerRotationState;

typedef enum {
	XVIEWER_PAN_ACTION_NONE,
	XVIEWER_PAN_ACTION_NEXT,
	XVIEWER_PAN_ACTION_PREV
} XviewerPanAction;

/* Drag 'n Drop */
static GtkTargetEntry target_table[] = {
	{ "text/uri-list", 0, 0},
};

enum {
	PROP_0,
	PROP_ANTIALIAS_IN,
	PROP_ANTIALIAS_OUT,
	PROP_BACKGROUND_COLOR,
	PROP_IMAGE,
	PROP_SCROLLWHEEL_ZOOM,
	PROP_TRANSP_COLOR,
	PROP_TRANSPARENCY_STYLE,
	PROP_USE_BG_COLOR,
	PROP_ZOOM_MODE,
	PROP_ZOOM_MULTIPLIER
};

/* Private part of the XviewerScrollView structure */
struct _XviewerScrollViewPrivate {
	/* some widgets we rely on */
	GtkWidget *display;
	GtkAdjustment *hadj;
	GtkAdjustment *vadj;
	GtkWidget *hbar;
	GtkWidget *vbar;
	GtkWidget *menu;

	/* actual image */
	XviewerImage *image;
	guint image_changed_id;
	guint frame_changed_id;
	GdkPixbuf *pixbuf;
	cairo_surface_t *surface;

	GSettings           *view_settings;

	/* zoom mode, either ZOOM_MODE_FIT or ZOOM_MODE_FREE */
	XviewerZoomMode zoom_mode;

	/* whether to allow zoom > 1.0 on zoom fit */
	gboolean upscale;

	/* the actual zoom factor */
	double zoom;

	/* the minimum possible (reasonable) zoom factor */
	double min_zoom;

	/* Current scrolling offsets */
	int xofs, yofs;

#if 0
	/* Microtile arrays for dirty region.  This represents the dirty region
	 * for interpolated drawing.
	 */
	XviewerUta *uta;
#endif

	/* handler ID for paint idle callback */
	guint idle_id;

	/* Interpolation type when zoomed in*/
	cairo_filter_t interp_type_in;

	/* Interpolation type when zoomed out*/
	cairo_filter_t interp_type_out;

	/* Scroll wheel zoom */
	gboolean scroll_wheel_zoom;

	/* Scroll wheel zoom */
	gdouble zoom_multiplier;

	/* dragging stuff */
	int drag_anchor_x, drag_anchor_y;
	int drag_ofs_x, drag_ofs_y;
	guint dragging : 1;

#if 0
	/* status of progressive loading */
	ProgressiveState progressive_state;
#endif

	/* how to indicate transparency in images */
	XviewerTransparencyStyle transp_style;
	GdkRGBA transp_color;

	/* the type of the cursor we are currently showing */
	XviewerScrollViewCursor cursor;

	gboolean  use_bg_color;
	GdkRGBA *background_color;
	GdkRGBA *override_bg_color;

	cairo_surface_t *background_surface;

#if GTK_CHECK_VERSION (3, 14, 0)
	GtkGesture *pan_gesture;
	GtkGesture *zoom_gesture;
	GtkGesture *rotate_gesture;
#endif
	gdouble initial_zoom;
	XviewerRotationState rotate_state;
	XviewerPanAction pan_action;

	/* Two-pass filtering */
	GSource *hq_redraw_timeout_source;
	gboolean force_unfiltered;
};

static void scroll_by (XviewerScrollView *view, int xofs, int yofs);
static void set_zoom_fit (XviewerScrollView *view);
/* static void request_paint_area (XviewerScrollView *view, GdkRectangle *area); */
static void set_minimum_zoom_factor (XviewerScrollView *view);
static void view_on_drag_begin_cb (GtkWidget *widget, GdkDragContext *context,
				   gpointer user_data);
static void view_on_drag_data_get_cb (GtkWidget *widget,
				      GdkDragContext*drag_context,
				      GtkSelectionData *data, guint info,
				      guint time, gpointer user_data);
static void _set_zoom_mode_internal (XviewerScrollView *view, XviewerZoomMode mode);
static gboolean xviewer_scroll_view_get_image_coords (XviewerScrollView *view, gint *x,
                                                  gint *y, gint *width,
                                                  gint *height);
static gboolean _xviewer_gdk_rgba_equal0 (const GdkRGBA *a, const GdkRGBA *b);


G_DEFINE_TYPE_WITH_PRIVATE (XviewerScrollView, xviewer_scroll_view, GTK_TYPE_GRID)


/*===================================
    widget size changing handler &
        util functions
  ---------------------------------*/

static cairo_surface_t *
create_surface_from_pixbuf (XviewerScrollView *view, GdkPixbuf *pixbuf)
{
	cairo_surface_t *surface;
	cairo_t *cr;
	gint w, h;
	gboolean size_invalid = FALSE;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

    if (w > _CAIRO_MAX_IMAGE_SIZE || h > _CAIRO_MAX_IMAGE_SIZE) {
        g_warning ("Image dimensions too large to process");
        w = 50;
        h = 50;
        size_invalid = TRUE;
    }

	surface = gdk_window_create_similar_surface (gtk_widget_get_window (view->priv->display),
						     CAIRO_CONTENT_COLOR | CAIRO_CONTENT_ALPHA,
						     w, h);

	if (size_invalid) {
		return surface;
	}

	cr = cairo_create (surface);
	gdk_cairo_set_source_pixbuf (cr, pixbuf, 0, 0);
	cairo_paint (cr);
	cairo_destroy (cr);

	return surface;
}

/* Disconnects from the XviewerImage and removes references to it */
static void
free_image_resources (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->image_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->image_changed_id);
		priv->image_changed_id = 0;
	}

	if (priv->frame_changed_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (priv->image), priv->frame_changed_id);
		priv->frame_changed_id = 0;
	}

	if (priv->image != NULL) {
		xviewer_image_data_unref (priv->image);
		priv->image = NULL;
	}

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (priv->surface != NULL) {
		cairo_surface_destroy (priv->surface);
		priv->surface = NULL;
	}
}

/* Computes the size in pixels of the scaled image */
static void
compute_scaled_size (XviewerScrollView *view, double zoom, int *width, int *height)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf) {
		*width = floor (gdk_pixbuf_get_width (priv->pixbuf) * zoom + 0.5);
		*height = floor (gdk_pixbuf_get_height (priv->pixbuf) * zoom + 0.5);
	} else
		*width = *height = 0;
}

/* Computes the offsets for the new zoom value so that they keep the image
 * centered on the view.
 */
static void
compute_center_zoom_offsets (XviewerScrollView *view,
			     double old_zoom, double new_zoom,
			     int width, int height,
			     double zoom_x_anchor, double zoom_y_anchor,
			     int *xofs, int *yofs)
{
	XviewerScrollViewPrivate *priv;
	int old_scaled_width, old_scaled_height;
	int new_scaled_width, new_scaled_height;
	double view_cx, view_cy;
	GtkRequisition req;

	priv = view->priv;

	compute_scaled_size (view, old_zoom,
			     &old_scaled_width, &old_scaled_height);

	if (old_scaled_width < width)
		view_cx = (zoom_x_anchor * old_scaled_width) / old_zoom;
	else
		view_cx = (priv->xofs + zoom_x_anchor * width) / old_zoom;

	if (old_scaled_height < height)
		view_cy = (zoom_y_anchor * old_scaled_height) / old_zoom;
	else
		view_cy = (priv->yofs + zoom_y_anchor * height) / old_zoom;

	compute_scaled_size (view, new_zoom,
			     &new_scaled_width, &new_scaled_height);

	if (new_scaled_width < width)
		*xofs = 0;
	else {
		if (old_scaled_width < width)
			*xofs = floor (view_cx * new_zoom - zoom_x_anchor * old_scaled_width - ((width - old_scaled_width) / 2) + 0.5);
		else
			*xofs = floor (view_cx * new_zoom - zoom_x_anchor * width + 0.5);
		if (*xofs < 0)
			*xofs = 0;
	}

	if (new_scaled_height < height)
		*yofs = 0;
	else {
		if (old_scaled_height < height)
			*yofs = floor (view_cy * new_zoom - zoom_y_anchor * old_scaled_height - ((height - old_scaled_height) / 2) + 0.5);
		else
			*yofs = floor (view_cy * new_zoom - zoom_y_anchor * height + 0.5);
		if (*yofs < 0)
			*yofs = 0;
	}
}

/* Sets the scrollbar values based on the current scrolling offset */
static void
update_scrollbar_values (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	gdouble page_size,page_increment,step_increment;
	gdouble lower, upper;
	GtkAllocation allocation;

	priv = view->priv;

	if (!gtk_widget_get_visible (GTK_WIDGET (priv->hbar))
	    && !gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);
	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (gtk_widget_get_visible (GTK_WIDGET (priv->hbar))) {
		/* Set scroll increments */
		page_size = MIN (scaled_width, allocation.width);
		page_increment = allocation.width / 2;
		step_increment = SCROLL_STEP_SIZE;

		/* Set scroll bounds and new offsets */
		lower = 0;
		upper = scaled_width;
		priv->xofs = CLAMP (priv->xofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		gtk_adjustment_configure (priv->hadj, priv->xofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->hadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}

	if (gtk_widget_get_visible (GTK_WIDGET (priv->vbar))) {
		page_size = MIN (scaled_height, allocation.height);
		page_increment = allocation.height / 2;
		step_increment = SCROLL_STEP_SIZE;

		lower = 0;
		upper = scaled_height;
		priv->yofs = CLAMP (priv->yofs, 0, upper - page_size);

		g_signal_handlers_block_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);

		gtk_adjustment_configure (priv->vadj, priv->yofs, lower,
					  upper, step_increment,
					  page_increment, page_size);

		g_signal_handlers_unblock_matched (
			priv->vadj, G_SIGNAL_MATCH_DATA,
			0, 0, NULL, NULL, view);
	}
}

static void
xviewer_scroll_view_set_cursor (XviewerScrollView *view, XviewerScrollViewCursor new_cursor)
{
	GdkCursor *cursor = NULL;
	GdkDisplay *display;
	GtkWidget *widget;

	if (view->priv->cursor == new_cursor) {
		return;
	}

	widget = gtk_widget_get_toplevel (GTK_WIDGET (view));
	display = gtk_widget_get_display (widget);
	view->priv->cursor = new_cursor;

	switch (new_cursor) {
		case XVIEWER_SCROLL_VIEW_CURSOR_NORMAL:
			gdk_window_set_cursor (gtk_widget_get_window (widget), NULL);
			break;
                case XVIEWER_SCROLL_VIEW_CURSOR_HIDDEN:
                        cursor = gdk_cursor_new (GDK_BLANK_CURSOR);
                        break;
		case XVIEWER_SCROLL_VIEW_CURSOR_DRAG:
			cursor = gdk_cursor_new_for_display (display, GDK_FLEUR);
			break;
	}

	if (cursor) {
		gdk_window_set_cursor (gtk_widget_get_window (widget), cursor);
		g_object_unref (cursor);
		gdk_flush();
	}
}

/* Changes visibility of the scrollbars based on the zoom factor and the
 * specified allocation, or the current allocation if NULL is specified.
 */
static void
check_scrollbar_visibility (XviewerScrollView *view, GtkAllocation *alloc)
{
	XviewerScrollViewPrivate *priv;
	int bar_height;
	int bar_width;
	int img_width;
	int img_height;
	GtkRequisition req;
	int width, height;
	gboolean hbar_visible, vbar_visible;

	priv = view->priv;

	if (alloc) {
		width = alloc->width;
		height = alloc->height;
	} else {
		GtkAllocation allocation;

		gtk_widget_get_allocation (GTK_WIDGET (view), &allocation);
		width = allocation.width;
		height = allocation.height;
	}

	compute_scaled_size (view, priv->zoom, &img_width, &img_height);

	/* this should work fairly well in this special case for scrollbars */
	gtk_widget_get_preferred_size (priv->hbar, &req, NULL);
	bar_height = req.height;
	gtk_widget_get_preferred_size (priv->vbar, &req, NULL);
	bar_width = req.width;

	xviewer_debug_message (DEBUG_WINDOW, "Widget Size allocate: %i, %i   Bar: %i, %i\n",
			   width, height, bar_width, bar_height);

	hbar_visible = vbar_visible = FALSE;
	if (priv->zoom_mode == XVIEWER_ZOOM_MODE_SHRINK_TO_FIT)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width <= width && img_height <= height)
		hbar_visible = vbar_visible = FALSE;
	else if (img_width > width && img_height > height)
		hbar_visible = vbar_visible = TRUE;
	else if (img_width > width) {
		hbar_visible = TRUE;
		if (img_height <= (height - bar_height))
			vbar_visible = FALSE;
		else
			vbar_visible = TRUE;
	}
        else if (img_height > height) {
		vbar_visible = TRUE;
		if (img_width <= (width - bar_width))
			hbar_visible = FALSE;
		else
			hbar_visible = TRUE;
	}

	if (hbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->hbar)))
		g_object_set (G_OBJECT (priv->hbar), "visible", hbar_visible, NULL);

	if (vbar_visible != gtk_widget_get_visible (GTK_WIDGET (priv->vbar)))
		g_object_set (G_OBJECT (priv->vbar), "visible", vbar_visible, NULL);
}

#define DOUBLE_EQUAL_MAX_DIFF 1e-6
#define DOUBLE_EQUAL(a,b) (fabs (a - b) < DOUBLE_EQUAL_MAX_DIFF)

#if 0
/* Returns whether the zoom factor is 1.0 */
static gboolean
is_unity_zoom (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL (priv->zoom, 1.0);
}
#endif

/* Returns whether the image is zoomed in */
static gboolean
is_zoomed_in (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	return priv->zoom - 1.0 > DOUBLE_EQUAL_MAX_DIFF;
}

/* Returns whether the image is zoomed out */
static gboolean
is_zoomed_out (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	return DOUBLE_EQUAL_MAX_DIFF + priv->zoom - 1.0 < 0.0;
}

/* Returns wether the image is movable, that means if it is larger then
 * the actual visible area.
 */
static gboolean
is_image_movable (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	return (gtk_widget_get_visible (priv->hbar) || gtk_widget_get_visible (priv->vbar));
}


/* Computes the image offsets with respect to the window */
/*
static void
get_image_offsets (XviewerScrollView *view, int *xofs, int *yofs)
{
	XviewerScrollViewPrivate *priv;
	int scaled_width, scaled_height;
	int width, height;

	priv = view->priv;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	width = GTK_WIDGET (priv->display)->allocation.width;
	height = GTK_WIDGET (priv->display)->allocation.height;

	// Compute image offsets with respect to the window
	if (scaled_width <= width)
		*xofs = (width - scaled_width) / 2;
	else
		*xofs = -priv->xofs;

	if (scaled_height <= height)
		*yofs = (height - scaled_height) / 2;
	else
		*yofs = -priv->yofs;
}
*/

/*===================================
          drawing core
  ---------------------------------*/


#if 0
/* Pulls a rectangle from the specified microtile array.  The rectangle is the
 * first one that would be glommed together by art_rect_list_from_uta(), and its
 * size is bounded by max_width and max_height.  The rectangle is also removed
 * from the microtile array.
 */
static void
pull_rectangle (XviewerUta *uta, XviewerIRect *rect, int max_width, int max_height)
{
	uta_find_first_glom_rect (uta, rect, max_width, max_height);
	uta_remove_rect (uta, rect->x0, rect->y0, rect->x1, rect->y1);
}

/* Paints a rectangle with the background color if the specified rectangle
 * intersects the dirty rectangle.
 */
static void
paint_background (XviewerScrollView *view, XviewerIRect *r, XviewerIRect *rect)
{
	XviewerScrollViewPrivate *priv;
	XviewerIRect d;

	priv = view->priv;

	xviewer_irect_intersect (&d, r, rect);
	if (!xviewer_irect_empty (&d)) {
		gdk_window_clear_area (gtk_widget_get_window (priv->display),
				       d.x0, d.y0,
				       d.x1 - d.x0, d.y1 - d.y0);
	}
}
#endif

static void
get_transparency_params (XviewerScrollView *view, int *size, GdkRGBA *color1, GdkRGBA *color2)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	/* Compute transparency parameters */
	switch (priv->transp_style) {
	case XVIEWER_TRANSP_BACKGROUND: {
		/* Simply return fully transparent color */
		color1->red = color1->green = color1->blue = color1->alpha = 0.0;
		color2->red = color2->green = color2->blue = color2->alpha = 0.0;
		break;
	}

	case XVIEWER_TRANSP_CHECKED:
		g_warn_if_fail (gdk_rgba_parse (color1, CHECK_GRAY));
		g_warn_if_fail (gdk_rgba_parse (color2, CHECK_LIGHT));
		break;

	case XVIEWER_TRANSP_COLOR:
		*color1 = *color2 = priv->transp_color;
		break;

	default:
		g_assert_not_reached ();
	};

	*size = CHECK_MEDIUM;
}


static cairo_surface_t *
create_background_surface (XviewerScrollView *view)
{
	int check_size;
	GdkRGBA check_1;
	GdkRGBA check_2;
	cairo_surface_t *surface;

	get_transparency_params (view, &check_size, &check_1, &check_2);
	surface = gdk_window_create_similar_surface (gtk_widget_get_window (view->priv->display),
						     CAIRO_CONTENT_COLOR_ALPHA,
						     check_size * 2, check_size * 2);
	cairo_t* cr = cairo_create (surface);

	/* Use source operator to make fully transparent work */
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

	gdk_cairo_set_source_rgba(cr, &check_1);
	cairo_rectangle (cr, 0, 0, check_size, check_size);
	cairo_rectangle (cr, check_size, check_size, check_size, check_size);
	cairo_fill (cr);

	gdk_cairo_set_source_rgba(cr, &check_2);
	cairo_rectangle (cr, 0, check_size, check_size, check_size);
	cairo_rectangle (cr, check_size, 0, check_size, check_size);
	cairo_fill (cr);

	cairo_destroy (cr);

	return surface;
}

#if 0
#ifdef HAVE_RSVG
static cairo_surface_t *
create_background_surface (XviewerScrollView *view)
{
	int check_size;
	guint32 check_1 = 0;
	guint32 check_2 = 0;
	cairo_surface_t *surface;
	cairo_t *check_cr;

	get_transparency_params (view, &check_size, &check_1, &check_2);

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, check_size * 2, check_size * 2);
	check_cr = cairo_create (surface);
	cairo_set_source_rgba (check_cr,
			       ((check_1 & 0xff0000) >> 16) / 255.,
			       ((check_1 & 0x00ff00) >> 8)  / 255.,
			        (check_1 & 0x0000ff)        / 255.,
				1.);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_translate (check_cr, check_size, check_size);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);

	cairo_set_source_rgba (check_cr,
			       ((check_2 & 0xff0000) >> 16) / 255.,
			       ((check_2 & 0x00ff00) >> 8)  / 255.,
			        (check_2 & 0x0000ff)        / 255.,
				1.);
	cairo_translate (check_cr, -check_size, 0);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_translate (check_cr, check_size, -check_size);
	cairo_rectangle (check_cr, 0., 0., check_size, check_size);
	cairo_fill (check_cr);
	cairo_destroy (check_cr);

	return surface;
}

static void
draw_svg_background (XviewerScrollView *view, cairo_t *cr, XviewerIRect *render_rect, XviewerIRect *image_rect)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->background_surface == NULL)
		priv->background_surface = create_background_surface (view);

	cairo_set_source_surface (cr, priv->background_surface,
				  - (render_rect->x0 - image_rect->x0) % (CHECK_MEDIUM * 2),
				  - (render_rect->y0 - image_rect->y0) % (CHECK_MEDIUM * 2));
	cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
	cairo_rectangle (cr,
			 0,
			 0,
			 render_rect->x1 - render_rect->x0,
			 render_rect->y1 - render_rect->y0);
	cairo_fill (cr);
}

static cairo_surface_t *
draw_svg_on_image_surface (XviewerScrollView *view, XviewerIRect *render_rect, XviewerIRect *image_rect)
{
	XviewerScrollViewPrivate *priv;
	cairo_t *cr;
	cairo_surface_t *surface;
	cairo_matrix_t matrix, translate, scale;
	XviewerTransform *transform;

	priv = view->priv;

	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      render_rect->x1 - render_rect->x0,
					      render_rect->y1 - render_rect->y0);
	cr = cairo_create (surface);

	cairo_save (cr);
	draw_svg_background (view, cr, render_rect, image_rect);
	cairo_restore (cr);

	cairo_matrix_init_identity (&matrix);
	transform = xviewer_image_get_transform (priv->image);
	if (transform) {
		cairo_matrix_t affine;
		double image_offset_x = 0., image_offset_y = 0.;

		xviewer_transform_get_affine (transform, &affine);
		cairo_matrix_multiply (&matrix, &affine, &matrix);

		switch (xviewer_transform_get_transform_type (transform)) {
		case XVIEWER_TRANSFORM_ROT_90:
		case XVIEWER_TRANSFORM_FLIP_HORIZONTAL:
			image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
			break;
		case XVIEWER_TRANSFORM_ROT_270:
		case XVIEWER_TRANSFORM_FLIP_VERTICAL:
			image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
			break;
		case XVIEWER_TRANSFORM_ROT_180:
		case XVIEWER_TRANSFORM_TRANSPOSE:
		case XVIEWER_TRANSFORM_TRANSVERSE:
			image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
			image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
			break;
		case XVIEWER_TRANSFORM_NONE:
		default:
			break;
		}

		cairo_matrix_init_translate (&translate, image_offset_x, image_offset_y);
		cairo_matrix_multiply (&matrix, &matrix, &translate);
	}

	cairo_matrix_init_scale (&scale, priv->zoom, priv->zoom);
	cairo_matrix_multiply (&matrix, &matrix, &scale);
	cairo_matrix_init_translate (&translate, image_rect->x0, image_rect->y0);
	cairo_matrix_multiply (&matrix, &matrix, &translate);
	cairo_matrix_init_translate (&translate, -render_rect->x0, -render_rect->y0);
	cairo_matrix_multiply (&matrix, &matrix, &translate);

	cairo_set_matrix (cr, &matrix);

	rsvg_handle_render_cairo (xviewer_image_get_svg (priv->image), cr);
	cairo_destroy (cr);

	return surface;
}

static void
draw_svg (XviewerScrollView *view, XviewerIRect *render_rect, XviewerIRect *image_rect)
{
	XviewerScrollViewPrivate *priv;
	cairo_t *cr;
	cairo_surface_t *surface;
	GdkWindow *window;

	priv = view->priv;

	window = gtk_widget_get_window (GTK_WIDGET (priv->display));
	surface = draw_svg_on_image_surface (view, render_rect, image_rect);

	cr = gdk_cairo_create (window);
	cairo_set_source_surface (cr, surface, render_rect->x0, render_rect->y0);
	cairo_paint (cr);
	cairo_destroy (cr);
}
#endif

/* Paints a rectangle of the dirty region */
static void
paint_rectangle (XviewerScrollView *view, XviewerIRect *rect, cairo_filter_t interp_type)
{
	XviewerScrollViewPrivate *priv;
	GdkPixbuf *tmp;
	char *str;
	GtkAllocation allocation;
	int scaled_width, scaled_height;
	int xofs, yofs;
	XviewerIRect r, d;
	int check_size;
	guint32 check_1 = 0;
	guint32 check_2 = 0;

	priv = view->priv;

	if (!gtk_widget_is_drawable (priv->display))
		return;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (scaled_width < 1 || scaled_height < 1)
	{
		r.x0 = 0;
		r.y0 = 0;
		r.x1 = allocation.width;
		r.y1 = allocation.height;
		paint_background (view, &r, rect);
		return;
	}

	/* Compute image offsets with respect to the window */

	if (scaled_width <= allocation.width)
		xofs = (allocation.width - scaled_width) / 2;
	else
		xofs = -priv->xofs;

	if (scaled_height <= allocation.height)
		yofs = (allocation.height - scaled_height) / 2;
	else
		yofs = -priv->yofs;

	xviewer_debug_message (DEBUG_WINDOW, "zoom %.2f, xofs: %i, yofs: %i scaled w: %i h: %i\n",
			   priv->zoom, xofs, yofs, scaled_width, scaled_height);

	/* Draw background if necessary, in four steps */

	/* Top */
	if (yofs > 0) {
		r.x0 = 0;
		r.y0 = 0;
		r.x1 = allocation.width;
		r.y1 = yofs;
		paint_background (view, &r, rect);
	}

	/* Left */
	if (xofs > 0) {
		r.x0 = 0;
		r.y0 = yofs;
		r.x1 = xofs;
		r.y1 = yofs + scaled_height;
		paint_background (view, &r, rect);
	}

	/* Right */
	if (xofs >= 0) {
		r.x0 = xofs + scaled_width;
		r.y0 = yofs;
		r.x1 = allocation.width;
		r.y1 = yofs + scaled_height;
		if (r.x0 < r.x1)
			paint_background (view, &r, rect);
	}

	/* Bottom */
	if (yofs >= 0) {
		r.x0 = 0;
		r.y0 = yofs + scaled_height;
		r.x1 = allocation.width;
		r.y1 = allocation.height;
		if (r.y0 < r.y1)
			paint_background (view, &r, rect);
	}


	/* Draw the scaled image
	 *
	 * FIXME: this is not using the color correction tables!
	 */

	if (!priv->pixbuf)
		return;

	r.x0 = xofs;
	r.y0 = yofs;
	r.x1 = xofs + scaled_width;
	r.y1 = yofs + scaled_height;

	xviewer_irect_intersect (&d, &r, rect);
	if (xviewer_irect_empty (&d))
		return;

	switch (interp_type) {
	case CAIRO_FILTER_NEAREST:
		str = "NEAREST";
		break;
	default:
		str = "ALIASED";
	}

	xviewer_debug_message (DEBUG_WINDOW, "%s: x0: %i,\t y0: %i,\t x1: %i,\t y1: %i\n",
			   str, d.x0, d.y0, d.x1, d.y1);

#ifdef HAVE_RSVG
	if (xviewer_image_is_svg (view->priv->image) && interp_type != CAIRO_FILTER_NEAREST) {
		draw_svg (view, &d, &r);
		return;
	}
#endif
	/* Short-circuit the fast case to avoid a memcpy() */

	if (is_unity_zoom (view)
	    && gdk_pixbuf_get_colorspace (priv->pixbuf) == GDK_COLORSPACE_RGB
	    && !gdk_pixbuf_get_has_alpha (priv->pixbuf)
	    && gdk_pixbuf_get_bits_per_sample (priv->pixbuf) == 8) {
		guchar *pixels;
		int rowstride;

		rowstride = gdk_pixbuf_get_rowstride (priv->pixbuf);

		pixels = (gdk_pixbuf_get_pixels (priv->pixbuf)
			  + (d.y0 - yofs) * rowstride
			  + 3 * (d.x0 - xofs));

		gdk_draw_rgb_image_dithalign (gtk_widget_get_window (GTK_WIDGET (priv->display)),
					      gtk_widget_get_style (GTK_WIDGET (priv->display))->black_gc,
					      d.x0, d.y0,
					      d.x1 - d.x0, d.y1 - d.y0,
					      GDK_RGB_DITHER_MAX,
					      pixels,
					      rowstride,
					      d.x0 - xofs, d.y0 - yofs);
		return;
	}

	/* For all other cases, create a temporary pixbuf */

	tmp = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, d.x1 - d.x0, d.y1 - d.y0);

	if (!tmp) {
		g_message ("paint_rectangle(): Could not allocate temporary pixbuf of "
			   "size (%d, %d); skipping", d.x1 - d.x0, d.y1 - d.y0);
		return;
	}

	/* Compute transparency parameters */
	get_transparency_params (view, &check_size, &check_1, &check_2);

	/* Draw! */
	gdk_pixbuf_composite_color (priv->pixbuf,
				    tmp,
				    0, 0,
				    d.x1 - d.x0, d.y1 - d.y0,
				    -(d.x0 - xofs), -(d.y0 - yofs),
				    priv->zoom, priv->zoom,
				    is_unity_zoom (view) ? CAIRO_FILTER_NEAREST : interp_type,
				    255,
				    d.x0 - xofs, d.y0 - yofs,
				    check_size,
				    check_1, check_2);

	gdk_draw_rgb_image_dithalign (gtk_widget_get_window (priv->display),
				      gtk_widget_get_style (priv->display)->black_gc,
				      d.x0, d.y0,
				      d.x1 - d.x0, d.y1 - d.y0,
				      GDK_RGB_DITHER_MAX,
				      gdk_pixbuf_get_pixels (tmp),
				      gdk_pixbuf_get_rowstride (tmp),
				      d.x0 - xofs, d.y0 - yofs);

	g_object_unref (tmp);
}


/* Idle handler for the drawing process.  We pull a rectangle from the dirty
 * region microtile array, paint it, and leave the rest to the next idle
 * iteration.
 */
static gboolean
paint_iteration_idle (gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	XviewerIRect rect;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	g_assert (priv->uta != NULL);

	pull_rectangle (priv->uta, &rect, PAINT_RECT_WIDTH, PAINT_RECT_HEIGHT);

	if (xviewer_irect_empty (&rect)) {
		xviewer_uta_free (priv->uta);
		priv->uta = NULL;
	} else {
		if (is_zoomed_in (view))
			paint_rectangle (view, &rect, priv->interp_type_in);
		else if (is_zoomed_out (view))
			paint_rectangle (view, &rect, priv->interp_type_out);
		else
			paint_rectangle (view, &rect, CAIRO_FILTER_NEAREST);
	}
		
	if (!priv->uta) {
		priv->idle_id = 0;
		return FALSE;
	}

	return TRUE;
}

/* Paints the requested area in non-interpolated mode.  Then, if we are
 * configured to use interpolation, we queue an idle handler to redraw the area
 * with interpolation.  The area is in window coordinates.
 */
static void
request_paint_area (XviewerScrollView *view, GdkRectangle *area)
{
	XviewerScrollViewPrivate *priv;
	XviewerIRect r;
	GtkAllocation allocation;

	priv = view->priv;

	xviewer_debug_message (DEBUG_WINDOW, "x: %i, y: %i, width: %i, height: %i\n",
			   area->x, area->y, area->width, area->height);

	if (!gtk_widget_is_drawable (priv->display))
		return;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);
	r.x0 = MAX (0, area->x);
	r.y0 = MAX (0, area->y);
	r.x1 = MIN (allocation.width, area->x + area->width);
	r.y1 = MIN (allocation.height, area->y + area->height);

	xviewer_debug_message (DEBUG_WINDOW, "r: %i, %i, %i, %i\n", r.x0, r.y0, r.x1, r.y1);

	if (r.x0 >= r.x1 || r.y0 >= r.y1)
		return;

	/* Do nearest neighbor, 1:1 zoom or active progressive loading synchronously for speed.  */
	if ((is_zoomed_in (view) && priv->interp_type_in == CAIRO_FILTER_NEAREST) ||
	    (is_zoomed_out (view) && priv->interp_type_out == CAIRO_FILTER_NEAREST) ||
	    is_unity_zoom (view) ||
	    priv->progressive_state == PROGRESSIVE_LOADING) {
		paint_rectangle (view, &r, CAIRO_FILTER_NEAREST);
		return;
	}

	if (priv->progressive_state == PROGRESSIVE_POLISHING)
		/* We have already a complete image with nearest neighbor mode.
		 * It's sufficient to add only a antitaliased idle update
		 */
		priv->progressive_state = PROGRESSIVE_NONE;
	else if (!priv->image || !xviewer_image_is_animation (priv->image))
		/* do nearest neigbor before anti aliased version,
		   except for animations to avoid a "blinking" effect. */
		paint_rectangle (view, &r, CAIRO_FILTER_NEAREST);

	/* All other interpolation types are delayed.  */
	if (priv->uta)
		g_assert (priv->idle_id != 0);
	else {
		g_assert (priv->idle_id == 0);
		priv->idle_id = g_idle_add (paint_iteration_idle, view);
	}

	priv->uta = uta_add_rect (priv->uta, r.x0, r.y0, r.x1, r.y1);
}
#endif


/* =======================================

    scrolling stuff

    --------------------------------------*/


/* Scrolls the view to the specified offsets.  */
static void
scroll_to (XviewerScrollView *view, int x, int y, gboolean change_adjustments)
{
	XviewerScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	GdkWindow *window;
#if 0
	int src_x, src_y;
	int dest_x, dest_y;
	int twidth, theight;
#endif

	priv = view->priv;

	/* Check bounds & Compute offsets */
	if (gtk_widget_get_visible (priv->hbar)) {
		x = CLAMP (x, 0, gtk_adjustment_get_upper (priv->hadj)
				 - gtk_adjustment_get_page_size (priv->hadj));
		xofs = x - priv->xofs;
	} else
		xofs = 0;

	if (gtk_widget_get_visible (priv->vbar)) {
		y = CLAMP (y, 0, gtk_adjustment_get_upper (priv->vadj)
				 - gtk_adjustment_get_page_size (priv->vadj));
		yofs = y - priv->yofs;
	} else
		yofs = 0;

	if (xofs == 0 && yofs == 0)
		return;

	priv->xofs = x;
	priv->yofs = y;

	if (!gtk_widget_is_drawable (priv->display))
		goto out;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	if (abs (xofs) >= allocation.width || abs (yofs) >= allocation.height) {
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		goto out;
	}

	window = gtk_widget_get_window (GTK_WIDGET (priv->display));

	/* Ensure that the uta has the full size */
#if 0
	twidth = (allocation.width + XVIEWER_UTILE_SIZE - 1) >> XVIEWER_UTILE_SHIFT;
	theight = (allocation.height + XVIEWER_UTILE_SIZE - 1) >> XVIEWER_UTILE_SHIFT;

#if 0
	if (priv->uta)
		g_assert (priv->idle_id != 0);
	else
		priv->idle_id = g_idle_add (paint_iteration_idle, view);
#endif

	priv->uta = uta_ensure_size (priv->uta, 0, 0, twidth, theight);

	/* Copy the uta area.  Our synchronous handling of expose events, below,
	 * will queue the new scrolled-in areas.
	 */
	src_x = xofs < 0 ? 0 : xofs;
	src_y = yofs < 0 ? 0 : yofs;
	dest_x = xofs < 0 ? -xofs : 0;
	dest_y = yofs < 0 ? -yofs : 0;

	uta_copy_area (priv->uta,
		       src_x, src_y,
		       dest_x, dest_y,
		       allocation.width - abs (xofs),
		       allocation.height - abs (yofs));
#endif
	/* Scroll the window area and process exposure synchronously. */

#if GTK_CHECK_VERSION (3, 14, 0)
	if (!gtk_gesture_is_recognized (priv->zoom_gesture)) {
		gdk_window_scroll (window, -xofs, -yofs);
		gdk_window_process_updates (window, TRUE);
	}
#else
	gdk_window_scroll (window, -xofs, -yofs);
	gdk_window_process_updates (window, TRUE);
#endif

 out:
	if (!change_adjustments)
		return;

	g_signal_handlers_block_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_block_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);

	gtk_adjustment_set_value (priv->hadj, x);
	gtk_adjustment_set_value (priv->vadj, y);

	g_signal_handlers_unblock_matched (
		priv->hadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
	g_signal_handlers_unblock_matched (
		priv->vadj, G_SIGNAL_MATCH_DATA,
		0, 0, NULL, NULL, view);
}

/* Scrolls the image view by the specified offsets.  Notifies the adjustments
 * about their new values.
 */
static void
scroll_by (XviewerScrollView *view, int xofs, int yofs)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	scroll_to (view, priv->xofs + xofs, priv->yofs + yofs, TRUE);
}

static gboolean _hq_redraw_cb (gpointer user_data)
{
	XviewerScrollViewPrivate *priv = XVIEWER_SCROLL_VIEW (user_data)->priv;

	priv->force_unfiltered = FALSE;
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	priv->hq_redraw_timeout_source = NULL;
	return G_SOURCE_REMOVE;
}

static void
_clear_hq_redraw_timeout (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv = view->priv;

	if (priv->hq_redraw_timeout_source != NULL) {
		g_source_unref (priv->hq_redraw_timeout_source);
		g_source_destroy (priv->hq_redraw_timeout_source);
	}

	priv->hq_redraw_timeout_source = NULL;
}

static void
_set_hq_redraw_timeout (XviewerScrollView *view)
{
	GSource *source;

	_clear_hq_redraw_timeout (view);

	source = g_timeout_source_new (200);
	g_source_set_callback (source, &_hq_redraw_cb, view, NULL);

	g_source_attach (source, NULL);

	view->priv->hq_redraw_timeout_source = source;
}

/* Callback used when an adjustment is changed */
static void
adjustment_changed_cb (GtkAdjustment *adj, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	scroll_to (view, gtk_adjustment_get_value (priv->hadj),
		   gtk_adjustment_get_value (priv->vadj), FALSE);
}


/* Drags the image to the specified position */
static void
drag_to (XviewerScrollView *view, int x, int y)
{
	XviewerScrollViewPrivate *priv;
	int dx, dy;

	priv = view->priv;

	dx = priv->drag_anchor_x - x;
	dy = priv->drag_anchor_y - y;

	x = priv->drag_ofs_x + dx;
	y = priv->drag_ofs_y + dy;

	scroll_to (view, x, y, TRUE);
}

static void
set_minimum_zoom_factor (XviewerScrollView *view)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	view->priv->min_zoom = MAX (1.0 / gdk_pixbuf_get_width (view->priv->pixbuf),
				    MAX(1.0 / gdk_pixbuf_get_height (view->priv->pixbuf),
					MIN_ZOOM_FACTOR) );
	return;
}

/**
 * set_zoom:
 * @view: A scroll view.
 * @zoom: Zoom factor.
 * @have_anchor: Whether the anchor point specified by (@anchorx, @anchory)
 * should be used.
 * @anchorx: Horizontal anchor point in pixels.
 * @anchory: Vertical anchor point in pixels.
 *
 * Sets the zoom factor for an image view.  The anchor point can be used to
 * specify the point that stays fixed when the image is zoomed.  If @have_anchor
 * is %TRUE, then (@anchorx, @anchory) specify the point relative to the image
 * view widget's allocation that will stay fixed when zooming.  If @have_anchor
 * is %FALSE, then the center point of the image view will be used.
 **/
static void
set_zoom (XviewerScrollView *view, double zoom,
	  gboolean have_anchor, int anchorx, int anchory)
{
	XviewerScrollViewPrivate *priv;
	GtkAllocation allocation;
	int xofs, yofs;
	double x_rel, y_rel;
	int zoomed_img_height;
	int zoomed_img_width;

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return;

	if (zoom > MAX_ZOOM_FACTOR)
		zoom = MAX_ZOOM_FACTOR;
	else if (zoom < MIN_ZOOM_FACTOR)
		zoom = MIN_ZOOM_FACTOR;

	if (DOUBLE_EQUAL (priv->zoom, zoom))
		return;
	if (DOUBLE_EQUAL (priv->zoom, priv->min_zoom) && zoom < priv->zoom)
		return;

	xviewer_scroll_view_set_zoom_mode (view, XVIEWER_ZOOM_MODE_FREE);

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* compute new xofs/yofs values */
	if (have_anchor) {
		int image_to_window_edge;

		compute_scaled_size (view, priv->zoom, &zoomed_img_width, &zoomed_img_height);

		if (zoomed_img_height >= allocation.height)
			y_rel = (double) anchory / allocation.height;
		else
		{
			image_to_window_edge = (allocation.height - zoomed_img_height) / 2;
			if (anchory < image_to_window_edge)
				y_rel = 0.0;
			else
			{
				y_rel = (double)(anchory - image_to_window_edge)/zoomed_img_height;
				if (y_rel > 1.0)
					y_rel = 1.0;
				if (y_rel < 0.0)
					y_rel = 0.0;
			}
		}

		if (zoomed_img_width >= allocation.width)
			x_rel = (double) anchorx / allocation.width;
		else
		{
			image_to_window_edge = (allocation.width - zoomed_img_width) / 2;
			if (anchorx < image_to_window_edge)
				x_rel = 0.0;
			else
			{
				x_rel = (double)(anchorx - image_to_window_edge)/zoomed_img_width;
				if (x_rel > 1.0)
					x_rel = 1.0;
				if (x_rel < 0.0)
					x_rel = 0.0;
			}
		}
	} else {
		x_rel = 0.5;
		y_rel = 0.5;
	}

	compute_center_zoom_offsets (view, priv->zoom, zoom,
				     allocation.width, allocation.height,
				     x_rel, y_rel,
				     &xofs, &yofs);

	/* set new values */
	priv->xofs = xofs; /* (img_width * x_rel * zoom) - anchorx; */
	priv->yofs = yofs; /* (img_height * y_rel * zoom) - anchory; */

	if (priv->dragging) {
		priv->drag_anchor_x = anchorx;
		priv->drag_anchor_y = anchory;
		priv->drag_ofs_x = priv->xofs;
		priv->drag_ofs_y = priv->yofs;
	}
#if 0
	g_print ("xofs: %i  yofs: %i\n", priv->xofs, priv->yofs);
#endif
	if (zoom <= priv->min_zoom)
		priv->zoom = priv->min_zoom;
	else
		priv->zoom = zoom;

	/* we make use of the new values here */
	check_scrollbar_visibility (view, NULL);
	update_scrollbar_values (view);

	/* repaint the whole image */
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/* Zooms the image to fit the available allocation */
static void
set_zoom_fit (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv;
	GtkAllocation allocation;
	double new_zoom;

	priv = view->priv;

	priv->zoom_mode = XVIEWER_ZOOM_MODE_SHRINK_TO_FIT;

	if (!gtk_widget_get_mapped (GTK_WIDGET (view)))
		return;

	if (priv->pixbuf == NULL)
		return;

	gtk_widget_get_allocation (GTK_WIDGET(priv->display), &allocation);

	new_zoom = zoom_fit_scale (allocation.width, allocation.height,
				   gdk_pixbuf_get_width (priv->pixbuf),
				   gdk_pixbuf_get_height (priv->pixbuf),
				   priv->upscale);

	if (new_zoom > MAX_ZOOM_FACTOR)
		new_zoom = MAX_ZOOM_FACTOR;
	else if (new_zoom < MIN_ZOOM_FACTOR)
		new_zoom = MIN_ZOOM_FACTOR;

	priv->zoom = new_zoom;
	priv->xofs = 0;
	priv->yofs = 0;

	g_signal_emit (view, view_signals [SIGNAL_ZOOM_CHANGED], 0, priv->zoom);
}

/*===================================

   internal signal callbacks

  ---------------------------------*/

/* Key press event handler for the image view */
static gboolean
display_key_press_event (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	GtkAllocation allocation;
	gboolean do_zoom;
	double zoom;
	gboolean do_scroll;
	int xofs, yofs;
	GdkModifierType modifiers;
	int zoomed_img_height;
	int zoomed_img_width;
	GtkRequisition req;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	do_zoom = FALSE;
	do_scroll = FALSE;
	xofs = yofs = 0;
	zoom = 1.0;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	modifiers = gtk_accelerator_get_default_mod_mask ();

	switch (event->keyval) {
	case GDK_KEY_Up:
		if ((event->state & modifiers) == GDK_MOD1_MASK) {
			do_scroll = TRUE;
			xofs = 0;
			yofs = -SCROLL_STEP_SIZE;
		}
		break;

	case GDK_KEY_Page_Up:
		if ((event->state & GDK_MOD1_MASK) != 0) {
			do_scroll = TRUE;
			if (event->state & GDK_CONTROL_MASK) {
				xofs = -(allocation.width * 3) / 4;
				yofs = 0;
			} else {
				xofs = 0;
				yofs = -(allocation.height * 3) / 4;
			}
		}
		break;

	case GDK_KEY_Down:
		if ((event->state & modifiers) == GDK_MOD1_MASK) {
			do_scroll = TRUE;
			xofs = 0;
			yofs = SCROLL_STEP_SIZE;
		}
		break;

	case GDK_KEY_Page_Down:
		if ((event->state & GDK_MOD1_MASK) != 0) {
			do_scroll = TRUE;
			if (event->state & GDK_CONTROL_MASK) {
				xofs = (allocation.width * 3) / 4;
				yofs = 0;
			} else {
				xofs = 0;
				yofs = (allocation.height * 3) / 4;
			}
		}
		break;

	case GDK_KEY_Left:
		if ((event->state & modifiers) == GDK_MOD1_MASK) {
			do_scroll = TRUE;
			xofs = -SCROLL_STEP_SIZE;
			yofs = 0;
		}
		break;

	case GDK_KEY_Right:
		if ((event->state & modifiers) == GDK_MOD1_MASK) {
			do_scroll = TRUE;
			xofs = SCROLL_STEP_SIZE;
			yofs = 0;
		}
		break;

	case GDK_KEY_plus:
	case GDK_KEY_equal:
	case GDK_KEY_KP_Add:
		if (!(event->state & modifiers)) {
			do_zoom = TRUE;
			zoom = priv->zoom * priv->zoom_multiplier;
		}
		break;

	case GDK_KEY_minus:
	case GDK_KEY_KP_Subtract:
		if (!(event->state & modifiers)) {
			do_zoom = TRUE;
			zoom = priv->zoom / priv->zoom_multiplier;
		}
		break;

	case GDK_KEY_1:
        if (!(event->state & modifiers)) {

            double zoom_for_fit;

            compute_scaled_size (view, priv->zoom, &zoomed_img_width, &zoomed_img_height);

            if (gtk_widget_get_visible (GTK_WIDGET (priv->hbar))) {
                gtk_widget_get_preferred_size (priv->hbar, &req, NULL);
                allocation.height += req.height;
            }

            if (gtk_widget_get_visible (GTK_WIDGET (priv->vbar))) {
                gtk_widget_get_preferred_size (priv->vbar, &req, NULL);
                allocation.width += req.width;
            }

            zoom_for_fit = zoom_fit_scale (allocation.width, allocation.height,
                    gdk_pixbuf_get_width (priv->pixbuf),
                    gdk_pixbuf_get_height (priv->pixbuf),
                    priv->upscale);

            if (zoom_for_fit > MAX_ZOOM_FACTOR)
                zoom_for_fit = MAX_ZOOM_FACTOR;
            else if (zoom_for_fit < MIN_ZOOM_FACTOR)
                zoom_for_fit = MIN_ZOOM_FACTOR;

            if ((gdk_pixbuf_get_width (priv->pixbuf) <= allocation.width)
                && (gdk_pixbuf_get_height (priv->pixbuf) <= allocation.height))
                    zoom = 1.0;                         /* the 1:1 image fits in the window */
            else
            {
                if (DOUBLE_EQUAL(priv->zoom, 1.0))
                    zoom = zoom_for_fit;
                else
                    zoom = 1.0;
            }

                            /* the following two statements are necessary otherwise if the 1:1 image
                               is dragged the alignment is thrown out */
            if (DOUBLE_EQUAL(priv->zoom,zoom_for_fit))
            {
                priv->xofs = 0;
                priv->yofs = 0;
            }

            do_zoom = TRUE;
		}
		break;

	default:
		return FALSE;
	}

	if (do_zoom) {
		GdkDeviceManager *device_manager;
		GdkDevice *device;
		gint x, y;

		device_manager = gdk_display_get_device_manager (gtk_widget_get_display(widget));
		device = gdk_device_manager_get_client_pointer (device_manager);

		gdk_window_get_device_position (gtk_widget_get_window (widget), device,
					&x, &y, NULL);
		set_zoom (view, zoom, TRUE, x, y);
	}

	if (do_scroll)
		scroll_by (view, xofs, yofs);

	if(!do_scroll && !do_zoom)
		return FALSE;

	return TRUE;
}


/* Button press event handler for the image view */
static gboolean
xviewer_scroll_view_button_press_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	if (!gtk_widget_has_focus (priv->display))
		gtk_widget_grab_focus (GTK_WIDGET (priv->display));

	if (priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
                        if (event->button == 1 && !priv->scroll_wheel_zoom &&
			    !(event->state & GDK_CONTROL_MASK))
				break;

			if (is_image_movable (view)) {
				xviewer_scroll_view_set_cursor (view, XVIEWER_SCROLL_VIEW_CURSOR_DRAG);

				priv->dragging = TRUE;
				priv->drag_anchor_x = event->x;
				priv->drag_anchor_y = event->y;

				priv->drag_ofs_x = priv->xofs;
				priv->drag_ofs_y = priv->yofs;

				return TRUE;
			}
		default:
			break;
	}

	return FALSE;
}

static void
xviewer_scroll_view_style_set (GtkWidget *widget, GtkStyle *old_style)
{
	GtkStyle *style;
	XviewerScrollViewPrivate *priv;

	style = gtk_widget_get_style (widget);
	priv = XVIEWER_SCROLL_VIEW (widget)->priv;

	gtk_widget_set_style (priv->display, style);
}


/* Button release event handler for the image view */
static gboolean
xviewer_scroll_view_button_release_event (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	if (!priv->dragging)
		return FALSE;

	switch (event->button) {
		case 1:
		case 2:
			drag_to (view, event->x, event->y);
			priv->dragging = FALSE;

			xviewer_scroll_view_set_cursor (view, XVIEWER_SCROLL_VIEW_CURSOR_NORMAL);
			break;

		default:
			break;
	}

	return TRUE;
}

/* Scroll event handler for the image view */
static gboolean
xviewer_scroll_view_scroll_event (GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	double zoom_factor;
	int xofs, yofs;
    int button_combination;         /* 0 = scroll, 1 = scroll + shift, 2 = scroll + ctrl, 3 = scroll + shift + ctrl
                                        4..7 as 0..3 but for tilt wheel */
    int action;                     /* 0 = zoom, 1 = vertical pan, 2 = horizontal pan, 3 = next/prev image */
    static guint32 mouse_wheel_time = 0;     /* used to debounce the mouse wheel (scroll and tilt)
                                                         when used for next/previous image or rotate image */



	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	priv->view_settings = g_settings_new (XVIEWER_CONF_VIEW);

	/* Compute zoom factor and scrolling offsets; we'll only use either of them */
	/* same as in gtkscrolledwindow.c */
	xofs = gtk_adjustment_get_page_increment (priv->hadj) / 2;
	yofs = gtk_adjustment_get_page_increment (priv->vadj) / 2;

	switch (event->direction) {
	    case GDK_SCROLL_UP:
            button_combination = 0;     /* scroll wheel */
		    break;

	    case GDK_SCROLL_LEFT:
            button_combination = 4;     /* tilt wheel */
		    break;

	    case GDK_SCROLL_DOWN:
            button_combination = 0;     /* scroll wheel */
		    break;

	    case GDK_SCROLL_RIGHT:
            button_combination = 4;     /* tilt wheel */
		    break;

	    default:
		    g_assert_not_reached ();
		    return FALSE;
	}

    if (event->state & GDK_SHIFT_MASK)
        button_combination++;

    if (event->state & GDK_CONTROL_MASK)
        button_combination += 2;

    switch (button_combination)
    {
        case 0:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_SCROLL_ACTION);
            break;
        case 1:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_SCROLL_SHIFT_ACTION);
            break;
        case 2:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_SCROLL_CTRL_ACTION);
            break;
        case 3:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_SCROLL_SHIFT_CTRL_ACTION);
            break;
        case 4:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_TILT_ACTION);
            break;
        case 5:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_TILT_SHIFT_ACTION);
            break;
        case 6:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_TILT_CTRL_ACTION);
            break;
        case 7:
            action = g_settings_get_int(priv->view_settings,
				     XVIEWER_CONF_VIEW_TILT_SHIFT_CTRL_ACTION);
            break;
    }

    switch (action)
    {
        case 0:                             /* zoom */
            if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_RIGHT))
                zoom_factor = priv->zoom_multiplier;
            else
                zoom_factor = 1.0 / priv->zoom_multiplier;
            set_zoom (view, priv->zoom * zoom_factor, TRUE, event->x, event->y);
            break;

        case 1:                             /* vertical pan */
            xofs = 0;
            if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_RIGHT))
                yofs = -yofs;
            else
                yofs = yofs;
			scroll_by (view, xofs, yofs);
            break;

        case 2:                             /* horizontal pan */
            yofs = 0;
            if ((event->direction == GDK_SCROLL_DOWN) || (event->direction == GDK_SCROLL_RIGHT))
                xofs = xofs;
            else
                xofs = -xofs;
			scroll_by (view, xofs, yofs);
            break;

        case 3:                             /* move to next/prev image */
        {
            GdkEventButton button_event;

            button_event.type = GDK_BUTTON_PRESS;
            button_event.window = gtk_widget_get_window(widget);
            button_event.send_event = TRUE;
            button_event.time = g_get_monotonic_time() / 1000;
            button_event.x = 0.0;         /* coordinate parameters are irrelevant for this button press */
            button_event.y = 0.0;
            button_event.axes = NULL;
            button_event.state = 0;
            if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_LEFT))
                button_event.button = 8;
            else
                button_event.button = 9;

            button_event.device = event->device;
            button_event.x_root = 0.0;
            button_event.y_root = 0.0;


            if (button_event.time - mouse_wheel_time > 400) /* 400 msec debounce of mouse wheel */
            {
                gtk_main_do_event((GdkEvent *)&button_event);

                mouse_wheel_time = button_event.time;
            }

            break;
        }

        case 4:                             /* Rotate image 90 CW or CCW */
        {
           GdkKeymapKey* keys;
            gint n_keys;
            guint keyval;
            guint state;
            GdkEventKey key_event;

            int old_stderr, new_stderr;

            keyval = GDK_KEY_R;

            if ((event->direction == GDK_SCROLL_UP) || (event->direction == GDK_SCROLL_LEFT))
                state = GDK_CONTROL_MASK + GDK_SHIFT_MASK;
            else
                state = GDK_CONTROL_MASK;

            gdk_keymap_get_entries_for_keyval(gdk_keymap_get_for_display (gtk_widget_get_display(widget)),
                                          keyval,
                                          &keys,
                                          &n_keys);

 

            key_event.type = GDK_KEY_PRESS;
            key_event.window = gtk_widget_get_window(widget);
            key_event.send_event = TRUE;
            key_event.time = g_get_monotonic_time() / 1000;
            key_event.state = state;
            key_event.keyval = keyval;
            key_event.length = 0;
            key_event.string = NULL;
            key_event.hardware_keycode = keys[0].keycode;
            key_event.group = keys[0].group;
            key_event.is_modifier = FALSE;

            if (key_event.time - mouse_wheel_time > 400) /* 400 msec debounce of mouse wheel */
            {
                                /* When generating a mouse button event the event structure contains the device
                                   ID for the mouse (see case 3 above) and no Gdk-Warning is generated. The Key
                                   event structure has no device ID member and Gdk reports a warning that:

                                  "Event with type 8 not holding a GdkDevice. It is most likely synthesized
                                   outside Gdk/GTK+"

                                   The following code therefore temporarily suppresses stderr to avoid showing
                                   this warning when (given the Gdk implementation) it is expected - and untidy! */

                fflush(stderr);
                old_stderr = dup(2);
                new_stderr = open("/dev/null", O_WRONLY);
                dup2(new_stderr, 2);
                close(new_stderr);

                gtk_main_do_event((GdkEvent *)&key_event);

                fflush(stderr);             /* restore normal stderr output */
                dup2(old_stderr, 2);
                close(old_stderr);

                mouse_wheel_time = key_event.time;
            }
            break;
        }


        /* case 5 = no action */
    }

	return TRUE;
}

/* Motion event handler for the image view */
static gboolean
xviewer_scroll_view_motion_event (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	gint x, y;
	GdkModifierType mods;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

#if GTK_CHECK_VERSION (3, 14, 0)
	if (gtk_gesture_is_recognized (priv->zoom_gesture))
		return TRUE;
#endif

	if (!priv->dragging)
		return FALSE;

	if (event->is_hint)
		gdk_window_get_device_position (gtk_widget_get_window (GTK_WIDGET (priv->display)), event->device, &x, &y, &mods);
	else {
		x = event->x;
		y = event->y;
	}

	drag_to (view, x, y);
	return TRUE;
}

static void
display_map_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	xviewer_debug (DEBUG_WINDOW);

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (priv->display));
}

static void
xviewer_scroll_view_size_allocate (GtkWidget *widget, GtkAllocation *alloc)
{
	XviewerScrollView *view;

	view = XVIEWER_SCROLL_VIEW (widget);
	check_scrollbar_visibility (view, alloc);

	GTK_WIDGET_CLASS (xviewer_scroll_view_parent_class)->size_allocate (widget
									,alloc);
}

static void
display_size_change (GtkWidget *widget, GdkEventConfigure *event, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	if (priv->zoom_mode == XVIEWER_ZOOM_MODE_SHRINK_TO_FIT) {
		GtkAllocation alloc;

		alloc.width = event->width;
		alloc.height = event->height;

		set_zoom_fit (view);
		check_scrollbar_visibility (view, &alloc);
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	} else {
		int scaled_width, scaled_height;
		int x_offset = 0;
		int y_offset = 0;

		compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

		if (priv->xofs + event->width > scaled_width)
			x_offset = scaled_width - event->width - priv->xofs;

		if (priv->yofs + event->height > scaled_height)
			y_offset = scaled_height - event->height - priv->yofs;

		scroll_by (view, x_offset, y_offset);
	}

	update_scrollbar_values (view);
}


static gboolean
xviewer_scroll_view_focus_in_event (GtkWidget     *widget,
			    GdkEventFocus *event,
			    gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_in_event");
	return FALSE;
}

static gboolean
xviewer_scroll_view_focus_out_event (GtkWidget     *widget,
			     GdkEventFocus *event,
			     gpointer data)
{
	g_signal_stop_emission_by_name (G_OBJECT (widget), "focus_out_event");
	return FALSE;
}

static gboolean
display_draw (GtkWidget *widget, cairo_t *cr, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	GtkAllocation allocation;
	int scaled_width, scaled_height;
	int xofs, yofs;

	g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), FALSE);
	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (data), FALSE);

	view = XVIEWER_SCROLL_VIEW (data);

	priv = view->priv;

	if (priv->pixbuf == NULL)
		return TRUE;

	xviewer_scroll_view_get_image_coords (view, &xofs, &yofs,
	                                  &scaled_width, &scaled_height);

	xviewer_debug_message (DEBUG_WINDOW, "zoom %.2f, xofs: %i, yofs: %i scaled w: %i h: %i\n",
			   priv->zoom, xofs, yofs, scaled_width, scaled_height);

	/* Paint the background */
	cairo_set_source (cr, gdk_window_get_background_pattern (gtk_widget_get_window (priv->display)));
	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);
	cairo_rectangle (cr, 0, 0, allocation.width, allocation.height);
	cairo_rectangle (cr, MAX (0, xofs), MAX (0, yofs),
			 scaled_width, scaled_height);
	cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
	cairo_fill (cr);

	if (gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface == NULL) {
			priv->background_surface = create_background_surface (view);
		}
		cairo_set_source_surface (cr, priv->background_surface, xofs, yofs);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_REPEAT);
		cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
		cairo_fill (cr);
	}

	/* Make sure the image is only drawn as large as needed.
	 * This is especially necessary for SVGs where there might
	 * be more image data available outside the image boundaries.
	 */
	cairo_rectangle (cr, xofs, yofs, scaled_width, scaled_height);
	cairo_clip (cr);

#ifdef HAVE_RSVG
	if (xviewer_image_is_svg (view->priv->image)) {
		cairo_matrix_t matrix, translate, scale, original;
		XviewerTransform *transform = xviewer_image_get_transform (priv->image);
		cairo_matrix_init_identity (&matrix);
		if (transform) {
			cairo_matrix_t affine;
			double image_offset_x = 0., image_offset_y = 0.;

			xviewer_transform_get_affine (transform, &affine);
			cairo_matrix_multiply (&matrix, &affine, &matrix);

			switch (xviewer_transform_get_transform_type (transform)) {
			case XVIEWER_TRANSFORM_ROT_90:
			case XVIEWER_TRANSFORM_FLIP_HORIZONTAL:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				break;
			case XVIEWER_TRANSFORM_ROT_270:
			case XVIEWER_TRANSFORM_FLIP_VERTICAL:
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case XVIEWER_TRANSFORM_ROT_180:
			case XVIEWER_TRANSFORM_TRANSPOSE:
			case XVIEWER_TRANSFORM_TRANSVERSE:
				image_offset_x = (double) gdk_pixbuf_get_width (priv->pixbuf);
				image_offset_y = (double) gdk_pixbuf_get_height (priv->pixbuf);
				break;
			case XVIEWER_TRANSFORM_NONE:
			default:
				break;
			}
			cairo_matrix_init_translate (&translate, image_offset_x, image_offset_y);
			cairo_matrix_multiply (&matrix, &matrix, &translate);
		}
		cairo_matrix_init_scale (&scale, priv->zoom, priv->zoom);
		cairo_matrix_multiply (&matrix, &matrix, &scale);
		cairo_matrix_init_translate (&translate, xofs, yofs);
		cairo_matrix_multiply (&matrix, &matrix, &translate);

		cairo_get_matrix (cr, &original);
		cairo_matrix_multiply (&matrix, &matrix, &original);
		cairo_set_matrix (cr, &matrix);

		rsvg_handle_render_cairo (xviewer_image_get_svg (priv->image), cr);

	} else
#endif /* HAVE_RSVG */
	{
		cairo_filter_t interp_type;

		if(!DOUBLE_EQUAL(priv->zoom, 1.0) && priv->force_unfiltered)
		{
			if ((is_zoomed_in (view) && priv->interp_type_in != CAIRO_FILTER_NEAREST) ||
				(is_zoomed_out (view) && priv->interp_type_out != CAIRO_FILTER_NEAREST)) {
				// CAIRO_FILTER_GOOD is too slow during zoom changes, so use CAIRO_FILTER_BILINEAR instead
				interp_type = CAIRO_FILTER_BILINEAR;
			}
			else {
				interp_type = CAIRO_FILTER_NEAREST;
			}
			_set_hq_redraw_timeout(view);
		}
		else
		{
			if (is_zoomed_in (view))
				interp_type = priv->interp_type_in;
			else
				interp_type = priv->interp_type_out;

			_clear_hq_redraw_timeout (view);
			priv->force_unfiltered = TRUE;
		}
		cairo_scale (cr, priv->zoom, priv->zoom);
		cairo_set_source_surface (cr, priv->surface, xofs/priv->zoom, yofs/priv->zoom);
		cairo_pattern_set_extend (cairo_get_source (cr), CAIRO_EXTEND_PAD);
		if (is_zoomed_in (view) || is_zoomed_out (view))
			cairo_pattern_set_filter (cairo_get_source (cr), interp_type);
		cairo_paint (cr);
	}

	return TRUE;
}

#if GTK_CHECK_VERSION (3, 14, 0)
static void
zoom_gesture_begin_cb (GtkGestureZoom   *gesture,
		       GdkEventSequence *sequence,
		       XviewerScrollView    *view)
{
	gdouble center_x, center_y;
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	/* Displace dragging point to gesture center */
	gtk_gesture_get_bounding_box_center (GTK_GESTURE (gesture),
                                             &center_x, &center_y);
	priv->drag_anchor_x = center_x;
	priv->drag_anchor_y = center_y;
	priv->drag_ofs_x = priv->xofs;
	priv->drag_ofs_y = priv->yofs;
	priv->dragging = TRUE;
	priv->initial_zoom = priv->zoom;

        gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);
}

static void
zoom_gesture_update_cb (GtkGestureZoom   *gesture,
			GdkEventSequence *sequence,
			XviewerScrollView    *view)
{
	gdouble center_x, center_y, scale;
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	scale = gtk_gesture_zoom_get_scale_delta (gesture);
	gtk_gesture_get_bounding_box_center (GTK_GESTURE (gesture),
                                             &center_x, &center_y);

	drag_to (view, center_x, center_y);
	set_zoom (view, priv->initial_zoom * scale, TRUE,
		  center_x, center_y);
}

static void
zoom_gesture_end_cb (GtkGestureZoom   *gesture,
		     GdkEventSequence *sequence,
		     XviewerScrollView    *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	priv->dragging = FALSE;
        xviewer_scroll_view_set_cursor (view, XVIEWER_SCROLL_VIEW_CURSOR_NORMAL);
}

static void
rotate_gesture_begin_cb (GtkGesture       *gesture,
			 GdkEventSequence *sequence,
			 XviewerScrollView    *view)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;
	priv->rotate_state = XVIEWER_TRANSFORM_NONE;
}

static void
pan_gesture_pan_cb (GtkGesturePan   *gesture,
		    GtkPanDirection  direction,
		    gdouble          offset,
		    XviewerScrollView   *view)
{
	XviewerScrollViewPrivate *priv;

	if (xviewer_scroll_view_scrollbars_visible (view)) {
		gtk_gesture_set_state (GTK_GESTURE (gesture),
				       GTK_EVENT_SEQUENCE_DENIED);
		return;
	}

#define PAN_ACTION_DISTANCE 200

	priv = view->priv;
	priv->pan_action = XVIEWER_PAN_ACTION_NONE;
	gtk_gesture_set_state (GTK_GESTURE (gesture), GTK_EVENT_SEQUENCE_CLAIMED);

	if (offset > PAN_ACTION_DISTANCE) {
		if (direction == GTK_PAN_DIRECTION_LEFT ||
		    gtk_widget_get_direction (GTK_WIDGET (view)) == GTK_TEXT_DIR_RTL)
			priv->pan_action = XVIEWER_PAN_ACTION_NEXT;
		else
			priv->pan_action = XVIEWER_PAN_ACTION_PREV;
	}
#undef PAN_ACTION_DISTANCE
}

static void
pan_gesture_end_cb (GtkGesture       *gesture,
		    GdkEventSequence *sequence,
		    XviewerScrollView    *view)
{
	XviewerScrollViewPrivate *priv;

	if (!gtk_gesture_handles_sequence (gesture, sequence))
		return;

	priv = view->priv;

	if (priv->pan_action == XVIEWER_PAN_ACTION_PREV)
		g_signal_emit (view, view_signals [SIGNAL_PREVIOUS_IMAGE], 0);
	else if (priv->pan_action == XVIEWER_PAN_ACTION_NEXT)
		g_signal_emit (view, view_signals [SIGNAL_NEXT_IMAGE], 0);

	priv->pan_action = XVIEWER_PAN_ACTION_NONE;
}
#endif

static gboolean
scroll_view_check_angle (gdouble angle,
			 gdouble min,
			 gdouble max,
			 gdouble threshold)
{
	if (min < max) {
		return (angle > min - threshold &&
			angle < max + threshold);
	} else {
		return (angle < max + threshold ||
			angle > min - threshold);
	}
}

static XviewerRotationState
scroll_view_get_rotate_state (XviewerScrollView *view,
			      gdouble        delta)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

#define THRESHOLD (G_PI / 16)
	switch (priv->rotate_state) {
	case XVIEWER_ROTATION_0:
		if (scroll_view_check_angle (delta, G_PI * 7 / 4,
					     G_PI / 4, THRESHOLD))
			return priv->rotate_state;
		break;
	case XVIEWER_ROTATION_90:
		if (scroll_view_check_angle (delta, G_PI / 4,
					     G_PI * 3 / 4, THRESHOLD))
			return priv->rotate_state;
		break;
	case XVIEWER_ROTATION_180:
		if (scroll_view_check_angle (delta, G_PI * 3 / 4,
					     G_PI * 5 / 4, THRESHOLD))
			return priv->rotate_state;
		break;
	case XVIEWER_ROTATION_270:
		if (scroll_view_check_angle (delta, G_PI * 5 / 4,
					     G_PI * 7 / 4, THRESHOLD))
			return priv->rotate_state;
		break;
	default:
		g_assert_not_reached ();
	}

#undef THRESHOLD

	if (scroll_view_check_angle (delta, G_PI / 4, G_PI * 3 / 4, 0))
		return XVIEWER_ROTATION_90;
	else if (scroll_view_check_angle (delta, G_PI * 3 / 4, G_PI * 5 / 4, 0))
		return XVIEWER_ROTATION_180;
	else if (scroll_view_check_angle (delta, G_PI * 5 / 4, G_PI * 7 / 4, 0))
		return XVIEWER_ROTATION_270;

	return XVIEWER_ROTATION_0;
}

#if GTK_CHECK_VERSION (3, 14, 0)
static void
rotate_gesture_angle_changed_cb (GtkGestureRotate *rotate,
				 gdouble           angle,
				 gdouble           delta,
				 XviewerScrollView    *view)
{
	XviewerRotationState rotate_state;
	XviewerScrollViewPrivate *priv;
	gint angle_diffs [N_XVIEWER_ROTATIONS][N_XVIEWER_ROTATIONS] = {
		{ 0,   90,  180, 270 },
		{ 270, 0,   90,  180 },
		{ 180, 270, 0,   90 },
		{ 90,  180, 270, 0 }
	};
	gint rotate_angle;

	priv = view->priv;
	rotate_state = scroll_view_get_rotate_state (view, delta);

	if (priv->rotate_state == rotate_state)
		return;

	rotate_angle = angle_diffs[priv->rotate_state][rotate_state];
	g_signal_emit (view, view_signals [SIGNAL_ROTATION_CHANGED], 0, (gdouble) rotate_angle);
	priv->rotate_state = rotate_state;
}
#endif

/*==================================

   image loading callbacks

   -----------------------------------*/
/*
static void
image_loading_update_cb (XviewerImage *img, int x, int y, int width, int height, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;
	GdkRectangle area;
	int xofs, yofs;
	int sx0, sy0, sx1, sy1;

	view = (XviewerScrollView*) data;
	priv = view->priv;

	xviewer_debug_message (DEBUG_IMAGE_LOAD, "x: %i, y: %i, width: %i, height: %i\n",
			   x, y, width, height);

	if (priv->pixbuf == NULL) {
		priv->pixbuf = xviewer_image_get_pixbuf (img);
		set_zoom_fit (view);
		check_scrollbar_visibility (view, NULL);
	}
	priv->progressive_state = PROGRESSIVE_LOADING;

	get_image_offsets (view, &xofs, &yofs);

	sx0 = floor (x * priv->zoom + xofs);
	sy0 = floor (y * priv->zoom + yofs);
	sx1 = ceil ((x + width) * priv->zoom + xofs);
	sy1 = ceil ((y + height) * priv->zoom + yofs);

	area.x = sx0;
	area.y = sy0;
	area.width = sx1 - sx0;
	area.height = sy1 - sy0;

	if (GTK_WIDGET_DRAWABLE (priv->display))
		gdk_window_invalidate_rect (GTK_WIDGET (priv->display)->window, &area, FALSE);
}


static void
image_loading_finished_cb (XviewerImage *img, gpointer data)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	view = (XviewerScrollView*) data;
	priv = view->priv;

	if (priv->pixbuf == NULL) {
		priv->pixbuf = xviewer_image_get_pixbuf (img);
		priv->progressive_state = PROGRESSIVE_NONE;
		set_zoom_fit (view);
		check_scrollbar_visibility (view, NULL);
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));

	}
	else if (priv->interp_type != CAIRO_FILTER_NEAREST &&
		 !is_unity_zoom (view))
	{
		// paint antialiased image version
		priv->progressive_state = PROGRESSIVE_POLISHING;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}
}

static void
image_loading_failed_cb (XviewerImage *img, char *msg, gpointer data)
{
	XviewerScrollViewPrivate *priv;

	priv = XVIEWER_SCROLL_VIEW (data)->priv;

	g_print ("loading failed: %s.\n", msg);

	if (priv->pixbuf != 0) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = 0;
	}

	if (GTK_WIDGET_DRAWABLE (priv->display)) {
		gdk_window_clear (GTK_WIDGET (priv->display)->window);
	}
}

static void
image_loading_cancelled_cb (XviewerImage *img, gpointer data)
{
	XviewerScrollViewPrivate *priv;

	priv = XVIEWER_SCROLL_VIEW (data)->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	if (GTK_WIDGET_DRAWABLE (priv->display)) {
		gdk_window_clear (GTK_WIDGET (priv->display)->window);
	}
}
*/

/* Use when the pixbuf in the view is changed, to keep a
   reference to it and create its cairo surface. */
static void
update_pixbuf (XviewerScrollView *view, GdkPixbuf *pixbuf)
{
	XviewerScrollViewPrivate *priv;

	priv = view->priv;

	if (priv->pixbuf != NULL) {
		g_object_unref (priv->pixbuf);
		priv->pixbuf = NULL;
	}

	priv->pixbuf = pixbuf;

	if (priv->surface) {
		cairo_surface_destroy (priv->surface);
	}
	priv->surface = create_surface_from_pixbuf (view, priv->pixbuf);
}

static void
image_changed_cb (XviewerImage *img, gpointer data)
{
	update_pixbuf (XVIEWER_SCROLL_VIEW (data), xviewer_image_get_pixbuf (img));

	_set_zoom_mode_internal (XVIEWER_SCROLL_VIEW (data),
				 XVIEWER_ZOOM_MODE_SHRINK_TO_FIT);
}

/*===================================
         public API
  ---------------------------------*/

void
xviewer_scroll_view_hide_cursor (XviewerScrollView *view)
{
       xviewer_scroll_view_set_cursor (view, XVIEWER_SCROLL_VIEW_CURSOR_HIDDEN);
}

void
xviewer_scroll_view_show_cursor (XviewerScrollView *view)
{
       xviewer_scroll_view_set_cursor (view, XVIEWER_SCROLL_VIEW_CURSOR_NORMAL);
}

/* general properties */
void
xviewer_scroll_view_set_zoom_upscale (XviewerScrollView *view, gboolean upscale)
{
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->upscale != upscale) {
		priv->upscale = upscale;

		if (priv->zoom_mode == XVIEWER_ZOOM_MODE_SHRINK_TO_FIT) {
			set_zoom_fit (view);
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		}
	}
}

void
xviewer_scroll_view_set_antialiasing_in (XviewerScrollView *view, gboolean state)
{
	XviewerScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_in != new_interp_type) {
		priv->interp_type_in = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-in");
	}
}

void
xviewer_scroll_view_set_antialiasing_out (XviewerScrollView *view, gboolean state)
{
	XviewerScrollViewPrivate *priv;
	cairo_filter_t new_interp_type;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	new_interp_type = state ? CAIRO_FILTER_GOOD : CAIRO_FILTER_NEAREST;

	if (priv->interp_type_out != new_interp_type) {
		priv->interp_type_out = new_interp_type;
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		g_object_notify (G_OBJECT (view), "antialiasing-out");
	}
}

static void
_transp_background_changed (XviewerScrollView *view)
{
	XviewerScrollViewPrivate *priv = view->priv;

	if (priv->pixbuf != NULL && gdk_pixbuf_get_has_alpha (priv->pixbuf)) {
		if (priv->background_surface) {
			cairo_surface_destroy (priv->background_surface);
			/* Will be recreated if needed during redraw */
			priv->background_surface = NULL;
		}
		gtk_widget_queue_draw (GTK_WIDGET (priv->display));
	}

}

void
xviewer_scroll_view_set_transparency_color (XviewerScrollView *view, GdkRGBA *color)
{
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (!_xviewer_gdk_rgba_equal0 (&priv->transp_color, color)) {
		priv->transp_color = *color;
		if (priv->transp_style == XVIEWER_TRANSP_COLOR)
			_transp_background_changed (view);

		g_object_notify (G_OBJECT (view), "transparency-color");
	}
}

void
xviewer_scroll_view_set_transparency (XviewerScrollView        *view,
				  XviewerTransparencyStyle  style)
{
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));
	
	priv = view->priv;

	if (priv->transp_style != style) {
		priv->transp_style = style;
		_transp_background_changed (view);
		g_object_notify (G_OBJECT (view), "transparency-style");
	}
}

/* zoom api */

static double preferred_zoom_levels[] = {
	1.0 / 100, 1.0 / 50, 1.0 / 20,
	1.0 / 10.0, 1.0 / 5.0, 1.0 / 3.0, 1.0 / 2.0, 1.0 / 1.5,
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
        11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0
};
static const gint n_zoom_levels = (sizeof (preferred_zoom_levels) / sizeof (double));

void
xviewer_scroll_view_zoom_in (XviewerScrollView *view, gboolean smooth)
{
	XviewerScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom * priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = 0; i < n_zoom_levels; i++) {
			if (preferred_zoom_levels [i] - priv->zoom
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}

		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);

}

void
xviewer_scroll_view_zoom_out (XviewerScrollView *view, gboolean smooth)
{
	XviewerScrollViewPrivate *priv;
	double zoom;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (smooth) {
		zoom = priv->zoom / priv->zoom_multiplier;
	}
	else {
		int i;
		int index = -1;

		for (i = n_zoom_levels - 1; i >= 0; i--) {
			if (priv->zoom - preferred_zoom_levels [i]
					> DOUBLE_EQUAL_MAX_DIFF) {
				index = i;
				break;
			}
		}
		if (index == -1) {
			zoom = priv->zoom;
		}
		else {
			zoom = preferred_zoom_levels [i];
		}
	}
	set_zoom (view, zoom, FALSE, 0, 0);
}

static void
xviewer_scroll_view_zoom_fit (XviewerScrollView *view)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	set_zoom_fit (view);
	check_scrollbar_visibility (view, NULL);
	gtk_widget_queue_draw (GTK_WIDGET (view->priv->display));
}

void
xviewer_scroll_view_set_zoom (XviewerScrollView *view, double zoom)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	set_zoom (view, zoom, FALSE, 0, 0);
}

double
xviewer_scroll_view_get_zoom (XviewerScrollView *view)
{
	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view), 0.0);

	return view->priv->zoom;
}

gboolean
xviewer_scroll_view_get_zoom_is_min (XviewerScrollView *view)
{
	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view), FALSE);

	set_minimum_zoom_factor (view);

	return DOUBLE_EQUAL (view->priv->zoom, MIN_ZOOM_FACTOR) ||
	       DOUBLE_EQUAL (view->priv->zoom, view->priv->min_zoom);
}

gboolean
xviewer_scroll_view_get_zoom_is_max (XviewerScrollView *view)
{
	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view), FALSE);

	return DOUBLE_EQUAL (view->priv->zoom, MAX_ZOOM_FACTOR);
}

static void
display_next_frame_cb (XviewerImage *image, gint delay, gpointer data)
{
 	XviewerScrollViewPrivate *priv;
	XviewerScrollView *view;

	if (!XVIEWER_IS_SCROLL_VIEW (data))
		return;

	view = XVIEWER_SCROLL_VIEW (data);
	priv = view->priv;

	update_pixbuf (view, xviewer_image_get_pixbuf (image));

	gtk_widget_queue_draw (GTK_WIDGET (priv->display)); 
}

void
xviewer_scroll_view_set_image (XviewerScrollView *view, XviewerImage *image)
{
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (priv->image == image) {
		return;
	}

	if (priv->image != NULL) {
		free_image_resources (view);
	}
	g_assert (priv->image == NULL);
	g_assert (priv->pixbuf == NULL);

	/* priv->progressive_state = PROGRESSIVE_NONE; */
	if (image != NULL) {
		xviewer_image_data_ref (image);

		if (priv->pixbuf == NULL) {
			update_pixbuf (view, xviewer_image_get_pixbuf (image));
			/* priv->progressive_state = PROGRESSIVE_NONE; */
			_set_zoom_mode_internal (view,
						 XVIEWER_ZOOM_MODE_SHRINK_TO_FIT);

		}
#if 0
		else if ((is_zoomed_in (view) && priv->interp_type_in != CAIRO_FILTER_NEAREST) ||
			 (is_zoomed_out (view) && priv->interp_type_out != CAIRO_FILTER_NEAREST))
		{
			/* paint antialiased image version */
			priv->progressive_state = PROGRESSIVE_POLISHING;
			gtk_widget_queue_draw (GTK_WIDGET (priv->display));
		}
#endif

		priv->image_changed_id = g_signal_connect (image, "changed",
							   (GCallback) image_changed_cb, view);
		if (xviewer_image_is_animation (image) == TRUE ) {
			xviewer_image_start_animation (image);
			priv->frame_changed_id = g_signal_connect (image, "next-frame", 
								    (GCallback) display_next_frame_cb, view);
		}
	}

	priv->image = image;

	g_object_notify (G_OBJECT (view), "image");
}

/**
 * xviewer_scroll_view_get_image:
 * @view: An #XviewerScrollView.
 *
 * Gets the the currently displayed #XviewerImage.
 *
 * Returns: (transfer full): An #XviewerImage.
 **/
XviewerImage*
xviewer_scroll_view_get_image (XviewerScrollView *view)
{
	XviewerImage *img;

	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view), NULL);

	img = view->priv->image;

	if (img != NULL)
		g_object_ref (img);

	return img;
}

gboolean
xviewer_scroll_view_scrollbars_visible (XviewerScrollView *view)
{
	if (!gtk_widget_get_visible (GTK_WIDGET (view->priv->hbar)) &&
	    !gtk_widget_get_visible (GTK_WIDGET (view->priv->vbar)))
		return FALSE;

	return TRUE;
}

/*===================================
    object creation/freeing
  ---------------------------------*/

static gboolean
sv_string_to_rgba_mapping (GValue   *value,
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
sv_rgba_to_string_mapping (const GValue       *value,
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
xviewer_scroll_view_init (XviewerScrollView *view)
{
	GSettings *settings;
	XviewerScrollViewPrivate *priv;

	priv = view->priv = xviewer_scroll_view_get_instance_private (view);
	settings = g_settings_new (XVIEWER_CONF_VIEW);

	priv->zoom = 1.0;
	priv->min_zoom = MIN_ZOOM_FACTOR;
	priv->zoom_mode = XVIEWER_ZOOM_MODE_SHRINK_TO_FIT;
	priv->upscale = FALSE;
	//priv->uta = NULL;
	priv->interp_type_in = CAIRO_FILTER_GOOD;
	priv->interp_type_out = CAIRO_FILTER_GOOD;
	priv->scroll_wheel_zoom = FALSE;
	priv->zoom_multiplier = IMAGE_VIEW_ZOOM_MULTIPLIER;
	priv->image = NULL;
	priv->pixbuf = NULL;
	priv->surface = NULL;
	/* priv->progressive_state = PROGRESSIVE_NONE; */
	priv->transp_style = XVIEWER_TRANSP_BACKGROUND;
	g_warn_if_fail (gdk_rgba_parse(&priv->transp_color, CHECK_BLACK));
	priv->cursor = XVIEWER_SCROLL_VIEW_CURSOR_NORMAL;
	priv->menu = NULL;
	priv->override_bg_color = NULL;
	priv->background_surface = NULL;

	priv->hadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->hadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

	priv->hbar = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, priv->hadj);
	priv->vadj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 100, 0, 10, 10, 100));
	g_signal_connect (priv->vadj, "value_changed",
			  G_CALLBACK (adjustment_changed_cb),
			  view);

	priv->vbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, priv->vadj);
	priv->display = g_object_new (GTK_TYPE_DRAWING_AREA,
				      "can-focus", TRUE,
				      NULL);

	gtk_widget_add_events (GTK_WIDGET (priv->display),
			       GDK_EXPOSURE_MASK
			       | GDK_BUTTON_PRESS_MASK
			       | GDK_BUTTON_RELEASE_MASK
			       | GDK_POINTER_MOTION_MASK
			       | GDK_POINTER_MOTION_HINT_MASK
			       | GDK_TOUCH_MASK
			       | GDK_SCROLL_MASK
			       | GDK_KEY_PRESS_MASK);
	g_signal_connect (G_OBJECT (priv->display), "configure_event",
			  G_CALLBACK (display_size_change), view);
	g_signal_connect (G_OBJECT (priv->display), "draw",
			  G_CALLBACK (display_draw), view);
	g_signal_connect (G_OBJECT (priv->display), "map_event",
			  G_CALLBACK (display_map_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_press_event",
			  G_CALLBACK (xviewer_scroll_view_button_press_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "motion_notify_event",
			  G_CALLBACK (xviewer_scroll_view_motion_event), view);
	g_signal_connect (G_OBJECT (priv->display), "button_release_event",
			  G_CALLBACK (xviewer_scroll_view_button_release_event),
			  view);
	g_signal_connect (G_OBJECT (priv->display), "scroll_event",
			  G_CALLBACK (xviewer_scroll_view_scroll_event), view);
	g_signal_connect (G_OBJECT (priv->display), "focus_in_event",
			  G_CALLBACK (xviewer_scroll_view_focus_in_event), NULL);
	g_signal_connect (G_OBJECT (priv->display), "focus_out_event",
			  G_CALLBACK (xviewer_scroll_view_focus_out_event), NULL);

	g_signal_connect (G_OBJECT (view), "key_press_event",
			  G_CALLBACK (display_key_press_event), view);

	gtk_drag_source_set (priv->display, GDK_BUTTON1_MASK,
			     target_table, G_N_ELEMENTS (target_table),
			     GDK_ACTION_COPY | GDK_ACTION_MOVE |
			     GDK_ACTION_LINK | GDK_ACTION_ASK);
	g_signal_connect (G_OBJECT (priv->display), "drag-data-get",
			  G_CALLBACK (view_on_drag_data_get_cb), view);
	g_signal_connect (G_OBJECT (priv->display), "drag-begin",
			  G_CALLBACK (view_on_drag_begin_cb), view);

	gtk_grid_attach (GTK_GRID (view), priv->display,
			 0, 0, 1, 1);
	gtk_widget_set_hexpand (priv->display, TRUE);
	gtk_widget_set_vexpand (priv->display, TRUE);
	gtk_grid_attach (GTK_GRID (view), priv->hbar,
			 0, 1, 1, 1);
	gtk_widget_set_hexpand (priv->hbar, TRUE);
	gtk_grid_attach (GTK_GRID (view), priv->vbar,
			 1, 0, 1, 1);
	gtk_widget_set_vexpand (priv->vbar, TRUE);

	g_settings_bind (settings, XVIEWER_CONF_VIEW_USE_BG_COLOR, view,
			 "use-background-color", G_SETTINGS_BIND_DEFAULT);
	g_settings_bind_with_mapping (settings, XVIEWER_CONF_VIEW_BACKGROUND_COLOR,
				      view, "background-color",
				      G_SETTINGS_BIND_DEFAULT,
				      sv_string_to_rgba_mapping,
				      sv_rgba_to_string_mapping, NULL, NULL);
	g_settings_bind_with_mapping (settings, XVIEWER_CONF_VIEW_TRANS_COLOR,
				      view, "transparency-color",
				      G_SETTINGS_BIND_GET,
				      sv_string_to_rgba_mapping,
				      sv_rgba_to_string_mapping, NULL, NULL);
	g_settings_bind (settings, XVIEWER_CONF_VIEW_TRANSPARENCY, view,
			 "transparency-style", G_SETTINGS_BIND_GET);
	g_settings_bind (settings, XVIEWER_CONF_VIEW_EXTRAPOLATE, view,
			 "antialiasing-in", G_SETTINGS_BIND_GET);
	g_settings_bind (settings, XVIEWER_CONF_VIEW_INTERPOLATE, view,
			 "antialiasing-out", G_SETTINGS_BIND_GET);

	g_object_unref (settings);

#if GTK_CHECK_VERSION (3, 14, 0)
	priv->zoom_gesture = gtk_gesture_zoom_new (GTK_WIDGET (view));
	g_signal_connect (priv->zoom_gesture, "begin",
			  G_CALLBACK (zoom_gesture_begin_cb), view);
	g_signal_connect (priv->zoom_gesture, "update",
			  G_CALLBACK (zoom_gesture_update_cb), view);
	g_signal_connect (priv->zoom_gesture, "end",
			  G_CALLBACK (zoom_gesture_end_cb), view);
	g_signal_connect (priv->zoom_gesture, "cancel",
			  G_CALLBACK (zoom_gesture_end_cb), view);
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->zoom_gesture),
						    GTK_PHASE_CAPTURE);

	priv->rotate_gesture = gtk_gesture_rotate_new (GTK_WIDGET (view));
	gtk_gesture_group (priv->rotate_gesture, priv->zoom_gesture);
	g_signal_connect (priv->rotate_gesture, "angle-changed",
			  G_CALLBACK (rotate_gesture_angle_changed_cb), view);
	g_signal_connect (priv->rotate_gesture, "begin",
			  G_CALLBACK (rotate_gesture_begin_cb), view);
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->rotate_gesture),
						    GTK_PHASE_CAPTURE);

	priv->pan_gesture = gtk_gesture_pan_new (GTK_WIDGET (view),
						 GTK_ORIENTATION_HORIZONTAL);
	g_signal_connect (priv->pan_gesture, "pan",
			  G_CALLBACK (pan_gesture_pan_cb), view);
	g_signal_connect (priv->pan_gesture, "end",
			  G_CALLBACK (pan_gesture_end_cb), view);
	gtk_gesture_single_set_touch_only (GTK_GESTURE_SINGLE (priv->pan_gesture), 
					   TRUE);
	gtk_event_controller_set_propagation_phase (GTK_EVENT_CONTROLLER (priv->pan_gesture),
						    GTK_PHASE_CAPTURE);
#endif
}

static void
xviewer_scroll_view_dispose (GObject *object)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (object));

	view = XVIEWER_SCROLL_VIEW (object);
	priv = view->priv;

#if 0
	if (priv->uta != NULL) {
		xviewer_uta_free (priv->uta);
		priv->uta = NULL;
	}
#endif

	_clear_hq_redraw_timeout (view);

	if (priv->idle_id != 0) {
		g_source_remove (priv->idle_id);
		priv->idle_id = 0;
	}

	if (priv->background_color != NULL) {
		gdk_rgba_free (priv->background_color);
		priv->background_color = NULL;
	}

	if (priv->override_bg_color != NULL) {
		gdk_rgba_free (priv->override_bg_color);
		priv->override_bg_color = NULL;
	}

	if (priv->background_surface != NULL) {
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	free_image_resources (view);

#if GTK_CHECK_VERSION (3, 14, 0)
	if (priv->zoom_gesture) {
		g_object_unref (priv->zoom_gesture);
		priv->zoom_gesture = NULL;
	}

	if (priv->rotate_gesture) {
		g_object_unref (priv->rotate_gesture);
		priv->rotate_gesture = NULL;
	}

	if (priv->pan_gesture) {
		g_object_unref (priv->pan_gesture);
		priv->pan_gesture = NULL;
	}
#endif

	G_OBJECT_CLASS (xviewer_scroll_view_parent_class)->dispose (object);
}

static void
xviewer_scroll_view_get_property (GObject *object, guint property_id,
			      GValue *value, GParamSpec *pspec)
{
	XviewerScrollView *view;
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (object));

	view = XVIEWER_SCROLL_VIEW (object);
	priv = view->priv;

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
	{
		gboolean filter = (priv->interp_type_in != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_ANTIALIAS_OUT:
	{
		gboolean filter = (priv->interp_type_out != CAIRO_FILTER_NEAREST);
		g_value_set_boolean (value, filter);
		break;
	}
	case PROP_USE_BG_COLOR:
		g_value_set_boolean (value, priv->use_bg_color);
		break;
	case PROP_BACKGROUND_COLOR:
		//FIXME: This doesn't really handle the NULL color.
		g_value_set_boxed (value, priv->background_color);
		break;
	case PROP_SCROLLWHEEL_ZOOM:
		g_value_set_boolean (value, priv->scroll_wheel_zoom);
		break;
	case PROP_TRANSPARENCY_STYLE:
		g_value_set_enum (value, priv->transp_style);
		break;
	case PROP_ZOOM_MODE:
		g_value_set_enum (value, priv->zoom_mode);
		break;
	case PROP_ZOOM_MULTIPLIER:
		g_value_set_double (value, priv->zoom_multiplier);
		break;
	case PROP_IMAGE:
		g_value_set_object (value, priv->image);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
xviewer_scroll_view_set_property (GObject *object, guint property_id,
			      const GValue *value, GParamSpec *pspec)
{
	XviewerScrollView *view;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (object));

	view = XVIEWER_SCROLL_VIEW (object);

	switch (property_id) {
	case PROP_ANTIALIAS_IN:
		xviewer_scroll_view_set_antialiasing_in (view, g_value_get_boolean (value));
		break;
	case PROP_ANTIALIAS_OUT:
		xviewer_scroll_view_set_antialiasing_out (view, g_value_get_boolean (value));
		break;
	case PROP_USE_BG_COLOR:
		xviewer_scroll_view_set_use_bg_color (view, g_value_get_boolean (value));
		break;
	case PROP_BACKGROUND_COLOR:
	{
		const GdkRGBA *color = g_value_get_boxed (value);
		xviewer_scroll_view_set_background_color (view, color);
		break;
	}
	case PROP_SCROLLWHEEL_ZOOM:
		xviewer_scroll_view_set_scroll_wheel_zoom (view, g_value_get_boolean (value));
		break;
	case PROP_TRANSP_COLOR:
		xviewer_scroll_view_set_transparency_color (view, g_value_get_boxed (value));
		break;
	case PROP_TRANSPARENCY_STYLE:
		xviewer_scroll_view_set_transparency (view, g_value_get_enum (value));
		break;
	case PROP_ZOOM_MODE:
		xviewer_scroll_view_set_zoom_mode (view, g_value_get_enum (value));
		break;
	case PROP_ZOOM_MULTIPLIER:
		xviewer_scroll_view_set_zoom_multiplier (view, g_value_get_double (value));
		break;
	case PROP_IMAGE:
		xviewer_scroll_view_set_image (view, g_value_get_object (value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}


static void
xviewer_scroll_view_class_init (XviewerScrollViewClass *klass)
{
	GObjectClass *gobject_class;
	GtkWidgetClass *widget_class;

	gobject_class = (GObjectClass*) klass;
	widget_class = (GtkWidgetClass*) klass;

	gobject_class->dispose = xviewer_scroll_view_dispose;
        gobject_class->set_property = xviewer_scroll_view_set_property;
        gobject_class->get_property = xviewer_scroll_view_get_property;

	/**
	 * XviewerScrollView:antialiasing-in:
	 *
	 * If %TRUE the displayed image will be filtered in a second pass
	 * while being zoomed in.
	 */
	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_IN,
		g_param_spec_boolean ("antialiasing-in", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));
	/**
	 * XviewerScrollView:antialiasing-out:
	 *
	 * If %TRUE the displayed image will be filtered in a second pass
	 * while being zoomed out.
	 */
	g_object_class_install_property (
		gobject_class, PROP_ANTIALIAS_OUT,
		g_param_spec_boolean ("antialiasing-out", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * XviewerScrollView:background-color:
	 *
	 * This is the default background color used for painting the background
	 * of the image view. If set to %NULL the color is determined by the
	 * active GTK theme.
	 */
	g_object_class_install_property (
		gobject_class, PROP_BACKGROUND_COLOR,
		g_param_spec_boxed ("background-color", NULL, NULL,
				    GDK_TYPE_RGBA,
				    G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_USE_BG_COLOR,
		g_param_spec_boolean ("use-background-color", NULL, NULL, FALSE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * XviewerScrollView:zoom-multiplier:
	 *
	 * The current zoom factor is multiplied with this value + 1.0 when
	 * scrolling with the scrollwheel to determine the next zoom factor.
	 */
	g_object_class_install_property (
		gobject_class, PROP_ZOOM_MULTIPLIER,
		g_param_spec_double ("zoom-multiplier", NULL, NULL,
				     -G_MAXDOUBLE, G_MAXDOUBLE - 1.0, 0.05,
				     G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * XviewerScrollView:scrollwheel-zoom:
	 *
	 * If %TRUE the scrollwheel will zoom the view, otherwise it will be
	 * used for scrolling a zoomed image.
	 */
	g_object_class_install_property (
		gobject_class, PROP_SCROLLWHEEL_ZOOM,
		g_param_spec_boolean ("scrollwheel-zoom", NULL, NULL, TRUE,
				      G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * XviewerScrollView:image:
	 *
	 * This is the currently display #XviewerImage.
	 */
	g_object_class_install_property (
		gobject_class, PROP_IMAGE,
		g_param_spec_object ("image", NULL, NULL, XVIEWER_TYPE_IMAGE,
				     G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	/**
	 * XviewerScrollView:transparency-color:
	 *
	 * This is the color used to fill the transparent parts of an image
	 * if #XviewerScrollView:transparency-style is set to %XVIEWER_TRANSP_COLOR.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSP_COLOR,
		g_param_spec_boxed ("transparency-color", NULL, NULL,
				    GDK_TYPE_RGBA,
				    G_PARAM_WRITABLE | G_PARAM_STATIC_NAME));
	
	/**
	 * XviewerScrollView:transparency-style:
	 *
	 * Determines how to fill the shown image's transparent areas.
	 */
	g_object_class_install_property (
		gobject_class, PROP_TRANSPARENCY_STYLE,
		g_param_spec_enum ("transparency-style", NULL, NULL,
				   XVIEWER_TYPE_TRANSPARENCY_STYLE,
				   XVIEWER_TRANSP_CHECKED,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	g_object_class_install_property (
		gobject_class, PROP_ZOOM_MODE,
		g_param_spec_enum ("zoom-mode", NULL, NULL,
				   XVIEWER_TYPE_ZOOM_MODE,
				   XVIEWER_ZOOM_MODE_SHRINK_TO_FIT,
				   G_PARAM_READWRITE | G_PARAM_STATIC_NAME));

	view_signals [SIGNAL_ZOOM_CHANGED] =
		g_signal_new ("zoom_changed",
			      XVIEWER_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerScrollViewClass, zoom_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);
	view_signals [SIGNAL_ROTATION_CHANGED] =
		g_signal_new ("rotation-changed",
			      XVIEWER_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerScrollViewClass, rotation_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__DOUBLE,
			      G_TYPE_NONE, 1,
			      G_TYPE_DOUBLE);

	view_signals [SIGNAL_NEXT_IMAGE] =
		g_signal_new ("next-image",
			      XVIEWER_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerScrollViewClass, next_image),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);
	view_signals [SIGNAL_PREVIOUS_IMAGE] =
		g_signal_new ("previous-image",
			      XVIEWER_TYPE_SCROLL_VIEW,
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (XviewerScrollViewClass, previous_image),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	widget_class->size_allocate = xviewer_scroll_view_size_allocate;
	widget_class->style_set = xviewer_scroll_view_style_set;
}

static void
view_on_drag_begin_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gpointer          user_data)
{
	XviewerScrollView *view;
	XviewerImage *image;
	GdkPixbuf *thumbnail;
	gint width, height;

	view = XVIEWER_SCROLL_VIEW (user_data);
	image = view->priv->image;

	thumbnail = xviewer_image_get_thumbnail (image);

	if  (thumbnail) {
		width = gdk_pixbuf_get_width (thumbnail);
		height = gdk_pixbuf_get_height (thumbnail);
		gtk_drag_set_icon_pixbuf (context, thumbnail, width/2, height/2);
		g_object_unref (thumbnail);
	}
}

static void
view_on_drag_data_get_cb (GtkWidget        *widget,
			  GdkDragContext   *drag_context,
			  GtkSelectionData *data,
			  guint             info,
			  guint             time,
			  gpointer          user_data)
{
	XviewerScrollView *view;
	XviewerImage *image;
	gchar *uris[2];
	GFile *file;

	view = XVIEWER_SCROLL_VIEW (user_data);

	image = view->priv->image;

	file = xviewer_image_get_file (image);
	uris[0] = g_file_get_uri (file);
	uris[1] = NULL;

	gtk_selection_data_set_uris (data, uris);

	g_free (uris[0]);
	g_object_unref (file);
}

GtkWidget*
xviewer_scroll_view_new (void)
{
	GtkWidget *widget;

	widget = g_object_new (XVIEWER_TYPE_SCROLL_VIEW,
			       "can-focus", TRUE,
			       "row-homogeneous", FALSE,
			       "column-homogeneous", FALSE,
			       NULL);

	return widget;
}

static void
xviewer_scroll_view_popup_menu (XviewerScrollView *view, GdkEventButton *event)
{
	GtkWidget *popup;
	int button, event_time;

	popup = view->priv->menu;

	if (event) {
		button = event->button;
		event_time = event->time;
	} else {
		button = 0;
		event_time = gtk_get_current_event_time ();
	}

	gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL,
			button, event_time);
}

static gboolean
view_on_button_press_event_cb (GtkWidget *view, GdkEventButton *event,
			       gpointer user_data)
{
    /* Ignore double-clicks and triple-clicks */
    if (event->button == 3 && event->type == GDK_BUTTON_PRESS)
    {
	    xviewer_scroll_view_popup_menu (XVIEWER_SCROLL_VIEW (view), event);

	    return TRUE;
    }

    return FALSE;
}

void
xviewer_scroll_view_set_popup (XviewerScrollView *view,
			   GtkMenu *menu)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));
	g_return_if_fail (view->priv->menu == NULL);

	view->priv->menu = g_object_ref (menu);

	gtk_menu_attach_to_widget (GTK_MENU (view->priv->menu),
				   GTK_WIDGET (view),
				   NULL);

	g_signal_connect (G_OBJECT (view), "button_press_event",
			  G_CALLBACK (view_on_button_press_event_cb), NULL);
}

static gboolean
_xviewer_gdk_rgba_equal0 (const GdkRGBA *a, const GdkRGBA *b)
{
	if (a == NULL || b == NULL)
		return (a == b);

	return gdk_rgba_equal (a, b);
}

static gboolean
_xviewer_replace_gdk_rgba (GdkRGBA **dest, const GdkRGBA *src)
{
	GdkRGBA *old = *dest;

	if (_xviewer_gdk_rgba_equal0 (old, src))
		return FALSE;

	if (old != NULL)
		gdk_rgba_free (old);

	*dest = (src) ? gdk_rgba_copy (src) : NULL;

	return TRUE;
}

static void
_xviewer_scroll_view_update_bg_color (XviewerScrollView *view)
{
	const GdkRGBA *selected;
	XviewerScrollViewPrivate *priv = view->priv;

	if (priv->override_bg_color)
		selected = priv->override_bg_color;
	else if (priv->use_bg_color)
		selected = priv->background_color;
	else
		selected = NULL;

	if (priv->transp_style == XVIEWER_TRANSP_BACKGROUND
	    && priv->background_surface != NULL) {
		/* Delete the SVG background to have it recreated with
		 * the correct color during the next SVG redraw */
		cairo_surface_destroy (priv->background_surface);
		priv->background_surface = NULL;
	}

	/*gtk_widget_modify_bg (GTK_WIDGET (priv->display),
			      GTK_STATE_NORMAL,
			      selected);*/
	gtk_widget_override_background_color(GTK_WIDGET (priv->display), GTK_STATE_FLAG_NORMAL, selected);
}

void
xviewer_scroll_view_set_background_color (XviewerScrollView *view,
				      const GdkRGBA *color)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	if (_xviewer_replace_gdk_rgba (&view->priv->background_color, color))
		_xviewer_scroll_view_update_bg_color (view);
}

void
xviewer_scroll_view_override_bg_color (XviewerScrollView *view,
				   const GdkRGBA *color)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	if (_xviewer_replace_gdk_rgba (&view->priv->override_bg_color, color))
		_xviewer_scroll_view_update_bg_color (view);
}

void
xviewer_scroll_view_set_use_bg_color (XviewerScrollView *view, gboolean use)
{
	XviewerScrollViewPrivate *priv;

	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	priv = view->priv;

	if (use != priv->use_bg_color) {
		priv->use_bg_color = use;

		_xviewer_scroll_view_update_bg_color (view);

		g_object_notify (G_OBJECT (view), "use-background-color");
	}
}

void
xviewer_scroll_view_set_scroll_wheel_zoom (XviewerScrollView *view,
				       gboolean       scroll_wheel_zoom)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	if (view->priv->scroll_wheel_zoom != scroll_wheel_zoom) {
	        view->priv->scroll_wheel_zoom = scroll_wheel_zoom;
		g_object_notify (G_OBJECT (view), "scrollwheel-zoom");
	}
}

void
xviewer_scroll_view_set_zoom_multiplier (XviewerScrollView *view,
				     gdouble        zoom_multiplier)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

        view->priv->zoom_multiplier = 1.0 + zoom_multiplier;

	g_object_notify (G_OBJECT (view), "zoom-multiplier");
}

/* Helper to cause a redraw even if the zoom mode is unchanged */
static void
_set_zoom_mode_internal (XviewerScrollView *view, XviewerZoomMode mode)
{
	gboolean notify = (mode != view->priv->zoom_mode);


	if (mode == XVIEWER_ZOOM_MODE_SHRINK_TO_FIT)
		xviewer_scroll_view_zoom_fit (view);
	else
		view->priv->zoom_mode = mode;
	
	if (notify)
		g_object_notify (G_OBJECT (view), "zoom-mode");
}


void
xviewer_scroll_view_set_zoom_mode (XviewerScrollView *view, XviewerZoomMode mode)
{
	g_return_if_fail (XVIEWER_IS_SCROLL_VIEW (view));

	if (view->priv->zoom_mode == mode)
		return;

	_set_zoom_mode_internal (view, mode);
}

XviewerZoomMode
xviewer_scroll_view_get_zoom_mode (XviewerScrollView *view)
{
	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view),
			      XVIEWER_ZOOM_MODE_SHRINK_TO_FIT);

	return view->priv->zoom_mode;
}

static gboolean
xviewer_scroll_view_get_image_coords (XviewerScrollView *view, gint *x, gint *y,
                                  gint *width, gint *height)
{
	XviewerScrollViewPrivate *priv = view->priv;
	GtkAllocation allocation;
	gint scaled_width, scaled_height, xofs, yofs;

	compute_scaled_size (view, priv->zoom, &scaled_width, &scaled_height);

	if (G_LIKELY (width))
		*width = scaled_width;
	if (G_LIKELY (height))
		*height = scaled_height;

	/* If only width and height are needed stop here. */
	if (x == NULL && y == NULL)
		return TRUE;

	gtk_widget_get_allocation (GTK_WIDGET (priv->display), &allocation);

	/* Compute image offsets with respect to the window */

	if (scaled_width <= allocation.width)
		xofs = (allocation.width - scaled_width) / 2;
	else
		xofs = -priv->xofs;

	if (scaled_height <= allocation.height)
		yofs = (allocation.height - scaled_height) / 2;
	else
		yofs = -priv->yofs;

	if (G_LIKELY (x))
		*x = xofs;
	if (G_LIKELY (y))
		*y = yofs;

	return TRUE;
}

/**
 * xviewer_scroll_view_event_is_over_image:
 * @view: An #XviewerScrollView that has an image loaded.
 * @ev: A #GdkEvent which must have window-relative coordinates.
 *
 * Tells if @ev's originates from inside the image area. @view must be
 * realized and have an image set for this to work.
 *
 * It only works with #GdkEvent<!-- -->s that supply coordinate data,
 * i.e. #GdkEventButton.
 *
 * Returns: %TRUE if @ev originates from over the image, %FALSE otherwise.
 */
gboolean
xviewer_scroll_view_event_is_over_image (XviewerScrollView *view, const GdkEvent *ev)
{
	XviewerScrollViewPrivate *priv;
	GdkWindow *window;
	gdouble evx, evy;
	gint x, y, width, height;

	g_return_val_if_fail (XVIEWER_IS_SCROLL_VIEW (view), FALSE);
	g_return_val_if_fail (gtk_widget_get_realized(GTK_WIDGET(view)), FALSE);
	g_return_val_if_fail (ev != NULL, FALSE);

	priv = view->priv;
	window = gtk_widget_get_window (GTK_WIDGET (priv->display));

	if (G_UNLIKELY (priv->pixbuf == NULL 
	    || window != ((GdkEventAny*) ev)->window))
		return FALSE;

	if (G_UNLIKELY (!gdk_event_get_coords (ev, &evx, &evy)))
		return FALSE;

	if (!xviewer_scroll_view_get_image_coords (view, &x, &y, &width, &height))
		return FALSE;

	if (evx < x || evy < y || evx > (x + width) || evy > (y + height))
		return FALSE;

	return TRUE;
}

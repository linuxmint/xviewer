#ifndef _XVIEWER_SCROLL_VIEW_H_
#define _XVIEWER_SCROLL_VIEW_H_

#include <gtk/gtk.h>
#include "xviewer-image.h"

G_BEGIN_DECLS

typedef struct _XviewerScrollView XviewerScrollView;
typedef struct _XviewerScrollViewClass XviewerScrollViewClass;
typedef struct _XviewerScrollViewPrivate XviewerScrollViewPrivate;

#define XVIEWER_TYPE_SCROLL_VIEW              (xviewer_scroll_view_get_type ())
#define XVIEWER_SCROLL_VIEW(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), XVIEWER_TYPE_SCROLL_VIEW, XviewerScrollView))
#define XVIEWER_SCROLL_VIEW_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), XVIEWER_TYPE_SCROLL_VIEW, XviewerScrollViewClass))
#define XVIEWER_IS_SCROLL_VIEW(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XVIEWER_TYPE_SCROLL_VIEW))
#define XVIEWER_IS_SCROLL_VIEW_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), XVIEWER_TYPE_SCROLL_VIEW))


struct _XviewerScrollView {
	GtkGrid  base_instance;

	XviewerScrollViewPrivate *priv;
};

struct _XviewerScrollViewClass {
	GtkGridClass parent_class;

	void (* zoom_changed) (XviewerScrollView *view, double zoom);
	void (* rotation_changed) (XviewerScrollView *view, double degrees);
	void (* next_image) (XviewerScrollView *view);
	void (* previous_image) (XviewerScrollView *view);
};

/**
 * XviewerTransparencyStyle:
 * @XVIEWER_TRANSP_BACKGROUND: Use the background color of the current UI theme
 * @XVIEWER_TRANSP_CHECKED: Show transparent parts as a checkerboard pattern
 * @XVIEWER_TRANSP_COLOR: Show transparent parts in a user defined color
 *                    (see #XviewerScrollView:transparency-color )
 *
 * Used to define how transparent image parts are drawn.
 */
typedef enum {
	XVIEWER_TRANSP_BACKGROUND,
	XVIEWER_TRANSP_CHECKED,
	XVIEWER_TRANSP_COLOR
} XviewerTransparencyStyle;

/**
 * XviewerZoomMode:
 * @XVIEWER_ZOOM_MODE_FREE: Use the currently set zoom factor to display the image
 *                      (see xviewer_scroll_view_set_zoom()).
 * @XVIEWER_ZOOM_MODE_SHRINK_TO_FIT: If an image is to large for the window,
 *                               zoom out until the image is fully visible.
 *                               This will never zoom in on smaller images.
 *
 * Used to determine the zooming behaviour of an #XviewerScrollView.
 */
typedef enum {
	XVIEWER_ZOOM_MODE_FREE,
	XVIEWER_ZOOM_MODE_SHRINK_TO_FIT
} XviewerZoomMode;

GType    xviewer_scroll_view_get_type         (void) G_GNUC_CONST;
GtkWidget* xviewer_scroll_view_new            (void);

/* loading stuff */
void     xviewer_scroll_view_set_image        (XviewerScrollView *view, XviewerImage *image);
XviewerImage* xviewer_scroll_view_get_image       (XviewerScrollView *view);


/* general properties */
void     xviewer_scroll_view_set_scroll_wheel_zoom (XviewerScrollView *view, gboolean scroll_wheel_zoom);
void     xviewer_scroll_view_set_zoom_upscale (XviewerScrollView *view, gboolean upscale);
void     xviewer_scroll_view_set_zoom_multiplier (XviewerScrollView *view, gdouble multiplier);
void     xviewer_scroll_view_set_zoom_mode (XviewerScrollView *view, XviewerZoomMode mode);
XviewerZoomMode	xviewer_scroll_view_get_zoom_mode (XviewerScrollView *view);
void     xviewer_scroll_view_set_antialiasing_in (XviewerScrollView *view, gboolean state);
void     xviewer_scroll_view_set_antialiasing_out (XviewerScrollView *view, gboolean state);
void     xviewer_scroll_view_set_transparency_color (XviewerScrollView *view, GdkRGBA *color);
void     xviewer_scroll_view_set_transparency (XviewerScrollView *view, XviewerTransparencyStyle style);
gboolean xviewer_scroll_view_scrollbars_visible (XviewerScrollView *view);
void	 xviewer_scroll_view_set_popup (XviewerScrollView *view, GtkMenu *menu);
void	 xviewer_scroll_view_set_background_color (XviewerScrollView *view,
					       const GdkRGBA *color);
void	 xviewer_scroll_view_override_bg_color (XviewerScrollView *view,
					    const GdkRGBA *color);
void     xviewer_scroll_view_set_use_bg_color (XviewerScrollView *view, gboolean use);
/* zoom api */
void     xviewer_scroll_view_zoom_in          (XviewerScrollView *view, gboolean smooth);
void     xviewer_scroll_view_zoom_out         (XviewerScrollView *view, gboolean smooth);
void     xviewer_scroll_view_set_zoom         (XviewerScrollView *view, double zoom);
double   xviewer_scroll_view_get_zoom         (XviewerScrollView *view);
gboolean xviewer_scroll_view_get_zoom_is_min  (XviewerScrollView *view);
gboolean xviewer_scroll_view_get_zoom_is_max  (XviewerScrollView *view);
void     xviewer_scroll_view_show_cursor      (XviewerScrollView *view);
void     xviewer_scroll_view_hide_cursor      (XviewerScrollView *view);

gboolean xviewer_scroll_view_event_is_over_image	(XviewerScrollView *view,
						 const GdkEvent *ev);

G_END_DECLS

#endif /* _XVIEWER_SCROLL_VIEW_H_ */



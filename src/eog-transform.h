#ifndef _XVIEWER_TRANSFORM_H_
#define _XVIEWER_TRANSFORM_H_

#include <glib-object.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __XVIEWER_JOB_DECLR__
#define __XVIEWER_JOB_DECLR__
typedef struct _XviewerJob XviewerJob;
#endif

typedef enum {
	XVIEWER_TRANSFORM_NONE,
	XVIEWER_TRANSFORM_ROT_90,
	XVIEWER_TRANSFORM_ROT_180,
	XVIEWER_TRANSFORM_ROT_270,
	XVIEWER_TRANSFORM_FLIP_HORIZONTAL,
	XVIEWER_TRANSFORM_FLIP_VERTICAL,
	XVIEWER_TRANSFORM_TRANSPOSE,
	XVIEWER_TRANSFORM_TRANSVERSE
} XviewerTransformType;

#define XVIEWER_TYPE_TRANSFORM          (xviewer_transform_get_type ())
#define XVIEWER_TRANSFORM(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_TRANSFORM, XviewerTransform))
#define XVIEWER_TRANSFORM_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_TRANSFORM, XviewerTransformClass))
#define XVIEWER_IS_TRANSFORM(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_TRANSFORM))
#define XVIEWER_IS_TRANSFORM_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_TRANSFORM))
#define XVIEWER_TRANSFORM_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_TRANSFORM, XviewerTransformClass))

/* =========================================

    GObjecat wrapper around an affine transformation

   ----------------------------------------*/

typedef struct _XviewerTransform XviewerTransform;
typedef struct _XviewerTransformClass XviewerTransformClass;
typedef struct _XviewerTransformPrivate XviewerTransformPrivate;

struct _XviewerTransform {
	GObject parent;

	XviewerTransformPrivate *priv;
};

struct _XviewerTransformClass {
	GObjectClass parent_klass;
};

GType         xviewer_transform_get_type (void) G_GNUC_CONST;

GdkPixbuf*    xviewer_transform_apply   (XviewerTransform *trans, GdkPixbuf *pixbuf, XviewerJob *job);
XviewerTransform* xviewer_transform_reverse (XviewerTransform *trans);
XviewerTransform* xviewer_transform_compose (XviewerTransform *trans, XviewerTransform *compose);
gboolean      xviewer_transform_is_identity (XviewerTransform *trans);

XviewerTransform* xviewer_transform_identity_new (void);
XviewerTransform* xviewer_transform_rotate_new (int degree);
XviewerTransform* xviewer_transform_flip_new   (XviewerTransformType type /* only XVIEWER_TRANSFORM_FLIP_* are valid */);
#if 0
XviewerTransform* xviewer_transform_scale_new  (double sx, double sy);
#endif
XviewerTransform* xviewer_transform_new (XviewerTransformType trans);

XviewerTransformType xviewer_transform_get_transform_type (XviewerTransform *trans);

gboolean         xviewer_transform_get_affine (XviewerTransform *trans, cairo_matrix_t *affine);

G_END_DECLS

#endif /* _XVIEWER_TRANSFORM_H_ */



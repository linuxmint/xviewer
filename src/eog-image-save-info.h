#ifndef _XVIEWER_IMAGE_SAVE_INFO_H_
#define _XVIEWER_IMAGE_SAVE_INFO_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

G_BEGIN_DECLS

#ifndef __XVIEWER_IMAGE_DECLR__
#define __XVIEWER_IMAGE_DECLR__
typedef struct _XviewerImage XviewerImage;
#endif

#define XVIEWER_TYPE_IMAGE_SAVE_INFO            (xviewer_image_save_info_get_type ())
#define XVIEWER_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_IMAGE_SAVE_INFO, XviewerImageSaveInfo))
#define XVIEWER_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_IMAGE_SAVE_INFO, XviewerImageSaveInfoClass))
#define XVIEWER_IS_IMAGE_SAVE_INFO(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_IMAGE_SAVE_INFO))
#define XVIEWER_IS_IMAGE_SAVE_INFO_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_IMAGE_SAVE_INFO))
#define XVIEWER_IMAGE_SAVE_INFO_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_IMAGE_SAVE_INFO, XviewerImageSaveInfoClass))

typedef struct _XviewerImageSaveInfo XviewerImageSaveInfo;
typedef struct _XviewerImageSaveInfoClass XviewerImageSaveInfoClass;

struct _XviewerImageSaveInfo {
	GObject parent;

	GFile       *file;
	char        *format;
	gboolean     exists;
	gboolean     local;
	gboolean     has_metadata;
	gboolean     modified;
	gboolean     overwrite;

	float        jpeg_quality; /* valid range: [0.0 ... 1.0] */
};

struct _XviewerImageSaveInfoClass {
	GObjectClass parent_klass;
};

#define XVIEWER_FILE_FORMAT_JPEG   "jpeg"

GType             xviewer_image_save_info_get_type         (void) G_GNUC_CONST;

XviewerImageSaveInfo *xviewer_image_save_info_new_from_image   (XviewerImage *image);

XviewerImageSaveInfo *xviewer_image_save_info_new_from_uri     (const char      *uri,
							GdkPixbufFormat *format);

XviewerImageSaveInfo *xviewer_image_save_info_new_from_file    (GFile           *file,
							GdkPixbufFormat *format);

G_END_DECLS

#endif /* _XVIEWER_IMAGE_SAVE_INFO_H_ */

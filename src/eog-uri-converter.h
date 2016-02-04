#ifndef _XVIEWER_URI_CONVERTER_H_
#define _XVIEWER_URI_CONVERTER_H_

#include <glib-object.h>
#include "xviewer-image.h"

G_BEGIN_DECLS

#define XVIEWER_TYPE_URI_CONVERTER          (xviewer_uri_converter_get_type ())
#define XVIEWER_URI_CONVERTER(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), XVIEWER_TYPE_URI_CONVERTER, XviewerURIConverter))
#define XVIEWER_URI_CONVERTER_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST((k), XVIEWER_TYPE_URI_CONVERTER, XviewerURIConverterClass))
#define XVIEWER_IS_URI_CONVERTER(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), XVIEWER_TYPE_URI_CONVERTER))
#define XVIEWER_IS_URI_CONVERTER_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), XVIEWER_TYPE_URI_CONVERTER))
#define XVIEWER_URI_CONVERTER_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), XVIEWER_TYPE_URI_CONVERTER, XviewerURIConverterClass))

#ifndef __XVIEWER_URI_CONVERTER_DECLR__
#define __XVIEWER_URI_CONVERTER_DECLR__
typedef struct _XviewerURIConverter XviewerURIConverter;
#endif
typedef struct _XviewerURIConverterClass XviewerURIConverterClass;
typedef struct _XviewerURIConverterPrivate XviewerURIConverterPrivate;

typedef enum {
	XVIEWER_UC_STRING,
	XVIEWER_UC_FILENAME,
	XVIEWER_UC_COUNTER,
	XVIEWER_UC_COMMENT,
	XVIEWER_UC_DATE,
	XVIEWER_UC_TIME,
	XVIEWER_UC_DAY,
	XVIEWER_UC_MONTH,
	XVIEWER_UC_YEAR,
	XVIEWER_UC_HOUR,
	XVIEWER_UC_MINUTE,
	XVIEWER_UC_SECOND,
	XVIEWER_UC_END
} XviewerUCType;

typedef struct {
	char     *description;
	char     *rep;
	gboolean req_exif;
} XviewerUCInfo;

typedef enum {
	XVIEWER_UC_ERROR_INVALID_UNICODE,
	XVIEWER_UC_ERROR_INVALID_CHARACTER,
	XVIEWER_UC_ERROR_EQUAL_FILENAMES,
	XVIEWER_UC_ERROR_UNKNOWN
} XviewerUCError;

#define XVIEWER_UC_ERROR xviewer_uc_error_quark ()


struct _XviewerURIConverter {
	GObject parent;

	XviewerURIConverterPrivate *priv;
};

struct _XviewerURIConverterClass {
	GObjectClass parent_klass;
};

GType              xviewer_uri_converter_get_type      (void) G_GNUC_CONST;

GQuark             xviewer_uc_error_quark              (void);

XviewerURIConverter*   xviewer_uri_converter_new           (GFile *base_file,
                                                    GdkPixbufFormat *img_format,
						    const char *format_string);

gboolean           xviewer_uri_converter_check         (XviewerURIConverter *converter,
                                                    GList *img_list,
                                                    GError **error);

gboolean           xviewer_uri_converter_requires_exif (XviewerURIConverter *converter);

gboolean           xviewer_uri_converter_do            (XviewerURIConverter *converter,
                                                    XviewerImage *image,
                                                    GFile **file,
                                                    GdkPixbufFormat **format,
                                                    GError **error);

char*              xviewer_uri_converter_preview       (const char *format_str,
                                                    XviewerImage *img,
                                                    GdkPixbufFormat *format,
						    gulong counter,
						    guint n_images,
						    gboolean convert_spaces,
						    gunichar space_char);

/* for debugging purpose only */
G_GNUC_INTERNAL
void                xviewer_uri_converter_print_list (XviewerURIConverter *conv);

G_END_DECLS

#endif /* _XVIEWER_URI_CONVERTER_H_ */

#ifndef _XVIEWER_PIXBUF_UTIL_H_
#define _XVIEWER_PIXBUF_UTIL_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gio/gio.h>

G_GNUC_INTERNAL
GSList*          xviewer_pixbuf_get_savable_formats (void);

G_GNUC_INTERNAL
GdkPixbufFormat* xviewer_pixbuf_get_format_by_suffix (const char *suffix);

G_GNUC_INTERNAL
GdkPixbufFormat* xviewer_pixbuf_get_format (GFile *file);

G_GNUC_INTERNAL
char*            xviewer_pixbuf_get_common_suffix (GdkPixbufFormat *format);

#endif /* _XVIEWER_PIXBUF_UTIL_H_ */


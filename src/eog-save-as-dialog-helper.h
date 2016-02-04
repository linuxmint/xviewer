#ifndef _XVIEWER_SAVE_AS_DIALOG_HELPER_H_
#define _XVIEWER_SAVE_AS_DIALOG_HELPER_H_

#include <gtk/gtk.h>
#include <gio/gio.h>
#include "xviewer-uri-converter.h"


G_BEGIN_DECLS

G_GNUC_INTERNAL
GtkWidget*    xviewer_save_as_dialog_new       (GtkWindow *main, GList *images, GFile *base_file);

G_GNUC_INTERNAL
XviewerURIConverter* xviewer_save_as_dialog_get_converter (GtkWidget *dlg);


G_END_DECLS

#endif /* _XVIEWER_SAVE_DIALOG_HELPER_H_ */

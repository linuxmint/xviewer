#ifndef _XVIEWER_IMAGE_JPEG_H_
#define _XVIEWER_IMAGE_JPEG_H_

#if HAVE_JPEG

#include <glib.h>
#include "xviewer-image.h"
#include "xviewer-image-save-info.h"

/* Saves a source jpeg file in an arbitrary format (as specified by
 * target). The target pointer may be NULL, in which case the output
 * file is saved as jpeg too.  This method tries to be as smart as
 * possible. It will save the image as lossless as possible (if the
 * target is a jpeg image too).
 */
G_GNUC_INTERNAL
gboolean xviewer_image_jpeg_save_file (XviewerImage *image, const char *file,
				   XviewerImageSaveInfo *source, XviewerImageSaveInfo *target,
				   GError **error);
#endif

#endif /* _XVIEWER_IMAGE_JPEG_H_ */

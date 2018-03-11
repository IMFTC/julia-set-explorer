#ifndef __JSE_WINDOW_H__
#define __JSE_WINDOW_H__

#include <gtk/gtk.h>

#define JSE_TYPE_WINDOW (jse_window_get_type ())
G_DECLARE_FINAL_TYPE (JseWindow, jse_window, JSE, WINDOW, GtkApplicationWindow)

JseWindow *jse_window_new       (GtkApplication *app);
void       jse_window_set_image (struct _JseWindow *win, GtkImage *img);

#endif /* __JSE_WINDOW_H__ */

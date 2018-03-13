#ifndef __JSE_WINDOW_H__
#define __JSE_WINDOW_H__

#include <gtk/gtk.h>
#include "julia.h"

#define JSE_TYPE_WINDOW (jse_window_get_type ())
G_DECLARE_FINAL_TYPE (JseWindow, jse_window, JSE, WINDOW, GtkApplicationWindow)

JseWindow *jse_window_new       (GtkApplication *app);
void jse_window_set_image (struct _JseWindow *win, GtkImage *img);
void jse_window_set_zoom_level (JseWindow *win, gdouble zoom_level);
gint jse_window_get_zoom_level (JseWindow *win);

#endif /* __JSE_WINDOW_H__ */

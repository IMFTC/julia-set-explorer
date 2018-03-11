#ifndef __JSE_WINDOW_H
#define __JSE_WINDOW_H

#include <gtk/gtk.h>


#define JSE_TYPE_WINDOW (jse_window_get_type ())
G_DECLARE_FINAL_TYPE (JseWindow, jse_window, JSE, WINDOW, GtkApplicationWindow)

JseWindow       *jse_window_new          (void);

#endif /* __JSE_WINDOW_H */

#ifndef __JSE_WINDOW_H__
#define __JSE_WINDOW_H__

#include <gtk/gtk.h>
#include "julia.h"

#define JSE_TYPE_WINDOW (jse_window_get_type ())

G_DECLARE_FINAL_TYPE (JseWindow, jse_window, JSE, WINDOW, GtkApplicationWindow)

JseWindow *jse_window_new (GtkApplication *app);
void jse_window_set_cre (JseWindow *win,
                         double cre);
double jse_window_get_cre (JseWindow *win);

void jse_window_set_cim (JseWindow *win,
                         double cim);
double jse_window_get_cim (JseWindow *win);

void jse_window_get_center (JseWindow *win,
                            gdouble *x,
                            gdouble *y);
void jse_window_set_center (JseWindow *win,
                            gdouble x,
                            gdouble y);

gdouble jse_window_get_center_re (JseWindow *win);
void jse_window_set_center_re (JseWindow *win,
                               gdouble x);

gdouble jse_window_get_center_im (JseWindow *win);
void jse_window_set_center_im (JseWindow *win,
                               gdouble y);

void jse_window_set_iterations (JseWindow *win,
                                guint iterations);
guint jse_window_get_iterations (JseWindow *win);

void jse_window_set_zoom_level (JseWindow *win,
                                gdouble zoom_level);
gdouble jse_window_get_zoom_level (JseWindow *win);

#endif /* __JSE_WINDOW_H__ */

#include <gtk/gtk.h>

#include "jse-window.h"

struct _JseWindow
{
  GtkApplicationWindow parent;

  GtkApplication *app;
  GtkImage *image;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static void
jse_window_init (JseWindow *window)
{
  gtk_widget_init_template (GTK_WIDGET (window));
}

static void
jse_window_dispose (GObject *object)
{
  /* TODO: clear private objects once we have some */

  G_OBJECT_CLASS (jse_window_parent_class)->dispose (object);
}

static void
jse_window_class_init (JseWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = jse_window_dispose;
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/jse/window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class),
                                        JseWindow, image);
}

JseWindow *
jse_window_new (GtkApplication *app)
{
  return g_object_new (JSE_TYPE_WINDOW, "application", app, NULL);
}

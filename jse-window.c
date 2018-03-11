#include <gtk/gtk.h>

#include "jse-window.h"

struct _JseWindow
{
  GtkApplicationWindow parent;
};

typedef struct _JseWindowPrivate JseWindowPrivate;

struct _JseWindowPrivate
{
  GtkWidget *image;
};

G_DEFINE_TYPE_WITH_PRIVATE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static void
jse_window_init (JseWindow *window)
{
  // JseWindowPrivate *priv;

  // priv = jse_window_get_instance_private (window);

  gtk_widget_init_template (GTK_WIDGET (window));

  /* TODO */
}

static void
jse_window_dispose (GObject *object)
{
  /* TODO: clear private stuff */

  /* JseWindow *win; */
  /* JseWindowPrivate *priv; */

  /* win = JSE_WINDOW (object); */
  /* priv = jse_window_get_instance_private (win); */

  G_OBJECT_CLASS (jse_window_parent_class)->dispose (object);
}

static void
jse_window_class_init (JseWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = jse_window_dispose;
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/JuliaSetexplorer/window.glade");
  gtk_widget_class_bind_template_child_private (GTK_WIDGET_CLASS (class),
                                                JseWindow, image);
}

JseWindow *
jse_window_new (void)
{
  return g_object_new (JSE_TYPE_WINDOW, NULL);
}

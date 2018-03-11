#include <gtk/gtk.h>

#include "jse-window.h"

struct _JseWindow
{
  GtkApplicationWindow parent;

  GtkApplication *app;
  GtkImage *eventbox;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean image_scroll_event_cb (GtkWidget *unused,
                                       GdkEventScroll *event,
                                       gpointer user_data);


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
                                        JseWindow, eventbox);
}

JseWindow *
jse_window_new (GtkApplication *app)
{
  return g_object_new (JSE_TYPE_WINDOW, "application", app, NULL);
}

void
jse_window_set_image (struct _JseWindow *win, GtkImage *img)
{
  gtk_container_add (GTK_CONTAINER (win->eventbox), GTK_WIDGET (img));
}

static gboolean
image_scroll_event_cb (GtkWidget *unused,
                       GdkEventScroll *event,
                       gpointer user_data)
{
  printf("Got GdkEventScroll at (%3f, %3f)\n", event->x, event->y);

  /* unpack user_data */
  /* struct CallbackData *cb_data = (struct CallbackData*) user_data; */
  /* JuliaView *jv = cb_data->jv; */
  /* GtkImage *image = cb_data->image; */

  /* GdkPixbuf *gdk_pixbuf = gtk_image_get_pixbuf (image); */

  /* JuliaPixbuf *julia_pixbuf; */
  /* gpointer orig_key, value; */
  /* g_debug ("image_scroll_event_cb"); */

  /* switch (event->direction) */
  /*   { */
  /*   case GDK_SCROLL_DOWN: */
  /*     if (jv->zoom_level < MAX_ZOOM_LEVEL) */
  /*       jv->zoom_level++; */
  /*     else */
  /*       { */
  /*         g_debug ("Reached MAX_ZOOM_LEVEL of %d", MAX_ZOOM_LEVEL); */
  /*         return TRUE; */
  /*       } */
  /*     break; */
  /*   case GDK_SCROLL_UP: */
  /*     if (jv->zoom_level > MIN_ZOOM_LEVEL) */
  /*       jv->zoom_level--; */
  /*     else */
  /*       { */
  /*         g_debug ("Reached MIN_ZOOM_LEVEL of %d", MIN_ZOOM_LEVEL); */
  /*         return TRUE; */
  /*       } */
  /*     break; */
  /*   default: */
  /*     g_message ("Unhandled scroll direction!"); */
  /*   } */

  /* g_debug ("zoom level: %d", jv->zoom_level); */

  /* if (g_hash_table_lookup_extended (hashtable, */
  /*                                   GINT_TO_POINTER (jv->zoom_level), */
  /*                                   &orig_key, */
  /*                                   &value)) */
  /*   { */
  /*     /\* hash table hit *\/ */

  /*     gdk_pixbuf = (GdkPixbuf *) value; */
  /*     g_debug ("Cache hit for zoom level %d", jv->zoom_level); */
  /*   } */
  /* else */
  /*   { */
  /*     /\* hash table miss *\/ */

  /*     g_debug ("Cache miss for zoom level %d", jv->zoom_level); */
  /*     /\* create a new pixbuf for the new zoom value *\/ */
  /*     julia_pixbuf = julia_pixbuf_new (gdk_pixbuf_get_width (gdk_pixbuf), */
  /*                                      gdk_pixbuf_get_height (gdk_pixbuf)); */
  /*     julia_pixbuf_update_mt (julia_pixbuf, jv); */

  /*     /\* transfer ownership of julia_pixbuf->pixbuf to gdk_pixbuf *\/ */
  /*     gdk_pixbuf = gdk_pixbuf_new_from_data (julia_pixbuf->pixbuf, */
  /*                                            GDK_COLORSPACE_RGB, */
  /*                                            FALSE, */
  /*                                            8, */
  /*                                            julia_pixbuf->pix_width, */
  /*                                            julia_pixbuf->pix_height, */
  /*                                            julia_pixbuf->pix_width * 3, */
  /*                                            pixbuf_destroy_notify, */
  /*                                            NULL); */
  /*     free (julia_pixbuf); */

  /*     g_hash_table_insert (hashtable, */
  /*                          GINT_TO_POINTER (jv->zoom_level), */
  /*                          (gpointer) gdk_pixbuf); */
  /*   }; */

  /* g_debug ("setting image with zoom level %d", jv->zoom_level); */
  /* gtk_image_set_from_pixbuf (image, gdk_pixbuf); */

  /* stop further handling of event */
  return TRUE;
}

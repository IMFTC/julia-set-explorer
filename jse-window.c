#include <gtk/gtk.h>

#include "jse-window.h"
#include "julia.h"

/* TODO: Move to a better place / remove hard-coding */
#define PIXBUF_HEIGHT 800
#define PIXBUF_WIDTH  800

#define MAX_ZOOM_LEVEL 200
#define MIN_ZOOM_LEVEL -20
#define MAX_ITERATIONS 300

#define CX -0.7269
#define CY +0.1889

struct _JseWindow
{
  GtkApplicationWindow parent;

  GtkWidget *eventbox;
  JuliaView *jv;
  GHashTable *hashtable;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean image_scroll_event_cb (GtkWidget *unused,
                                       GdkEventScroll *event,
                                       gpointer user_data);
static void pixbuf_destroy_notify (guchar *pixels,
                                   gpointer data);


static void
jse_window_init (JseWindow *window)
{
  GdkPixbuf *pixbuf;
  JuliaPixbuf *jp;
  GtkImage *image;
  JuliaView *jv;

  gtk_widget_init_template (GTK_WIDGET (window));

  window->hashtable = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);

  /* TODO: make jv a property of the window */
  jv = julia_view_new (0, 0, 4, 4, 0, CX, CY, MAX_ITERATIONS);
  jp = julia_pixbuf_new (PIXBUF_WIDTH, PIXBUF_HEIGHT);
  julia_pixbuf_update_mt(jp, jv);

  /* move ownership of jp->pixbuf to GdkPixbuf pixbuf */
  pixbuf = gdk_pixbuf_new_from_data (jp->pixbuf,
                                     GDK_COLORSPACE_RGB,
                                     FALSE,
                                     8,
                                     PIXBUF_WIDTH,
                                     PIXBUF_HEIGHT,
                                     PIXBUF_WIDTH * 3,
                                     pixbuf_destroy_notify,
                                     NULL);
  jp->pixbuf = NULL;
  julia_pixbuf_destroy (jp);

  image = GTK_IMAGE (gtk_image_new_from_pixbuf (pixbuf));
  gtk_widget_set_visible (GTK_WIDGET (image), TRUE);
  gtk_container_add (GTK_CONTAINER (window->eventbox), GTK_WIDGET (image));
  gtk_widget_add_events (GTK_WIDGET (window->eventbox), GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK);

  /* insert pixbuf into hashtable */
  g_hash_table_insert (window->hashtable,
                       GINT_TO_POINTER (0),
                       (gpointer) pixbuf);
}

static void
jse_window_dispose (GObject *object)
{
  JseWindow *window = JSE_WINDOW (object);
  julia_view_destroy (window->jv);

  G_OBJECT_CLASS (jse_window_parent_class)->dispose (object);
}

static void
jse_window_class_init (JseWindowClass *class)
{
  G_OBJECT_CLASS (class)->dispose = jse_window_dispose;
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/jse/window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, eventbox);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_scroll_event_cb);
}

JseWindow *
jse_window_new (GtkApplication *app)
{
  return g_object_new (JSE_TYPE_WINDOW,
                       "application", app,
                       NULL);
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


static void
pixbuf_destroy_notify (guchar *pixels,
                       gpointer data)
{
  g_debug ("Freeing pixel data at %p", pixels);
  g_free (pixels);
}

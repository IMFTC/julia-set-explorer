#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include <string.h>
#include <math.h>
#include <stdlib.h>

#include "julia.h"

/* Use even numbers to allow mirroring! */
#define PIXBUF_HEIGHT 800
#define PIXBUF_WIDTH  800

#define MAX_ZOOM_LEVEL 200
#define MIN_ZOOM_LEVEL -20

/* TODO: Make values interactive */

/* #define CX -.4 */
/* #define CY .6 */

/* #define CX -0.79 */
/* #define CY 0.157 */

/* #define CX -0.835 */
/* #define CY -0.2321 */

#define CX -0.7269
#define CY +0.1889

#define MAX_ITERATIONS 300

static GHashTable *hashtable;

static gboolean scroll_event_cb (GtkWidget *widget,
                                 GdkEventScroll *event,
                                 gpointer user_data);

void
pixbuf_destroy_notify (guchar *pixels,
                       gpointer data)
{
  g_debug ("Freeing pixel data at %p", pixels);
  g_free (pixels);
}

static gboolean key_press_event_cb (GtkWidget *widget,
                                    GdkEventButton *event,
                                    gpointer user_data)
{
  printf("GdkEventKey at (%f,%f)\n", event->x, event->y);

  return TRUE;
}

struct CallbackData
{
  JuliaView *jv;
  GtkImage *image;
};

static gboolean
scroll_event_cb (GtkWidget *unused,
                 GdkEventScroll *event,
                 gpointer user_data)
{
  // printf("Got GdkEventScroll at (%3f, %3f)\n", event->x, event->y);

  /* unpack user_data */
  struct CallbackData *cb_data = (struct CallbackData*) user_data;
  JuliaView *jv = cb_data->jv;
  GtkImage *image = cb_data->image;

  GdkPixbuf *gdk_pixbuf = gtk_image_get_pixbuf (image);

  JuliaPixbuf *julia_pixbuf;
  gpointer orig_key, value;
  g_debug ("scroll_event_cb");

  switch (event->direction)
    {
    case GDK_SCROLL_DOWN:
      if (jv->zoom_level < MAX_ZOOM_LEVEL)
        jv->zoom_level++;
      else
        {
          g_debug ("Reached MAX_ZOOM_LEVEL of %d", MAX_ZOOM_LEVEL);
          return TRUE;
        }
      break;
    case GDK_SCROLL_UP:
      if (jv->zoom_level > MIN_ZOOM_LEVEL)
        jv->zoom_level--;
      else
        {
          g_debug ("Reached MIN_ZOOM_LEVEL of %d", MIN_ZOOM_LEVEL);
          return TRUE;
        }
      break;
    default:
      g_message ("Unhandled scroll direction!");
    }

  g_debug ("zoom level: %d", jv->zoom_level);

  if (g_hash_table_lookup_extended (hashtable,
                                    GINT_TO_POINTER (jv->zoom_level),
                                    &orig_key,
                                    &value))
    {
      /* hash table hit */

      gdk_pixbuf = (GdkPixbuf *) value;
      g_debug ("Cache hit for zoom level %d", jv->zoom_level);
    }
  else
    {
      /* hash table miss */

      g_debug ("Cache miss for zoom level %d", jv->zoom_level);
      /* create a new pixbuf for the new zoom value */
      julia_pixbuf = julia_pixbuf_new (gdk_pixbuf_get_width (gdk_pixbuf),
                                       gdk_pixbuf_get_height (gdk_pixbuf));
      julia_pixbuf_update_mt (julia_pixbuf, jv);

      /* transfer ownership of julia_pixbuf->pixbuf to gdk_pixbuf */
      gdk_pixbuf = gdk_pixbuf_new_from_data (julia_pixbuf->pixbuf,
                                             GDK_COLORSPACE_RGB,
                                             FALSE,
                                             8,
                                             julia_pixbuf->pix_width,
                                             julia_pixbuf->pix_height,
                                             julia_pixbuf->pix_width * 3,
                                             pixbuf_destroy_notify,
                                             NULL);
      free (julia_pixbuf);

      g_hash_table_insert (hashtable,
                           GINT_TO_POINTER (jv->zoom_level),
                           (gpointer) gdk_pixbuf);
    };

  printf("setting image with zoom level %d\n", jv->zoom_level);
  gtk_image_set_from_pixbuf (image, gdk_pixbuf);

  /* stop further handling of event */
  return TRUE;
}

static void
activate (GtkApplication *app,
          gpointer user_data)
{
  GtkWidget *window;
  GtkWidget *eventbox;
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  JuliaPixbuf *jp;
  JuliaView *jv;

  window = gtk_application_window_new (app);

  gtk_window_set_title (GTK_WINDOW (window), "Julia Set Explorer");

  jp = julia_pixbuf_new (PIXBUF_WIDTH, PIXBUF_HEIGHT);
  jv = julia_view_new (0, 0, 4, 4, 0, CX, CY, MAX_ITERATIONS);
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

  /* insert pixbuf into hashtable */
  g_hash_table_insert (hashtable,
                       GINT_TO_POINTER (0),
                       (gpointer) pixbuf);

  image = gtk_image_new_from_pixbuf (pixbuf);

  struct CallbackData *cb_data = calloc (1, sizeof (struct CallbackData));
  cb_data->jv = jv;
  cb_data->image = GTK_IMAGE (image);

  g_print("gdk_pixbuf_get_byte_length: %lu\n",
          gdk_pixbuf_get_byte_length (pixbuf));

  eventbox = gtk_event_box_new ();
  /* FIXME: There is some padding around the image that should
   * not be there and into which the eventbox expands. So the
   * next two lines are needed to ensure the eventbox and the
   * image share the same coordinates. */
  gtk_widget_set_halign (eventbox, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (eventbox, GTK_ALIGN_CENTER);

  // gtk_event_box_set_above_child(GTK_EVENT_BOX (eventbox), TRUE);

  gtk_container_add (GTK_CONTAINER (eventbox), image);

  /* GtkEventBox does not catch scroll events by default, add
   * them manually since we use them for zooming. */
  gtk_widget_add_events (eventbox, GDK_SCROLL_MASK);

  // g_signal_connect(window, "event", G_CALLBACK (scroll_event_cb), NULL);
  g_signal_connect(eventbox, "scroll-event", G_CALLBACK (scroll_event_cb), cb_data);
  g_signal_connect(eventbox, "key-press-event", G_CALLBACK (key_press_event_cb), cb_data);
  // g_signal_connect(eventbox, "key-press-event", G_CALLBACK (scroll_event_cb), NULL);

  /* TODO: Cleanup */
  // g_signal_connect(window, "delete-event", G_CALLBACK (key_press_event_cb), cb_data);

  gtk_container_add (GTK_CONTAINER (window), eventbox);
  gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{

  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gnome.JuliaSetExplorer", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  hashtable = g_hash_table_new_full (g_direct_hash,
                                     g_direct_equal,
                                     NULL,
                                     g_object_unref);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_hash_table_destroy (hashtable);
  g_object_unref (app);

  return status;
}

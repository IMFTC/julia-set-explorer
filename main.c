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

/* Scaling when zooming in one step */
#define ZOOM_FACTOR 0.9

/* TODO: Make values interactive */
#define	CX -.4
#define	CY .6
#define MAX_ITERATIONS 120

static gboolean scroll_event_cb (GtkWidget *widget, GdkEventScroll *event, gpointer user_data);

static gboolean
button_press_event_cb (GtkWidget *widget,
		       GdkEventButton *event,
		       gpointer user_data)
{
  printf("GdkEventButton at (%f,%f)\n", event->x, event->y);

  return TRUE;
}

struct CallbackData
{
  JuliaPixbuf *jp;
  JuliaView *jv;
  GtkImage *image;
};

static gboolean
scroll_event_cb (GtkWidget *widget,
		 GdkEventScroll *event,
		 gpointer user_data)
{
  printf("Got GdkEventScroll at (%3f, %3f)\n", event->x, event->y);

  struct CallbackData *cb_data = (struct CallbackData*) user_data;
  JuliaPixbuf *jp = cb_data->jp;
  JuliaView *jv = cb_data->jv;
  GtkImage *image = cb_data->image;
  GdkPixbuf *pixbuf = gtk_image_get_pixbuf (image);

  switch (event->direction) {
  case GDK_SCROLL_DOWN:
    jv->zoom_level++;
    break;
  case GDK_SCROLL_UP:
    jv->zoom_level--;
    break;
  default:
    g_message("Unhandled scroll direction!");
  }

  printf("Setting zoom level to %d\n", jv->zoom_level);
  julia_pixbuf_update(jp, jv);
  gtk_image_set_from_pixbuf (image, pixbuf);
  /* stop further handling of event */
  return TRUE;
}

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *eventbox;
  GtkWidget *image;
  GdkPixbuf *pixbuf;
  JuliaPixbuf *jp;
  JuliaView *jv;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Julia Set Explorer");

  jp = julia_pixbuf_new (PIXBUF_WIDTH, PIXBUF_HEIGHT);
  jv = julia_view_new (0, 0, 4, 4, 0, CX, CY, MAX_ITERATIONS);
  julia_pixbuf_update(jp, jv);

  struct CallbackData *cb_data = calloc (1, sizeof (struct CallbackData));
  cb_data->jp = jp;
  cb_data->jv = jv;

  pixbuf = gdk_pixbuf_new_from_data (jp->pixbuf,
				     GDK_COLORSPACE_RGB,
				     FALSE,
				     8,
				     PIXBUF_WIDTH,
				     PIXBUF_HEIGHT,
				     PIXBUF_WIDTH * 3,
				     NULL,
				     NULL);

  image = gtk_image_new_from_pixbuf (pixbuf);

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
  g_signal_connect(eventbox, "button-press-event", G_CALLBACK (button_press_event_cb), cb_data);

  // g_signal_connect(eventbox, "key-press-event", G_CALLBACK (scroll_event_cb), NULL);

  gtk_container_add (GTK_CONTAINER (window), eventbox);
  gtk_widget_show_all (GTK_WIDGET (window));

  gtk_main();

  julia_pixbuf_destroy(jp);
  julia_view_destroy(jv);


  return 0;
}

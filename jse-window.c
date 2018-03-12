#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>

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
  GtkImage *image;

  GdkPixbuf *gdk_pixbuf;
  gint pixbuf_width;
  gint pixbuf_height;

  JuliaView *jv;
  GHashTable *hashtable;
  GtkWidget *label_position;
  GtkWidget *label_zoom;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static gboolean image_scroll_event_cb (GtkWidget *unused,
                                       GdkEventScroll *event,
                                       JseWindow *user_data);
static gboolean image_motion_notify_event_cb (JseWindow *win,
                                              GdkEventMotion *event);
static void pixbuf_destroy_notify (guchar *pixels,
                                   gpointer data);

static void
jse_window_init (JseWindow *window)
{
  GdkPixbuf *pixbuf;
  JuliaPixbuf *jp;
  GtkImage *image;

  gtk_widget_init_template (GTK_WIDGET (window));

  window->hashtable = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);

  /* TODO: make jv a property of the window */
  window->jv = julia_view_new (0, 0, 4, 4, 0, CX, CY, MAX_ITERATIONS);
  jp = julia_pixbuf_new (PIXBUF_WIDTH, PIXBUF_HEIGHT);
  julia_pixbuf_update_mt(jp, window->jv);

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
  window->gdk_pixbuf = pixbuf;

  image = GTK_IMAGE (gtk_image_new_from_pixbuf (pixbuf));
  window->image = image;
  gtk_widget_set_visible (GTK_WIDGET (image), TRUE);
  gtk_container_add (GTK_CONTAINER (window->eventbox), GTK_WIDGET (image));
  gtk_widget_add_events (GTK_WIDGET (window->eventbox),
                         GDK_SCROLL_MASK
                         /* TODO: Implement smooth scrolling support */
                         // | GDK_SMOOTH_SCROLL_MASK
                         | GDK_POINTER_MOTION_MASK);

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
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, label_position);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_scroll_event_cb);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_motion_notify_event_cb);
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
                       JseWindow *window)
{
  g_debug ("Got GdkEventScroll at (%3f, %3f)", event->x, event->y);


  JuliaView *jv = window->jv;
  GHashTable *hashtable = window->hashtable;
  GtkImage *image = window->image;
  GdkPixbuf *gdk_pixbuf;
  JuliaPixbuf *julia_pixbuf;
  gpointer orig_key, value;
  g_debug ("image_scroll_event_cb");

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
    case GDK_SCROLL_SMOOTH:
      g_message ("Smooth scrolling not implementet yet!");
      /* fall through */
    default:
      g_message ("Unhandled scroll direction!");
      return TRUE;
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
      gdk_pixbuf = gtk_image_get_pixbuf (image);
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

  gtk_image_set_from_pixbuf (image, gdk_pixbuf);

  /* stop further handling of event */
  return TRUE;
}

static void
update_position_label (JseWindow *win, gdouble x, gdouble y)
{
  JuliaView *jv = win->jv;
  int zoom_level = jv->zoom_level;

  double width = jv->default_width * pow(ZOOM_FACTOR, zoom_level);
  double height = jv->default_height * pow(ZOOM_FACTOR, zoom_level);

  double pos_re = jv->center_re + width * (x / PIXBUF_WIDTH - 0.5);
  double pos_im = jv->center_im + height * (y / PIXBUF_HEIGHT - 0.5);

  GString *text = g_string_new (NULL);
  g_string_printf (text, "pos: %+8.3f %+8.3fi", pos_re, pos_im);
  gtk_label_set_text (GTK_LABEL (win->label_position), text->str);
  g_string_free (text, TRUE);
}

static gboolean
image_motion_notify_event_cb (JseWindow *win,
                              GdkEventMotion *event)

{
  g_debug ("motion-notify event at (%f, %f)", event->x, event->y);

  update_position_label (win, event->x, event->y);

  return TRUE;
}

static void
pixbuf_destroy_notify (guchar *pixels,
                       gpointer data)
{
  g_debug ("Freeing pixel data at %p", pixels);
  g_free (pixels);
}

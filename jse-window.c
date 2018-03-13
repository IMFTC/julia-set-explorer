#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "jse-window.h"
#include "julia.h"

/* TODO: Move to a better place / remove hard-coding */
#define PIXBUF_HEIGHT 800
#define PIXBUF_WIDTH  800

#define MAX_ZOOM_LEVEL 200
#define MIN_ZOOM_LEVEL -20
#define MAX_ITERATIONS 300

/* TODO: The user should be able to set these! */
/* The view of the complex plane to be displayed by the window */
#define VIEW_WIDTH 4
#define VIEW_HEIGHT 4
/* TODO: This MUST be (0, 0) for now since we assume symmetry */
#define VIEW_CENTER_RE 0
#define VIEW_CENTER_IM 0
/* c as in f(z) = z^2 + c */
#define C_RE -0.7269
#define C_IM +0.1889

struct _JseWindow
{
  GtkApplicationWindow parent;

  GtkWidget *eventbox;
  GtkImage *image;

  /* GdkPixbuf *gdk_pixbuf; */
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
static void update_position_label (JseWindow *win,
                                   gdouble x,
                                   gdouble y);
void jse_window_set_zoom_level (JseWindow *win,
                                gint zoom_level);


static void
jse_window_init (JseWindow *window)
{
  gtk_widget_init_template (GTK_WIDGET (window));

  window->hashtable = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);

  gtk_widget_add_events (GTK_WIDGET (window->eventbox),
                         GDK_SCROLL_MASK
                         /* TODO: Implement smooth scrolling support */
                         // | GDK_SMOOTH_SCROLL_MASK
                         | GDK_POINTER_MOTION_MASK);

  window->jv = julia_view_new (VIEW_CENTER_RE, VIEW_CENTER_IM,
                               VIEW_WIDTH, VIEW_HEIGHT, 0,
                               C_RE, C_IM, MAX_ITERATIONS);

  window->pixbuf_width = PIXBUF_WIDTH;
  window->pixbuf_height = PIXBUF_HEIGHT;
  /* TODO: make jv a property of the window */
  jse_window_set_zoom_level (window, 0);
}

static void
jse_window_finalize (GObject *object)
{
  JseWindow *window = JSE_WINDOW (object);

  g_debug ("jse_window_finalize (%p)", object);

  julia_view_destroy (window->jv);
  g_hash_table_destroy (window->hashtable);

  G_OBJECT_CLASS (jse_window_parent_class)->finalize (object);
}

static void
jse_window_class_init (JseWindowClass *class)
{
  G_OBJECT_CLASS (class)->finalize = jse_window_finalize;
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/jse/window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, eventbox);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, image);
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

GdkPixbuf *
get_pixbuf_for_zoom_level (JseWindow *win, gint zoom_level)
{
  GHashTable *hashtable = win->hashtable;
  JuliaView *jv = win->jv;
  GdkPixbuf *gdk_pixbuf;
  JuliaPixbuf *jp;
  gpointer orig_key;
  gpointer value;

  /* create dummy view used to update the pixbuf with the requested
     zoom_level */
  JuliaView jv_tmp;
  memcpy(&jv_tmp, jv, sizeof (JuliaView));
  jv_tmp.zoom_level = zoom_level;

  if (g_hash_table_lookup_extended (hashtable,
                                    GINT_TO_POINTER (zoom_level),
                                    &orig_key,
                                    &value))
    {
      /* hash table hit */

      gdk_pixbuf = (GdkPixbuf *) value;
      g_debug ("Cache hit for zoom level %d", zoom_level);
    }
  else
    {
      /* hash table miss */

      g_debug ("Cache miss for zoom level %d", zoom_level);
      /* create a new pixbuf for the new zoom value */
      jp = julia_pixbuf_new (win->pixbuf_width,
                             win->pixbuf_height);
      julia_pixbuf_update_mt (jp, &jv_tmp);

      /* transfer ownership of jp->pixbuf to gdk_pixbuf */
      gdk_pixbuf = gdk_pixbuf_new_from_data (jp->pixbuf,
                                             GDK_COLORSPACE_RGB,
                                             FALSE,
                                             8,
                                             jp->pix_width,
                                             jp->pix_height,
                                             jp->pix_width * 3,
                                             pixbuf_destroy_notify,
                                             NULL);
      free (jp);

      g_hash_table_insert (hashtable,
                           GINT_TO_POINTER (zoom_level),
                           (gpointer) gdk_pixbuf);
    };

  g_debug ("get_pixbuf_for_zoom_level (%p, %d): %p", win, zoom_level, gdk_pixbuf);

  return gdk_pixbuf;
}

void
jse_window_set_zoom_level (JseWindow *win, gint zoom_level)
{
  g_debug ("zoom level: %d", zoom_level);

  GdkPixbuf *gdk_pixbuf = get_pixbuf_for_zoom_level (win, zoom_level);
  gtk_image_set_from_pixbuf (win->image, gdk_pixbuf);
  g_debug ("set pixbuf: %p", gdk_pixbuf);
  win->jv->zoom_level = zoom_level;
}

static gboolean
image_scroll_event_cb (GtkWidget *unused,
                       GdkEventScroll *event,
                       JseWindow *window)
{
  g_debug ("scroll-event at (%3f, %3f)", event->x, event->y);
  gint zoom_level = window->jv->zoom_level;

  switch (event->direction)
    {
    case GDK_SCROLL_DOWN:
      if (zoom_level < MAX_ZOOM_LEVEL)
        zoom_level++;
      else
        {
          g_debug ("Reached MAX_ZOOM_LEVEL of %d", MAX_ZOOM_LEVEL);
          return TRUE;
        }
      break;
    case GDK_SCROLL_UP:
      if (zoom_level > MIN_ZOOM_LEVEL)
        zoom_level--;
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

  jse_window_set_zoom_level (window, zoom_level);
  update_position_label (window, event->x, event->y);

  /* stop further handling of event */
  return TRUE;
}

static void
update_position_label (JseWindow *win, gdouble x, gdouble y)
{
  /* TODO: Reduce calculations by saving values somewhere */
  JuliaView *jv = win->jv;
  int zoom_level = jv->zoom_level;

  double width = jv->default_width * pow(ZOOM_FACTOR, zoom_level);
  double height = jv->default_height * pow(ZOOM_FACTOR, zoom_level);

  double pos_re = jv->center_re + width * (x / PIXBUF_WIDTH - 0.5);
  double pos_im = jv->center_im + height * (0.5 - y / PIXBUF_HEIGHT);

  GString *text = g_string_new (NULL);

  g_string_printf (text, "pos: %+.15f %+1.15fi", pos_re, pos_im);
  gtk_label_set_text (GTK_LABEL (win->label_position), text->str);
  g_string_free (text, TRUE);
}

static gboolean
image_motion_notify_event_cb (JseWindow *win,
                              GdkEventMotion *event)

{
  // g_debug ("motion-notify event at (%f, %f)", event->x, event->y);

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

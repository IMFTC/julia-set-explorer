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
#define MIN_ZOOM_LEVEL -10
#define MAX_ITERATIONS 200

/* TODO: The user should be able to set these! */
/* The view of the complex plane to be displayed by the window */
#define VIEW_WIDTH 4
#define VIEW_HEIGHT 4
/* TODO: This MUST be (0, 0) for now since we assume symmetry */
#define VIEW_CENTER_RE 0
#define VIEW_CENTER_IM 0
/* c as in f(z) = z^2 + c */
#define C_RE -0.4
#define C_IM +0.6

enum {
  PROP_0,
  PROP_CRE,
  PROP_CIM,
  PROP_ZOOM_LEVEL,
  N_PROPS
};

static GParamSpec *props[N_PROPS] = {NULL, };

struct _JseWindow
{
  GtkApplicationWindow parent;

  /* dimensions at zoom level 0 */
  gdouble view_width;
  gdouble view_height;

  /* TODO allow center to differ from (0, 0) */
  gdouble view_center_re;
  gdouble view_center_im;

  /* c in: f(z) = z^2 + c */
  double cre;
  double cim;

  /* GdkPixbuf *gdk_pixbuf; */
  gint pixbuf_width;
  gint pixbuf_height;

  /* UI parts */
  GtkWidget *eventbox;
  GtkImage *image;

  /* TODO: Use GActions */
  GtkToggleButton *button_c;
  GtkLabel *button_c_label;
  GtkPopover *button_c_popover;

  GHashTable *hashtable;
  GtkWidget *label_position;
  GtkAdjustment *adjustment_zoom;
  GtkAdjustment *adjustment_cre;
  GtkAdjustment *adjustment_cim;
  GtkScale *scale_zoom;

  double zoom_level;
  gboolean pointer_in_image;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static void pixbuf_destroy_notify (guchar *pixels,
                                   gpointer data);
static void update_position_label (JseWindow *win,
                                   gdouble x,
                                   gdouble y);
static gboolean image_scroll_event_cb (GtkWidget *unused,
                                       GdkEventScroll *event,
                                       JseWindow *user_data);
static gboolean image_motion_notify_event_cb (JseWindow *win,
                                              GdkEventMotion *event);
static gboolean image_enter_notify_event_cb (JseWindow *win);
static gboolean image_leave_notify_event_cb (JseWindow *win);
static void update_button_c_label (JseWindow *win);

static void
jse_window_init (JseWindow *window)
{
  GtkAdjustment *adjustment_zoom;

  window->view_center_re = VIEW_CENTER_RE;
  window->view_center_im = VIEW_CENTER_IM;

  window->view_width = VIEW_WIDTH;
  window->view_height = VIEW_HEIGHT;

  window->pixbuf_width = PIXBUF_WIDTH;
  window->pixbuf_height = PIXBUF_HEIGHT;

  window->cre = C_RE;
  window->cim = C_IM;

  gtk_widget_init_template (GTK_WIDGET (window));

  window->hashtable = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);

  gtk_widget_add_events (GTK_WIDGET (window->eventbox),
                         GDK_SCROLL_MASK
                         /* TODO: Implement smooth scrolling support */
                         // | GDK_SMOOTH_SCROLL_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_ENTER_NOTIFY_MASK
                         | GDK_LEAVE_NOTIFY_MASK);

  jse_window_set_zoom_level (window, 0);

  adjustment_zoom = window->adjustment_zoom;
  gtk_adjustment_set_lower (adjustment_zoom, MIN_ZOOM_LEVEL);
  gtk_adjustment_set_upper (adjustment_zoom, MAX_ZOOM_LEVEL);

  /* TODO: Use GActions? */
  g_object_bind_property (window->button_c, "active",
                          window->button_c_popover, "visible",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  /* bind the zoom slider value to the zoom level of the image */
  g_object_bind_property (window, "zoom-level",
                          window->adjustment_zoom, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  g_object_bind_property (window, "cre",
                          window->adjustment_cre, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  g_object_bind_property (window, "cim",
                          window->adjustment_cim, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  window->pointer_in_image = FALSE;
  update_button_c_label (window);
}

static void
jse_window_finalize (GObject *object)
{
  JseWindow *window = JSE_WINDOW (object);

  g_debug ("jse_window_finalize (%p)", object);

  g_hash_table_destroy (window->hashtable);

  G_OBJECT_CLASS (jse_window_parent_class)->finalize (object);
}

static void
jse_window_set_property (GObject *object,
                         guint property_id,
                         const GValue *value,
                         GParamSpec *pspec)
{
  JseWindow *win = JSE_WINDOW (object);

  switch (property_id)
    {
    case PROP_ZOOM_LEVEL:
      jse_window_set_zoom_level (win, g_value_get_double (value));
      break;
    case PROP_CRE:
      jse_window_set_cre (win, g_value_get_double (value));
      break;
    case PROP_CIM:
      jse_window_set_cim (win, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
jse_window_get_property (GObject *object,
                         guint property_id,
                         GValue *value,
                         GParamSpec *pspec)
{
  JseWindow *win = JSE_WINDOW (object);

  switch (property_id)
    {
    case PROP_ZOOM_LEVEL:
      g_value_set_double (value, jse_window_get_zoom_level (win));
      break;
    case PROP_CRE:
      g_value_set_double (value, jse_window_get_cre (win));
      break;
    case PROP_CIM:
      g_value_set_double (value, jse_window_get_cim (win));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
jse_window_class_init (JseWindowClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = jse_window_finalize;
  gobject_class->set_property = jse_window_set_property;
  gobject_class->get_property = jse_window_get_property;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/jse/window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, eventbox);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, image);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, label_position);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_zoom);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c_label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c_popover);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_cre);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_cim);

  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_scroll_event_cb);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_motion_notify_event_cb);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_enter_notify_event_cb);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (class), image_leave_notify_event_cb);

  props[PROP_ZOOM_LEVEL] =
    g_param_spec_double ("zoom-level", "Zoom level", "Zoom level",
                         MIN_ZOOM_LEVEL, MAX_ZOOM_LEVEL, 0,
                         G_PARAM_READWRITE);
  props[PROP_CRE] =
    g_param_spec_double ("cre", "c_re", "real part of c",
                         -1.d, 1.d, 0.d,
                         G_PARAM_READWRITE);
  props[PROP_CIM] =
    g_param_spec_double ("cim", "c_im", "im part of c",
                         -1.d, 1.d, 0.d,
                         G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPS,
                                     props);
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
  GdkPixbuf *gdk_pixbuf;
  JuliaPixbuf *jp;
  gpointer orig_key;
  gpointer value;

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
      JuliaView jv_tmp = {win->view_center_re,
                          win->view_center_im,
                          win->view_width,
                          win->view_height,
                          zoom_level,
                          win->cre,
                          win->cim,
                          MAX_ITERATIONS};
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
update_image (JseWindow *win)
{
  GdkPixbuf *gdk_pixbuf = get_pixbuf_for_zoom_level (win, win->zoom_level);

  gtk_image_set_from_pixbuf (win->image, gdk_pixbuf);
  g_debug ("update_image: using GdkPixbuf %p", gdk_pixbuf);
}


void
jse_window_set_zoom_level (JseWindow *win,
                           gdouble zoom_level)
{
  g_debug ("zoom level: %f", zoom_level);

  win->zoom_level = zoom_level;
  update_image (win);

  g_object_notify_by_pspec (G_OBJECT (win), props[PROP_ZOOM_LEVEL]);
}


double
jse_window_get_zoom_level (JseWindow *win)
{
  return win->zoom_level;
}

void
jse_window_set_cre (JseWindow *win, double cre)
{
  if (cre == win->cre)
    return;

  win->cre = cre;

  /* don't blow up the memory! */
  g_hash_table_remove_all (win->hashtable);
  update_image (win);

  update_button_c_label (win);

  g_object_notify_by_pspec (G_OBJECT (win), props[PROP_CRE]);
}

double
jse_window_get_cre (JseWindow *win)
{
  return win->cre;
}

void
jse_window_set_cim (JseWindow *win, double cim)
{
  if (cim == win->cim)
    return;

  win->cim = cim;

  /* don't blow up the memory! */
  g_hash_table_remove_all (win->hashtable);
  update_image (win);

  update_button_c_label (win);

  g_object_notify_by_pspec (G_OBJECT (win), props[PROP_CIM]);
}

double
jse_window_get_cim (JseWindow *win)
{
  return win->cim;
}

static gboolean
image_scroll_event_cb (GtkWidget *unused,
                       GdkEventScroll *event,
                       JseWindow *win)
{
  g_debug ("scroll-event at (%3f, %3f)", event->x, event->y);
  gint zoom_level = win->zoom_level;

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

  jse_window_set_zoom_level (win, zoom_level);

  update_position_label (win, event->x, event->y);

  /* stop further handling of event */
  return TRUE;
}

static void
update_button_c_label (JseWindow *win)
{
  GString *text = g_string_new (NULL);
  g_string_printf (text, "c = %+.3f %+1.3fi", win->cre, win->cim);
  gtk_label_set_text (win->button_c_label, text->str);
  g_string_free (text, TRUE);
}

static void
update_position_label (JseWindow *win,
                       gdouble x,
                       gdouble y)
{
  /* TODO: Reduce calculations by saving values somewhere */
  int zoom_level = win->zoom_level;

  double width = win->view_width * pow(ZOOM_FACTOR, zoom_level);
  double height = win->view_height * pow(ZOOM_FACTOR, zoom_level);

  double pos_re = win->view_center_re + width * (x / win->pixbuf_width - 0.5);
  double pos_im = win->view_center_im + height * (0.5 - y / win->pixbuf_height);

  GString *text = g_string_new (NULL);
  if (win->pointer_in_image)
    g_string_printf (text, "pos: %+.15f %+1.15fi", pos_re, pos_im);
  else
    g_string_printf (text, "pos: (pointer outside of image)");

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

static gboolean
image_enter_notify_event_cb (JseWindow *win)
{
  win->pointer_in_image = TRUE;

  return TRUE;
}

static gboolean
image_leave_notify_event_cb (JseWindow *win)
{
  win->pointer_in_image = FALSE;
  update_position_label (win, 0, 0);

  return TRUE;
}

static void
pixbuf_destroy_notify (guchar *pixels,
                       gpointer data)
{
  g_debug ("Freeing pixel data at %p", pixels);
  g_free (pixels);
}

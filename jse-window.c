#include <gtk/gtk.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <clutter-gtk/clutter-gtk.h>

#include "jse-window.h"
#include "julia.h"

/* TODO: Move to a better place / remove hard-coding */
#define PIXBUF_HEIGHT 800
#define PIXBUF_WIDTH  800

#define MAX_ZOOM_LEVEL 200
#define MIN_ZOOM_LEVEL -10

#define MAX_ITERATIONS 5000
#define MIN_ITERATIONS 0
#define ITERATIONS 500

/* TODO: The user should be able to set these! */
/* The view of the complex plane to be displayed by the window */
#define VIEW_WIDTH 4
#define VIEW_HEIGHT 4
/* TODO: This MUST be (0, 0) for now since we assume symmetry */
#define VIEW_CENTER_RE 0
#define VIEW_CENTER_IM 0
/* c as in f(z) = z^2 + c */

/* #define C_RE -0.4 */
/* #define C_IM +0.6 */

#define C_RE -0.7269
#define C_IM +0.1889

/* #define C_RE -0.8 */
/* #define C_IM +0.156 */


enum {
  PROP_0,
  PROP_CRE,
  PROP_CIM,
  PROP_ITERATIONS,
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

  guint iterations;

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
  GtkAdjustment *adjustment_cre;
  GtkAdjustment *adjustment_cim;
  GtkAdjustment *adjustment_iterations;
  GtkAdjustment *adjustment_zoom;
  GtkScale *scale_zoom;
  GtkWidget *vbox;
  GtkWidget *clutter_embed;
  ClutterActor *stage;

  ClutterActor *current_actor;

  double zoom_level;
  gboolean pointer_in_stage;
};

/* final types don't need private data */
G_DEFINE_TYPE (JseWindow, jse_window, GTK_TYPE_APPLICATION_WINDOW);

static void update_position_label (JseWindow *win,
                                   gdouble x,
                                   gdouble y);
static gboolean stage_leave_notify_event_cb (ClutterActor *stage,
                                             ClutterEvent *event,
                                             JseWindow *win);
static gboolean stage_enter_notify_event_cb (ClutterActor *stage,
                                             ClutterEvent *event,
                                             JseWindow *win);
static gboolean stage_motion_notify_event_cb (ClutterActor *stage,
                                              ClutterMotionEvent *event,
                                              JseWindow *win);

static gboolean stage_scroll_event_cb (ClutterActor *actor,
                                       ClutterScrollEvent *event,
                                       JseWindow *win);

static void update_button_c_label (JseWindow *win);
static void blend_to_new_actor (JseWindow *win, gdouble old_zoom_level);

static void
jse_window_init (JseWindow *window)
{
  window->view_center_re = VIEW_CENTER_RE;
  window->view_center_im = VIEW_CENTER_IM;

  window->view_width = VIEW_WIDTH;
  window->view_height = VIEW_HEIGHT;

  window->pixbuf_width = PIXBUF_WIDTH;
  window->pixbuf_height = PIXBUF_HEIGHT;

  window->cre = C_RE;
  window->cim = C_IM;

  window->current_actor = NULL;

  window->zoom_level = 0;

  window->iterations = ITERATIONS;

  gtk_widget_init_template (GTK_WIDGET (window));

  window->hashtable = g_hash_table_new_full (g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_object_unref);


  gtk_adjustment_set_lower (window->adjustment_zoom, MIN_ZOOM_LEVEL);
  gtk_adjustment_set_upper (window->adjustment_zoom, MAX_ZOOM_LEVEL);

  gtk_adjustment_set_lower (window->adjustment_iterations, MIN_ITERATIONS);
  gtk_adjustment_set_upper (window->adjustment_iterations, MAX_ITERATIONS);

  g_object_bind_property (window, "cre",
                          window->adjustment_cre, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  g_object_bind_property (window, "cim",
                          window->adjustment_cim, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  g_object_bind_property (window, "iterations",
                          window->adjustment_iterations, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  g_object_bind_property (window, "zoom-level",
                          window->adjustment_zoom, "value",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  /* TODO: Use GActions? */
  g_object_bind_property (window->button_c, "active",
                          window->button_c_popover, "visible",
                          G_BINDING_BIDIRECTIONAL
                          | G_BINDING_SYNC_CREATE);

  /* ensure fixed size and aspect ratio */
  gtk_widget_set_size_request (window->clutter_embed, PIXBUF_WIDTH, PIXBUF_HEIGHT);
  gtk_widget_set_halign (window->clutter_embed, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (window->clutter_embed, GTK_ALIGN_CENTER);

  window->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (window->clutter_embed));
  window->pointer_in_stage = FALSE;

  g_signal_connect (window->stage,"scroll-event",
                    G_CALLBACK (stage_scroll_event_cb), window);
  g_signal_connect (window->stage,"leave-event",
                    G_CALLBACK (stage_leave_notify_event_cb), window);
  g_signal_connect (window->stage,"enter-event",
                    G_CALLBACK (stage_enter_notify_event_cb), window);
  g_signal_connect (window->stage,"motion-event",
                    G_CALLBACK (stage_motion_notify_event_cb), window);
}

static void jse_window_constructed (GObject *object)
{
  JseWindow *window = JSE_WINDOW (object);

  blend_to_new_actor (window, 0);

  update_position_label (window, 0, 0);
  update_button_c_label (window);

  G_OBJECT_CLASS (jse_window_parent_class)->constructed (object);
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
    case PROP_CRE:
      jse_window_set_cre (win, g_value_get_double (value));
      break;

    case PROP_CIM:
      jse_window_set_cim (win, g_value_get_double (value));
      break;

    case PROP_ITERATIONS:
      jse_window_set_iterations (win, g_value_get_uint (value));
      break;

    case PROP_ZOOM_LEVEL:
      jse_window_set_zoom_level (win, g_value_get_double (value));
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
    case PROP_CRE:
      g_value_set_double (value, jse_window_get_cre (win));
      break;

    case PROP_CIM:
      g_value_set_double (value, jse_window_get_cim (win));
      break;

    case PROP_ITERATIONS:
      g_value_set_uint (value, jse_window_get_iterations (win));
      break;

    case PROP_ZOOM_LEVEL:
      g_value_set_double (value, jse_window_get_zoom_level (win));
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

  gobject_class->constructed = jse_window_constructed;
  gobject_class->finalize = jse_window_finalize;
  gobject_class->set_property = jse_window_set_property;
  gobject_class->get_property = jse_window_get_property;

  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (class),
                                               "/org/gnome/jse/window.ui");
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, vbox);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, label_position);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_zoom);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c_label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, button_c_popover);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_cre);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_cim);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, adjustment_iterations);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (class), JseWindow, clutter_embed);

  props[PROP_CRE] =
    g_param_spec_double ("cre", "c_re", "real part of c",
                         -1.d, 1.d, 0.d,
                         G_PARAM_READWRITE);
  props[PROP_CIM] =
    g_param_spec_double ("cim", "c_im", "im part of c",
                         -1.d, 1.d, 0.d,
                         G_PARAM_READWRITE);

  props[PROP_ITERATIONS] =
    g_param_spec_uint ("iterations", "iterations", "The maximum number of Iterations",
                       MIN_ITERATIONS, MAX_ITERATIONS, ITERATIONS,
                       G_PARAM_READWRITE);

  props[PROP_ZOOM_LEVEL] =
    g_param_spec_double ("zoom-level", "Zoom level", "Zoom level",
                         MIN_ZOOM_LEVEL, MAX_ZOOM_LEVEL, 0,
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

ClutterActor *
get_clutter_actor_for_zoom_level (JseWindow *win, gint zoom_level)
{
  GHashTable *hashtable = win->hashtable;
  ClutterContent *image;
  ClutterActor *actor;
  JuliaPixbuf *jp;
  gpointer orig_key;
  gpointer value;

  if (g_hash_table_lookup_extended (hashtable,
                                    GINT_TO_POINTER (zoom_level),
                                    &orig_key,
                                    &value))
    {
      /* hash table hit */

      actor = (ClutterActor *) value;
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
                          win->iterations};
      julia_pixbuf_update_mt (jp, &jv_tmp);

      image = clutter_image_new ();
      clutter_image_set_data (CLUTTER_IMAGE (image),
                              jp->pixbuf,
                              COGL_PIXEL_FORMAT_RGB_888,
                              jp->pix_width,
                              jp->pix_height,
                              jp->pix_width * 3,
                              NULL);
      julia_pixbuf_destroy (jp);

      /* returns a floating reference */
      actor = clutter_actor_new();

      clutter_actor_set_content (actor, image);
      /* clutter_actor_set_content_scaling_filters (actor, */
      /*                                            CLUTTER_SCALING_FILTER_LINEAR, */
      /*                                            CLUTTER_SCALING_FILTER_LINEAR); */

      g_object_unref (image);

      g_hash_table_insert (hashtable,
                           GINT_TO_POINTER (zoom_level),
                           g_object_ref (actor));
    };

  /* (re)set defaults  */
  clutter_actor_set_size (actor, PIXBUF_WIDTH, PIXBUF_HEIGHT);
  clutter_actor_set_scale (actor, 1.0, 1.0);
  clutter_actor_set_pivot_point (actor, 0.5, 0.5);

  g_debug ("get_clutter_actor_for_zoom_level (%p, %d): %p", win, zoom_level, actor);

  return actor;
}

static void
on_transition_stopped_cb (ClutterActor *actor,
                          gchar *name,
                          gboolean is_finished,
                          gpointer data)
{
  g_debug ("on_transition_stopped (%p), transition: %s", actor, name);

  JseWindow *win = (JseWindow *) data;

  if (win->current_actor)
    {
      g_debug ("  removing old_actor %p", win->current_actor);
      clutter_actor_remove_child (win->stage, win->current_actor);
    }

  g_debug ("  setting current_actor %p", actor);
  win->current_actor = actor;

  g_signal_handlers_disconnect_by_func (actor, on_transition_stopped_cb, data);
}


/* TODO: So far I think this should probably work like this: When
   zooming (scroll wheel, pinching, ...) the current_actor should be
   scaled accordingly and after a small timeout (or when the touch
   stops) the new actor for the destination zoom level should be
   calculated and faded in over the old (scaled) actor. */
static void
blend_to_new_actor (JseWindow *win, gdouble old_zoom_level)
{
  ClutterActor *old_actor = win->current_actor;
  gdouble zoom_time;

  /* TODO: Cancel any already running blending, as the code is now,
     zooming again before a blend is done results in 'undefined'
     behavior. */

  if (old_actor && (old_zoom_level != win->zoom_level))
    {
      /* The time for the zoom animation should at least somewhat
         correspond to the zoom level distance. */
      zoom_time = 100 + 100 * log (1 + ABS (old_zoom_level - win->zoom_level));
      g_debug ("zoom_time: %f", zoom_time);
      clutter_actor_save_easing_state (old_actor);
      clutter_actor_set_easing_duration (old_actor, zoom_time);
      clutter_actor_set_scale (old_actor,
                               pow (ZOOM_FACTOR, (old_zoom_level - win->zoom_level)),
                               pow (ZOOM_FACTOR, (old_zoom_level - win->zoom_level)));
      clutter_actor_restore_easing_state (old_actor);
    }

  ClutterActor *new_actor = get_clutter_actor_for_zoom_level (win, win->zoom_level);

  clutter_actor_show (new_actor);
  clutter_actor_set_opacity (new_actor, 0);
  clutter_actor_add_child (win->stage, new_actor);

  clutter_actor_save_easing_state (new_actor);

  /* wait for a possible scale transition to finish before fading in
     the new actor */
  if (old_actor && (old_zoom_level != win->zoom_level))
    clutter_actor_set_easing_delay (new_actor, zoom_time);
  clutter_actor_set_easing_duration (new_actor, 200);
  clutter_actor_set_opacity (new_actor, 255);
  clutter_actor_restore_easing_state (new_actor);

  g_signal_connect (new_actor, "transition-stopped::opacity",
                    G_CALLBACK (on_transition_stopped_cb), win);

  /* FIXME: This should not be necessary, why isn't
     on_transition_stopped_cb called for the first actor that is
     created? */
  if (!old_actor)
    on_transition_stopped_cb (new_actor, "unused", TRUE, win);

  g_debug ("blend_to_new_actor: actors: %p -> %p", old_actor, new_actor);
}


void
jse_window_set_zoom_level (JseWindow *win,
                           gdouble zoom_level)
{
  g_debug ("zoom level: %f", zoom_level);

  gdouble old_zoom_level = win->zoom_level;

  win->zoom_level = zoom_level;
  blend_to_new_actor (win, old_zoom_level);

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

  g_hash_table_remove_all (win->hashtable);

  blend_to_new_actor (win, win->zoom_level);

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

  g_hash_table_remove_all (win->hashtable);
  blend_to_new_actor (win, win->zoom_level);

  update_button_c_label (win);

  g_object_notify_by_pspec (G_OBJECT (win), props[PROP_CIM]);
}

double
jse_window_get_cim (JseWindow *win)
{
  return win->cim;
}

void
jse_window_set_iterations (JseWindow *win,
                           guint iterations)
{
  if (win->iterations == iterations)
    return;

  win->iterations = iterations;
  g_hash_table_remove_all (win->hashtable);
  blend_to_new_actor (win, win->zoom_level);

  g_object_notify_by_pspec (G_OBJECT (win), props[PROP_ITERATIONS]);
}

guint
jse_window_get_iterations (JseWindow *win)
{
  return win->iterations;
}

static gboolean
stage_scroll_event_cb (ClutterActor *stage,
                       ClutterScrollEvent *event,
                       JseWindow *win)
{
  g_debug ("scroll-event at (%3f, %3f)", event->x, event->y);
  gint zoom_level = win->zoom_level;
  gdouble dx, dy;

  switch (event->direction)
    {
    case CLUTTER_SCROLL_DOWN:
      if (zoom_level < MAX_ZOOM_LEVEL)
        zoom_level++;
      else
        {
          g_debug ("Reached MAX_ZOOM_LEVEL of %d", MAX_ZOOM_LEVEL);
          return TRUE;
        }
      break;

    case CLUTTER_SCROLL_UP:
      if (zoom_level > MIN_ZOOM_LEVEL)
        zoom_level--;
      else
        {
          g_debug ("Reached MIN_ZOOM_LEVEL of %d", MIN_ZOOM_LEVEL);
          return TRUE;
        }
      break;

    case CLUTTER_SCROLL_SMOOTH:
      clutter_event_get_scroll_delta ((ClutterEvent *) event, &dx, &dy);
      g_debug ("CLUTTER_SCROLL_SMOOTH with dx=%f, dy=%f\n", dx, dy);
      /* TODO: take the actual values into account? */
      if (dy > 0)
        zoom_level++;
      else if (dy < 0)
        zoom_level--;
      else
        g_warning ("Smooth scroll delta of 0\n");
      break;

    default:
      g_message ("Unhandled scroll direction!");
      return TRUE;
    }

  jse_window_set_zoom_level (win, zoom_level);

  update_position_label (win, event->x, event->y);

  /* stop further handling of event */
  return CLUTTER_EVENT_STOP;
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
  if (win->pointer_in_stage)
    g_string_printf (text, "pos: %+.15f %+1.15fi", pos_re, pos_im);
  else
    g_string_printf (text, "pos: (pointer outside of image)");

  gtk_label_set_text (GTK_LABEL (win->label_position), text->str);
  g_string_free (text, TRUE);

}

static gboolean
stage_motion_notify_event_cb (ClutterActor *stage,
                              ClutterMotionEvent *event,
                              JseWindow *win)
{
  // g_debug ("motion-notify event at (%f, %f)", event->x, event->y);

  update_position_label (win, event->x, event->y);


  return TRUE;
}

static gboolean
stage_enter_notify_event_cb (ClutterActor *stage,
                             ClutterEvent *event,
                             JseWindow *win)
{
  win->pointer_in_stage = TRUE;

  return TRUE;
}

static gboolean
stage_leave_notify_event_cb (ClutterActor *stage,
                             ClutterEvent *event,
                             JseWindow *win)
{
  win->pointer_in_stage = FALSE;
  update_position_label (win, 0, 0);

  return TRUE;
}

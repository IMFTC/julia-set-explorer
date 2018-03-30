#include <gtk/gtk.h>
#include <clutter/clutter.h>
#include <clutter-gtk/clutter-gtk.h>
#include "jse-window.h"

static void
activate (GtkApplication *app,
          gpointer user_data)
{
  JseWindow *window;

  window = jse_window_new (app);

  gtk_widget_show_all (GTK_WIDGET (window));
}

int
main (int    argc,
      char **argv)
{

  /* FIXME: This is weird, I don't know yet where this really belongs */
  ClutterInitError error = gtk_clutter_init (&argc, &argv);
  if (error != CLUTTER_INIT_SUCCESS)
    {
      g_critical ("Failed to initialize Clutter");
      return 1;
    }

  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gnome.JuliaSetExplorer", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);

  return status;
}

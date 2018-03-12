#include <gtk/gtk.h>

#include "jse-window.h"

static void
activate (GtkApplication *app,
          gpointer user_data)
{
  JseWindow *window;

  window = jse_window_new (app);

  gtk_widget_show (GTK_WIDGET (window));
}

int
main (int    argc,
      char **argv)
{

  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gnome.JuliaSetExplorer", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);

  g_object_unref (app);

  return status;
}

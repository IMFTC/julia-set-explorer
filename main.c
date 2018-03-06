#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

enum {
	RED,
	GREEN,
	BLUE
} colors;


gboolean scroll_event_cb(GtkWidget *widget,
			 GdkEvent *event,
			 gpointer user_data)
{
	printf("Got scroll event!\n");
	/* stop further handling of event */
	return TRUE;
}

/* void update_pixbuf() */

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *eventbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	int rowstride;
	int iteration;
	int max_iteration;

	gtk_init(&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Julia Set Explorer");


	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				 FALSE,
				 8,
				 800,
				 800);

	g_print("gdk_pixbuf_get_byte_length: %d\n",
		gdk_pixbuf_get_byte_length (pixbuf));

	guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	/* for (int r = 0; r < 800; r++) { */
	/* 	for (int i = 0; i < 800 * 3; i++) { */
	/* 		pixels[r * rowstride + 3 * i] = i % 255; */
	/* 		pixels[r * rowstride + 3 * i + 1] = i % 255; */
	/* 		pixels[r * rowstride + 3 * i + 2] = i % 255; */
	/* 	} */
	/* } */

	double cx = -0.8;
	double cy = 0.156;

	/* double cx = 1.5; */
	/* double cy = 1.5; */

	double ax, ay, aa, bb, _2ab;
	guchar *pixel;

	max_iteration = 100;
	/* Update every pixel */
	for (int x = 0; x < 800; x++)
	{
		for (int y = 0; y < 800; y++)
		{
			/* get the re and im parts for complex number
			 * located at the current pixel */
			ax = -2.0 + 4. / 800. * x;
			ay = -2.0 + 4. / 800. * y;
			// printf("z = %f,%f\n", ax, ay);

			iteration = 0;
			while (iteration < max_iteration) {
				aa = ax * ax;
				bb = ay * ay;

				/* Leave loop if |z_n| > 2 */
				if (aa + bb > 4.)
					break;

				// printf("%f\n", z_re * z_re + z_im * z_im);
				_2ab = 2.0 * ax * ay;
				ax = aa - bb + cx;
				ay = _2ab + cy;

				iteration++;
			}
			// printf("%d iterations\n", iteration);

			pixel = pixels + (y * rowstride + 3 * x);

			if (iteration == max_iteration) {
				pixel[RED] = 0;
				pixel[GREEN] = 0;
				pixel[BLUE] = 0;
			} else {
				pixel[RED] =
					(guchar) (255 - (255. / max_iteration) * iteration);
				pixel[GREEN] =
					(guchar) (255 - (255. / max_iteration * iteration));
				pixel[BLUE] =
					(guchar) (255 - (255. / max_iteration * iteration));
			}
		}
	}

	image = gtk_image_new_from_pixbuf (pixbuf);

	eventbox = gtk_event_box_new ();
	gtk_event_box_set_above_child(GTK_EVENT_BOX (eventbox), TRUE);

	gtk_container_add (GTK_CONTAINER (eventbox), image);
	g_signal_connect(eventbox, "scroll-event", G_CALLBACK (scroll_event_cb), NULL);
	g_signal_connect(eventbox, "button-press-event", G_CALLBACK (scroll_event_cb), NULL);
	gtk_container_add (GTK_CONTAINER (window), eventbox);
	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main();
	return 0;

}

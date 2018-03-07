#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include <string.h>
#include <math.h>

/* Use even numbers to allow mirroring! */
#define PIXBUF_HEIGHT 800
#define PIXBUF_WIDTH  800

/* Scaling when zooming in one step */
#define ZOOM_FACTOR 0.9

/* TODO: Make values interactive */
#define	CX -.4
#define	CY .6
#define MAX_ITERATIONS 120

struct julia_settings {
	/* Rectangle in the complex plane that will be drawn */
	double center_x;
	double center_y;

	/* width and height for zoom level 0 */
	double default_width;
	double default_height;

	int zoom_level;

	/* Complex number c used in
	 * z_(n+1) = (z_n)^2 + c
	 */
	double cx;
	double cy;

	gint max_iterations;
};

struct julia_set {
	GtkImage *image;
	struct julia_settings *settings;
};

enum {
	RED = 0,
	GREEN,
	BLUE
};

static gboolean update_pixbuf (GdkPixbuf *pixbuf, struct julia_settings *settings);
static gboolean scroll_event_cb (GtkWidget *widget, GdkEventScroll *event, gpointer user_data);
static gboolean update_pixbuf (GdkPixbuf *pixbuf, struct julia_settings *settings);

static gboolean
button_press_event_cb (GtkWidget *widget,
		       GdkEventButton *event,
		       gpointer user_data)
{
	printf("GdkEventButton at (%f,%f)\n", event->x, event->y);

	return TRUE;
}

static gboolean
scroll_event_cb (GtkWidget *widget,
		 GdkEventScroll *event,
		 gpointer user_data)
{
	printf("Got GdkEventScroll at (%3f, %3f)\n", event->x, event->y);

	struct julia_set *julia = (struct julia_set*) user_data;
	GtkImage *image = julia->image;
	GdkPixbuf *pixbuf = gtk_image_get_pixbuf (image);
	struct julia_settings *settings = julia->settings;

	switch (event->direction) {
	case GDK_SCROLL_DOWN:
		settings->zoom_level++;
		break;
	case GDK_SCROLL_UP:
		settings->zoom_level--;
		break;
	default:
		g_message("Unhandled scroll direction!");
	}

	printf("Setting zoom level to %d\n", settings->zoom_level);
	update_pixbuf (pixbuf, settings);
	gtk_image_set_from_pixbuf (image, pixbuf);
	/* stop further handling of event */
	return TRUE;
}

/* Draws the julia set for the rectangle between (x_min, y_min) and
 * (x_max, y_max). */
static gboolean
update_pixbuf (GdkPixbuf *pixbuf,
	       struct julia_settings *settings)
{
	double center_x = settings->center_x;
	double center_y = settings->center_y;

	double default_width = settings->default_width;
	double default_height = settings->default_height;
	gint zoom_level = settings->zoom_level;

	gint max_iterations = settings->max_iterations;
	double cx = settings->cx;
	double cy = settings->cy;

	double ax, ay, aa, bb, _2ab;
	guchar *first_pixel, *last_pixel, *pixel, *pixel_mirrored;
	gsize pixbuf_size;
	gint pixbuf_width, pixbuf_height, iteration, row_offset, rowstride;

	/* Dimensions of the rectangle in the complex plane after
	 * applying the zoom factor. */
	double width = default_width * pow(ZOOM_FACTOR, zoom_level);
	double height = default_height * pow(ZOOM_FACTOR, zoom_level);

	char max_iter_color[3] = {0};
	double color_scale = 255. / max_iterations;

	pixbuf_width = gdk_pixbuf_get_width (pixbuf);
	pixbuf_height = gdk_pixbuf_get_height (pixbuf);
	pixbuf_size = gdk_pixbuf_get_byte_length (pixbuf);

	/* Address of first byte of  pixel (one pixel is 3 bytes). */
	first_pixel = gdk_pixbuf_get_pixels (pixbuf);
	/* Address of first byte of last pixel (one pixel is 3 bytes). */
	last_pixel = first_pixel + pixbuf_size - 3;

	/* Number of bytes per line. */
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	/* Update every pixel taking advantage of the symmetry by
	 * filling the left and right half at once. */
	for (int x = 0; x < pixbuf_width / 2; x++) {
		row_offset = 3 * x;

		for (int y = 0; y < pixbuf_height; y++) {
			/* Get the re and im parts for the complex number
			 * corresponding to the current pixel. */
			ax = center_x + width * ((double) x / (double) pixbuf_width - 0.5);
			ay = center_y + height * ((double) y / (double) pixbuf_height - 0.5);

			iteration = 0;
			while (iteration < max_iterations) {
				aa = ax * ax;
				bb = ay * ay;

				/* Leave loop if |z_n| > 2 */
				if (aa + bb > 4.)
					break;

				_2ab = 2.0 * ax * ay;
				ax = aa - bb + cx;
				ay = _2ab + cy;

				iteration++;
			}

			gint position = y * rowstride + row_offset;
			pixel = first_pixel + position;
			pixel_mirrored = last_pixel - position;

			if (iteration == max_iterations) {
				memcpy(pixel, max_iter_color, 3);
				memcpy(pixel_mirrored, max_iter_color, 3);
				/* DEBUG: use some hue for the mirrored part: */
				// pixel_mirrored[RED] = 200;
			} else {
				pixel[RED] = 255 - (color_scale * iteration);
				pixel[GREEN] = pixel[RED];
				pixel[BLUE] = pixel[RED];
				memcpy(pixel_mirrored, pixel, 3);
				/* DEBUG: use some hue for the mirrored part: */
				// pixel_mirrored[RED] = 200;
			}
		}
	}

	return TRUE;
}

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *eventbox;
	GtkWidget *image;
	GdkPixbuf *pixbuf;

	int max_iterations;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW (window), "Julia Set Explorer");

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
				 FALSE,
				 8,
				 PIXBUF_WIDTH,
				 PIXBUF_HEIGHT);
	image = gtk_image_new_from_pixbuf (pixbuf);

	g_print("gdk_pixbuf_get_byte_length: %lu\n",
		gdk_pixbuf_get_byte_length (pixbuf));

	max_iterations = MAX_ITERATIONS;

	/* Draw inital state to pixbuf */
	struct julia_settings *settings = g_malloc (sizeof (struct julia_settings));
	settings->center_x = 0;
	settings->center_y = 0;
	settings->default_width = 4;
	settings->default_height = 4;
	settings->zoom_level = 0;
	settings->cx = CX;
	settings->cy = CY;
	settings->max_iterations = max_iterations;

	struct julia_set *jset = g_malloc (sizeof (struct julia_set));
	jset->image = GTK_IMAGE (image);
	jset->settings = settings;

	update_pixbuf(pixbuf, settings);

	eventbox = gtk_event_box_new ();
	// gtk_event_box_set_above_child(GTK_EVENT_BOX (eventbox), TRUE);

	gtk_container_add (GTK_CONTAINER (eventbox), image);

	/* GtkEventBox does not catch scroll events by default, add
	 * them manually since we use them for zooming. */
	gtk_widget_add_events (eventbox, GDK_SCROLL_MASK);

	// g_signal_connect(window, "event", G_CALLBACK (scroll_event_cb), NULL);
	g_signal_connect(eventbox, "scroll-event", G_CALLBACK (scroll_event_cb), jset);
	g_signal_connect(eventbox, "button-press-event", G_CALLBACK (button_press_event_cb), jset);

	// g_signal_connect(eventbox, "key-press-event", G_CALLBACK (scroll_event_cb), NULL);

	gtk_container_add (GTK_CONTAINER (window), eventbox);
	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main();

	g_free (jset->settings);
	g_free (jset);

	return 0;
}

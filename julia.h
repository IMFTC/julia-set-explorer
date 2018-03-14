#ifndef JULIA_H
#define JULIA_H

/* Scaling factor for zooming in one level */
#define ZOOM_FACTOR 0.9

typedef struct _JuliaPixbuf JuliaPixbuf;
typedef struct _JuliaView JuliaView;

struct _JuliaView
{
  /* center of the view to be drawn */
  double centerx;               /* Re(center) */
  double centery;               /* Im(center) */

  /* dimensions of the view in the complex plane (not pixels in
     image!) at zoom level 0 */
  double default_width;
  double default_height;

  int zoom_level;

  /* Complex number c used in
   * z_(n+1) = (z_n)^2 + c
   */
  double cx;                    /* Re(c) */
  double cy;                    /* Im(c) */

  int max_iterations;

  /* TODO: color settings, function pointer? */
};

/* struct for wrapping the RGB pixel data in memory */
struct _JuliaPixbuf
{
  /* RGB pixel buffer */
  unsigned char *pixbuf;

  /* dimension in pixels */
  int pix_height;
  int pix_width;

  /* size of the pixbuf in bytes */
  int size;
};

JuliaPixbuf *
julia_pixbuf_new (int pix_height,
                  int pix_width);
void
julia_pixbuf_destroy (JuliaPixbuf *jsp);

void julia_pixbuf_update (JuliaPixbuf *pixbuf,
                          JuliaView *view);

void julia_pixbuf_update_mt (JuliaPixbuf *pixbuf, JuliaView *view);

JuliaView *julia_view_new (double centerx,
                           double centery,
                           double default_width,
                           double default_height,
                           int zoom_level,
                           double cx,
                           double cy,
                           int max_iterations);
void julia_view_destroy (JuliaView *jv);

#endif /* JULIA_H */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <math.h>
#include <pthread.h>

#include "julia.h"

enum {
  RED = 0,
  GREEN,
  BLUE
};

typedef struct _JuliaThreadArgs JuliaThreadArgs;

struct _JuliaThreadArgs
{
  JuliaPixbuf *pixbuf;
  JuliaView *view;
  int thread;
  int n_threads;
};

JuliaPixbuf *
julia_pixbuf_new (int pix_width, int pix_height)
{
  JuliaPixbuf *jp = calloc (1, sizeof (JuliaPixbuf));

  if (jp == NULL)
    {
      perror("julia_pixbuf_new");
      exit (EXIT_FAILURE);
    }

  jp->pix_width = pix_width;
  jp->pix_height = pix_height;
  jp->size = 3 *  pix_width * pix_height;

  /* allocate pixbuf holding 3 bytes (RGB) per pixel */
  jp->pixbuf = calloc (1, jp->size);

  if (jp->pixbuf == NULL)
    {
      perror("julia_pixbuf_new");
      exit (EXIT_FAILURE);
    }

  return jp;
}

void
julia_pixbuf_destroy (JuliaPixbuf *jpixbuf)
{
  if (jpixbuf->pixbuf)
    free (jpixbuf->pixbuf);
  free (jpixbuf);
}

void *
julia_pixbuf_update_partial (void *data)
{
  JuliaThreadArgs *args = (JuliaThreadArgs *) data;
  JuliaPixbuf *pixbuf = args->pixbuf;
  JuliaView *view = args->view;
  int thread = args->thread;
  int n_threads = args->n_threads;

  /* unpack settings */
  unsigned char *pixel = pixbuf->pixbuf;
  int pix_height = pixbuf->pix_height;
  int pix_width = pixbuf->pix_width;
  int pixbuf_size = pixbuf->size;

  double centerx = view->centerx;
  double centery = view->centery;
  double cx = view->cx;
  double cy = view->cy;
  int zoom_level = view->zoom_level;
  int max_iterations = view->max_iterations;

  /* Dimensions of the rectangle in the complex plane after
   * applying the zoom factor. */
  double width = view->default_width * pow(ZOOM_FACTOR, zoom_level);
  double height = view->default_height * pow(ZOOM_FACTOR, zoom_level);

  int iteration, position;
  double zx_tmp, zx, zy, zx_2, zy_2;
  double color_scale = 255. / max_iterations;
  char max_iter_color[3] = {0};
  unsigned char *first_pixel, *last_pixel, *mirrored_pixel;

  /* is mirroring at (0,0) is possible? */
  int use_mirroring = (centerx == 0) && (centery == 0);

  // printf("thread number: %d\n", thread);

  /* Address of first byte of  pixel (one pixel is 3 bytes). */
  first_pixel = pixel;
  /* Address of first byte of last pixel (one pixel is 3 bytes). */
  last_pixel = first_pixel + pixbuf_size - 3;

  /* Update every pixel taking advantage of the symmetry by filling
   * the left and right half at once. */
  int x_max = use_mirroring ? pix_width / 2 : pix_width;

  for (int x = thread; x < x_max; x += n_threads) {
    zx_tmp = centerx + width * ((double) x / (double) pix_width - 0.5);
    for (int y = 0; y < pix_height; y++) {
      /* Get the re and im parts for the complex number corresponding
       * to the current pixel. */
      zx = zx_tmp;
      zy = centery + height * (0.5 - (double) y / (double) pix_height);

      iteration = 0;
      while (iteration < max_iterations) {
        zx_2 = zx * zx;      /* Re(z)^2 */
        zy_2 = zy * zy;      /* Im(z)^2 */

        /* Leave loop if |z_n| > 2 */
        if (zx_2 + zy_2 > 4.)
          break;

        zy = 2.0 * zx * zy + cy;
        zx = zx_2 - zy_2 + cx;

        iteration++;
      }

      position = 3 * (y * pix_width + x);
      pixel = first_pixel + position;
      mirrored_pixel = last_pixel - position;

      /* TODO: Use a proper colormap */
      if (iteration == max_iterations) {
        memcpy(pixel, max_iter_color, 3);
        if (use_mirroring)
          memcpy(mirrored_pixel, max_iter_color, 3);
        /* DEBUG: use some hue for the mirrored/thread part: */
        /* pixel[thread % 3] = 20; */
        /* mirrored_pixel[thread % 3] = 100; */
      } else {
        pixel[RED] = 255 - (color_scale * iteration);
        pixel[GREEN] = pixel[RED];
        pixel[BLUE] = pixel[RED];
        if (use_mirroring)
          memcpy(mirrored_pixel, pixel, 3);
        /* DEBUG: use some hue for the mirrored part: */
        /* pixel[thread % 3] = 20; */
        /* mirrored_pixel[thread % 3] = 100; */
      }
    }
  }

  return NULL;
}

/* Updates @pixbuf according to @view using as many threads as there
   are online CPUs. */
void
julia_pixbuf_update_mt (JuliaPixbuf *pixbuf,
                        JuliaView *view)
{
  int n_threads = sysconf (_SC_NPROCESSORS_ONLN);

  pthread_t *thread_ids = calloc (n_threads, sizeof (pthread_t));
  JuliaThreadArgs *args = calloc (n_threads, sizeof (JuliaThreadArgs));

  for (int i = 0; i < n_threads; i++)
    {
      args[i].pixbuf = pixbuf;
      args[i].view = view;
      args[i].thread = i;
      args[i].n_threads = n_threads;

      pthread_create (&thread_ids[i], NULL, julia_pixbuf_update_partial, &args[i]);
    }

  for (int i = 0; i < n_threads; i++)
    {
      pthread_join(thread_ids[i] , NULL);
    }

  free (thread_ids);
  free (args);
}

/* Updates the content of @pixbuf according to @view. */
void
julia_pixbuf_update (JuliaPixbuf *pixbuf,
                     JuliaView *view)
{
  JuliaThreadArgs args = {pixbuf, view, 0, 1};
  julia_pixbuf_update_partial ((void *) &args);
}

JuliaView *
julia_view_new (double centerx,
                double centery,
                double default_width,
                double default_height,
                int zoom_level,
                double cx,
                double cy,
                int max_iterations)
{
  JuliaView *jv = calloc (1, sizeof (JuliaView));

  if (jv == NULL)
    {
      perror("julia_pixbuf_new");
      exit (EXIT_FAILURE);
    }

  jv->centerx = centerx;
  jv->centery = centery;
  jv->default_width = default_width;
  jv->default_height = default_height;
  jv->zoom_level = zoom_level;
  jv->cx = cx;
  jv->cy = cy;
  jv->max_iterations = max_iterations;

  return jv;
}

void
julia_view_destroy (JuliaView *jv)
{
  free (jv);
}

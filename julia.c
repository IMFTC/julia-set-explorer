#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <gmp.h>
#include <math.h>


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
  int thread = args->thread;
  int n_threads = args->n_threads;

  JuliaView *view = args->view;
  int zoom_level = view->zoom_level;
  int max_iterations = view->max_iterations;

  JuliaPixbuf *pixbuf = args->pixbuf;
  unsigned char *pixel = pixbuf->pixbuf;
  unsigned int pix_height = pixbuf->pix_height;
  unsigned int pix_width = pixbuf->pix_width;
  int pixbuf_size = pixbuf->size;

  /* TODO: view->centerx should itself be mpf_t and calculated using
     gmp functions, otherwise at zoom levels near 400 the whole stage
     returns the same (centerx, centery) values and navigation becomes
     impossible. */
  mpf_t _centerx;
  mpf_t _centery;
  mpf_init_set_d (_centerx, view->centerx);
  mpf_init_set_d (_centery, view->centery);

  mpf_t _cx;
  mpf_t _cy;
  mpf_init_set_d (_cx, view->cx);
  mpf_init_set_d (_cy, view->cy);

  mpf_t _default_width;
  mpf_t _default_height;
  mpf_init_set_d (_default_width, view->default_width);
  mpf_init_set_d (_default_height, view->default_height);
  /* Dimensions of the rectangle in the complex plane after
   * applying the zoom factor. */
  mpf_t _width;
  mpf_t _height;
  /* width = view->default_width * pow(ZOOM_FACTOR, zoom_level);   */
  mpf_init_set_d (_width, ZOOM_FACTOR);
  if (zoom_level < 0)
    {
    mpf_pow_ui (_width, _width, -zoom_level);
    mpf_ui_div (_width, 1, _width);
  }
  else
    mpf_pow_ui (_width, _width, zoom_level);
  mpf_mul (_width, _default_width, _width);
  /* height = view->default_height * pow(ZOOM_FACTOR, zoom_level); */
  mpf_init_set_d (_height, ZOOM_FACTOR);
  if (zoom_level < 0)
    {
    mpf_pow_ui (_height, _height, -zoom_level);
    mpf_ui_div (_height, 1, _height);
  }
  else
    mpf_pow_ui (_height, _height, zoom_level);
  mpf_mul (_height, _default_height, _height);

  // printf("_width : %f, height: %f\n", mpf_get_d (_width), mpf_get_d (_height));

  int iteration, position;
  /* double zx_tmp, zx, zy, zx_2, zy_2; */
  double color_scale = 255. / max_iterations;
  char max_iter_color[3] = {0};
  unsigned char *first_pixel, *last_pixel, *mirrored_pixel;

  /* is mirroring at (0,0) possible? */

  int use_mirroring = (!mpf_cmp_ui (_centerx, 0) && !mpf_cmp_ui (_centery, 0));

  // printf("thread number: %d\n", thread);

  /* Address of first byte of  pixel (one pixel is 3 bytes). */
  first_pixel = pixel;
  /* Address of first byte of last pixel (one pixel is 3 bytes). */
  last_pixel = first_pixel + pixbuf_size - 3;

  mpf_set_default_prec(64);
  mpf_t _zx, _zx_2, _zx_tmp, _zy, _zy_2, _tmp;
  mpf_inits (_zx, _zx_2, _zx_tmp, _zy, _zy_2, _tmp, NULL);
  mpf_t _0_5;
  mpf_init_set_d (_0_5, 0.5);

  // printf ("_centerx: %f, _centery: %f\n", mpf_get_d (_centerx), mpf_get_d (_centery));

  int x_max = use_mirroring ? pix_width / 2 : pix_width;

  for (int x = thread; x < x_max; x += n_threads) {
    /* zx_tmp = centerx + width * ((double) x / (double) pix_width - 0.5); */
    mpf_set_ui (_zx_tmp, pix_width);
    mpf_ui_div (_zx_tmp, x, _zx_tmp);
    mpf_sub (_zx_tmp, _zx_tmp, _0_5);
    mpf_mul (_zx_tmp, _width, _zx_tmp);
    mpf_add (_zx_tmp, _centerx, _zx_tmp);

    for (int y = 0; y < pix_height; y++) {
      // printf("x: %d, y: %d\n", x, y);
      /* Get the re and im parts for the complex number corresponding
       * to the current pixel. */

      /* zx = zx_tmp; */
      mpf_set (_zx, _zx_tmp);

      /* TODO: Save these to an array to avoid calculating _zy again
         for each row. */

      /* zy = centery + height * (0.5 - (double) y / (double) pix_height); */
      mpf_set_ui (_zy, pix_height);
      mpf_ui_div (_zy, y, _zy);
      mpf_sub (_zy, _0_5, _zy);
      mpf_mul (_zy, _height, _zy);
      mpf_add (_zy, _centery, _zy);
      /* printf ("zx: %f, zy: %f\n", mpf_get_d (_zx), mpf_get_d (_zy)); */

      iteration = 0;
      while (iteration < max_iterations) {
        /* zx_2 = _zx * _zx;      /\* Re(z)^2 *\/ */
        mpf_mul (_zx_2, _zx, _zx);
        /* zy_2 = zy * zy;      /\* Im(z)^2 *\/ */
        mpf_mul (_zy_2, _zy, _zy);

        /* Leave loop if |z_n| > 2 */
        /* if (zx_2 + zy_2 > 4.) */
        mpf_add (_tmp, _zx_2, _zy_2);
        if (mpf_cmp_ui (_tmp, 4) > 0)
          break;

        /* zy = 2.0 * zx * zy + cy; */
        /* Mind the order of operations here since we overwrite _zy
           with intermediate values! */
        mpf_mul (_zy, _zx, _zy);
        mpf_mul_ui (_zy, _zy, 2);
        mpf_add (_zy, _zy, _cy);

        /* zx = zx_2 - zy_2 + cx; */
        mpf_sub (_zx, _zx_2, _zy_2);
        mpf_add (_zx, _zx, _cx);

        iteration++;
      }

      position = 3 * (y * pix_width + x);
      pixel = first_pixel + position;
      mirrored_pixel = last_pixel - position;

      if (iteration == max_iterations) {
        memcpy(pixel, max_iter_color, 3);
        if (use_mirroring)
          memcpy(mirrored_pixel, max_iter_color, 3);
        /* DEBUG: use some hue for the mirrored/thread part: */
        /* pixel[thread % 3] = 20; */
        /* mirrored_pixel[thread % 3] = 100; */
      } else {
        /* TODO: Use a proper color struct and color map where we can
           copy R, G and B in one go using memcpy */
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

  mpf_clears (_centerx, _centery, _cx, _cy, _default_width, _default_height, _width, _height, NULL);
  mpf_clears (_zx, _zx_2, _zx_tmp, _zy, _zy_2, _0_5, _tmp, NULL);

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

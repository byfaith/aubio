/*
  Copyright (C) 2007-2009 Paul Brossier <piem@aubio.org>
                      and Amaury Hazan <ahazan@iua.upf.edu>

  This file is part of Aubio.

  Aubio is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Aubio is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Aubio.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "aubio_priv.h"
#include "fvec.h"
#include "cvec.h"
#include "spectral/fft.h"
#include "spectral/filterbank_mel.h"
#include "spectral/mfcc.h"

/** Internal structure for mfcc object */

struct aubio_mfcc_t_
{
  uint_t win_s;             /** grain length */
  uint_t samplerate;        /** sample rate (needed?) */
  uint_t n_filters;         /** number of  *filters */
  uint_t n_coefs;           /** number of coefficients (<= n_filters/2 +1) */
  aubio_filterbank_t *fb;   /** filter bank */
  fvec_t *in_dct;           /** input buffer for dct * [fb->n_filters] */
  fvec_t *dct_coeffs;       /** DCT transform n_filters * n_coeffs */
};


aubio_mfcc_t *
new_aubio_mfcc (uint_t win_s, uint_t samplerate, uint_t n_filters,
    uint_t n_coefs)
{

  /* allocate space for mfcc object */
  aubio_mfcc_t *mfcc = AUBIO_NEW (aubio_mfcc_t);

  uint_t i, j;

  mfcc->win_s = win_s;
  mfcc->samplerate = samplerate;
  mfcc->n_filters = n_filters;
  mfcc->n_coefs = n_coefs;

  /* filterbank allocation */
  mfcc->fb = new_aubio_filterbank (n_filters, mfcc->win_s);
  aubio_filterbank_set_mel_coeffs_slaney (mfcc->fb, samplerate);

  /* allocating buffers */
  mfcc->in_dct = new_fvec (n_filters, 1);

  mfcc->dct_coeffs = new_fvec (n_coefs, n_filters);

  /* compute DCT transform dct_coeffs[i][j] as
     cos ( j * (i+.5) * PI / n_filters ) */
  smpl_t scaling = 1. / SQRT (n_filters / 2.);
  for (i = 0; i < n_filters; i++) {
    for (j = 0; j < n_coefs; j++) {
      mfcc->dct_coeffs->data[i][j] =
          scaling * COS (j * (i + 0.5) * PI / n_filters);
    }
    mfcc->dct_coeffs->data[i][0] *= SQRT (2.) / 2.;
  }

  return mfcc;
};

void
del_aubio_mfcc (aubio_mfcc_t * mf)
{

  /* delete filterbank */
  del_aubio_filterbank (mf->fb);

  /* delete buffers */
  del_fvec (mf->in_dct);

  /* delete mfcc object */
  AUBIO_FREE (mf);
}


void
aubio_mfcc_do (aubio_mfcc_t * mf, cvec_t * in, fvec_t * out)
{
  uint_t i, j;

  /* compute filterbank */
  aubio_filterbank_do (mf->fb, in, mf->in_dct);

  /* zeros output */
  fvec_zeros(out);

  /* compute discrete cosine transform */
  for (i = 0; i < mf->n_filters; i++) {
    for (j = 0; j < mf->n_coefs; j++) {
      out->data[0][j] += mf->in_dct->data[0][i]
          * mf->dct_coeffs->data[i][j];
    }
  }

  return;
}

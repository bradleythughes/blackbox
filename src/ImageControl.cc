// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// ImageControl.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000 Brad Hughes (bhughes@tcac.net)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H

#include <X11/Xlib.h>

#include <algorithm>

#include "blackbox.hh"
#include "i18n.hh"
#include "BaseDisplay.hh"
#include "Image.hh"
#include "Texture.hh"

BImageControl::BImageControl(unsigned int screen_num, Bool _dither,
                             int _cpc, unsigned long cache_timeout,
                             unsigned long cmax)
  : screen_number(screen_num), dither(_dither),
    colors_per_channel(_cpc), cache_max(cmax),
    red_color_table(256), green_color_table(256), blue_color_table(256)
{
  BaseDisplay *display = BaseDisplay::instance();
  ScreenInfo *screeninfo = display->screenInfo(screen_number);

#ifdef    TIMEDCACHE
  if (cache_timeout) {
    timer = new BTimer(display, this);
    timer->setTimeout(cache_timeout);
    timer->start();
  } else {
    timer = (BTimer *) 0;
  }
#endif // TIMEDCACHE

  colors = (XColor *) 0;
  ncolors = 0;

  int count, bits_per_pixel;
  XPixmapFormatValues *pmv = XListPixmapFormats(*display, &count);
  if (pmv) {
    bits_per_pixel = 0;
    for (int i = 0; i < count; i++)
      if (pmv[i].depth == screeninfo->depth()) {
	bits_per_pixel = pmv[i].bits_per_pixel;
	break;
      }

    XFree(pmv);
  }

  if (bits_per_pixel == 0) bits_per_pixel = screeninfo->depth();
  if (bits_per_pixel >= 24) setDither(False);

  red_offset = green_offset = blue_offset = 0;

  Visual *v = screeninfo->visual();
  switch (v->c_class) {
  case TrueColor: {
    int i;

    // compute color tables
    unsigned long red_mask = v->red_mask,
                green_mask = v->green_mask,
                 blue_mask = v->blue_mask;

    while (! (red_mask & 1)) { red_offset++; red_mask >>= 1; }
    while (! (green_mask & 1)) { green_offset++; green_mask >>= 1; }
    while (! (blue_mask & 1)) { blue_offset++; blue_mask >>= 1; }

    red_bits = 255 / red_mask;
    green_bits = 255 / green_mask;
    blue_bits = 255 / blue_mask;

    for (i = 0; i < 256; i++) {
      red_color_table[i] = i / red_bits;
      green_color_table[i] = i / green_bits;
      blue_color_table[i] = i / blue_bits;
    }
    break;
  }

  case PseudoColor:
  case StaticColor: {
    ncolors = colors_per_channel * colors_per_channel * colors_per_channel;

    if (ncolors > (1 << screeninfo->depth())) {
      colors_per_channel = (1 << screeninfo->depth()) / 3;
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;
    }

    if (colors_per_channel < 2 || ncolors > (1 << screeninfo->depth())) {
      fprintf(stderr,
	      i18n(ImageSet, ImageInvalidColormapSize,
                   "BImageControl::BImageControl: invalid colormap size %d "
                   "(%d/%d/%d) - reducing"),
	      ncolors, colors_per_channel, colors_per_channel,
	      colors_per_channel);

      colors_per_channel = (1 << screeninfo->depth()) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors) {
      fprintf(stderr, i18n(ImageSet, ImageErrorAllocatingColormap,
                           "BImageControl::BImageControl: error allocating "
                           "colormap\n"));
      exit(1);
    }

    int i = 0, ii, p, r, g, b,

#ifdef ORDEREDPSEUDO
     bits = 256 / colors_per_channel;
#else // !ORDEREDPSEUDO
    bits = 255 / (colors_per_channel - 1);
#endif // ORDEREDPSEUDO

    red_bits = green_bits = blue_bits = bits;

    for (i = 0; i < 256; i++)
      red_color_table[i] = green_color_table[i] = blue_color_table[i] =
                           i / bits;

    for (r = 0, i = 0; r < colors_per_channel; r++)
      for (g = 0; g < colors_per_channel; g++)
	for (b = 0; b < colors_per_channel; b++, i++) {
	  colors[i].red = (r * 0xffff) / (colors_per_channel - 1);
	  colors[i].green = (g * 0xffff) / (colors_per_channel - 1);
	  colors[i].blue = (b * 0xffff) / (colors_per_channel - 1);;
	  colors[i].flags = DoRed|DoGreen|DoBlue;
	}

    for (i = 0; i < ncolors; i++) {
      if (! XAllocColor(*display, screeninfo->colormap(), &colors[i])) {
	fprintf(stderr, i18n(ImageSet, ImageColorAllocFail,
                             "couldn't alloc color %i %i %i\n"),
		colors[i].red, colors[i].green, colors[i].blue);
	colors[i].flags = 0;
      } else {
	colors[i].flags = DoRed|DoGreen|DoBlue;
      }
    }

    XColor icolors[256];
    int incolors = (((1 << screeninfo->depth()) > 256) ? 256 : (1 << screeninfo->depth()));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(*display, screeninfo->colormap(), icolors, incolors);
    for (i = 0; i < ncolors; i++) {
      if (! colors[i].flags) {
	unsigned long chk = 0xffffffff, pixel, close = 0;

	p = 2;
	while (p--) {
	  for (ii = 0; ii < incolors; ii++) {
	    r = (colors[i].red - icolors[i].red) >> 8;
	    g = (colors[i].green - icolors[i].green) >> 8;
	    b = (colors[i].blue - icolors[i].blue) >> 8;
	    pixel = (r * r) + (g * g) + (b * b);

	    if (pixel < chk) {
	      chk = pixel;
	      close = ii;
	    }

	    colors[i].red = icolors[close].red;
	    colors[i].green = icolors[close].green;
	    colors[i].blue = icolors[close].blue;

	    if (XAllocColor(*display, screeninfo->colormap(),
			    &colors[i])) {
	      colors[i].flags = DoRed|DoGreen|DoBlue;
	      break;
	    }
	  }
	}
      }
    }

    break;
  }

  case GrayScale:
  case StaticGray: {
    if (v->c_class == StaticGray) {
      ncolors = 1 << screeninfo->depth();
    } else {
      ncolors = colors_per_channel * colors_per_channel * colors_per_channel;

      if (ncolors > (1 << screeninfo->depth())) {
	colors_per_channel = (1 << screeninfo->depth()) / 3;
	ncolors =
	  colors_per_channel * colors_per_channel * colors_per_channel;
      }
    }

    if (colors_per_channel < 2 || ncolors > (1 << screeninfo->depth())) {
      fprintf(stderr,
              i18n(ImageSet, ImageInvalidColormapSize,
	           "BImageControl::BImageControl: invalid screeninfo->colormap() size %d "
                   "(%d/%d/%d) - reducing"),
	      ncolors, colors_per_channel, colors_per_channel,
	      colors_per_channel);

      colors_per_channel = (1 << screeninfo->depth()) / 3;
    }

    colors = new XColor[ncolors];
    if (! colors) {
      fprintf(stderr,
              i18n(ImageSet, ImageErrorAllocatingColormap,
                   "BImageControl::BImageControl: error allocating screeninfo->colormap()\n"));
      exit(1);
    }

    int i = 0, ii, p, bits = 255 / (colors_per_channel - 1);
    red_bits = green_bits = blue_bits = bits;

    for (i = 0; i < 256; i++)
      red_color_table[i] = green_color_table[i] = blue_color_table[i] =
                           i / bits;

    for (i = 0; i < ncolors; i++) {
      colors[i].red = (i * 0xffff) / (colors_per_channel - 1);
      colors[i].green = (i * 0xffff) / (colors_per_channel - 1);
      colors[i].blue = (i * 0xffff) / (colors_per_channel - 1);;
      colors[i].flags = DoRed|DoGreen|DoBlue;

      if (! XAllocColor(*display, screeninfo->colormap(),
			&colors[i])) {
	fprintf(stderr, i18n(ImageSet, ImageColorAllocFail,
			     "couldn't alloc color %i %i %i\n"),
		colors[i].red, colors[i].green, colors[i].blue);
	colors[i].flags = 0;
      } else {
	colors[i].flags = DoRed|DoGreen|DoBlue;
      }
    }

    XColor icolors[256];
    int incolors = (((1 << screeninfo->depth()) > 256) ? 256 :
		    (1 << screeninfo->depth()));

    for (i = 0; i < incolors; i++)
      icolors[i].pixel = i;

    XQueryColors(*display, screeninfo->colormap(), icolors, incolors);
    for (i = 0; i < ncolors; i++) {
      if (! colors[i].flags) {
	unsigned long chk = 0xffffffff, pixel, close = 0;

	p = 2;
	while (p--) {
	  for (ii = 0; ii < incolors; ii++) {
	    int r = (colors[i].red - icolors[i].red) >> 8;
	    int g = (colors[i].green - icolors[i].green) >> 8;
	    int b = (colors[i].blue - icolors[i].blue) >> 8;
	    pixel = (r * r) + (g * g) + (b * b);

	    if (pixel < chk) {
	      chk = pixel;
	      close = ii;
	    }

	    colors[i].red = icolors[close].red;
	    colors[i].green = icolors[close].green;
	    colors[i].blue = icolors[close].blue;

	    if (XAllocColor(*display, screeninfo->colormap(),
			    &colors[i])) {
	      colors[i].flags = DoRed|DoGreen|DoBlue;
	      break;
	    }
	  }
	}
      }
    }

    break;
  }

  default:
    fprintf(stderr,
            i18n(ImageSet, ImageUnsupVisual,
                 "BImageControl::BImageControl: unsupported visual %d\n"),
	    v->c_class);
    exit(1);
  }
}


BImageControl::~BImageControl(void) {
  BaseDisplay *display = BaseDisplay::instance();
  ScreenInfo *screeninfo = display->screenInfo(screen_number);

  if (colors) {
    unsigned long *pixels = new unsigned long [ncolors];

    int i;
    for (i = 0; i < ncolors; i++)
      *(pixels + i) = (*(colors + i)).pixel;

    XFreeColors(*display, screeninfo->colormap(), pixels, ncolors, 0);

    delete [] colors;
  }

  if (!cache.empty()) {
    //#ifdef DEBUG
    fprintf(stderr, i18n(ImageSet, ImagePixmapRelease,
		         "BImageContol::~BImageControl: pixmap cache - "
	                 "releasing %d pixmaps\n"), cache.size());
    //#endif
    CacheContainer::iterator it = cache.begin();
    const CacheContainer::iterator end = cache.end();
    for (; it != end; ++it) {
      XFreePixmap(*display, (*it).pixmap);
    }
  }
#ifdef    TIMEDCACHE
  if (timer) {
    timer->stop();
    delete timer;
  }
#endif // TIMEDCACHE
}


Pixmap BImageControl::searchCache(unsigned int width, unsigned int height,
                                  unsigned long texture,
                                  const BColor &c1, const BColor &c2) {
  if (cache.empty())
    return None;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage& tmp = *it;
    if ((tmp.width == width) && (tmp.height == height) &&
        (tmp.texture == texture) && (tmp.pixel1 == c1.pixel()))
      if (texture & BImage_Gradient) {
        if (tmp.pixel2 == c2.pixel()) {
          tmp.count++;
          return tmp.pixmap;
        }
      } else {
        tmp.count++;
        return tmp.pixmap;
      }
  }
  return None;
}


Pixmap BImageControl::renderImage(unsigned int width, unsigned int height,
                                  const BTexture &texture) {
  if (texture.texture() & BImage_ParentRelative) return ParentRelative;

  Pixmap pixmap = searchCache(width, height, texture.texture(),
			      texture.color(), texture.colorTo());
  if (pixmap) return pixmap;

  BImage image(width, height, screen_number);
  pixmap = image.render(texture);

  if (!pixmap) return None;

  CachedImage tmp;

  tmp.pixmap = pixmap;
  tmp.width = width;
  tmp.height = height;
  tmp.count = 1;
  tmp.texture = texture.texture();
  tmp.pixel1 = texture.color().pixel();

  if (texture.texture() & BImage_Gradient)
    tmp.pixel2 = texture.colorTo().pixel();
  else
    tmp.pixel2 = 0l;

  cache.push_back(tmp);

  if ((unsigned) cache.size() > cache_max) {
    //#ifdef    DEBUG
    fprintf(stderr, i18n(ImageSet, ImagePixmapCacheLarge,
			 "BImageControl::renderImage: cache is large, "
			 "forcing cleanout\n"));
    //#endif // DEBUG

    timeout();
  }

  return pixmap;
}


void BImageControl::removeImage(Pixmap pixmap) {
  if (!pixmap)
    return;

  CacheContainer::iterator it = cache.begin();
  const CacheContainer::iterator end = cache.end();
  for (; it != end; ++it) {
    CachedImage &tmp = *it;
    if (tmp.pixmap == pixmap && tmp.count > 0)
      tmp.count--;
  }

#ifdef    TIMEDCACHE
  if (! timer)
#endif // TIMEDCACHE
    timeout();
}


void BImageControl::colorTables(int *roff, int *goff, int *boff,
                                int *rbit, int *gbit, int *bbit) {
  if (roff) *roff = red_offset;
  if (goff) *goff = green_offset;
  if (boff) *boff = blue_offset;

  if (rbit) *rbit = red_bits;
  if (gbit) *gbit = green_bits;
  if (bbit) *bbit = blue_bits;
}


void BImageControl::getXColorTable(XColor **c, int *n) {
  if (c) *c = colors;
  if (n) *n = ncolors;
}


void BImageControl::setColorsPerChannel(unsigned int cpc) {
  if (cpc < 2) cpc = 2;
  if (cpc > 6) cpc = 6;
  colors_per_channel = cpc;
}


struct ZeroRefCheck {
  bool operator()(const BImageControl::CachedImage &image) const {
    return (image.count == 0);
  }
};

struct CacheCleaner {
  Display *display;
  CacheCleaner(Display *d): display(d) {}
  void operator()(const BImageControl::CachedImage& image) const {
    if (image.count == 0)
      XFreePixmap(display, image.pixmap);
  }
};

struct ZeroRefCount {
  bool operator()(const BImageControl::CachedImage& image) const {
    return (image.count == 0);
  }
};

void BImageControl::timeout(void) {
  std::for_each(cache.begin(), cache.end(),
                CacheCleaner(*BaseDisplay::instance()));
  cache.remove_if(ZeroRefCount());
}


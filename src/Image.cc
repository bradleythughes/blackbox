// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Bradley T Hughes <bhughes at trolltech.com>
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

#include <algorithm>
using std::max;
using std::min;

#include "i18n.hh"
#include "BaseDisplay.hh"
#include "GCCache.hh"
#include "Image.hh"
#include "Screen.hh"
#include "Texture.hh"
#include "blackbox.hh"

static inline unsigned long bsqrt(register unsigned long x) {
  if (x <= 0) return 0;
  if (x == 1) return 1;

  register unsigned long r = x >> 1;
  register unsigned long q;

  while (1) {
    q = x / r;
    if (q >= r) return r;
    r = (r + q) >> 1;
  }
}

BImage::GradientTable BImage::xtable;
BImage::GradientTable BImage::ytable;

BImage::BImage(unsigned int w, unsigned int h, unsigned int s)
  : width(w), height(h), screen_number(s)
{
  assert(width > 0);
  assert(height > 0);

  red   = new unsigned char[width * height];
  green = new unsigned char[width * height];
  blue  = new unsigned char[width * height];

  xtable.reserve(width * 3);
  ytable.reserve(height * 3);

  control =
    Blackbox::instance()->screen(screen_number)->getImageControl();
  control->colorTables(&red_offset, &green_offset, &blue_offset,
                       &red_bits, &green_bits, &blue_bits);
}


BImage::~BImage(void) {
  delete [] red;
  delete [] green;
  delete [] blue;
}


Pixmap BImage::render(const BTexture &texture) {
  if (texture.texture() & BImage_ParentRelative) return ParentRelative;
  if (texture.texture() & BImage_Solid)          return render_solid(texture);
  if (texture.texture() & BImage_Gradient)       return render_gradient(texture);
  return None;
}


Pixmap BImage::render_solid(const BTexture &texture)
{
  BaseDisplay *display = BaseDisplay::instance();
  ScreenInfo *screeninfo = display->screenInfo(screen_number);
  Pixmap pixmap = XCreatePixmap(*display, screeninfo->rootWindow(),
                                width, height, screeninfo->depth());
  if (pixmap == None) {
    fprintf(stderr, i18n(ImageSet, ImageErrorCreatingSolidPixmap,
                         "BImage::render_solid: error creating pixmap\n"));
    return None;
  }

  BGCCache *cache = BGCCache::instance();
  BGCCache::Item &gc = cache->find(texture.color()),
                &lgc = cache->find(texture.lightColor()),
                &sgc = cache->find(texture.shadowColor());

  XFillRectangle(*display, pixmap, gc.gc(), 0, 0, width, height);

#ifdef    INTERLACE
  if (texture.texture() & BImage_Interlaced) {
    BGCCache::Item &igc = cache->find(texture.colorTo());

    register unsigned int i = 0;
    for (; i < height; i += 2)
      XDrawLine(*display, pixmap, igc.gc(), 0, i, width, i);

    cache->release(igc);
  }
#endif // INTERLACE


  if (texture.texture() & BImage_Bevel1) {
    if (texture.texture() & BImage_Raised) {
      XDrawLine(*display, pixmap, sgc.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(*display, pixmap, sgc.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(*display, pixmap, lgc.gc(),
                0, 0, width - 1, 0);
      XDrawLine(*display, pixmap, lgc.gc(),
                0, height - 1, 0, 0);
    } else if (texture.texture() & BImage_Sunken) {
      XDrawLine(*display, pixmap, lgc.gc(),
                0, height - 1, width - 1, height - 1);
      XDrawLine(*display, pixmap, lgc.gc(),
                width - 1, height - 1, width - 1, 0);

      XDrawLine(*display, pixmap, sgc.gc(),
                0, 0, width - 1, 0);
      XDrawLine(*display, pixmap, sgc.gc(),
                0, height - 1, 0, 0);
    }
  } else if (texture.texture() & BImage_Bevel2) {
    if (texture.texture() & BImage_Raised) {
      XDrawLine(*display, pixmap, sgc.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(*display, pixmap, sgc.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(*display, pixmap, lgc.gc(),
                1, 1, width - 3, 1);
      XDrawLine(*display, pixmap, lgc.gc(),
                1, height - 3, 1, 1);
    } else if (texture.texture() & BImage_Sunken) {
      XDrawLine(*display, pixmap, lgc.gc(),
                1, height - 3, width - 3, height - 3);
      XDrawLine(*display, pixmap, lgc.gc(),
                width - 3, height - 3, width - 3, 1);

      XDrawLine(*display, pixmap, sgc.gc(),
                1, 1, width - 3, 1);
      XDrawLine(*display, pixmap, sgc.gc(),
                1, height - 3, 1, 1);
    }
  }

  cache->release(gc);
  cache->release(lgc);
  cache->release(sgc);

  return pixmap;
}


Pixmap BImage::render_gradient(const BTexture &texture) {
  BColor from, to;
  int inverted = 0;

#ifdef    INTERLACE
  interlaced = texture.texture() & BImage_Interlaced;
#endif // INTERLACE

  if (texture.texture() & BImage_Sunken) {
    from = texture.colorTo();
    to = texture.color();

    if (! (texture.texture() & BImage_Invert))
      inverted = 1;
  } else {
    from = texture.color();
    to = texture.colorTo();

    if (texture.texture() & BImage_Invert)
      inverted = 1;
  }

  if (texture.texture() & BImage_Diagonal)            dgradient(from, to);
  else if (texture.texture() & BImage_Elliptic)       egradient(from, to);
  else if (texture.texture() & BImage_Horizontal)     hgradient(from, to);
  else if (texture.texture() & BImage_Pyramid)        pgradient(from, to);
  else if (texture.texture() & BImage_Rectangle)      rgradient(from, to);
  else if (texture.texture() & BImage_Vertical)       vgradient(from, to);
  else if (texture.texture() & BImage_CrossDiagonal) cdgradient(from, to);
  else if (texture.texture() & BImage_PipeCross)     pcgradient(from, to);

  if (texture.texture() & BImage_Bevel1)      bevel1();
  else if (texture.texture() & BImage_Bevel2) bevel2();

  if (inverted) invert();

  return renderPixmap();
}


static const unsigned char dither4[4][4] = {
  {0, 4, 1, 5},
  {6, 2, 7, 3},
  {1, 5, 0, 4},
  {7, 3, 6, 2}
};


/*
 * Helper function for TrueColorDither and renderXImage
 *
 * This handles the proper setting of the image data based on the image depth
 * and the machine's byte ordering
 */
static inline void assignPixelData(unsigned int bit_depth,
                                   unsigned char **data,
                                   unsigned long pixel) {
  unsigned char *pixel_data = *data;
  switch (bit_depth) {
  case  8: //  8bpp
    *pixel_data++ = pixel;
    break;

  case 16: // 16bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    break;

  case 17: // 16bpp MSB
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 24: // 24bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    break;

  case 25: // 24bpp MSB
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;

  case 32: // 32bpp LSB
    *pixel_data++ = pixel;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 24;
    break;

  case 33: // 32bpp MSB
    *pixel_data++ = pixel >> 24;
    *pixel_data++ = pixel >> 16;
    *pixel_data++ = pixel >> 8;
    *pixel_data++ = pixel;
    break;
  }
  *data = pixel_data; // assign back so we don't lose our place
}


// algorithm: ordered dithering... many many thanks to rasterman
// (raster@rasterman.com) for telling me about this... portions of this
// code is based off of his code in Imlib
void BImage::TrueColorDither(unsigned int bit_depth, int bytes_per_line,
			     unsigned char *pixel_data) {

  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned char *ppixel_data = pixel_data;
  unsigned long pixel;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 0x3;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 0x3;
      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = control->red_color_table[r];
      g = control->green_color_table[g];
      b = control->blue_color_table[b];

      if ((dither4[dithy][dithx] < er) && (r < control->red_color_table[255])) r++;
      if ((dither4[dithy][dithx] < eg) && (g < control->green_color_table[255])) g++;
      if ((dither4[dithy][dithx] < eb) && (b < control->blue_color_table[255])) b++;

      pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);
      assignPixelData(bit_depth, &pixel_data, pixel);
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}

#ifdef ORDEREDPSEUDO
const static unsigned char dither8[8][8] = {
  { 0,  32, 8,  40, 2,  34, 10, 42},
  { 48, 16, 56, 24, 50, 18, 58, 26},
  { 12, 44, 4,  36, 14, 46, 6,  38},
  { 60, 28, 52, 20, 62, 30, 54, 22},
  { 3,  35, 11, 43, 1,  33, 9,  41},
  { 51, 19, 59, 27, 49, 17, 57, 25},
  { 15, 47, 7,  39, 13, 45, 5,  37},
  { 63, 31, 55, 23, 61, 29, 53, 21}
};

void BImage::OrderedPseudoColorDither(int bytes_per_line,
				      unsigned char *pixel_data) {
  unsigned int x, y, dithx, dithy, r, g, b, er, eg, eb, offset;
  unsigned long pixel;
  unsigned char *ppixel_data = pixel_data;

  for (y = 0, offset = 0; y < height; y++) {
    dithy = y & 7;

    for (x = 0; x < width; x++, offset++) {
      dithx = x & 7;

      r = red[offset];
      g = green[offset];
      b = blue[offset];

      er = r & (red_bits - 1);
      eg = g & (green_bits - 1);
      eb = b & (blue_bits - 1);

      r = control->red_color_table[r];
      g = control->green_color_table[g];
      b = control->blue_color_table[b];

      if ((dither8[dithy][dithx] < er) && (r < control->red_color_table[255])) r++;
      if ((dither8[dithy][dithx] < eg) && (g < control->green_color_table[255])) g++;
      if ((dither8[dithy][dithx] < eb) && (b < control->blue_color_table[255])) b++;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *(pixel_data++) = colors[pixel].pixel;
    }

    pixel_data = (ppixel_data += bytes_per_line);
  }
}
#endif

void BImage::PseudoColorDither(int bytes_per_line, unsigned char *pixel_data) {
  BImageControl *control =
    Blackbox::instance()->screen(screen_number)->getImageControl();

  short *terr,
    *rerr = new short[width + 2],
    *gerr = new short[width + 2],
    *berr = new short[width + 2],
    *nrerr = new short[width + 2],
    *ngerr = new short[width + 2],
    *nberr = new short[width + 2];

  int rr, gg, bb, rer, ger, ber;
  // ### TODO - fix me!
  int dd = 255 / control->getColorsPerChannel();
  unsigned int x, y, r, g, b, offset;
  unsigned long pixel;
  unsigned char *ppixel_data = pixel_data;

  for (x = 0; x < width; x++) {
    *(rerr + x) = *(red + x);
    *(gerr + x) = *(green + x);
    *(berr + x) = *(blue + x);
  }

  *(rerr + x) = *(gerr + x) = *(berr + x) = 0;

  for (y = 0, offset = 0; y < height; y++) {
    if (y < (height - 1)) {
      int i = offset + width;
      for (x = 0; x < width; x++, i++) {
	*(nrerr + x) = *(red + i);
	*(ngerr + x) = *(green + i);
	*(nberr + x) = *(blue + i);
      }

      *(nrerr + x) = *(red + (--i));
      *(ngerr + x) = *(green + i);
      *(nberr + x) = *(blue + i);
    }

    for (x = 0; x < width; x++) {
      rr = rerr[x];
      gg = gerr[x];
      bb = berr[x];

      if (rr > 255) rr = 255; else if (rr < 0) rr = 0;
      if (gg > 255) gg = 255; else if (gg < 0) gg = 0;
      if (bb > 255) bb = 255; else if (bb < 0) bb = 0;

      r = control->red_color_table[rr];
      g = control->green_color_table[gg];
      b = control->blue_color_table[bb];

      rer = rerr[x] - r*dd;
      ger = gerr[x] - g*dd;
      ber = berr[x] - b*dd;

      pixel = (r * cpccpc) + (g * cpc) + b;
      *pixel_data++ = colors[pixel].pixel;

      r = rer >> 1;
      g = ger >> 1;
      b = ber >> 1;
      rerr[x+1] += r;
      gerr[x+1] += g;
      berr[x+1] += b;
      nrerr[x] += r;
      ngerr[x] += g;
      nberr[x] += b;
    }

    offset += width;

    pixel_data = (ppixel_data += bytes_per_line);

    terr = rerr;
    rerr = nrerr;
    nrerr = terr;

    terr = gerr;
    gerr = ngerr;
    ngerr = terr;

    terr = berr;
    berr = nberr;
    nberr = terr;
  }

  delete [] rerr;
  delete [] gerr;
  delete [] berr;
  delete [] nrerr;
  delete [] ngerr;
  delete [] nberr;
}

XImage *BImage::renderXImage(void) {
  BaseDisplay *display = BaseDisplay::instance();
  ScreenInfo *screeninfo = display->screenInfo(screen_number);
  XImage *image =
    XCreateImage(*display, screeninfo->visual(), screeninfo->depth(),
                 ZPixmap, 0, 0, width, height, 32, 0);

  if (! image) {
    fprintf(stderr, i18n(ImageSet, ImageErrorCreatingXImage,
                         "BImage::renderXImage: error creating XImage\n"));
    return (XImage *) 0;
  }

  // insurance policy
  image->data = (char *) 0;

  unsigned char *d = new unsigned char[image->bytes_per_line * (height + 1)];

  unsigned int o = image->bits_per_pixel + ((image->byte_order == MSBFirst) ? 1 : 0);

  if (control->doDither() && width > 1 && height > 1) {
    switch (screeninfo->visual()->c_class) {
    case TrueColor:
      TrueColorDither(o, image->bytes_per_line, d);
      break;

    case StaticColor:
    case PseudoColor: {
#ifdef ORDEREDPSEUDO
      OrderedPseudoColorDither(image->bytes_per_line, d);
#else
      PseudoColorDither(image->bytes_per_line, d);
#endif
      break;
    }

    default:
      fprintf(stderr, i18n(ImageSet, ImageUnsupVisual,
                           "BImage::renderXImage: unsupported visual\n"));
      delete [] d;
      XDestroyImage(image);
      return (XImage *) 0;
    }
  } else {
    register unsigned int x, y, r, g, b, offset;

    unsigned char *pixel_data = d, *ppixel_data = d;
    unsigned long pixel;

    switch (screeninfo->visual()->c_class) {
    case StaticColor:
    case PseudoColor:
      for (y = 0, offset = 0; y < height; y++) {
        for (x = 0; x < width; x++, offset++) {
  	  r = control->red_color_table[red[offset]];
          g = control->green_color_table[green[offset]];
	  b = control->blue_color_table[blue[offset]];

	  pixel = (r * cpccpc) + (g * cpc) + b;
	  *pixel_data++ = colors[pixel].pixel;
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    case TrueColor:
      for (y = 0, offset = 0; y < height; y++) {
        for (x = 0; x < width; x++, offset++) {
	  r = control->red_color_table[red[offset]];
	  g = control->green_color_table[green[offset]];
	  b = control->blue_color_table[blue[offset]];

	  pixel = (r << red_offset) | (g << green_offset) | (b << blue_offset);
	  assignPixelData(o, &pixel_data, pixel);
        }

        pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    case StaticGray:
    case GrayScale:
      for (y = 0, offset = 0; y < height; y++) {
	for (x = 0; x < width; x++, offset++) {
	  r = control->red_color_table[red[offset]];
	  g = control->green_color_table[green[offset]];
	  b = control->blue_color_table[blue[offset]];

	  g = ((r * 30) + (g * 59) + (b * 11)) / 100;
	  *pixel_data++ = colors[g].pixel;
	}

	pixel_data = (ppixel_data += image->bytes_per_line);
      }

      break;

    default:
      fprintf(stderr, i18n(ImageSet, ImageUnsupVisual,
                           "BImage::renderXImage: unsupported visual\n"));
      delete [] d;
      XDestroyImage(image);
      return (XImage *) 0;
    }
  }

  image->data = (char *) d;
  return image;
}


Pixmap BImage::renderPixmap(void)
{
  BaseDisplay *display = BaseDisplay::instance();
  ScreenInfo *screeninfo = display->screenInfo(screen_number);
  Pixmap pixmap =
    XCreatePixmap(*display, screeninfo->rootWindow(), width, height,
                  screeninfo->depth());

  if (pixmap == None) {
    fprintf(stderr, i18n(ImageSet, ImageErrorCreatingPixmap,
                         "BImage::renderPixmap: error creating pixmap\n"));
    return None;
  }

  XImage *image = renderXImage();

  if (! image) {
    XFreePixmap(*display, pixmap);
    return None;
  } else if (! image->data) {
    XDestroyImage(image);
    XFreePixmap(*display, pixmap);
    return None;
  }

  XPutImage(*display, pixmap, DefaultGC(display->x11Display(), screen_number),
            image, 0, 0, 0, 0, width, height);

  if (image->data) {
    delete [] image->data;
    image->data = NULL;
  }

  XDestroyImage(image);

  return pixmap;
}


void BImage::bevel1(void) {
  if (width > 2 && height > 2) {
    unsigned char *pr = red, *pg = green, *pb = blue;

    register unsigned char r, g, b, rr ,gg ,bb;
    register unsigned int w = width, h = height - 1, wh = w * h;

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    r = *(pr + wh);
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *(pg + wh);
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *(pb + wh);
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *(pr + wh) = rr;
    *(pg + wh) = gg;
    *(pb + wh) = bb;

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      pr += width - 1;
      pg += width - 1;
      pb += width - 1;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;
    }

    r = *pr;
    rr = r + (r >> 1);
    if (rr < r) rr = ~0;
    g = *pg;
    gg = g + (g >> 1);
    if (gg < g) gg = ~0;
    b = *pb;
    bb = b + (b >> 1);
    if (bb < b) bb = ~0;

    *pr = rr;
    *pg = gg;
    *pb = bb;

    pr += width - 1;
    pg += width - 1;
    pb += width - 1;

    r = *pr;
    rr = (r >> 2) + (r >> 1);
    if (rr > r) rr = 0;
    g = *pg;
    gg = (g >> 2) + (g >> 1);
    if (gg > g) gg = 0;
    b = *pb;
    bb = (b >> 2) + (b >> 1);
    if (bb > b) bb = 0;

    *pr = rr;
    *pg = gg;
    *pb = bb;
  }
}


void BImage::bevel2(void) {
  if (width > 4 && height > 4) {
    unsigned char r, g, b, rr ,gg ,bb, *pr = red + width + 1,
      *pg = green + width + 1, *pb = blue + width + 1;
    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);

    while (--w) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *pr = rr;
      *pg = gg;
      *pb = bb;

      r = *(pr + wh);
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *(pg + wh);
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *(pb + wh);
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *((pr++) + wh) = rr;
      *((pg++) + wh) = gg;
      *((pb++) + wh) = bb;
    }

    pr = red + width;
    pg = green + width;
    pb = blue + width;

    while (--h) {
      r = *pr;
      rr = r + (r >> 1);
      if (rr < r) rr = ~0;
      g = *pg;
      gg = g + (g >> 1);
      if (gg < g) gg = ~0;
      b = *pb;
      bb = b + (b >> 1);
      if (bb < b) bb = ~0;

      *(++pr) = rr;
      *(++pg) = gg;
      *(++pb) = bb;

      pr += width - 3;
      pg += width - 3;
      pb += width - 3;

      r = *pr;
      rr = (r >> 2) + (r >> 1);
      if (rr > r) rr = 0;
      g = *pg;
      gg = (g >> 2) + (g >> 1);
      if (gg > g) gg = 0;
      b = *pb;
      bb = (b >> 2) + (b >> 1);
      if (bb > b) bb = 0;

      *(pr++) = rr;
      *(pg++) = gg;
      *(pb++) = bb;

      pr++; pg++; pb++;
    }
  }
}


void BImage::invert(void) {
  register unsigned int i, j, wh = (width * height) - 1;
  unsigned char tmp;

  for (i = 0, j = wh; j > i; j--, i++) {
    tmp = *(red + j);
    *(red + j) = *(red + i);
    *(red + i) = tmp;

    tmp = *(green + j);
    *(green + j) = *(green + i);
    *(green + i) = tmp;

    tmp = *(blue + j);
    *(blue + j) = *(blue + i);
    *(blue + i) = tmp;
  }
}


void BImage::dgradient(const BColor &from, const BColor &to) {
  // diagonal gradient code was written by Mike Cole <mike@mydot.com>
  // modified for interlacing by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2;
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  it = xtable.begin();
  for (x = 0; x < width; x++) {
    *it++ = (unsigned char) (xr);
    *it++ = (unsigned char) (xg);
    *it++ = (unsigned char) (xb);

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = ((unsigned char) yr);
    *it++ = ((unsigned char) yg);
    *it++ = ((unsigned char) yb);

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal dgradient
    for (y = 0; y < height; y++, yt+=3) {
      for (x = 0; x < width; x++) {
        *(pr++) = *xt++ + *(yt);
        *(pg++) = *xt++ + *(yt + 1);
        *(pb++) = *xt++ + *(yt + 2);
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE

}


void BImage::hgradient(const BColor &from, const BColor &to) {
  float drx, dgx, dbx,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;

  register unsigned int x, y;

  drx = (float) (to.red() - from.red());
  dgx = (float) (to.green() - from.green());
  dbx = (float) (to.blue() - from.blue());

  drx /= width;
  dgx /= width;
  dbx /= width;

#ifdef    INTERLACE
  if (interlaced && height > 2) {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (x = 0; x < width; x++, pr++, pg++, pb++) {
      channel = (unsigned char) xr;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pr = channel2;

      channel = (unsigned char) xg;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pg = channel2;

      channel = (unsigned char) xb;
      channel2 = (channel >> 1) + (channel >> 2);
      if (channel2 > channel) channel2 = 0;
      *pb = channel2;


      channel = (unsigned char) xr;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pr + width) = channel2;

      channel = (unsigned char) xg;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pg + width) = channel2;

      channel = (unsigned char) xb;
      channel2 = channel + (channel >> 3);
      if (channel2 < channel) channel2 = ~0;
      *(pb + width) = channel2;

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    pr += width;
    pg += width;
    pb += width;

    int offset;

    for (y = 2; y < height; y++, pr += width, pg += width, pb += width) {
      if (y & 1) offset = width; else offset = 0;

      memcpy(pr, (red + offset), width);
      memcpy(pg, (green + offset), width);
      memcpy(pb, (blue + offset), width);
    }
  } else {
#endif // INTERLACE

    // normal hgradient
    for (x = 0; x < width; x++) {
      *(pr++) = (unsigned char) (xr);
      *(pg++) = (unsigned char) (xg);
      *(pb++) = (unsigned char) (xb);

      xr += drx;
      xg += dgx;
      xb += dbx;
    }

    for (y = 1; y < height; y++, pr += width, pg += width, pb += width) {
      memcpy(pr, red, width);
      memcpy(pg, green, width);
      memcpy(pb, blue, width);
    }

#ifdef    INTERLACE
  }
#endif // INTERLACE

}


void BImage::vgradient(const BColor &from, const BColor &to) {
  float dry, dgy, dby,
    yr = (float) from.red(),
    yg = (float) from.green(),
    yb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;

  register unsigned int y;

  dry = (float) (to.red() - from.red());
  dgy = (float) (to.green() - from.green());
  dby = (float) (to.blue() - from.blue());

  dry /= height;
  dgy /= height;
  dby /= height;

#ifdef    INTERLACE
  if (interlaced) {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      if (y & 1) {
        channel = (unsigned char) yr;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pr, channel2, width);

        channel = (unsigned char) yg;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pg, channel2, width);

        channel = (unsigned char) yb;
        channel2 = (channel >> 1) + (channel >> 2);
        if (channel2 > channel) channel2 = 0;
        memset(pb, channel2, width);
      } else {
        channel = (unsigned char) yr;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pr, channel2, width);

        channel = (unsigned char) yg;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pg, channel2, width);

        channel = (unsigned char) yb;
        channel2 = channel + (channel >> 3);
        if (channel2 < channel) channel2 = ~0;
        memset(pb, channel2, width);
      }

      yr += dry;
      yg += dgy;
      yb += dby;
    }
  } else {
#endif // INTERLACE

    // normal vgradient
    for (y = 0; y < height; y++, pr += width, pg += width, pb += width) {
      memset(pr, (unsigned char) yr, width);
      memset(pg, (unsigned char) yg, width);
      memset(pb, (unsigned char) yb, width);

      yr += dry;
      yg += dgy;
      yb += dby;
    }

#ifdef    INTERLACE
  }
#endif // INTERLACE

}


void BImage::pgradient(const BColor &from, const BColor &to) {
  // pyramid gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float yr, yg, yb, drx, dgx, dbx, dry, dgy, dby,
    xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  it = xtable.begin();
  for (x = 0; x < width; x++) {
    *it++ = (unsigned char) ((xr < 0) ? -xr : xr);
    *it++ = (unsigned char) ((xg < 0) ? -xg : xg);
    *it++ = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = ((unsigned char) ((yr < 0) ? -yr : yr));
    *it++ = ((unsigned char) ((yg < 0) ? -yg : yg));
    *it++ = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal pgradient
    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * (*(xt++) + *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * (*(xt++) + *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * (*(xt++) + *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
}


void BImage::rgradient(const BColor &from, const BColor &to) {
  // rectangle gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(), tg = to.green(), tb = to.blue();
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  it = xtable.begin();
  for (x = 0; x < width; x++) {
    *it++ = (unsigned char) ((xr < 0) ? -xr : xr);
    *it++ = (unsigned char) ((xg < 0) ? -xg : xg);
    *it++ = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = ((unsigned char) ((yr < 0) ? -yr : yr));
    *it++ = ((unsigned char) ((yg < 0) ? -yg : yg));
    *it++ = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal rgradient
    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * max(*(xt++), *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * max(*(xt++), *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * max(*(xt++), *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
}


void BImage::egradient(const BColor &from, const BColor &to) {
  // elliptic gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr, yg, yb, xr, xg, xb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = (unsigned long) to.red(),
               tg = (unsigned long) to.green(),
               tb = (unsigned long) to.blue();
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -1 : 1;
  gsign = (dgx < 0) ? -1 : 1;
  bsign = (dbx < 0) ? -1 : 1;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  it = xtable.begin();
  for (x = 0; x < width; x++) {
    *it++ = (unsigned long) (xr * xr);
    *it++ = (unsigned long) (xg * xg);
    *it++ = (unsigned long) (xb * xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = (unsigned long) (yr * yr);
    *it++ = (unsigned long) (yg * yg);
    *it++ = (unsigned long) (yb * yb);

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal egradient
    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        *(pr++) = (unsigned char)
                  (tr - (rsign * bsqrt(*(xt++) + *(yt))));
        *(pg++) = (unsigned char)
                  (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
        *(pb++) = (unsigned char)
                  (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char)
                    (tr - (rsign * bsqrt(*(xt++) + *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char)
                    (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char)
                    (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char)
                    (tr - (rsign * bsqrt(*(xt++) + *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char)
                    (tg - (gsign * bsqrt(*(xt++) + *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char)
                    (tb - (bsign * bsqrt(*(xt++) + *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
}


void BImage::pcgradient(const BColor &from, const BColor &to) {
  // pipe cross gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, xr, xg, xb, yr, yg, yb;
  int rsign, gsign, bsign;
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int tr = to.red(),
               tg = to.green(),
               tb = to.blue();
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  rsign = (drx < 0) ? -2 : 2;
  gsign = (dgx < 0) ? -2 : 2;
  bsign = (dbx < 0) ? -2 : 2;

  xr = yr = (drx / 2);
  xg = yg = (dgx / 2);
  xb = yb = (dbx / 2);

  // Create X table
  drx /= width;
  dgx /= width;
  dbx /= width;

  it = xtable.begin();
  for (x = 0; x < width; x++) {
    *it++ = (unsigned char) ((xr < 0) ? -xr : xr);
    *it++ = (unsigned char) ((xg < 0) ? -xg : xg);
    *it++ = (unsigned char) ((xb < 0) ? -xb : xb);

    xr -= drx;
    xg -= dgx;
    xb -= dbx;
  }

  // Create Y table
  dry /= height;
  dgy /= height;
  dby /= height;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = ((unsigned char) ((yr < 0) ? -yr : yr));
    *it++ = ((unsigned char) ((yg < 0) ? -yg : yg));
    *it++ = ((unsigned char) ((yb < 0) ? -yb : yb));

    yr -= dry;
    yg -= dgy;
    yb -= dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal pcgradient
    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        *(pr++) = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
        *(pg++) = (unsigned char) (tg - (gsign * min(*(xt++), *(yt + 1))));
        *(pb++) = (unsigned char) (tb - (bsign * min(*(xt++), *(yt + 2))));
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (bsign * min(*(xt++), *(yt + 1))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (gsign * min(*(xt++), *(yt + 2))));
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = (unsigned char) (tr - (rsign * min(*(xt++), *(yt))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = (unsigned char) (tg - (gsign * min(*(xt++), *(yt + 1))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = (unsigned char) (tb - (bsign * min(*(xt++), *(yt + 2))));
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
}


void BImage::cdgradient(const BColor &from, const BColor &to) {
  // cross diagonal gradient -  based on original dgradient, written by
  // Mosfet (mosfet@kde.org)
  // adapted from kde sources for Blackbox by Brad Hughes

  float drx, dgx, dbx, dry, dgy, dby, yr = 0.0, yg = 0.0, yb = 0.0,
    xr = (float) from.red(),
    xg = (float) from.green(),
    xb = (float) from.blue();
  unsigned char *pr = red, *pg = green, *pb = blue;
  unsigned int w = width * 2, h = height * 2;
  GradientTable::reverse_iterator rt;
  GradientTable::iterator it;

  register unsigned int x, y;

  dry = drx = (float) (to.red() - from.red());
  dgy = dgx = (float) (to.green() - from.green());
  dby = dbx = (float) (to.blue() - from.blue());

  // Create X table
  drx /= w;
  dgx /= w;
  dbx /= w;

  rt = xtable.rbegin();
  for (x = 0; x < width; x++) {
    *rt++ = (unsigned char) xb;
    *rt++ = (unsigned char) xg;
    *rt++ = (unsigned char) xr;

    xr += drx;
    xg += dgx;
    xb += dbx;
  }

  // Create Y table
  dry /= h;
  dgy /= h;
  dby /= h;

  it = ytable.begin();
  for (y = 0; y < height; y++) {
    *it++ = (unsigned char) yr;
    *it++ = (unsigned char) yg;
    *it++ = (unsigned char) yb;

    yr += dry;
    yg += dgy;
    yb += dby;
  }

  // Combine tables to create gradient
  GradientTable::const_iterator xt = xtable.begin(), yt = ytable.begin();

#ifdef    INTERLACE
  if (! interlaced) {
#endif // INTERLACE

    // normal cdgradient
    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        *(pr++) = *(xt++) + *(yt);
        *(pg++) = *(xt++) + *(yt + 1);
        *(pb++) = *(xt++) + *(yt + 2);
      }
    }

#ifdef    INTERLACE
  } else {
    // faked interlacing effect
    unsigned char channel, channel2;

    for (y = 0; y < height; y++, yt += 3) {
      for (x = 0; x < width; x++) {
        if (y & 1) {
          channel = *(xt++) + *(yt);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = (channel >> 1) + (channel >> 2);
          if (channel2 > channel) channel2 = 0;
          *(pb++) = channel2;
        } else {
          channel = *(xt++) + *(yt);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pr++) = channel2;

          channel = *(xt++) + *(yt + 1);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pg++) = channel2;

          channel = *(xt++) + *(yt + 2);
          channel2 = channel + (channel >> 3);
          if (channel2 < channel) channel2 = ~0;
          *(pb++) = channel2;
        }
      }
    }
  }
#endif // INTERLACE
}

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Image.hh for Blackbox - an X11 Window manager
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

#ifndef   __Image_hh
#define   __Image_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>
#include <vector>

#include "BaseDisplay.hh"
#include "Color.hh"
#include "Timer.hh"

class BTexture;
class BImageControl;

class BImage {
private:
  unsigned int width, height;
  unsigned int screen_number;
  unsigned int depth;

#ifdef    INTERLACE
  Bool interlaced;
#endif // INTERLACE

  XColor *colors;

  typedef std::vector<unsigned int> GradientTable;
  static GradientTable xtable, ytable;

  BImageControl *control;
  int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits,
    ncolors, cpc, cpccpc;
  unsigned char *red, *green, *blue;

  void TrueColorDither(unsigned int bit_depth, int bytes_per_line,
		       unsigned char *pixel_data);
  void PseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#ifdef ORDEREDPSEUDO
  void OrderedPseudoColorDither(int bytes_per_line, unsigned char *pixel_data);
#endif

protected:
  Pixmap renderPixmap(void);

  XImage *renderXImage(void);

  void invert(void);
  void bevel1(void);
  void bevel2(void);
  void dgradient(const BColor &from, const BColor &to);
  void egradient(const BColor &from, const BColor &to);
  void hgradient(const BColor &from, const BColor &to);
  void pgradient(const BColor &from, const BColor &to);
  void rgradient(const BColor &from, const BColor &to);
  void vgradient(const BColor &from, const BColor &to);
  void cdgradient(const BColor &from, const BColor &to);
  void pcgradient(const BColor &from, const BColor &to);

public:
  BImage(unsigned int w, unsigned int h, unsigned int s);
  ~BImage(void);

  Pixmap render(const BTexture &texture);
  Pixmap render_solid(const BTexture &texture);
  Pixmap render_gradient(const BTexture &texture);
};


class BImageControl : public TimeoutHandler {
public:
  BImageControl(unsigned int screen_num, Bool _dither= False,
                int _cpc = 4, unsigned long cache_timeout = 300000l,
                unsigned long cmax = 200l);
  virtual ~BImageControl(void);

  inline const unsigned int screenNumber() const { return screen_number; }

  void setDither(Bool d) { dither = d; }
  inline const Bool doDither(void) const { return dither; }

  void setColorsPerChannel(unsigned int cpc);
  inline const unsigned int getColorsPerChannel(void) const
  { return colors_per_channel; }

  Pixmap renderImage(unsigned int width, unsigned int height,
                     const BTexture &texture);
  void removeImage(Pixmap pixmap);

  void colorTables(int *roff, int *goff, int *boff,
                   int *rbit, int *gbit, int *bbit);

  void getXColorTable(XColor **c, int *n);

  virtual void timeout(void);

  typedef std::vector<unsigned char> ColorTable;
  ColorTable red_color_table, green_color_table, blue_color_table;

protected:
  Pixmap searchCache(unsigned int width, unsigned int height,
                     unsigned long texture,
                     const BColor &c1, const BColor &c2);

private:
  unsigned int screen_number;
  Bool dither;
  unsigned int colors_per_channel;
  unsigned long cache_max;

  XColor *colors;
  int ncolors;

  int red_offset, green_offset, blue_offset, red_bits, green_bits, blue_bits;

  struct CachedImage {
    Pixmap pixmap;

    unsigned int count, width, height;
    unsigned long pixel1, pixel2, texture;
  };

  typedef std::list<CachedImage> CacheContainer;
  CacheContainer cache;

#ifdef    TIMEDCACHE
  BTimer *timer;
#endif // TIMEDCACHE
};

#endif // __Image_hh

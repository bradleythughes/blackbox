// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// GCCache.cc for Blackbox - an X11 Window manager
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

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "GCCache.hh"
#include "BaseDisplay.hh"
#include "Color.hh"

#include <stdio.h>


void BGCCache::Context::set(const BColor &_color,
                            const XFontStruct * const _font,
                            const int _function,
                            const int _subwindow) {
  XGCValues gcv;
  pixel = gcv.foreground = _color.pixel();
  function = gcv.function = _function;
  subwindow = gcv.subwindow_mode = _subwindow;
  unsigned long mask = GCForeground | GCFunction | GCSubwindowMode;

  if (_font) {
    fontid = gcv.font = _font->fid;
    mask |= GCFont;
  } else {
    fontid = 0;
  }

  XChangeGC(display->getXDisplay(), gc, mask, &gcv);
}

void BGCCache::Context::set(const XFontStruct * const _font) {
  if (! _font) {
    fontid = 0;
    return;
  }

  XGCValues gcv;
  fontid = gcv.font = _font->fid;
  XChangeGC(display->getXDisplay(), gc, GCFont, &gcv);
}


BGCCache::BGCCache(BaseDisplay *_display)
  : display(_display), context_count(128), cache_size(16), cache_buckets(8)
{
  contexts = new Context[ context_count ](display);

  cache = new Item*[ cache_size * cache_buckets ];
  int i, b, s;
  for (i = 0, s = 0; i < cache_size; i++) {
    for (b = 0; b < cache_buckets; b++)
      cache[ s++ ] = new Item;
  }
}

BGCCache::~BGCCache()
{
  int i, b, s;
  for (i = 0, s = 0; i < cache_size; i++) {
    for (b = 0; b < cache_buckets; b++)
      delete cache[ s++ ];
  }
  delete [] cache;
  delete [] contexts;
}

BGCCache::Context *BGCCache::nextContext(unsigned int scr)
{
  Window hd = display->getScreenInfo(scr)->getRootWindow();

  register Context *c = contexts;
  register int i = context_count;
  while (i--) {
    if (! c->gc) {
      c->gc = XCreateGC(display->getXDisplay(), hd, 0, 0);
      c->used = false;
      c->screen = scr;
    }
    if (! c->used && c->screen == scr) {
      c->used = true;
      return c;
    }
    c++;
  }

  fprintf(stderr, "BGCCache: context fault!\n");
  exit(1);
  return 0; // shutup
}

void BGCCache::release(Context *ctx)
{
  ctx->used = false;
}

BGCCache::Item *BGCCache::find(const BColor &_color,
                               const XFontStruct * const _font,
                               int _function, int _subwindow)
{
  unsigned long pixel = _color.pixel();
  unsigned long fontid = _font ? _font->fid : 0;
  unsigned int screen = _color.screen();
  int key = _color.red() ^ _color.green() ^ _color.blue();
  int k = (key % cache_size) * cache_buckets;
  Item *c = cache[ k ], *prev = 0;

#define NOMATCH (c->ctx && (c->ctx->pixel != pixel || \
                              c->ctx->function != _function || \
                              c->ctx->subwindow != _subwindow || \
                              c->ctx->screen != screen))

  if (NOMATCH) {
    prev = c;
    c = cache[ ++k ];
    if (NOMATCH) {
      prev = c;
      c = cache[ ++k ];
      if (NOMATCH) {
        prev = c;
        c = cache[ ++k ];
        if (NOMATCH) {
          prev = c;
          c = cache[ ++k ];
          if (NOMATCH) {
            prev = c;
            c = cache[ ++k ];
            if (NOMATCH) {
              prev = c;
              c = cache[ ++k ];
              if (NOMATCH) {
                prev = c;
                c = cache[ ++k ];
                if (NOMATCH) {
                  if (c->count == 0 && c->ctx->screen == screen) {
                    // use this cache item
                    c->ctx->set(_color, _font, _function, _subwindow);
                    c->ctx->used = true;
                    c->count = 1;
                    c->hits = 1;
                    return c;
                  } else {
                    // cache fault!
                    fprintf(stderr, "BGCCache: cache fault\n");
                    abort();
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (c->ctx) {
    // reuse existing context
    if (fontid && fontid != c->ctx->fontid)
      c->ctx->set(_font);
    c->count++;
    c->hits++;
    if (prev && c->hits > prev->hits) {
      cache[ k     ] = prev;
      cache[ k - 1 ] = c;
    }
    return c;
  }

  c->ctx = nextContext(screen);
  c->ctx->set(_color, _font, _function, _subwindow);
  c->ctx->used = true;
  c->count = 1;
  c->hits = 1;
  return c;
}

void BGCCache::release(Item *_item)
{
  _item->count--;
}

void BGCCache::purge()
{
  // fprintf(stderr, "BGCCache::purge\n");
  int i, b, s;
  for (i = 0, s = 0; i < cache_size; i++) {
    for (b = 0; b < cache_buckets; b++) {
      Item *d = cache[ s++ ];

      if (d->ctx) {
        // fprintf(stderr, "  cache %4d,%2d=%2d: count %4d",
        // i, b, s - 1, d->count);
        // fprintf(stderr, " pixel %6lx f %d sub %d screen %d",
        // d->ctx->pixel, d->ctx->function,
        // d->ctx->subwindow, d->ctx->screen);

        if (d->count == 0) {
          release(d->ctx);
          d->ctx = 0;
          // fprintf(stderr, " released\n");
          // } else {
          // fprintf(stderr, "\n");
        }
      }
    }
  }
}

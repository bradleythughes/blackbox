// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Brad Hughes <bhughes at trolltech.com>
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

extern "C" {
#ifdef HAVE_STDLIB_H
#  include <stdlib.h>
#endif // HAVE_STDLIB_H

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    HAVE_CTYPE_H
#  include <ctype.h>
#endif // HAVE_CTYPE_H
}

#include "../src/i18n.hh"
#include "../src/GCCache.hh"
#include "../src/Texture.hh"
#include "../src/Util.hh"
#include "bsetroot.hh"


I18n i18n;

bsetroot::bsetroot(int argc, char **argv, char *dpy_name)
  : BaseDisplay(argv[0], dpy_name) {

  bool mod = False, sol = False, grd = False;
  int mod_x = 0, mod_y = 0;

  for (int i = 1; i < argc; i++) {
    if (! strcmp("-help", argv[i])) {
      usage();
    } else if ((! strcmp("-fg", argv[i])) ||
               (! strcmp("-foreground", argv[i])) ||
               (! strcmp("-from", argv[i]))) {
      if ((++i) >= argc) usage(1);

      fore = argv[i];
    } else if ((! strcmp("-bg", argv[i])) ||
               (! strcmp("-background", argv[i])) ||
               (! strcmp("-to", argv[i]))) {
      if ((++i) >= argc) usage(1);

      back = argv[i];
    } else if (! strcmp("-solid", argv[i])) {
      if ((++i) >= argc) usage(1);

      fore = argv[i];
      sol = True;
    } else if (! strcmp("-mod", argv[i])) {
      if ((++i) >= argc) usage();

      mod_x = atoi(argv[i]);

      if ((++i) >= argc) usage();

      mod_y = atoi(argv[i]);

      if (mod_x < 1) mod_x = 1;
      if (mod_y < 1) mod_y = 1;

      mod = True;
    } else if (! strcmp("-gradient", argv[i])) {
      if ((++i) >= argc) usage();

      grad = argv[i];
      grd = True;
    } else if (! strcmp("-display", argv[i])) {
      // -display passed through tests ealier... we just skip it now
      i++;
    } else {
      usage();
    }
  }

  if ((mod + sol + grd) != True) {
    fprintf(stderr,
	    i18n(bsetrootSet, bsetrootMustSpecify,
	         "%s: error: must specify one of: -solid, -mod, -gradient\n"),
	    getApplicationName());

    usage(2);
  }

  img_ctrl = new BImageControl*[getNumberOfScreens()];
  for (unsigned int s = 0; s < getNumberOfScreens(); ++s)
    img_ctrl[s] = new BImageControl(this, getScreenInfo(s), True);

  if (sol && ! fore.empty())
    solid();
  else if (mod && mod_x && mod_y && ! (fore.empty() || back.empty()))
    modula(mod_x, mod_y);
  else if (grd && ! (grad.empty() || fore.empty() || back.empty()))
    gradient();
  else usage();
}


bsetroot::~bsetroot(void) {
  XSetCloseDownMode(getXDisplay(), RetainPermanent);

  XKillClient(getXDisplay(), AllTemporary);

  std::for_each(img_ctrl, img_ctrl + getNumberOfScreens(), PointerAssassin());

  delete [] img_ctrl;
}


// adapted from wmsetbg
void bsetroot::setPixmapProperty(int screen, Pixmap pixmap) {
  static Atom rootpmap_id = None, esetroot_id = None;
  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data;
  const ScreenInfo *screen_info = getScreenInfo(screen);

  if (rootpmap_id == None) {
    rootpmap_id = XInternAtom(getXDisplay(), "_XROOTPMAP_ID", False);
    esetroot_id = XInternAtom(getXDisplay(), "ESETROOT_PMAP_ID", False);
  }

  XGrabServer(getXDisplay());

  /* Clear out the old pixmap */
  XGetWindowProperty(getXDisplay(), screen_info->getRootWindow(),
		     rootpmap_id, 0L, 1L, False, AnyPropertyType,
		     &type, &format, &length, &after, &data);

  if ((type == XA_PIXMAP) && (format == 32) && (length == 1)) {
    unsigned char* data_esetroot = 0;
    XGetWindowProperty(getXDisplay(), screen_info->getRootWindow(),
                       esetroot_id, 0L, 1L, False, AnyPropertyType,
                       &type, &format, &length, &after, &data_esetroot);
    if (data && data_esetroot && *((Pixmap *) data)) {
      XKillClient(getXDisplay(), *((Pixmap *) data));
      XSync(getXDisplay(), False);
      XFree(data_esetroot);
    }
    XFree(data);
  }

  if (pixmap) {
    XChangeProperty(getXDisplay(), screen_info->getRootWindow(),
		    rootpmap_id, XA_PIXMAP, 32, PropModeReplace,
		    (unsigned char *) &pixmap, 1);
    XChangeProperty(getXDisplay(), screen_info->getRootWindow(),
		    esetroot_id, XA_PIXMAP, 32, PropModeReplace,
		    (unsigned char *) &pixmap, 1);
  } else {
    XDeleteProperty(getXDisplay(), screen_info->getRootWindow(),
		    rootpmap_id);
    XDeleteProperty(getXDisplay(), screen_info->getRootWindow(),
		    esetroot_id);
  }

  XUngrabServer(getXDisplay());
  XFlush(getXDisplay());
}


// adapted from wmsetbg
Pixmap bsetroot::duplicatePixmap(int screen, Pixmap pixmap,
				 int width, int height) {
  XSync(getXDisplay(), False);

  Pixmap copyP = XCreatePixmap(getXDisplay(),
			       getScreenInfo(screen)->getRootWindow(),
			       width, height,
			       DefaultDepth(getXDisplay(), screen));
  XCopyArea(getXDisplay(), pixmap, copyP, DefaultGC(getXDisplay(), screen),
	    0, 0, width, height, 0, 0);
  XSync(getXDisplay(), False);

  return copyP;
}


void bsetroot::solid(void) {
  for (unsigned int screen = 0; screen < getNumberOfScreens(); screen++) {
    BColor c(fore, this, screen);
    const ScreenInfo *screen_info = getScreenInfo(screen);

    XSetWindowBackground(getXDisplay(), screen_info->getRootWindow(),
                         c.pixel());
    XClearWindow(getXDisplay(), screen_info->getRootWindow());

    Pixmap pixmap = XCreatePixmap(getXDisplay(),
				  screen_info->getRootWindow(),
				  8, 8, DefaultDepth(getXDisplay(), screen));
    BPen pen(c);
    XFillRectangle(getXDisplay(), pixmap, pen.gc(), 0, 0, 8, 8);

    setPixmapProperty(screen, duplicatePixmap(screen, pixmap, 8, 8));

    XFreePixmap(getXDisplay(), pixmap);
  }
}


void bsetroot::modula(int x, int y) {
  char data[32];
  long pattern;

  unsigned int screen, i;

  for (pattern = 0, screen = 0; screen < getNumberOfScreens(); screen++) {
    for (i = 0; i < 16; i++) {
      pattern <<= 1;
      if ((i % x) == 0)
        pattern |= 0x0001;
    }

    for (i = 0; i < 16; i++) {
      if ((i %  y) == 0) {
        data[(i * 2)] = static_cast<char>(0xff);
        data[(i * 2) + 1] = static_cast<char>(0xff);
      } else {
        data[(i * 2)] = pattern & 0xff;
        data[(i * 2) + 1] = (pattern >> 8) & 0xff;
      }
    }

    BColor f(fore, this, screen), b(back, this, screen);
    GC gc;
    Pixmap bitmap;
    const ScreenInfo *screen_info = getScreenInfo(screen);

    bitmap =
      XCreateBitmapFromData(getXDisplay(),
                            screen_info->getRootWindow(), data,
                            16, 16);

    XGCValues gcv;
    gcv.foreground = f.pixel();
    gcv.background = b.pixel();

    gc = XCreateGC(getXDisplay(), screen_info->getRootWindow(),
                   GCForeground | GCBackground, &gcv);

    Pixmap pixmap = XCreatePixmap(getXDisplay(),
				  screen_info->getRootWindow(),
				  16, 16, screen_info->getDepth());

    XCopyPlane(getXDisplay(), bitmap, pixmap, gc,
               0, 0, 16, 16, 0, 0, 1l);
    XSetWindowBackgroundPixmap(getXDisplay(),
                               screen_info->getRootWindow(),
                               pixmap);
    XClearWindow(getXDisplay(), screen_info->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap, 16, 16));

    XFreeGC(getXDisplay(), gc);
    XFreePixmap(getXDisplay(), bitmap);

    if (! (screen_info->getVisual()->c_class & 1))
      XFreePixmap(getXDisplay(), pixmap);
  }
}


void bsetroot::gradient(void) {
  /*
    we have to be sure that neither raised nor sunken is specified otherwise
    odd looking borders appear.  So we convert to lowercase then look for
    'raised' or 'sunken' in the description and erase them.  To be paranoid
    the search is done in a loop.
  */
  std::string descr;
  descr.reserve(grad.size());

  std::string::const_iterator it = grad.begin(), end = grad.end();
  for (; it != end; ++it)
    descr += tolower(*it);

  std::string::size_type pos;
  while ((pos = descr.find("raised")) != std::string::npos)
    descr.erase(pos, 6); // 6 is strlen raised

  while ((pos = descr.find("sunken")) != std::string::npos)
    descr.erase(pos, 6);

  // now add on 'flat' to prevent the bevels from being added
  descr += "flat";

  for (unsigned int screen = 0; screen < getNumberOfScreens(); screen++) {
    BTexture texture(descr, this, screen, img_ctrl[screen]);
    const ScreenInfo *screen_info = getScreenInfo(screen);

    texture.setColor(BColor(fore, this, screen));
    texture.setColorTo(BColor(back, this, screen));

    Pixmap pixmap =
      img_ctrl[screen]->renderImage(screen_info->getWidth(),
                                    screen_info->getHeight(),
                                    texture);

    XSetWindowBackgroundPixmap(getXDisplay(),
                               screen_info->getRootWindow(),
                               pixmap);
    XClearWindow(getXDisplay(), screen_info->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap,
				      screen_info->getWidth(),
				      screen_info->getHeight()));

    if (! (screen_info->getVisual()->c_class & 1)) {
      img_ctrl[screen]->removeImage(pixmap);
    }
  }
}


void bsetroot::usage(int exit_code) {
    fprintf(stderr,
	    i18n(bsetrootSet, bsetrootUsage,
		 "%s 2.0\n\n"
		 "Copyright (c) 1997-2000, 2002 Bradley T Hughes\n"
		 "Copyright (c) 2001-2002 Sean 'Shaleh' Perry\n\n"
		 "  -display <string>        use display connection\n"
		 "  -mod <x> <y>             modula pattern\n"
		 "  -foreground, -fg <color> modula foreground color\n"
		 "  -background, -bg <color> modula background color\n\n"
		 "  -gradient <texture>      gradient texture\n"
		 "  -from <color>            gradient start color\n"
		 "  -to <color>              gradient end color\n\n"
		 "  -solid <color>           solid color\n\n"
		 "  -help                    print this help text and exit\n"),
	    getApplicationName());

    exit(exit_code);
}

int main(int argc, char **argv) {
  char *display_name = (char *) 0;

  i18n.openCatalog("blackbox.cat");

  for (int i = 1; i < argc; i++) {
    if (! strcmp(argv[i], "-display")) {
      // check for -display option

      if ((++i) >= argc) {
        fprintf(stderr, i18n(mainSet, mainDISPLAYRequiresArg,
		             "error: '-display' requires an argument\n"));

        ::exit(1);
      }

      display_name = argv[i];
    }
  }

  bsetroot app(argc, argv, display_name);

  return 0;
}

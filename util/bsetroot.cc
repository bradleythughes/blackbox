#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#ifdef    STDC_HEADERS
#  include <string.h>
#  include <stdlib.h>
#endif // STDC_HEADERS

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#include "../src/i18n.hh"
#include "bsetroot.hh"


I18n i18n;

bsetroot::bsetroot(int argc, char **argv, char *dpy_name)
  : BaseDisplay(argv[0], dpy_name)
{
  grad = fore = back = (char *) 0;

  Bool mod = False, sol = False, grd = False;
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
    } else
      usage();
  }

  if ((mod + sol + grd) != True) {
    fprintf(stderr,
	    i18n(bsetrootSet, bsetrootMustSpecify,
	         "%s: error: must specify one of: -solid, -mod, -gradient\n"),
	    getApplicationName());
    
    usage(2);
  }
  
  img_ctrl = new BImageControl*[getNumberOfScreens()];
  for (int i = 0; i < getNumberOfScreens(); i++)
    img_ctrl[i] = new BImageControl(this, getScreenInfo(i), True);

  if (sol && fore) solid();
  else if (mod && mod_x && mod_y && fore && back) modula(mod_x, mod_y);
  else if (grd && grad && fore && back) gradient();
  else usage();
}


bsetroot::~bsetroot(void) {
  XSetCloseDownMode(getXDisplay(), RetainPermanent);

  XKillClient(getXDisplay(), AllTemporary);

  for (int i = 0; i < getNumberOfScreens(); i++)
    delete img_ctrl[i];

  delete [] img_ctrl;
}

// adapted from wmsetbg
void bsetroot::setPixmapProperty(int screen, Pixmap pixmap) {
  static Atom rootpmap_id = None;
  Atom type;
  int format;
  unsigned long length, after;
  unsigned char *data;
  int mode;
  
  if (rootpmap_id == None)
    rootpmap_id = XInternAtom(getXDisplay(), "_XROOTPMAP_ID", False);

  XGrabServer(getXDisplay());
  
  /* Clear out the old pixmap */
  XGetWindowProperty(getXDisplay(), getScreenInfo(screen)->getRootWindow(),
		     rootpmap_id, 0L, 1L, False, AnyPropertyType,
		     &type, &format, &length, &after, &data);
  
  if ((type == XA_PIXMAP) && (format == 32) && (length == 1)) {
    XKillClient(getXDisplay(), *((Pixmap *)data));
    XSync(getXDisplay(), False);
    mode = PropModeReplace;
  } else {
    mode = PropModeAppend;
  }
  if (pixmap) {
    XChangeProperty(getXDisplay(), getScreenInfo(screen)->getRootWindow(),
		    rootpmap_id, XA_PIXMAP, 32, mode,
		    (unsigned char *) &pixmap, 1);
  } else {
    XDeleteProperty(getXDisplay(), getScreenInfo(screen)->getRootWindow(),
		    rootpmap_id);
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
  for (int screen = 0; screen < getNumberOfScreens(); screen++) {
    BColor c;

    img_ctrl[screen]->parseColor(&c, fore);
    if (! c.isAllocated()) c.setPixel(BlackPixel(getXDisplay(), screen));

    XSetWindowBackground(getXDisplay(), getScreenInfo(screen)->getRootWindow(),
                         c.getPixel());
    XClearWindow(getXDisplay(), getScreenInfo(screen)->getRootWindow());

    Pixmap pixmap = XCreatePixmap(getXDisplay(),
				  getScreenInfo(screen)->getRootWindow(),
				  8, 8, DefaultDepth(getXDisplay(), screen));
    XSetForeground(getXDisplay(), DefaultGC(getXDisplay(), screen),
		   c.getPixel());
    XFillRectangle(getXDisplay(), pixmap, DefaultGC(getXDisplay(), screen),
		   0, 0, 8, 8);

    setPixmapProperty(screen, duplicatePixmap(screen, pixmap, 8, 8));

    XFreePixmap(getXDisplay(), pixmap);
  }
}


void bsetroot::modula(int x, int y) {
  char data[32];
  long pattern;

  int screen, i;

  for (pattern = 0, screen = 0; screen < getNumberOfScreens(); screen++) {
    for (i = 0; i < 16; i++) {
      pattern <<= 1;
      if ((i % x) == 0)
        pattern |= 0x0001;
    }

    for (i = 0; i < 16; i++) {
      if ((i %  y) == 0) {
        data[(i * 2)] = (char) 0xff;
        data[(i * 2) + 1] = (char) 0xff;
      } else {
        data[(i * 2)] = pattern & 0xff;
        data[(i * 2) + 1] = (pattern >> 8) & 0xff;
      }
    }

    BColor f, b;
    GC gc;
    Pixmap bitmap;
    XGCValues gcv;

    bitmap =
      XCreateBitmapFromData(getXDisplay(),
                            getScreenInfo(screen)->getRootWindow(), data,
                            16, 16);

    img_ctrl[screen]->parseColor(&f, fore);
    img_ctrl[screen]->parseColor(&b, back);

    if (! f.isAllocated()) f.setPixel(WhitePixel(getXDisplay(), screen));
    if (! b.isAllocated()) b.setPixel(BlackPixel(getXDisplay(), screen));

    gcv.foreground = f.getPixel();
    gcv.background = b.getPixel();

    gc = XCreateGC(getXDisplay(), getScreenInfo(screen)->getRootWindow(),
                   GCForeground | GCBackground, &gcv);

    Pixmap pixmap = XCreatePixmap(getXDisplay(),
				  getScreenInfo(screen)->getRootWindow(),
				  16, 16, getScreenInfo(screen)->getDepth());

    XCopyPlane(getXDisplay(), bitmap, pixmap, gc,
               0, 0, 16, 16, 0, 0, 1l);
    XSetWindowBackgroundPixmap(getXDisplay(),
                               getScreenInfo(screen)->getRootWindow(),
                               pixmap);
    XClearWindow(getXDisplay(), getScreenInfo(screen)->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap, 16, 16));

    XFreeGC(getXDisplay(), gc);
    XFreePixmap(getXDisplay(), bitmap);

    if (! (getScreenInfo(screen)->getVisual()->c_class & 1))
      XFreePixmap(getXDisplay(), pixmap);
  }
}


void bsetroot::gradient(void) {
  for (int screen = 0; screen < getNumberOfScreens(); screen++) {
    BTexture texture;
    img_ctrl[screen]->parseTexture(&texture, grad);
    img_ctrl[screen]->parseColor(texture.getColor(), fore);
    img_ctrl[screen]->parseColor(texture.getColorTo(), back);

    if (! texture.getColor()->isAllocated())
      texture.getColor()->setPixel(WhitePixel(getXDisplay(), screen));
    if (! texture.getColorTo()->isAllocated())
      texture.getColorTo()->setPixel(BlackPixel(getXDisplay(), screen));

    Pixmap pixmap =
      img_ctrl[screen]->renderImage(getScreenInfo(screen)->getWidth(),
                                    getScreenInfo(screen)->getHeight(),
                                    &texture);

    XSetWindowBackgroundPixmap(getXDisplay(),
                               getScreenInfo(screen)->getRootWindow(),
                               pixmap);
    XClearWindow(getXDisplay(), getScreenInfo(screen)->getRootWindow());

    setPixmapProperty(screen,
		      duplicatePixmap(screen, pixmap,
				      getScreenInfo(screen)->getWidth(), 
				      getScreenInfo(screen)->getHeight()));
      
    if (! (getScreenInfo(screen)->getVisual()->c_class & 1)) {
      img_ctrl[screen]->removeImage(pixmap);
    }
  }
}


void bsetroot::usage(int exit_code) {
  fprintf(stderr,
          i18n(bsetrootSet, bsetrootUsage,
	       "%s 2.0 : (c) 1997-1999 Brad Hughes\n\n"
	       "  -display <string>        display connection\n"
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

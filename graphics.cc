//
// graphics.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#define _GNU_SOURCE
#include "graphics.hh"


BImage::BImage(BlackboxSession *s, unsigned int w, unsigned int h, int d,
	       unsigned char r, unsigned char g, unsigned char b)
{
  session = s;
  width = w;
  height = h;
  depth = d;
  
  unsigned int wh = width * height;
  data = new unsigned long[wh];
  setBackgroundColor(r, g, b);
}


BImage::BImage(BlackboxSession *s, unsigned int w, unsigned int h, int d,
	       const BColor &c)
{
  session = s;
  width = w;
  height = h;
  depth = d;

  unsigned int wh = width * height;
  data = new unsigned long [wh];
  setBackgroundColor(c);
}


BImage::~BImage(void) {
  if (data) delete [] data;
}


Bool BImage::getPixel(unsigned int x, unsigned int y, BColor &color)
{
  if (x > width ||y > height) {
    color.pixel = color.r = color.g = color.b = 0;
    return False;
  }

  unsigned int xy = x * y;
  unsigned long *p = data +xy;

  color.pixel = *p;
  color.r = (*p >> 16) & 0xff;
  color.g = (*p >> 8) & 0xff;
  color.b = (*p) & 0xff;

  return True;
}


Bool BImage::getPixel(unsigned int x, unsigned int y, unsigned long *pixel) {
  // NOTE:  pixel will be returned a 32bpp pixel value.

  if (x > width || y > height) {
    *pixel = 0;
    return False;
  }
  
  unsigned int xy = x * y;
  *pixel = *(data + xy);
  return True;
}


Bool BImage::putPixel(unsigned int x, unsigned int y, const BColor &color)
{
  if (x > width || y > height) return False;
  
  unsigned int xy = x * y;
  *(data + xy) = (color.r << 16) + (color.g << 8) + color.b;
  return True;
}


Bool BImage::putPixel(unsigned int x, unsigned int y, unsigned long pixel) {
  // NOTE:  pixel must be a 32bpp pixel value.

  if (x > width || y > height) return False;

  unsigned int xy = x * y; 
  *(data + xy) = pixel;
  return True;
}



XImage *BImage::convertToXImage(void) {
  char *d = new char[width * height * (depth / 8)];
  if (! d) return 0;

  XImage *image = XCreateImage(session->control(), session->visual(), depth,
			       ZPixmap, 0, d, width, height, 8, 0);
  
  if (image == NULL) return 0;
  
  unsigned char r = 0, g = 0, b = 0;
  unsigned int wh = width * height;
  unsigned long *p = data;
    
  switch (depth) {
  case 32: break;

  case 24: {
    unsigned char *im = (unsigned char *) image->data;

    while (--wh) {
      *(im++) = (unsigned char) ((*p));
      *(im++) = (unsigned char) (((*p))>>8);
      *(im++) = (unsigned char) (((*(p++)))>>16);
    }

    *(im++) = (unsigned char) ((*p));
    *(im++) = (unsigned char) (((*p))>>8);
    *(im) = (unsigned char) (((*p))>>16);

    break; }

  case 16: {
    unsigned short *im = (unsigned short *) image->data;

    while (--wh) {
      r = (unsigned char) (((*p) & 0xff0000)>>16);
      g = (unsigned char) (((*p) & 0xff00)>>8);
      b = (unsigned char) (*(p++) & 0xff);
      *(im++) = (((b>>3) & image->blue_mask)|
		 ((g<<3) & image->green_mask)|
		 ((r<<8) & image->red_mask));
    }
    
    r = (unsigned char) (((*p) & 0xff0000)>>16);
    g = (unsigned char) (((*p) & 0xff00)>>8);
    b = (unsigned char) (*(p++) & 0xff);
    *im = (((b>>3) & image->blue_mask)|
	   ((g<<3) & image->green_mask)|
	   ((r<<8) & image->red_mask));
    
    break; }

  case 15:
  case 8: {
    int rr, gg, bb;
    unsigned char *im = (unsigned char *) image->data;

    while (--wh) {
      r = (unsigned char) (((*p) & 0xff0000)>>16);
      g = (unsigned char) (((*p) & 0xff00)>>8);
      b = (unsigned char) (*(p++) & 0xff);

      rr = (r * 6) / 0xff;
      gg = (g * 6) / 0xff;
      bb = (b * 6) / 0xff;

      if (rr < 0) rr = 0;
      else if (rr > 5) rr = 5;
      if (gg < 0) gg = 0;
      else if (gg > 5) gg = 5;
      if (bb < 0) bb = 0;
      else if (bb > 5) bb = 5;
      *(im++) = session->Colors8bpp()[(rr * 36) + (gg * 6) + bb].pixel;
    }

    r = (unsigned char) (((*p) & 0xff0000)>>16);
    g = (unsigned char) (((*p) & 0xff00)>>8);
    b = (unsigned char) (*(p++) & 0xff);
    
    rr = (r * 6) / 0xff;
    gg = (g * 6) / 0xff;
    bb = (b * 6) / 0xff;
    
    if (rr < 0) rr = 0;
    else if (rr > 5) rr = 5;
    if (gg < 0) gg = 0;
    else if (gg > 5) gg = 5;
    if (bb < 0) bb = 0;
    else if (bb > 5) bb = 5;
    *(im) = session->Colors8bpp()[(rr * 36) + (gg * 6) + bb].pixel;
    
    break; }
  }

  return image;
}


Pixmap BImage::convertToPixmap(void) {
  Pixmap pixmap = XCreatePixmap(session->control(), session->Root(), width,
				height,	(unsigned) depth);
  
  if (pixmap == None) return None;
  
  XImage *img = convertToXImage();

  if (! img) {
    XFreePixmap(session->control(), pixmap);
    return None;
  }

  XPutImage(session->control(), pixmap,
	    DefaultGC(session->control(), DefaultScreen(session->control())),
	    img, 0, 0, 0, 0, width, height);
  
  XDestroyImage(img);
  return pixmap;
}


void BImage::setBackgroundColor(const BColor &c) {
  bg_color = c;
  
  unsigned int wh = width * height;
  unsigned long *p = data;

  bg_color.pixel = ((c.r<<16)|(c.g<<8)|(c.b));
  
  while (--wh) *(p++) = bg_color.pixel;
  *p = bg_color.pixel;
}

  
void BImage::setBackgroundColor(unsigned char r, unsigned char g,
				unsigned char b)
{
  bg_color.r = r;
  bg_color.g = g;
  bg_color.b = b;

  unsigned int wh = width * height; 
  unsigned long *p = data;

  bg_color.pixel = ((r<<16)|(g<<8)|(b));
  
  while (--wh) *(p++) = bg_color.pixel;
  *p = bg_color.pixel;
}


void BImage::renderBevel(void) {
  if (height > 4) {
    unsigned long pix0 = 0, pix1 = 0;
    unsigned char r, g, b, rr, gg, bb;

    // lets do the easy parts... top and bottom lines
    unsigned int w = width - 2, h = height - 1, wh = width * (height - 3);
    unsigned long *p = data + width + 1;
    
    if (bg_color.r == bg_color.g && bg_color.g == bg_color.b &&
	bg_color.b == 0) {
      pix0 = 0x808080;
      pix1 = 0x404040;
      
      while (--w) {
	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      p = data + width;
      while (--h) {
	*(++p) = pix0;
	p += (width - 3);
	*(p++) = pix1;
	p++;
      }
    } else {
      while (--w) {
	r = ((unsigned char) (((*p) & 0xff0000)>>16));
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = ((unsigned char) (((*p) & 0x00ff00)>>8));
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = ((unsigned char) ((*p) & 0x0000ff));
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));
	
	r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));

	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      p = data + width;
      while (--h) {
	r = ((unsigned char) (((*p) & 0xff0000)>>16));
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = ((unsigned char) (((*p) & 0x00ff00)>>8));
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = ((unsigned char) ((*p) & 0x0000ff));
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));
	
	r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));

	*(++p) = pix0;
	p += (width - 3);
	*(p++) = pix1;
	p++;
      }
    }
  }
}


void BImage::renderButton(void) {
  if (height > 2) {
    unsigned long pix0 = 0, pix1 = 0;
    unsigned char r, g, b, rr ,gg ,bb;
    
    // lets do the easy parts... top and bottom lines
    unsigned int w = width, h = height - 1, wh = w * h;
    unsigned long *p = data;
    
    if (bg_color.r == bg_color.g && bg_color.g == bg_color.b &&
	bg_color.b == 0) {
      pix0 = 0x808080;
      pix1 = 0x404040;
      
      while (--w) {
	*p = pix0;
	*((p++) + wh) = pix1;
      }

      *p = pix0;
      *(p + wh) = pix1;
      
      p = data + width;
      while (--h) {
	*p = pix0;
	p += (width - 1);
	*(p++) = pix1;
      }

      *p = pix0;
      p += (width - 1);
      *p = pix1;
    } else {
      while (--w) {
	r = ((unsigned char) (((*p) & 0xff0000)>>16));
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = ((unsigned char) (((*p) & 0x00ff00)>>8));
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = ((unsigned char) ((*p) & 0x0000ff));
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));
	
	r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));
	*p = pix0;
	*((p++) + wh) = pix1;
      }
      
      r = ((unsigned char) (((*p) & 0xff0000)>>16));
      rr = r * 3 / 2;
      if (rr < r) rr = ~0;
      g = ((unsigned char) (((*p) & 0x00ff00)>>8));
      gg = g * 3 / 2;
      if (gg < g) gg = ~0;
      b = ((unsigned char) ((*p) & 0x0000ff));
      bb = b * 3 / 2;
      if (bb < b) bb = ~0;
      pix0 = ((((unsigned char) rr)<<16)|
	      (((unsigned char) gg)<<8)|
	      ((unsigned char) bb));
      
      r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
      rr = r * 3 / 4;
      if (rr > (unsigned char) r) rr = 0;
      g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
      gg = g * 3 / 4;
      if (gg > (unsigned char) g) gg = 0;
      b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
      bb = b * 3 / 4;
      if (bb > (unsigned char) b) bb = 0;
      pix1 = ((((unsigned char) rr)<<16)|
	      (((unsigned char) gg)<<8)|
	      ((unsigned char) bb));
      *p = pix0;
      *(p + wh) = pix1;
      
      p = data + width;
      while (--h) {
	r = ((unsigned char) (((*p) & 0xff0000)>>16));
	rr = r * 3 / 2;
	if (rr < r) rr = ~0;
	g = ((unsigned char) (((*p) & 0x00ff00)>>8));
	gg = g * 3 / 2;
	if (gg < g) gg = ~0;
	b = ((unsigned char) ((*p) & 0x0000ff));
	bb = b * 3 / 2;
	if (bb < b) bb = ~0;
	pix0 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));
	
	r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
	rr = r * 3 / 4;
	if (rr > (unsigned char) r) rr = 0;
	g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
	gg = g * 3 / 4;
	if (gg > (unsigned char) g) gg = 0;
	b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
	bb = b * 3 / 4;
	if (bb > (unsigned char) b) bb = 0;
	pix1 = ((((unsigned char) rr)<<16)|
		(((unsigned char) gg)<<8)|
		((unsigned char) bb));

	*p = pix0;
	p += (width - 1);
	*(p++) = pix1;
      }
      
      r = ((unsigned char) (((*p) & 0xff0000)>>16));
      rr = r * 3 / 2;
      if (rr < r) rr = ~0;
      g = ((unsigned char) (((*p) & 0x00ff00)>>8));
      gg = g * 3 / 2;
      if (gg < g) gg = ~0;
      b = ((unsigned char) ((*p) & 0x0000ff));
      bb = b * 3 / 2;
      if (bb < b) bb = ~0;
      pix0 = ((((unsigned char) rr)<<16)|
	      (((unsigned char) gg)<<8)|
	      ((unsigned char) bb));
      
      r = ((unsigned char) (((*(p)) & 0xff0000)>>16));
      rr = r * 3 / 4;
      if (rr > (unsigned char) r) rr = 0;
      g = ((unsigned char) (((*(p)) & 0x00ff00)>> 8));
      gg = g * 3 / 4;
      if (gg > (unsigned char) g) gg = 0;
      b = ((unsigned char) (((*(p)) & 0x0000ff)    ));
      bb = b * 3 / 4;
      if (bb > (unsigned char) b) bb = 0;
      pix1 = ((((unsigned char) rr)<<16)|
	      (((unsigned char) gg)<<8)|
	      ((unsigned char) bb));
      *p = pix0;
      p += (width - 1);
      *p = pix1;
    }
  }
}


void BImage::invertImage(void) {
  unsigned int wh = width * height;
  unsigned long *p, *pp, *new_data = new unsigned long[wh];
  pp = new_data;
  p = data + wh - 1;

  while (--wh) *(pp++) = *(p--);
  *pp = *p;
  
  delete [] data;
  data = new_data;
  data = new_data;
}


void BImage::renderDGradient(const BColor &from, const BColor &to) {
  float fr, fg, fb, tr, tg, tb, dr, dg, db,
    w = (float) width, h = (float) height,
    dx, dy, xr, xg, xb, yr, yg, yb;
  
  unsigned char r, g, b;
  unsigned int i, ii;
  unsigned long *p = data;
  
  fr = (float) from.r;
  fg = (float) from.g;
  fb = (float) from.b;
  
  tr = (float) to.r;
  tg = (float) to.g;
  tb = (float) to.b;
  
  dr = tr - fr;
  dg = tg - fg;
  db = tb - fb;

  for (i = 0; i < height; ++i) {
    dy = i / h;

    yr = (dr * dy) + fr;
    yg = (dg * dy) + fg;
    yb = (db * dy) + fb;

    for (ii = 0; ii < width; ++ii) {
      dx = ii / w;

      xr = (dr * dx) + fr;
      xg = (dg * dx) + fg;
      xb = (db * dx) + fb;
      
      r = (unsigned char) ((xr + yr) / 2);
      g = (unsigned char) ((xg + yg) / 2);
      b = (unsigned char) ((xb + yb) / 2);
      *(p++) = (unsigned long) ((((unsigned char) r)<<16)|
				(((unsigned char) g)<<8)|
				((unsigned char) b));
    }
  }
}


Pixmap BImage::renderImage(int texture, int bevel, const BColor &color1,
			   const BColor &color2)
{
  switch (texture) {
  case BlackboxSession::B_TextureRSolid:
  case BlackboxSession::B_TextureSSolid:
  case BlackboxSession::B_TextureFSolid:
    return renderSolidImage(texture, bevel, color1);
    break;

  case BlackboxSession::B_TextureRDGradient:
  case BlackboxSession::B_TextureSDGradient:
  case BlackboxSession::B_TextureFDGradient:
    return renderGradientImage(texture, bevel, color1, color2);
    break;
  }

  return None;
}


Pixmap BImage::renderInvertedImage(int texture, int bevel,
				   const BColor &color1,
				   const BColor &color2)
{
  switch (texture) {
  case BlackboxSession::B_TextureRSolid:
    setBackgroundColor(color1);
    if (bevel)
      renderBevel();
    else
      renderButton();

    invertImage();
    return convertToPixmap();
    break;
    
  case BlackboxSession::B_TextureSSolid:
    setBackgroundColor(color1);
    if (bevel)
      renderBevel();
    else
      renderButton();

    return convertToPixmap();
    break;
    
  case BlackboxSession::B_TextureFSolid:
    setBackgroundColor(color1);
    invertImage();
    return convertToPixmap();
    break;

  case BlackboxSession::B_TextureRDGradient:
    renderDGradient(color1, color2);
    if (bevel)
      renderBevel();
    else
      renderButton();
    
    invertImage();
    return convertToPixmap();
    break;

  case BlackboxSession::B_TextureSDGradient:
    renderDGradient(color2, color1);
    if (bevel)
      renderBevel();
    else
      renderButton();
    
    return convertToPixmap();
    break;

  case BlackboxSession::B_TextureFDGradient:
    renderDGradient(color1, color2);
    invertImage();
    return convertToPixmap();
    break;
  }
  
  return None;
}


Pixmap BImage::renderSolidImage(int texture, int bevel, const BColor &color) {
  switch(texture) {
  case BlackboxSession::B_TextureRSolid:
    setBackgroundColor(color);
    if (bevel)
      renderBevel();
    else
      renderButton();

    return convertToPixmap();
    break;
    
  case BlackboxSession::B_TextureSSolid:
    setBackgroundColor(color);
    if (bevel)
      renderBevel();
    else
      renderButton();

    invertImage();
    return convertToPixmap();
    break;
    
  case BlackboxSession::B_TextureFSolid:
    setBackgroundColor(color);
    return convertToPixmap();
    break;
  }

  return None;
}


Pixmap BImage::renderGradientImage(int texture, int bevel,
				   const BColor &from,
				   const BColor &to)
{
  switch(texture) {
  case BlackboxSession::B_TextureRDGradient:
    renderDGradient(from, to);
    if (bevel)
      renderBevel();
    else
      renderButton();
    
    return convertToPixmap();
    break;

  case BlackboxSession::B_TextureSDGradient:
    renderDGradient(to, from);
    if (bevel)
      renderBevel();
    else
      renderButton();
    
    invertImage();
    return convertToPixmap();
    break;

  case BlackboxSession::B_TextureFDGradient:
    renderDGradient(from, to);
    return convertToPixmap();
    break;
  }

  return None;
}

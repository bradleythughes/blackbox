// Basemenu.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "i18n.hh"
#include "blackbox.hh"
#include "Basemenu.hh"
#include "Screen.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS


static Basemenu *shown = (Basemenu *) 0;

Basemenu::Basemenu(BScreen *scrn) {
#ifdef    DEBUG
  allocate(sizeof(Basemenu), "Basemenu.cc");
#endif // DEBUG

  screen = scrn;
  blackbox = screen->getBlackbox();
  image_ctrl = screen->getImageControl();
  display = blackbox->getXDisplay();
  parent = (Basemenu *) 0;
  alignment = AlignDontCare;

  title_vis =
    movable =
    hide_tree = True;

  shifted =
    internal_menu =
    moving =
    torn =
    visible = False;

  menu.x =
    menu.y =
    menu.x_shift =
    menu.y_shift =
    menu.x_move =
    menu.y_move = 0;

  which_sub =
    which_press =
    which_sbl = -1;

  menu.frame_pixmap =
    menu.title_pixmap =
    menu.hilite_pixmap =
    menu.sel_pixmap = None;

  menu.bevel_w = screen->getBevelWidth();
  menu.width =
    menu.title_h =
    menu.item_w =
    menu.frame_h =
#ifdef    NLS
    screen->getMenuStyle()->t_extents->max_ink_extent.height +
#else // !NLS
    screen->getMenuStyle()->t_font->ascent +
    screen->getMenuStyle()->t_font->descent +
#endif // NLS
    (menu.bevel_w * 2);

  menu.label = 0;

  menu.sublevels =
    menu.persub =
    menu.minsub = 0;

  menu.item_h =
#ifdef    NLS
    screen->getMenuStyle()->f_extents->max_ink_extent.height +
#else // !NLS
    screen->getMenuStyle()->f_font->ascent +
    screen->getMenuStyle()->f_font->descent +
#endif // NLS
    (menu.bevel_w);

  menu.height = menu.title_h + screen->getBorderWidth() + menu.frame_h;

  unsigned long attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
			      CWOverrideRedirect | CWEventMask;
  XSetWindowAttributes attrib;
  attrib.background_pixmap = None;
  attrib.background_pixel = attrib.border_pixel =
			    screen->getBorderColor()->getPixel();
  attrib.override_redirect = True;
  attrib.event_mask = ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
		      ExposureMask;

  menu.window =
    XCreateWindow(display, screen->getRootWindow(), menu.x, menu.y, menu.width,
		  menu.height, screen->getBorderWidth(), screen->getDepth(),
                  InputOutput, screen->getVisual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.window, this);

  attrib_mask = CWBackPixmap | CWBackPixel | CWBorderPixel | CWEventMask;
  attrib.background_pixel = screen->getBorderColor()->getPixel();
  attrib.event_mask |= EnterWindowMask | LeaveWindowMask;

  menu.title =
    XCreateWindow(display, menu.window, 0, 0, menu.width, menu.height, 0,
		  screen->getDepth(), InputOutput, screen->getVisual(),
		  attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.title, this);

  attrib.event_mask |= PointerMotionMask;
  menu.frame = XCreateWindow(display, menu.window, 0,
			     menu.title_h + screen->getBorderWidth(),
			     menu.width, menu.frame_h, 0,
			     screen->getDepth(), InputOutput,
			     screen->getVisual(), attrib_mask, &attrib);
  blackbox->saveMenuSearch(menu.frame, this);

  menuitems = new LinkedList<BasemenuItem>;

  // even though this is the end of the constructor the menu is still not
  // completely created.  items must be inserted and it must be update()'d
}


Basemenu::~Basemenu(void) {
#ifdef    DEBUG
  deallocate(sizeof(Basemenu), "Basemenu.cc");
#endif // DEBUG

  XUnmapWindow(display, menu.window);

  if (shown && shown->getWindowID() == getWindowID())
    shown = (Basemenu *) 0;

  int n = menuitems->count();
  for (int i = 0; i < n; ++i)
    remove(0);

  delete menuitems;

  if (menu.label) {
#ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(menu.label) + 1), "Basemenu.cc");
#endif // DEBUG

    delete [] menu.label;
  }

  if (menu.title_pixmap)
    image_ctrl->removeImage(menu.title_pixmap);

  if (menu.frame_pixmap)
    image_ctrl->removeImage(menu.frame_pixmap);

  if (menu.hilite_pixmap)
    image_ctrl->removeImage(menu.hilite_pixmap);

  if (menu.sel_pixmap)
    image_ctrl->removeImage(menu.sel_pixmap);

  blackbox->removeMenuSearch(menu.title);
  XDestroyWindow(display, menu.title);

  blackbox->removeMenuSearch(menu.frame);
  XDestroyWindow(display, menu.frame);

  blackbox->removeMenuSearch(menu.window);
  XDestroyWindow(display, menu.window);
}


int Basemenu::insert(const char *l, int function, const char *e, int pos) {
  char *label = 0, *exec = 0;
  
  if (l) label = bstrdup(l);
  if (e) exec = bstrdup(e);
  
  BasemenuItem *item = new BasemenuItem(label, function, exec);
  menuitems->insert(item, pos);

  return menuitems->count();
}


int Basemenu::insert(const char *l, Basemenu *submenu, int pos) {
  char *label = 0;

  if (l) label = bstrdup(l);

  BasemenuItem *item = new BasemenuItem(label, submenu);
  menuitems->insert(item, pos);

  submenu->parent = this;

  return menuitems->count();
}


int Basemenu::insert(const char **ulabel, int pos, int function) {
  BasemenuItem *item = new BasemenuItem(ulabel, function);
  menuitems->insert(item, pos);

  return menuitems->count();
}


int Basemenu::remove(int index) {
  if (index < 0 || index > menuitems->count()) return -1;

  BasemenuItem *item = menuitems->remove(index);

  if (item) {
    if ((! internal_menu) && (item->submenu())) {
      Basemenu *tmp = (Basemenu *) item->submenu();

      if (! tmp->internal_menu) {
	delete tmp;
      } else
	tmp->internal_hide();
    }

    if (item->label()) {
#ifdef    DEBUG
      deallocate(sizeof(char) * (strlen(item->label()) + 1), "Basemenu.cc");
#endif // DEBUG

      delete [] item->label();
    }

    if (item->exec()) {
#ifdef    DEBUG
      deallocate(sizeof(char) * (strlen(item->exec()) + 1), "Basemenu.cc");
#endif // DEBUG

      delete [] item->exec();
    }

    delete item;
  }

  if (which_sub == index)
    which_sub = -1;
  else if (which_sub > index)
    which_sub--;

  return menuitems->count();
}


void Basemenu::update(void) {
  menu.item_h =
#ifdef    NLS
    screen->getMenuStyle()->f_extents->max_ink_extent.height +
#else // !NLS
    screen->getMenuStyle()->f_font->ascent +
    screen->getMenuStyle()->f_font->descent +
#endif // NLS
    menu.bevel_w;

  if (title_vis) {
    const char *s = (menu.label) ? menu.label :
		    i18n->getMessage(
#ifdef    NLS
				     BasemenuSet, BasemenuBlackboxMenu,
#else // !NLS
				     0, 0,
#endif // NLS
				     "Blackbox Menu");
    int l = strlen(s);

#ifdef    NLS
    XRectangle ink, logical;
    XmbTextExtents(screen->getMenuStyle()->t_font, s, l, &ink, &logical);
    menu.item_w = logical.width;
#else // !NLS
    menu.item_w =
      XTextWidth(screen->getMenuStyle()->t_font, s, l);
#endif // NLS

    menu.item_w += (menu.bevel_w * 2);
  }  else
    menu.item_w = 1;

  int ii = 0;
  LinkedListIterator<BasemenuItem> it(menuitems);
  for (; it.current(); it++) {
    BasemenuItem *itmp = it.current();

    const char *s = ((itmp->u && *itmp->u) ? *itmp->u :
		     ((itmp->l) ? itmp->l : (const char *) 0));
    int l = strlen(s);

#ifdef    NLS
    XRectangle ink, logical;
    XmbTextExtents(screen->getMenuStyle()->f_font, s, l, &ink, &logical);
    ii = logical.width;
#else // !NLS
    ii = XTextWidth(screen->getMenuStyle()->f_font, s, l);
#endif // NLS

    ii += (menu.bevel_w * 2) + (menu.item_h * 2);

    menu.item_w = ((menu.item_w < (unsigned int) ii) ? ii : menu.item_w);
  }

  if (menuitems->count()) {
    menu.sublevels = 1;

    while (((menu.item_h * (menuitems->count() + 1) / menu.sublevels)
	    + menu.title_h + screen->getBorderWidth()) >
	   screen->getHeight())
      menu.sublevels++;

    if (menu.sublevels < menu.minsub) menu.sublevels = menu.minsub;

    menu.persub = menuitems->count() / menu.sublevels;
    if (menuitems->count() % menu.sublevels) menu.persub++;
  } else {
    menu.sublevels = 0;
    menu.persub = 0;
  }

  menu.width = (menu.sublevels * (menu.item_w));
  if (! menu.width) menu.width = menu.item_w;

  menu.title_h =
#ifdef    NLS
    screen->getMenuStyle()->t_extents->max_ink_extent.height +
#else // !NLS
    screen->getMenuStyle()->t_font->ascent +
    screen->getMenuStyle()->t_font->descent +
#endif // NLS
    (menu.bevel_w * 2);

  menu.frame_h = (menu.item_h * menu.persub);
  menu.height = ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) +
    menu.frame_h;
  if (! menu.frame_h) menu.frame_h = 1;
  if (menu.height < 1) menu.height = 1;

  Pixmap tmp;
  if (title_vis) {
    tmp = menu.title_pixmap;
    menu.title_pixmap =
      image_ctrl->renderImage(menu.width, menu.title_h,
			      &(screen->getMenuStyle()->title));
    if (tmp) image_ctrl->removeImage(tmp);
    XSetWindowBackgroundPixmap(display, menu.title, menu.title_pixmap);
    XClearWindow(display, menu.title);
  }

  tmp = menu.frame_pixmap;
  menu.frame_pixmap =
    image_ctrl->renderImage(menu.width, menu.frame_h,
			    &(screen->getMenuStyle()->frame));
  if (tmp) image_ctrl->removeImage(tmp);
  XSetWindowBackgroundPixmap(display, menu.frame, menu.frame_pixmap);

  tmp = menu.hilite_pixmap;
  menu.hilite_pixmap =
    image_ctrl->renderImage(menu.item_w, menu.item_h,
			    &screen->getMenuStyle()->hilite);
  if (tmp) image_ctrl->removeImage(tmp);

  int hw = menu.item_h / 2;
  tmp = menu.sel_pixmap;
  menu.sel_pixmap =
    image_ctrl->renderImage(hw, hw, &screen->getMenuStyle()->hilite);
  if (tmp) image_ctrl->removeImage(tmp);

  XResizeWindow(display, menu.window, menu.width, menu.height);

  if (title_vis)
    XResizeWindow(display, menu.title, menu.width, menu.title_h);

  XMoveResizeWindow(display, menu.frame, 0,
		    ((title_vis) ? menu.title_h +
                                   screen->getBorderWidth() : 0), menu.width,
		    menu.frame_h);

  XClearWindow(display, menu.window);
  XClearWindow(display, menu.title);
  XClearWindow(display, menu.frame);

  if (title_vis && visible) redrawTitle();

  int i = 0;
  for (i = 0; visible && i < menuitems->count(); i++)
    if (i == which_sub) {
      drawItem(i, True, 0);
      drawSubmenu(i);
    } else
      drawItem(i, False, 0);

  if (parent && visible)
    parent->drawSubmenu(parent->which_sub);

  XMapSubwindows(display, menu.window);
}


void Basemenu::show(void) {
  XMapSubwindows(display, menu.window);
  XMapWindow(display, menu.window);
  visible = True;

  if (! parent) {
    if (shown && (! shown->torn))
       shown->hide();

    shown = this;
  }
}


void Basemenu::hide(void) {
  if ((! torn) && hide_tree && parent && parent->isVisible()) {
    Basemenu *p = parent;

    while (p->isVisible() && (! p->torn) && p->parent) p = p->parent;
    p->internal_hide();
  } else
    internal_hide();
}


void Basemenu::internal_hide(void) {
  if (which_sub != -1) {
    BasemenuItem *tmp = menuitems->find(which_sub);
    tmp->submenu()->internal_hide();
  }

  if (parent && (! torn)) {
    parent->drawItem(parent->which_sub, False, True);

    parent->which_sub = -1;
  } else if (shown && shown->menu.window == menu.window)
    shown = (Basemenu *) 0;

  torn = visible = False;
  which_sub = which_press = which_sub = -1;

  XUnmapWindow(display, menu.window);
}


void Basemenu::move(int x, int y) {
  menu.x = x;
  menu.y = y;
  XMoveWindow(display, menu.window, x, y);
  if (which_sub != -1)
    drawSubmenu(which_sub);
}


void Basemenu::redrawTitle(void) {
  char *text = (char *) ((menu.label) ? menu.label :
			 i18n->getMessage(
#ifdef    NLS
					  BasemenuSet, BasemenuBlackboxMenu,
#else // !NLS
					  0, 0,
#endif // NLS
					  "Blackbox Menu"));
  int dx = menu.bevel_w, len = strlen(text);
  unsigned int l;

#ifdef   NLS
  XRectangle ink, logical;
  XmbTextExtents(screen->getMenuStyle()->t_font, text, len, &ink, &logical);
  l = logical.width;
#else // !NLS
  l = XTextWidth(screen->getMenuStyle()->t_font, text, len);
#endif // NLS

  l +=  (menu.bevel_w * 2);

  switch (screen->getMenuStyle()->t_justify) {
  case BScreen::RightJustify:
    dx += menu.width - l;
    break;

  case BScreen::CenterJustify:
    dx += (menu.width - l) / 2;
    break;
  }

#ifdef    NLS
  XmbDrawString(display, menu.title, screen->getMenuStyle()->t_font,
		screen->getMenuStyle()->t_text_gc, dx, menu.bevel_w -
		screen->getMenuStyle()->t_extents->max_ink_extent.y, text, len);
#else // !NLS
  XDrawString(display, menu.title, screen->getMenuStyle()->t_text_gc, dx,
	      screen->getMenuStyle()->t_font->ascent + menu.bevel_w,
              text, len);
#endif // NLS
}


void Basemenu::drawSubmenu(int index) {
  if (which_sub != -1 && which_sub != index) {
    BasemenuItem *itmp = menuitems->find(which_sub);

    if (! itmp->submenu()->isTorn())
      itmp->submenu()->internal_hide();
  }

  if (index >= 0 && index < menuitems->count()) {
    BasemenuItem *item = menuitems->find(index);
    if (item->submenu() && visible && (! item->submenu()->isTorn()) &&
	item->isEnabled()) {
      if (item->submenu()->parent != this) item->submenu()->parent = this;
      drawItem(index, True);

      int sbl = index / menu.persub, i = index - (sbl * menu.persub),
	x = (((shifted) ? menu.x_shift : menu.x) +
	     (menu.item_w * (sbl + 1)) + screen->getBorderWidth()), y;

      if (alignment == AlignTop)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));
      else
	y = (((shifted) ? menu.y_shift : menu.y) +
	     (menu.item_h * i) +
             ((title_vis) ? menu.title_h + screen->getBorderWidth() : 0) -
	     ((item->submenu()->title_vis) ?
	      item->submenu()->menu.title_h + screen->getBorderWidth() : 0));

      if (alignment == AlignBottom &&
	  (y + item->submenu()->menu.height) > ((shifted) ? menu.y_shift :
						menu.y) + menu.height)
	y = (((shifted) ? menu.y_shift : menu.y) +
	     menu.height - item->submenu()->menu.height);

      if ((x + item->submenu()->getWidth()) > screen->getWidth())
	x = ((shifted) ? menu.x_shift : menu.x) -
	  item->submenu()->getWidth() - screen->getBorderWidth();
      if (x < 0) x = 0;

      if ((y + item->submenu()->getHeight()) > screen->getHeight())
	y = screen->getHeight() - item->submenu()->getHeight() -
          screen->getBorderWidth2x();
      if (y < 0) y = 0;

      item->submenu()->move(x, y);

      if (! item->submenu()->isVisible())
	item->submenu()->show();
      which_sub = index;
    } else
      which_sub = -1;
  }
}


Bool Basemenu::hasSubmenu(int index) {
  if ((index >= 0) && (index < menuitems->count()))
    if (menuitems->find(index)->submenu())
      return True;
    else
      return False;
  else
    return False;
}


void Basemenu::drawItem(int index, Bool highlight, Bool clear) {
  if (index < 0 || index > menuitems->count()) return;

  int sbl = index / menu.persub, i = index - (sbl * menu.persub),
       ix = (sbl * menu.item_w), iy = (i * menu.item_h), tx = 0;
  BasemenuItem *item = menuitems->find(index);

  if (! item) return;

  const char *text = (item->ulabel()) ? *item->ulabel() : item->label();
  int len = strlen(text), itw;

#ifdef    NLS
  XRectangle ink, logical;
  XmbTextExtents(screen->getMenuStyle()->f_font, text, len, &ink, &logical);
  itw = logical.width;
#else // !NLS
  itw = XTextWidth(screen->getMenuStyle()->f_font, text, len);
#endif // NLS

  switch(screen->getMenuStyle()->f_justify) {
  case BScreen::LeftJustify:
    tx = ix + menu.bevel_w + menu.item_h + 1;
    break;

  case BScreen::RightJustify:
    tx = ix + menu.item_w - (menu.item_h + menu.bevel_w + itw);
    break;

  case BScreen::CenterJustify:
    tx = ix + ((menu.item_w + 1 - itw) / 2);
    break;
  }

  if (clear) {
    XClearArea(display, menu.frame, ix, iy, menu.item_w, menu.item_h,
	       False);
  }

  GC gc =
    ((highlight || item->isSelected()) ? screen->getMenuStyle()->h_text_gc :
     screen->getMenuStyle()->f_text_gc),
    tgc =
    ((highlight) ? screen->getMenuStyle()->h_text_gc :
     ((item->isEnabled()) ? screen->getMenuStyle()->f_text_gc :
      screen->getMenuStyle()->d_text_gc));

  int fx = ix, hw = menu.item_h / 2, qw = menu.item_h / 4;

  if (screen->getMenuStyle()->bullet_pos == Right)
    fx += (menu.item_w - menu.item_h - menu.bevel_w);

  if (highlight && (menu.hilite_pixmap != ParentRelative))
    XCopyArea(display, menu.hilite_pixmap, menu.frame,
              screen->getMenuStyle()->h_text_gc,
              0, 0, menu.item_w, menu.item_h, ix, iy);
  else if (item->isSelected() && (menu.sel_pixmap != ParentRelative))
    XCopyArea(display, menu.sel_pixmap, menu.frame,
	      screen->getMenuStyle()->h_text_gc, 0, 0, hw, hw,
	      fx + qw, iy + qw);

  if (text) {
#ifdef    NLS
    XmbDrawString(display, menu.frame, screen->getMenuStyle()->f_font,
		  tgc, tx, iy + (menu.bevel_w / 2) -
		  screen->getMenuStyle()->f_extents->max_ink_extent.y,
		  text, len);
#else // !NLS
    XDrawString(display, menu.frame, tgc, tx, iy +
		screen->getMenuStyle()->f_font->ascent +
		(menu.bevel_w / 2), text, len);
#endif // NLS
  }

  //   if (item->ulabel())
  //     XDrawString(display, menu.frame, tgc, tx,
  //                 iy + screen->getMenuStyle()->f_font->ascent +
  // 		(menu.bevel_w / 2),
  //                 (char *) *item->ulabel(), strlen(*item->ulabel()));
  //   else if (item->label())
  //     XDrawString(display, menu.frame, tgc, tx,
  //                 iy + screen->getMenuStyle()->f_font->ascent +
  // 		(menu.bevel_w / 2),
  //                 (char *) item->label(), strlen(item->label()));

  if (item->submenu()) {
    switch (screen->getMenuStyle()->bullet) {
    case Empty:
      break;

    case Square:
      XDrawRectangle(display, menu.frame, gc, fx + qw, iy + qw, hw, hw);
      break;

    case Triangle:
      XPoint tri[3];

      if (screen->getMenuStyle()->bullet_pos == Right) {
        tri[0].x = fx + hw - 2; tri[0].y = iy + hw - 2;
        tri[1].x = 4; tri[1].y = 2;
        tri[2].x = -4; tri[2].y = 2;
      } else {
        tri[0].x = fx + hw - 2; tri[0].y = iy + hw;
        tri[1].x = 4; tri[1].y = 2;
        tri[2].x = 0; tri[2].y = -4;
      }

      XFillPolygon(display, menu.frame, gc, tri, 3, Convex,
                   CoordModePrevious);
      break;

    case Diamond:
      XPoint dia[4];

      dia[0].x = fx + hw - 3; dia[0].y = iy + hw;
      dia[1].x = 3; dia[1].y = -3;
      dia[2].x = 3; dia[2].y = 3;
      dia[3].x = -3; dia[3].y = 3;

      XFillPolygon(display, menu.frame, gc, dia, 4, Convex,
                   CoordModePrevious);
      break;
    }
  }
}


void Basemenu::setLabel(const char *l) {
  if (menu.label) {
#ifdef    DEBUG
    deallocate(sizeof(char) * (strlen(menu.label) + 1), "Basemenu.cc");
#endif // DEBUG

    delete [] menu.label;
  }

  if (l) menu.label = bstrdup(l);
  else menu.label = 0;
}


void Basemenu::setItemSelected(int index, Bool sel) {
  if (index < 0 || index >= menuitems->count()) return;

  BasemenuItem *item = find(index);
  if (! item) return;

  item->setSelected(sel);
  if (visible) drawItem(index, (index == which_sub), True);
}


Bool Basemenu::isItemSelected(int index) {
  if (index < 0 || index >= menuitems->count()) return False;

  BasemenuItem *item = find(index);
  if (! item) return False;

  return item->isSelected();
}


void Basemenu::setItemEnabled(int index, Bool enable) {
  if (index < 0 || index >= menuitems->count()) return;

  BasemenuItem *item = find(index);
  if (! item) return;

  item->setEnabled(enable);
  if (visible) drawItem(index, (index == which_sub), True);
}


Bool Basemenu::isItemEnabled(int index) {
  if (index < 0 || index >= menuitems->count()) return False;

  BasemenuItem *item = find(index);
  if (! item) return False;

  return item->isEnabled();
}


void Basemenu::buttonPressEvent(XButtonEvent *be) {
  if (be->window == menu.frame) {
    int sbl = (be->x / menu.item_w), i = (be->y / menu.item_h);
    int w = (sbl * menu.persub) + i;

    if (w < menuitems->count() && w >= 0) {
      which_press = i;
      which_sbl = sbl;

      BasemenuItem *item = menuitems->find(w);

      if (item->submenu())
	drawSubmenu(w);
      else
	drawItem(w, (item->isEnabled()), True);
    }
  } else {
    menu.x_move = be->x_root - menu.x;
    menu.y_move = be->y_root - menu.y;
  }
}


void Basemenu::buttonReleaseEvent(XButtonEvent *re) {
  if (re->window == menu.title) {
    if (moving) {
      moving = False;
      if (which_sub != -1)
	drawSubmenu(which_sub);
    }

    if (re->x >= 0 && re->x <= (signed) menu.width &&
	re->y >= 0 && re->y <= (signed) menu.title_h)
      if (re->button == 3)
	hide();
  } else if (re->window == menu.frame &&
	     re->x >= 0 && re->x < (signed) menu.width &&
	     re->y >= 0 && re->y < (signed) menu.frame_h) {
    if (re->button == 3) {
      hide();
    } else {
      int sbl = (re->x / menu.item_w), i = (re->y / menu.item_h),
        ix = sbl * menu.item_w, iy = i * menu.item_h,
        w = (sbl * menu.persub) + i,
        p = (which_sbl * menu.persub) + which_press;

      if (w < menuitems->count() && w >= 0) {
	drawItem(p, (p == which_sub), True);

        if  (p == w && isItemEnabled(w)) {
	  if (re->x > ix && re->x < (signed) (ix + menu.item_w) &&
	      re->y > iy && re->y < (signed) (iy + menu.item_h)) {
	    itemSelected(re->button, w);
	  }
        }
      } else
        drawItem(p, False, True);
    }
  }
}


void Basemenu::motionNotifyEvent(XMotionEvent *me) {
  if (me->window == menu.title && (me->state & Button1Mask)) {
    if (movable) {
      if (! moving) {
	if (parent && (! torn)) {
	  parent->drawItem(parent->which_sub, False, True);
	  parent->which_sub = -1;
	}

        moving = torn = True;

	if (which_sub != -1)
	  drawSubmenu(which_sub);
      } else {
	menu.x = me->x_root - menu.x_move,
	menu.y = me->y_root - menu.y_move;
	XMoveWindow(display, menu.window, menu.x, menu.y);

	if (which_sub != -1)
	  drawSubmenu(which_sub);
      }
    }
  } else if ((! (me->state & Button1Mask)) && me->window == menu.frame &&
	     me->x >= 0 && me->x < (signed) menu.width &&
	     me->y >= 0 && me->y < (signed) menu.frame_h) {
    int sbl = (me->x / menu.item_w), i = (me->y / menu.item_h),
      w = (sbl * menu.persub) + i;

    if ((i != which_press || sbl != which_sbl) &&
	(w < menuitems->count() && w >= 0)) {
      if (which_press != -1 && which_sbl != -1) {
	int p = (which_sbl * menu.persub) + which_press;
	BasemenuItem *item = menuitems->find(p);

	drawItem(p, False, True);
	if (item->submenu())
	  if (item->submenu()->isVisible() &&
	      (! item->submenu()->isTorn())) {
	    item->submenu()->internal_hide();
	    which_sub = -1;
	  }
      }

      which_press = i;
      which_sbl = sbl;

      BasemenuItem *itmp = menuitems->find(w);

      if (itmp->submenu())
	drawSubmenu(w);
      else
	drawItem(w, (itmp->isEnabled()), True);
    }
  }
}


void Basemenu::exposeEvent(XExposeEvent *ee) {
  if (ee->window == menu.title) {
    redrawTitle();
  } else if (ee->window == menu.frame) {
    LinkedListIterator<BasemenuItem> it(menuitems);

    // this is a compilicated algorithm... lets do it step by step...
    // first... we see in which sub level the expose starts... and how many
    // items down in that sublevel

    int sbl = (ee->x / menu.item_w), id = (ee->y / menu.item_h),
      // next... figure out how many sublevels over the redraw spans
      sbl_d = ((ee->x + ee->width) / menu.item_w),
      // then we see how many items down to redraw
      id_d = ((ee->y + ee->height) / menu.item_h);

    if (id_d > menu.persub) id_d = menu.persub;

    // draw the sublevels and the number of items the exposure spans
    int i, ii;
    for (i = sbl; i <= sbl_d; i++) {
      // set the iterator to the first item in the sublevel needing redrawing
      it.set(id + (i * menu.persub));
      for (ii = id; ii <= id_d && it.current(); it++, ii++) {
	int index = ii + (i * menu.persub);
	// redraw the item
	drawItem(index, (which_sub == index));
      }
    }
  }
}


void Basemenu::enterNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {
    menu.x_shift = menu.x, menu.y_shift = menu.y;
    if (menu.x + menu.width > screen->getWidth()) {
      menu.x_shift = screen->getWidth() - menu.width -
        screen->getBorderWidth();
      shifted = True;
    } else if (menu.x < 0) {
      menu.x_shift = -screen->getBorderWidth();
      shifted = True;
    }

    if (menu.y + menu.height > screen->getHeight()) {
      menu.y_shift = screen->getHeight() - menu.height -
        screen->getBorderWidth();
      shifted = True;
    } else if (menu.y + (signed) menu.title_h < 0) {
      menu.y_shift = -screen->getBorderWidth();
      shifted = True;
    }

    if (shifted)
      XMoveWindow(display, menu.window, menu.x_shift, menu.y_shift);

    if (which_sub != -1) {
      BasemenuItem *tmp = menuitems->find(which_sub);
      if (tmp->submenu()->isVisible()) {
	int sbl = (ce->x / menu.item_w), i = (ce->y / menu.item_h),
	  w = (sbl * menu.persub) + i;

	if (w != which_sub && (! tmp->submenu()->isTorn())) {
	  tmp->submenu()->internal_hide();

	  drawItem(which_sub, False, True);
	  which_sub = -1;
	}
      }
    }
  }
}


void Basemenu::leaveNotifyEvent(XCrossingEvent *ce) {
  if (ce->window == menu.frame) {
    if (which_press != -1 && which_sbl != -1 && menuitems->count() > 0) {
      int p = (which_sbl * menu.persub) + which_press;

      drawItem(p, (p == which_sub), True);

      which_sbl = which_press = -1;
    }

    if (shifted) {
      XMoveWindow(display, menu.window, menu.x, menu.y);
      shifted = False;

      if (which_sub != -1) drawSubmenu(which_sub);
    }
  }
}


void Basemenu::reconfigure(void) {
  XSetWindowBackground(display, menu.window,
		       screen->getBorderColor()->getPixel());
  XSetWindowBorder(display, menu.window,
		   screen->getBorderColor()->getPixel());
  XSetWindowBorderWidth(display, menu.window, screen->getBorderWidth());

  menu.bevel_w = screen->getBevelWidth();
  update();
}

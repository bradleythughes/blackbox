// bASEMEnu.hh for Blackbox - an X11 Window manager
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

#ifndef   __Basemenu_hh
#define   __Basemenu_hh

#include <X11/Xlib.h>

// forward declarations
class Basemenu;
class BasemenuItem;

class Blackbox;
class BImageControl;
class BScreen;

#include "LinkedList.hh"

#ifdef    DEBUG
#  include "mem.h"
#endif // DEBUG


class Basemenu {
private:
  LinkedList<BasemenuItem> *menuitems;
  Blackbox *blackbox;
  Basemenu *parent;
  BImageControl *image_ctrl;
  BScreen *screen;

  Bool moving, visible, movable, torn, internal_menu, title_vis, shifted,
    hide_tree;
  Display *display;
  int which_sub, which_press, which_sbl, alignment;

  struct _menu {
    Pixmap frame_pixmap, title_pixmap, hilite_pixmap, sel_pixmap;
    Window window, frame, title;
    
    char *label;
    int x, y, x_move, y_move, x_shift, y_shift, sublevels, persub, minsub,
      grab_x, grab_y;
    unsigned int width, height, title_h, frame_h, item_w, item_h, bevel_w,
      bevel_h;
  } menu;
  

protected:
  inline BasemenuItem *find(int index) { return menuitems->find(index); }
  inline void setTitleVisibility(Bool b) { title_vis = b; }
  inline void setMovable(Bool b) { movable = b; }
  inline void setHideTree(Bool h) { hide_tree = h; }
  inline void setMinimumSublevels(int m) { menu.minsub = m; }

  virtual void itemSelected(int, int) = 0;
  virtual void drawSubmenu(int);
  virtual void drawItem(int, Bool = False, Bool = False);
  virtual void redrawTitle();
  virtual void internal_hide(void);


public:
  Basemenu(BScreen *);
  virtual ~Basemenu(void);

  inline const Bool &isTorn(void) const { return torn; }
  inline const Bool &isVisible(void) const { return visible; }
  
  inline BScreen *getScreen(void) { return screen; }

  inline const Window &getWindowID(void) const { return menu.window; }

  inline const char *getLabel(void) const { return menu.label; }

  int insert(char *, int = 0, char * = (char *) 0, int = -1);
  int insert(char **, int = -1, int = 0);
  int insert(char *, Basemenu *, int = -1);
  int remove(int);

  inline const int &getX(void) const { return menu.x; }
  inline const int &getY(void) const { return menu.y; }
  inline int getCount(void) { return menuitems->count(); }
  
  inline const unsigned int &getWidth(void) const { return menu.width; }
  inline const unsigned int &getHeight(void) const { return menu.height; }
  inline const unsigned int &getTitleHeight(void) const { return menu.title_h; }

  inline void setInternalMenu(void) { internal_menu = True; }
  inline void setAlignment(int a) { alignment = a; }
  inline void setTorn(void) { torn = True; }
  inline void removeParent(void)
    { if (internal_menu) parent = (Basemenu *) 0; }
  
  Bool hasSubmenu(int);
  Bool isItemSelected(int);
  Bool isItemEnabled(int);
  
  void buttonPressEvent(XButtonEvent *);
  void buttonReleaseEvent(XButtonEvent *);
  void motionNotifyEvent(XMotionEvent *);
  void enterNotifyEvent(XCrossingEvent *);
  void leaveNotifyEvent(XCrossingEvent *);
  void exposeEvent(XExposeEvent *);
  void reconfigure(void);
  void setLabel(char *n);
  void move(int, int);
  void update(void);
  void setItemSelected(int, Bool);
  void setItemEnabled(int, Bool);
   
  virtual void show(void);
  virtual void hide(void);

  enum { AlignDontCare = 1, AlignTop, AlignBottom };
  enum { Right = 1, Left };
  enum { Empty = 0, Square, Triangle, Diamond };
};


class BasemenuItem {
private:
  Basemenu *s;
  char **u, *l, *e;
  int f, enabled, selected;

  friend Basemenu;


protected:


public:
  BasemenuItem(char *lp, int fp, char *ep = (char *) 0) {
    l = lp;
    e = ep;
    s = 0;
    f = fp;
    u = 0;
    enabled = 1;
    selected = 0;

#ifdef    DEBUG
    allocate(sizeof(BasemenuItem), __FUNCTION__);
#endif // DEBUG
  }

  BasemenuItem(char *lp, Basemenu *mp) {
    l = lp;
    s = mp;
    e = 0;
    f = 0;
    u = 0;
    enabled = 1;
    selected = 0;

#ifdef    DEBUG
    allocate(sizeof(BasemenuItem), __FUNCTION__);
#endif // DEBUG
  }

  BasemenuItem(char **up, int fp) {
    u = up;
    l = e = 0;
    f = fp;
    s = 0;
    enabled = 1;
    selected = 0;

#ifdef    DEBUG
    allocate(sizeof(BasemenuItem), __FUNCTION__);
#endif // DEBUG
  }

#ifdef    DEBUG
  ~BasemenuItem(void) {
    deallocate(sizeof(BasemenuItem), __FUNCTION__);
  }
#endif // DEBUG

  inline char *exec(void) { return e; }
  inline char *label(void) { return l; }
  inline char **ulabel(void) { return u; }
  inline const int &function(void) const { return f; }
  inline Basemenu *submenu(void) { return s; }

  inline const int &isEnabled(void) const { return enabled; }
  inline void setEnabled(int e) { enabled = e; }
  inline const int &isSelected(void) const { return selected; }
  inline void setSelected(int s) { selected = s; }
};


#endif // __Basemenu_hh

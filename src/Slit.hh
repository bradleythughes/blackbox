// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Slit.hh for Blackbox - an X11 Window manager
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

#ifndef   __Slit_hh
#define   __Slit_hh

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <list>

#include "Screen.hh"
#include "Basemenu.hh"

// forward declaration
class Slit;
class Slitmenu;

class Slitmenu : public Basemenu
{
public:
  Slitmenu(Slit *);

  Basemenu *getDirectionmenu() const { return directionmenu; }
  Basemenu *getPlacementmenu() const { return placementmenu; }

  virtual void reconfigure();
  virtual void hide();

protected:
  virtual void itemClicked(const Item &item, int button);

private:
  class Directionmenu : public Basemenu {
  private:
    Slitmenu *slitmenu;

    Directionmenu(const Directionmenu&);
    Directionmenu& operator=(const Directionmenu&);

  protected:
    virtual void itemClicked(const Item &, int);

  public:
    Directionmenu(Slitmenu *sm);
  };

  class Placementmenu : public Basemenu {
  private:
    Slitmenu *slitmenu;

  protected:
    virtual void itemClicked(const Item &, int);

  public:
    Placementmenu(Slitmenu *sm);
  };

  Directionmenu *directionmenu;
  Placementmenu *placementmenu;

  Slit *slit;

  friend class Directionmenu;
  friend class Placementmenu;
  friend class Slit;
};

class Slit : public TimeoutHandler {
private:
  struct SlitClient {
    Window window, client_window, icon_window;

    int x, y;
    unsigned int width, height;
  };
  typedef std::list<SlitClient*> SlitClientList;

  Bool on_top, hidden, do_auto_hide;

  Blackbox *blackbox;
  BScreen *screen;
  BTimer *timer;
  NETStrut strut;

  SlitClientList clientList;
  Slitmenu *slitmenu;

  struct SlitFrame {
    Pixmap pixmap;
    Window window;

    int x, y, x_hidden, y_hidden;
    unsigned int width, height;
  } frame;

  friend class Slitmenu;
  friend class Slitmenu::Directionmenu;
  friend class Slitmenu::Placementmenu;

  Slit(const Slit&);
  Slit& operator=(const Slit&);

public:
  Slit(BScreen *scr);
  virtual ~Slit(void);

  inline const Bool isOnTop(void) const { return on_top; }
  inline const Bool isHidden(void) const { return hidden; }
  inline const Bool doAutoHide(void) const { return do_auto_hide; }

  inline Slitmenu *getMenu(void) { return slitmenu; }

  inline const Window windowID(void) const { return frame.window; }

  inline const int getX(void) const
  { return ((hidden) ? frame.x_hidden : frame.x); }
  inline const int getY(void) const
  { return ((hidden) ? frame.y_hidden : frame.y); }

  inline const unsigned int width() const { return frame.width; }
  inline const unsigned int height() const { return frame.height; }

  void addClient(Window w);
  void removeClient(SlitClient *client, Bool remap = True);
  void removeClient(Window w, Bool remap = True);
  void reconfigure(void);
  void reposition(void);
  void shutdown(void);

  void buttonPressEvent(XButtonEvent *e);
  void buttonReleaseEvent(XButtonEvent *e);
  void enterNotifyEvent(XCrossingEvent * /*unused*/);
  void leaveNotifyEvent(XCrossingEvent * /*unused*/);
  void configureRequestEvent(XConfigureRequestEvent *e);

  virtual void timeout();

  enum { Vertical = 1, Horizontal };
  enum { TopLeft = 1, CenterLeft, BottomLeft, TopCenter, BottomCenter,
         TopRight, CenterRight, BottomRight };
};


#endif // __Slit_hh

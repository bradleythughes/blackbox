// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2003 Sean 'Shaleh' Perry <shaleh@debian.org>
// Copyright (c) 1997 - 2000, 2002 - 2003
//         Bradley T Hughes <bhughes at trolltech.com>
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

#ifndef   __Screen_hh
#define   __Screen_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifdef    TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
#else // !TIME_WITH_SYS_TIME
#  ifdef    HAVE_SYS_TIME_H
#    include <sys/time.h>
#  else // !HAVE_SYS_TIME_H
#    include <time.h>
#  endif // HAVE_SYS_TIME_H
#endif // TIME_WITH_SYS_TIME
}

#include <list>
#include <vector>
typedef std::vector<Window> WindowStack;

#include "Configmenu.hh"
#include "Netwm.hh"
#include "blackbox.hh"
#include "BlackboxResource.hh"

// forward declaration
class Rootmenu;
class Slit;

typedef std::list<BlackboxWindow*> BlackboxWindowList;
typedef std::list<Window> WindowList;

class BScreen : public bt::NoCopy, public bt::EventHandler {
private:
  bool root_colormap_installed, managed, geom_visible;
  GC opGC;
  Pixmap geom_pixmap;
  Window geom_window;

  const bt::ScreenInfo& screen_info;
  Blackbox *blackbox;
  Configmenu *configmenu;
  Rootmenu *rootmenu;

  BlackboxWindowList windowList;

  Slit *slit;
  Toolbar *toolbar;

  unsigned int geom_w, geom_h;

  bt::Rect usableArea;
  int cascade_x, cascade_y;

  typedef std::list<bt::Netwm::Strut*> StrutList;
  StrutList strutList;

  ScreenResource& _resource;

  void updateOpGC(void);
  void updateGeomWindow(void);

  bool parseMenuFile(FILE *file, Rootmenu *menu);

  void InitMenu(void);
  void LoadStyle(void);

  void manageWindow(Window w);
  void unmanageWindow(BlackboxWindow *w, bool remap);
  void placeWindow(BlackboxWindow *win);
  bool cascadePlacement(bt::Rect& win);
  bool smartPlacement(bt::Rect& win);

  void updateAvailableArea(void);
  void updateWorkareaHint(void) const;

public:
  enum { RoundBullet = 1, TriangleBullet, SquareBullet, NoBullet };
  enum { Restart = 1, RestartOther, Exit, Shutdown, Execute, Reconfigure,
         WindowShade, WindowIconify, WindowMaximize, WindowClose, WindowRaise,
         WindowLower, WindowKill, SetStyle };
  enum FocusModel { SloppyFocus, ClickToFocus };

  BScreen(Blackbox *bb, unsigned int scrn);
  ~BScreen(void);

  // information about the screen
  const bt::ScreenInfo &screenInfo(void) const { return screen_info; }
  unsigned int screenNumber(void) const { return screen_info.screenNumber(); }

  ScreenResource& resource(void) { return _resource; }
  void saveResource(void) { blackbox->resource().save(*blackbox); }

  bool isScreenManaged(void) const { return managed; }
  bool isRootColormapInstalled(void) const { return root_colormap_installed; }

  const GC &getOpGC(void) const { return opGC; }
  Blackbox *getBlackbox(void) { return blackbox; }

  Rootmenu *getRootmenu(void) { return rootmenu; }

  Slit *getSlit(void) { return slit; }

  Toolbar *getToolbar(void) { return toolbar; }

  void setRootColormapInstalled(bool r) { root_colormap_installed = r; }

  const bt::Rect& availableArea(void);
  void addStrut(bt::Netwm::Strut *strut);
  void removeStrut(bt::Netwm::Strut *strut);
  void updateStrut(void);

  void updateClientListHint(void) const;
  void updateClientListStackingHint(void) const;
  void updateDesktopNamesHint(void) const;
  void getDesktopNames(void);

  void addWindow(Window w);
  void releaseWindow(BlackboxWindow *w, bool remap);
  void reassociateWindow(BlackboxWindow *w, unsigned int wkspc_id);
  void propagateWindowName(const BlackboxWindow *w);
  void iconifyWindow(BlackboxWindow *w);
  void raiseWindow(BlackboxWindow *w);
  void lowerWindow(BlackboxWindow *w);

  void raiseWindows(const bt::Netwm::WindowList* const workspace_stack);

  void nextFocus(void) const;
  void prevFocus(void) const;
  void raiseFocus(void);
  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);
  void showPosition(int x, int y);
  void showGeometry(unsigned int gx, unsigned int gy);
  void hideGeometry(void);

  void clientMessageEvent(const XClientMessageEvent * const event);
  void buttonPressEvent(const XButtonEvent * const event);
  void configureRequestEvent(const XConfigureRequestEvent * const event);
};


#endif // __Screen_hh

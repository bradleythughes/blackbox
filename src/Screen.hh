// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Screen.hh for Blackbox - an X11 Window manager
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

#ifndef   __Screen_hh
#define   __Screen_hh

#include <X11/Xlib.h>

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

#include <list>

#include "Configmenu.hh"
#include "Iconmenu.hh"
#include "Netizen.hh"
#include "Rootmenu.hh"
#include "Style.hh"
#include "Timer.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"
#include "blackbox.hh"
#include "Image.hh"

#include <string>
using std::string;

// forward declarations
class Blackbox;
class BImageControl;
class Configmenu;
class Iconmenu;
class Netizen;
class Rootmenu;
class Style;
class Toolbar;
class Toolbar2;
class Workspace;
class Workspacemenu;
class BlackboxWindow;

struct NETStrut {
  NETStrut(void): top(0), bottom(0), left(0), right(0) {}
  unsigned int top, bottom, left, right;
};

// should probably be BScreenManager, so as not to confuse it with ScreenInfo
class BScreen {
public:
  enum WindowPlacement { SmartRow, SmartColumn, Cascade };
  enum FocusModel { SloppyFocus, ClickToFocus };

  BScreen(Blackbox *, int);
  ~BScreen(void);

  ScreenInfo *screenInfo(void) const { return screeninfo; }
  int screenNumber(void) const { return screeninfo->screenNumber(); }

  void initialize(void);

  void setWindowPlacement(WindowPlacement wp) { windowplacement = wp; }
  WindowPlacement windowPlacement(void) const { return windowplacement; }

  BImageControl *getImageControl(void) { return image_control; }
  Rootmenu *getRootmenu(void) { return rootmenu; }
  Toolbar *getToolbar(void) { return toolbar; }
  Toolbar2 *toolbar2(void) const { return _toolbar2; }

  Workspace *getWorkspace(unsigned int w) const;
  Workspace *getCurrentWorkspace(void) const { return current_workspace; }

  Workspacemenu *getWorkspacemenu(void) { return workspacemenu; }

  int getCurrentWorkspaceID(void) const;
  const unsigned int getWorkspaceCount(void) const { return workspacesList.size(); }
  const unsigned int getIconCount(void) const { return iconList.size(); }

  void setRootColormapInstalled(Bool r) { root_colormap_installed = r; }

  BStyle *style(void) { return &_style; }

  BlackboxWindow *icon(unsigned int) const;

  void addStrut(NETStrut *strut);
  void removeStrut(NETStrut *strut);
  void updateAvailableArea(void);
  const Rect& availableArea(void) const { return usableArea; }

  int addWorkspace(void);
  int removeLastWorkspace(void);

  void removeWorkspaceNames(void);
  void addWorkspaceName(char *);
  void addNetizen(Netizen *);
  void removeNetizen(Window);
  void addIcon(BlackboxWindow *);
  void removeIcon(BlackboxWindow *);
  void changeIconName(BlackboxWindow *);
  const char *getNameOfWorkspace(unsigned int) const;
  void changeWorkspaceID(unsigned int);
  void raiseWindows(Window *, int);
  void reassociateWindow(BlackboxWindow *, int, Bool);
  void prevFocus(void);
  void nextFocus(void);
  void raiseFocus(void);
  void reconfigure(void);
  void toggleFocusModel(FocusModel model);
  void rereadMenu(void);
  void shutdown(void);
  void showPosition(int, int);
  void showGeometry(unsigned int, unsigned int);
  void hideGeometry(void);

  void updateNetizenCurrentWorkspace(void);
  void updateNetizenWorkspaceCount(void);
  void updateNetizenWindowFocus(void);
  void updateNetizenWindowAdd(Window, unsigned long);
  void updateNetizenWindowDel(Window);
  void updateNetizenConfigNotify(XEvent *);
  void updateNetizenWindowRaise(Window);
  void updateNetizenWindowLower(Window);






  // this should all go away


  Bool isToolbarOnTop(void) const { return resource.toolbar_on_top; }
  Bool doToolbarAutoHide(void) const { return resource.toolbar_auto_hide; }
  Bool isSloppyFocus(void) const { return resource.sloppy_focus; }
  Bool isRootColormapInstalled(void) const { return root_colormap_installed; }
  Bool doAutoRaise(void) const { return resource.auto_raise; }
  Bool isScreenManaged(void) const { return managed; }
  Bool doImageDither(void) const { return resource.image_dither; }
  Bool doOrderedDither(void) const { return resource.ordered_dither; }
  Bool doOpaqueMove(void) const { return resource.opaque_move; }
  Bool doFullMax(void) const { return resource.full_max; }
  Bool doFocusNew(void) const { return resource.focus_new; }
  Bool doFocusLast(void) const { return resource.focus_last; }

  Bool isSlitOnTop(void) const { return resource.slit_on_top; }
  Bool doSlitAutoHide(void) const { return resource.slit_auto_hide; }
  Slit *getSlit(void) { return slit; }
  const int &getSlitPlacement(void) const { return resource.slit_placement; }
  const int &getSlitDirection(void) const { return resource.slit_direction; }
  void saveSlitPlacement(int p) { resource.slit_placement = p; }
  void saveSlitDirection(int d) { resource.slit_direction = d; }
  void saveSlitOnTop(Bool t)    { resource.slit_on_top = t; }
  void saveSlitAutoHide(Bool t) { resource.slit_auto_hide = t; }

  int getNumberOfWorkspaces(void) const { return resource.workspaces; }
  int getToolbarPlacement(void) const { return resource.toolbar_placement; }
  int getToolbarWidthPercent(void) const { return resource.toolbar_width_percent; }
  int getEdgeSnapThreshold(void) const { return resource.edge_snap_threshold; }
  int getRowPlacementDirection(void) const { return resource.row_direction; }
  int getColPlacementDirection(void) const { return resource.col_direction; }

  void saveSloppyFocus(Bool s) { resource.sloppy_focus = s; }
  void saveAutoRaise(Bool a) { resource.auto_raise = a; }
  void saveWorkspaces(int w) { resource.workspaces = w; }
  void saveToolbarOnTop(Bool r) { resource.toolbar_on_top = r; }
  void saveToolbarAutoHide(Bool r) { resource.toolbar_auto_hide = r; }
  void saveToolbarWidthPercent(int w) { resource.toolbar_width_percent = w; }
  void saveToolbarPlacement(int p) { resource.toolbar_placement = p; }
  void saveEdgeSnapThreshold(int t) { resource.edge_snap_threshold = t; }
  void saveImageDither(Bool d) { resource.image_dither = d; }
  void saveOpaqueMove(Bool o) { resource.opaque_move = o; }
  void saveFullMax(Bool f) { resource.full_max = f; }
  void saveFocusNew(Bool f) { resource.focus_new = f; }
  void saveFocusLast(Bool f) { resource.focus_last = f; }

#ifdef    HAVE_STRFTIME
  char *getStrftimeFormat(void) { return resource.strftime_format; }
  void saveStrftimeFormat(char *);
#else // !HAVE_STRFTIME
  int getDateFormat(void) { return resource.date_format; }
  void saveDateFormat(int f) { resource.date_format = f; }
  Bool isClock24Hour(void) { return resource.clock24hour; }
  void saveClock24Hour(Bool c) { resource.clock24hour = c; }
#endif // HAVE_STRFTIME

protected:
  Bool parseMenuFile(FILE *, Rootmenu *);
  void InitMenu(void);

private:
  ScreenInfo *screeninfo;

  Blackbox *blackbox;

  Bool root_colormap_installed, managed;

  BImageControl *image_control;
  Configmenu *configmenu;
  Iconmenu *iconmenu;
  Rootmenu *rootmenu;

  typedef std::list<Rootmenu*> RootmenuList;
  RootmenuList rootmenuList;

  typedef std::list<Netizen*> NetizenList;
  NetizenList netizenList;

  typedef std::list<BlackboxWindow*> BlackboxWindowList;
  BlackboxWindowList iconList;

  Slit *slit;
  Toolbar *toolbar;
  Toolbar2 *_toolbar2;
  Workspace *current_workspace;
  Workspacemenu *workspacemenu;

  unsigned int geom_w, geom_h;
  unsigned long event_mask;

  Rect usableArea;

  typedef std::list<NETStrut*> StrutList;
  StrutList strutList;
  typedef std::list<std::string> WorkspaceNamesList;
  WorkspaceNamesList workspaceNames;
  typedef std::list<Workspace*> WorkspaceList;
  WorkspaceList workspacesList;

  struct resource {
    Bool toolbar_on_top, toolbar_auto_hide, sloppy_focus, auto_raise,
      auto_edge_balance, image_dither, ordered_dither, opaque_move, full_max,
      focus_new, focus_last;

    int workspaces, toolbar_placement, toolbar_width_percent, placement_policy,
      edge_snap_threshold, row_direction, col_direction;

    Bool slit_on_top, slit_auto_hide;
    int slit_placement, slit_direction;

#ifdef    HAVE_STRFTIME
    char *strftime_format;
#else // !HAVE_STRFTIME
    Bool clock24hour;
    int date_format;
#endif // HAVE_STRFTIME

  } resource;

  BStyle _style;
  WindowPlacement windowplacement;

  BScreen(const BScreen&);
  BScreen& operator=(const BScreen&);
};


#endif // __Screen_hh

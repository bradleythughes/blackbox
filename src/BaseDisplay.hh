// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// BaseDisplay.hh for Blackbox - an X11 Window manager
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

#ifndef   __BaseDisplay_hh
#define   __BaseDisplay_hh

#include <X11/Xlib.h>
#include <X11/Xatom.h>

// forward declaration
class BaseDisplay;
class ScreenInfo;

#include "LinkedList.hh"
#include "Timer.hh"
#include "Widget.hh"

#include <deque>
#include <string>

#define AttribShaded      (1l << 0)
#define AttribMaxHoriz    (1l << 1)
#define AttribMaxVert     (1l << 2)
#define AttribOmnipresent (1l << 3)
#define AttribWorkspace   (1l << 4)
#define AttribStack       (1l << 5)
#define AttribDecoration  (1l << 6)

#define StackTop          (0)
#define StackNormal       (1)
#define StackBottom       (2)

#define DecorNone         (0)
#define DecorNormal       (1)
#define DecorTiny         (2)
#define DecorTool         (3)

typedef struct _blackbox_hints {
  unsigned long flags, attrib, workspace, stack, decoration;
} BlackboxHints;

typedef struct _blackbox_attributes {
  unsigned long flags, attrib, workspace, stack, decoration;
  int premax_x, premax_y;
  unsigned int premax_w, premax_h;
} BlackboxAttributes;

#define PropBlackboxHintsElements      (5)
#define PropBlackboxAttributesElements (9)

typedef std::deque<Widget*> WidgetQueue;


class BaseDisplay : public TimerQueueManager {
private:
  struct cursor {
    Cursor session, move, ll_angle, lr_angle;
  } cursor;

  struct shape {
    Bool extensions;
    int event_basep, error_basep;
  } shape;

  unsigned int MaskList[8];
  size_t MaskListLength;

  Atom xa_wm_colormap_windows, xa_wm_protocols, xa_wm_state,
    xa_wm_delete_window, xa_wm_take_focus, xa_wm_change_state,
    motif_wm_hints;

  // NETAttributes
  Atom blackbox_attributes, blackbox_change_attributes, blackbox_hints;

  // NETStructureMessages
  Atom blackbox_structure_messages, blackbox_notify_startup,
    blackbox_notify_window_add, blackbox_notify_window_del,
    blackbox_notify_window_focus, blackbox_notify_current_workspace,
    blackbox_notify_workspace_count, blackbox_notify_window_raise,
    blackbox_notify_window_lower;

  // message_types for client -> wm messages
  Atom blackbox_change_workspace, blackbox_change_window_focus,
    blackbox_cycle_window_focus;

#ifdef    NEWWMSPEC

  // root window properties
  Atom net_supported, net_client_list, net_client_list_stacking,
    net_number_of_desktops, net_desktop_geometry, net_desktop_viewport,
    net_current_desktop, net_desktop_names, net_active_window, net_workarea,
    net_supporting_wm_check, net_virtual_roots;

  // root window messages
  Atom net_close_window, net_wm_moveresize;

  // application window properties
  Atom net_properties, net_wm_name, net_wm_desktop, net_wm_window_type,
    net_wm_state, net_wm_strut, net_wm_icon_geometry, net_wm_icon, net_wm_pid,
    net_wm_handled_icons;

  // application protocols
  Atom net_wm_ping;

#endif // NEWWMSPEC

  Bool _startup, _shutdown;
  Display *_display;
  LinkedList<ScreenInfo> *screenInfoList;
  TimerQueue timerList;

  char *display_name, *application_name;
  int screen_count, colors_per_channel;


protected:
  virtual void process_event(XEvent *);

  // the masks of the modifiers which are ignored in button events.
  int NumLockMask, ScrollLockMask;


public:
  BaseDisplay(char *, char * = 0);
  virtual ~BaseDisplay(void);

  static BaseDisplay *instance();

  inline const Atom &getWMChangeStateAtom(void) const
  { return xa_wm_change_state; }
  inline const Atom &getWMStateAtom(void) const
  { return xa_wm_state; }
  inline const Atom &getWMDeleteAtom(void) const
  { return xa_wm_delete_window; }
  inline const Atom &getWMProtocolsAtom(void) const
  { return xa_wm_protocols; }
  inline const Atom &getWMTakeFocusAtom(void) const
  { return xa_wm_take_focus; }
  inline const Atom &getWMColormapAtom(void) const
  { return xa_wm_colormap_windows; }
  inline const Atom &getMotifWMHintsAtom(void) const
  { return motif_wm_hints; }

  // this atom is for normal app->WM hints about decorations, stacking,
  // starting workspace etc...
  inline const Atom &getBlackboxHintsAtom(void) const
  { return blackbox_hints;}

  // these atoms are for normal app->WM interaction beyond the scope of the
  // ICCCM...
  inline const Atom &getBlackboxAttributesAtom(void) const
  { return blackbox_attributes; }
  inline const Atom &getBlackboxChangeAttributesAtom(void) const
  { return blackbox_change_attributes; }

  // these atoms are for window->WM interaction, with more control and
  // information on window "structure"... common examples are
  // notifying apps when windows are raised/lowered... when the user changes
  // workspaces... i.e. "pager talk"
  inline const Atom &getBlackboxStructureMessagesAtom(void) const
  { return blackbox_structure_messages; }

  // *Notify* portions of the NETStructureMessages protocol
  inline const Atom &getBlackboxNotifyStartupAtom(void) const
  { return blackbox_notify_startup; }
  inline const Atom &getBlackboxNotifyWindowAddAtom(void) const
  { return blackbox_notify_window_add; }
  inline const Atom &getBlackboxNotifyWindowDelAtom(void) const
  { return blackbox_notify_window_del; }
  inline const Atom &getBlackboxNotifyWindowFocusAtom(void) const
  { return blackbox_notify_window_focus; }
  inline const Atom &getBlackboxNotifyCurrentWorkspaceAtom(void) const
  { return blackbox_notify_current_workspace; }
  inline const Atom &getBlackboxNotifyWorkspaceCountAtom(void) const
  { return blackbox_notify_workspace_count; }
  inline const Atom &getBlackboxNotifyWindowRaiseAtom(void) const
  { return blackbox_notify_window_raise; }
  inline const Atom &getBlackboxNotifyWindowLowerAtom(void) const
  { return blackbox_notify_window_lower; }

  // atoms to change that request changes to the desktop environment during
  // runtime... these messages can be sent by any client... as the sending
  // client window id is not included in the ClientMessage event...
  inline const Atom &getBlackboxChangeWorkspaceAtom(void) const
  { return blackbox_change_workspace; }
  inline const Atom &getBlackboxChangeWindowFocusAtom(void) const
  { return blackbox_change_window_focus; }
  inline const Atom &getBlackboxCycleWindowFocusAtom(void) const
  { return blackbox_cycle_window_focus; }

#ifdef    NEWWMSPEC

  // root window properties
  inline const Atom &getNETSupportedAtom(void) const
  { return net_supported; }
  inline const Atom &getNETClientListAtom(void) const
  { return net_client_list; }
  inline const Atom &getNETClientListStackingAtom(void) const
  { return net_client_list_stacking; }
  inline const Atom &getNETNumberOfDesktopsAtom(void) const
  { return net_number_of_desktops; }
  inline const Atom &getNETDesktopGeometryAtom(void) const
  { return net_desktop_geometry; }
  inline const Atom &getNETDesktopViewportAtom(void) const
  { return net_desktop_viewport; }
  inline const Atom &getNETCurrentDesktopAtom(void) const
  { return net_current_desktop; }
  inline const Atom &getNETDesktopNamesAtom(void) const
  { return net_desktop_names; }
  inline const Atom &getNETActiveWindowAtom(void) const
  { return net_active_window; }
  inline const Atom &getNETWorkareaAtom(void) const
  { return net_workarea; }
  inline const Atom &getNETSupportingWMCheckAtom(void) const
  { return net_supporting_wm_check; }
  inline const Atom &getNETVirtualRootsAtom(void) const
  { return net_virtual_roots; }

  // root window messages
  inline const Atom &getNETCloseWindowAtom(void) const
  { return net_close_window; }
  inline const Atom &getNETWMMoveResizeAtom(void) const
  { return net_wm_moveresize; }

  // application window properties
  inline const Atom &getNETPropertiesAtom(void) const
  { return net_properties; }
  inline const Atom &getNETWMNameAtom(void) const
  { return net_wm_name; }
  inline const Atom &getNETWMDesktopAtom(void) const
  { return net_wm_desktop; }
  inline const Atom &getNETWMWindowTypeAtom(void) const
  { return net_wm_window_type; }
  inline const Atom &getNETWMStateAtom(void) const
  { return net_wm_state; }
  inline const Atom &getNETWMStrutAtom(void) const
  { return net_wm_strut; }
  inline const Atom &getNETWMIconGeometryAtom(void) const
  { return net_wm_icon_geometry; }
  inline const Atom &getNETWMIconAtom(void) const
  { return net_wm_icon; }
  inline const Atom &getNETWMPidAtom(void) const
  { return net_wm_pid; }
  inline const Atom &getNETWMHandledIconsAtom(void) const
  { return net_wm_handled_icons; }

  // application protocols
  inline const Atom &getNETWMPingAtom(void) const
  { return net_wm_ping; }

#endif // NEWWMSPEC

  int screenCount() const { return screen_count; }
  ScreenInfo *screenInfo( int s ) { return (ScreenInfo *) screenInfoList->find( s ); }

  inline const Bool &hasShapeExtensions(void) const
  { return shape.extensions; }
  inline const Bool &doShutdown(void) const
  { return _shutdown; }
  inline const Bool &isStartup(void) const
  { return _startup; }

  inline const Cursor &getSessionCursor(void) const
  { return cursor.session; }
  inline const Cursor &getMoveCursor(void) const
  { return cursor.move; }
  inline const Cursor &getLowerLeftAngleCursor(void) const
  { return cursor.ll_angle; }
  inline const Cursor &getLowerRightAngleCursor(void) const
  { return cursor.lr_angle; }

  inline Display *x11Display(void) { return _display; }

  inline const char *getXDisplayName(void) const
  { return (const char *) display_name; }
  inline const char *getApplicationName(void) const
  { return (const char *) application_name; }

  inline const int &getShapeEventBase(void) const
  { return shape.event_basep; }

  inline void shutdown(void) { _shutdown = True; }
  inline void run(void) { _startup = _shutdown = False; }

  const Bool validateWindow(Window);

  void grabButton(unsigned int, unsigned int, Window, Bool, unsigned int, int,
                  int, Window, Cursor) const;
  void ungrabButton(unsigned int button, unsigned int modifiers,
                    Window grab_window) const;

  void eventLoop(void);

  // from TimerQueueManager interface
  virtual void addTimer(BTimer *timer);
  virtual void removeTimer(BTimer *timer);

  // another pure virtual... this is used to handle signals that BaseDisplay
  // doesn't understand itself
  virtual Bool handleSignal(int) = 0;

  operator Display *() const { return _display; }

  // popup handling
  void popup( Widget * );
  void popdown( Widget * );

private:
  Widget *popwidget;
  WidgetQueue popwidgets;
  bool popup_grab;
};

class ScreenInfo {
public:
  ScreenInfo( BaseDisplay *, int );

  const std::string &displayString() const { return displaystring; }

  BaseDisplay *display(void) const { return _display; }

  Visual *visual(void) const { return _visual; }
  Window rootWindow(void) const { return _rootwindow; }
  Colormap colormap(void) const { return _colormap; }

  int depth(void) const { return _depth; }
  int screenNumber(void) const { return _screen; }

  int width(void) const { return _rect.width; }
  int height(void) const { return _rect.height; }

  // ### TODO - make this use Rect instead
  const XRectangle &rect(void) const { return _rect; }

private:
  BaseDisplay *_display;
  Visual *_visual;
  Window _rootwindow;
  Colormap _colormap;

  int _depth, _screen;
  XRectangle _rect;

  std::string displaystring;
};

#endif // __BaseDisplay_hh

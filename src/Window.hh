// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.hh for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifndef   __Window_hh
#define   __Window_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef    SHAPE
#  include <X11/extensions/shape.h>
#endif // SHAPE
}

#include <string>

#include "BaseDisplay.hh"
#include "Timer.hh"
#include "Util.hh"
#include "Windowmenu.hh"

#define MwmHintsFunctions     (1l << 0)
#define MwmHintsDecorations   (1l << 1)

#define MwmFuncAll            (1l << 0)
#define MwmFuncResize         (1l << 1)
#define MwmFuncMove           (1l << 2)
#define MwmFuncIconify        (1l << 3)
#define MwmFuncMaximize       (1l << 4)
#define MwmFuncClose          (1l << 5)

#define MwmDecorAll           (1l << 0)
#define MwmDecorBorder        (1l << 1)
#define MwmDecorHandle        (1l << 2)
#define MwmDecorTitle         (1l << 3)
#define MwmDecorMenu          (1l << 4) // not used
#define MwmDecorIconify       (1l << 5)
#define MwmDecorMaximize      (1l << 6)

// this structure only contains 3 elements... the Motif 2.0 structure contains
// 5... we only need the first 3... so that is all we will define
typedef struct MwmHints {
  unsigned long flags, functions, decorations;
} MwmHints;

#define PropMwmHintsElements  3

class BWindowGroup {
private:
  Blackbox *blackbox;
  Window group;
  BlackboxWindowList windowList;

public:
  BWindowGroup(Blackbox *b, Window _group);
  ~BWindowGroup(void);

  inline Window groupWindow(void) const { return group; }

  inline bool empty(void) const { return windowList.empty(); }

  void addWindow(BlackboxWindow *w) { windowList.push_back(w); }
  void removeWindow(BlackboxWindow *w) { windowList.remove(w); }

  /*
    find a window on the specified screen. the focused window (if any) is
    checked first, otherwise the first matching window found is returned.
    transients are returned only if allow_transients is True.
  */
  BlackboxWindow *find(BScreen *screen, bool allow_transients = False) const;
};


class BlackboxWindow : public TimeoutHandler, public EventHandler {
public:
  enum Function { Func_Resize   = (1l << 0),
                  Func_Move     = (1l << 1),
                  Func_Iconify  = (1l << 2),
                  Func_Maximize = (1l << 3),
                  Func_Close    = (1l << 4) };
  typedef unsigned char FunctionFlags;

  enum Decoration { Decor_Titlebar = (1l << 0),
                    Decor_Handle   = (1l << 1),
                    Decor_Border   = (1l << 2),
                    Decor_Iconify  = (1l << 3),
                    Decor_Maximize = (1l << 4),
                    Decor_Close    = (1l << 5) };
  typedef unsigned char DecorationFlags;

private:
  Blackbox *blackbox;
  BScreen *screen;
  BTimer *timer;

  Time lastButtonPressTime;  // used for double clicks, when were we clicked
  Windowmenu *windowmenu;

  unsigned int window_number;

  enum FocusMode { F_NoInput = 0, F_Passive,
                   F_LocallyActive, F_GloballyActive };
  enum WMLayer { LAYER_NORMAL, LAYER_FULLSCREEN, LAYER_ABOVE, LAYER_BELOW,
                  LAYER_DESKTOP };
  enum WMSkip { SKIP_NONE, SKIP_TASKBAR, SKIP_PAGER, SKIP_BOTH };

  struct WMState {
    bool modal,              // is modal? (must be dismissed to continue)
      shaded,                // is shaded?
      iconic,                // is iconified?
      fullscreen,            // is a full screen window
      moving,                // is moving?
      resizing,              // is resizing?
      focused,               // has focus?
      send_focus_message,    // should we send focus messages to our client?
      shaped;                // does the frame use the shape extension?
    unsigned int maximized;  // maximize is special, the number corresponds
                             // with a mouse button
                             // if 0, not maximized
                             // 1 = HorizVert, 2 = Vertical, 3 = Horizontal
    WMLayer layer;           // full screen, above, normal, below, desktop
    WMSkip skip;             // none, taskbar, pager, both
  };

  struct _client {
    Window window,                  // the client's window
      window_group;
    BlackboxWindow *transient_for;  // which window are we a transient for?
    BlackboxWindowList transientList; // which windows are our transients?

    std::string title, icon_title;

    Rect rect, premax;

    int old_bw;                       // client's borderwidth

    unsigned int
      min_width, min_height,        // can not be resized smaller
      max_width, max_height,        // can not be resized larger
      width_inc, height_inc,        // increment step
#if 0 // not supported at the moment
      min_aspect_x, min_aspect_y,   // minimum aspect ratio
      max_aspect_x, max_aspect_y,   // maximum aspect ratio
#endif
      base_width, base_height,
      win_gravity;

    unsigned long current_state, normal_hint_flags;
    unsigned int workspace;

    Netwm::Strut *strut;
    FocusMode focus_mode;
    WMState state;
    Atom window_type;
    FunctionFlags functions;
    /*
     * what decorations do we have?
     * this is based on the type of the client window as well as user input
     * the menu is not really decor, but it goes hand in hand with the decor
     */
    DecorationFlags decorations;
  } client;

  /*
   * client window = the application's window
   * frame window = the window drawn around the outside of the client window
   *                by the window manager which contains items like the
   *                titlebar and close button
   * title = the titlebar drawn above the client window, it displays the
   *         window's name and any buttons for interacting with the window,
   *         such as iconify, maximize, and close
   * label = the window in the titlebar where the title is drawn
   * buttons = maximize, iconify, close
   * handle = the bar drawn at the bottom of the window, which contains the
   *          left and right grips used for resizing the window
   * grips = the smaller reactangles in the handle, one of each side of it.
   *         When clicked and dragged, these resize the window interactively
   * border = the line drawn around the outside edge of the frame window,
   *          between the title, the bordered client window, and the handle.
   *          Also drawn between the grips and the handle
   */

  struct _frame {
    // u -> unfocused, f -> has focus
    unsigned long ulabel_pixel, flabel_pixel, utitle_pixel,
      ftitle_pixel, uhandle_pixel, fhandle_pixel, ubutton_pixel,
      fbutton_pixel, pbutton_pixel, uborder_pixel, fborder_pixel,
      ugrip_pixel, fgrip_pixel;
    Pixmap ulabel, flabel, utitle, ftitle, uhandle, fhandle,
      ubutton, fbutton, pbutton, ugrip, fgrip;

    Window window,       // the frame
      plate,             // holds the client
      title,
      label,
      handle,
      close_button, iconify_button, maximize_button,
      right_grip, left_grip;

    /*
     * size and location of the box drawn while the window dimensions or
     * location is being changed, ie. resized or moved
     */
    Rect changing;

    Rect rect;                  // frame geometry
    Netwm::Strut margin;        // margins between the frame and client,
                                // this has nothing to do with netwm, it is
                                // simply code reuse for similar functionality

    int grab_x, grab_y;         // where was the window when it was grabbed?

    unsigned int inside_w, inside_h, // window w/h without border_w
      title_h, label_w, label_h, handle_h,
      button_w, grip_w, mwm_border_w, border_w,
      bevel_w;
  } frame;

  BlackboxWindow(const BlackboxWindow&);
  BlackboxWindow& operator=(const BlackboxWindow&);

  bool getState(void);
  Window createToplevelWindow();
  Window createChildWindow(Window parent, unsigned long event_mask,
                           Cursor = None);

  void getNetwmHints(void);
  void getWMName(void);
  void getWMIconName(void);
  void getWMNormalHints(void);
  void getWMProtocols(void);
  void getWMHints(void);
  void getMWMHints(void);
  void getTransientInfo(void);
  void setNetWMAttributes(void);
  void associateClientWindow(void);
  void decorate(void);
  void decorateLabel(void);
  void positionButtons(bool redecorate_label = False);
  void positionWindows(void);
  void createHandle(void);
  void destroyHandle(void);
  void createTitlebar(void);
  void destroyTitlebar(void);
  void createCloseButton(void);
  void destroyCloseButton(void);
  void createIconifyButton(void);
  void destroyIconifyButton(void);
  void createMaximizeButton(void);
  void destroyMaximizeButton(void);
  void redrawWindowFrame(void) const;
  void redrawLabel(void) const;
  void redrawAllButtons(void) const;
  void redrawCloseButton(bool pressed) const;
  void redrawIconifyButton(bool pressed) const;
  void redrawMaximizeButton(bool pressed) const;
  void applyGravity(Rect &r);
  void restoreGravity(Rect &r);
  void setState(unsigned long new_state, bool closing = False);
  void upsize(void);

  enum Corner { TopLeft, TopRight };
  void constrain(Corner anchor, unsigned int *pw = 0, unsigned int *ph = 0);

public:
  BlackboxWindow(Blackbox *b, Window w, BScreen *s);
  virtual ~BlackboxWindow(void);

  inline bool isTransient(void) const { return client.transient_for != 0; }
  inline bool isFocused(void) const { return client.state.focused; }
  inline bool isVisible(void) const
  { return (! (client.current_state == WithdrawnState ||
               client.state.iconic)); }
  inline bool isIconic(void) const { return client.state.iconic; }
  inline bool isShaded(void) const { return client.state.shaded; }
  inline bool isMaximized(void) const { return client.state.maximized; }
  inline bool isModal(void) const { return client.state.modal; }
  inline bool isIconifiable(void) const
  { return client.functions & Func_Iconify; }
  inline bool isMaximizable(void) const
  { return client.functions & Func_Maximize; }
  inline bool isResizable(void) const
  { return client.functions & Func_Resize; }
  inline bool isClosable(void) const { return client.functions & Func_Close; }

  inline bool hasTitlebar(void) const
  { return client.decorations & Decor_Titlebar; }

  inline const BlackboxWindowList &getTransients(void) const
  { return client.transientList; }
  BlackboxWindow *getTransientFor(void) const;

  inline BScreen *getScreen(void) const { return screen; }

  inline Window getFrameWindow(void) const { return frame.window; }
  inline Window getClientWindow(void) const { return client.window; }
  inline Window getGroupWindow(void) const { return client.window_group; }

  inline Windowmenu* getWindowmenu(void) const { return windowmenu; }

  inline const char *getTitle(void) const
  { return client.title.c_str(); }
  inline const char *getIconTitle(void) const
  { return client.icon_title.c_str(); }

  inline unsigned int getWorkspaceNumber(void) const
  { return client.workspace; }
  inline unsigned int getWindowNumber(void) const { return window_number; }

  inline const Rect &frameRect(void) const { return frame.rect; }
  inline const Rect &clientRect(void) const { return client.rect; }

  inline unsigned int getTitleHeight(void) const
  { return frame.title_h; }

  inline void setWindowNumber(int n) { window_number = n; }

  inline void setModal(bool flag) { client.state.modal = flag; }

  bool validateClient(void) const;
  bool setInputFocus(void);

  void setFocusFlag(bool focus);
  void iconify(void);
  void deiconify(bool reassoc = True, bool raise = True);
  void show(void);
  void close(void);
  void withdraw(void);
  void maximize(unsigned int button);
  void remaximize(void);
  void shade(void);
  void reconfigure(void);
  void grabButtons(void);
  void ungrabButtons(void);
  void installColormap(bool install);
  void restore(bool remap);
  void configure(int dx, int dy, unsigned int dw, unsigned int dh);
  void setWorkspace(unsigned int n);

  void clientMessageEvent(const XClientMessageEvent * const ce);
  void buttonPressEvent(const XButtonEvent * const be);
  void buttonReleaseEvent(const XButtonEvent * const re);
  void motionNotifyEvent(const XMotionEvent * const me);
  void destroyNotifyEvent(const XDestroyWindowEvent * const /*unused*/);
  void mapRequestEvent(const XMapRequestEvent * const /*unused*/);
  void unmapNotifyEvent(const XUnmapEvent * const /*unused*/);
  void reparentNotifyEvent(const XReparentEvent * const /*unused*/);
  void propertyNotifyEvent(const XPropertyEvent * const pe);
  void exposeEvent(const XExposeEvent * const ee);
  void configureRequestEvent(const XConfigureRequestEvent * const cr);
  void enterNotifyEvent(const XCrossingEvent * const ce);
  void leaveNotifyEvent(const XCrossingEvent * const /*unused*/);

#ifdef    SHAPE
  void configureShape(void);
  void shapeEvent(const XShapeEvent * const /*unused*/);
#endif // SHAPE

  virtual void timeout(void);
};


#endif // __Window_hh

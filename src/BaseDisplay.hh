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

#include <list>

// forward declaration
class BaseDisplay;
class ScreenInfo;

#include "Timer.hh"
#include "Widget.hh"

#include <deque>
#include <string>


typedef std::deque<Widget*> WidgetQueue;

class BaseDisplay : public TimerQueueManager {
private:
  struct BShape {
    Bool extensions;
    int event_basep, error_basep;
  };
  BShape shape;

  unsigned int MaskList[8];
  size_t MaskListLength;
  Bool _startup, _shutdown;
  Display *_display;
  typedef std::list<ScreenInfo*> ScreenInfoList;
  ScreenInfoList screenInfoList;
  TimerQueue timerList;

  std::string display_name, application_name;
  int screen_count, colors_per_channel;

  // no copying!
  BaseDisplay(const BaseDisplay &none);
  BaseDisplay& operator=(const BaseDisplay &none);

protected:
  virtual void process_event(XEvent *event);

  // the masks of the modifiers which are ignored in button events.
  int NumLockMask, ScrollLockMask;


public:
  BaseDisplay(const std::string &app_name, const std::string &dpy_name);
  virtual ~BaseDisplay(void);

  static BaseDisplay *instance(void);

  inline Display *x11Display(void) { return _display; }
  inline operator Display *(void) const { return _display; }


  int screenCount(void) const { return screen_count; }
  ScreenInfo *screenInfo(unsigned int s);

  inline const Bool hasShapeExtensions(void) const
    { return shape.extensions; }
  inline const Bool doShutdown(void) const
    { return _shutdown; }
  inline const Bool isStartup(void) const
    { return _startup; }

  inline const std::string &displayName(void) const
    { return display_name; }
  inline const std::string &applicationName(void) const
    { return application_name; }

  inline const int shapeEventBase(void) const
    { return shape.event_basep; }

  inline void shutdown(void) { _shutdown = True; }
  inline void run(void) { _startup = _shutdown = False; }

  void grabButton(unsigned int button, unsigned int modifiers,
                  Window grab_window, Bool owner_events,
                  unsigned int event_mask, int pointer_mode,
                  int keyboard_mode, Window confine_to, Cursor cursor) const;
  void ungrabButton(unsigned int button, unsigned int modifiers,
                    Window grab_window) const;

  void eventLoop(void);

  // from TimerQueueManager interface
  virtual void addTimer(BTimer *timer);
  virtual void removeTimer(BTimer *timer);

  // another pure virtual... this is used to handle signals that BaseDisplay
  // doesn't understand itself
  virtual const Bool handleSignal(int sig) = 0;

  // popup handling
  void popup(Widget *);
  void popdown(Widget *);

private:
  Widget *popwidget;
  WidgetQueue popwidgets;
  bool popup_grab;
};

class ScreenInfo {
public:
  ScreenInfo(BaseDisplay *d, int num);

  const std::string &displayString(void) const { return displaystring; }

  BaseDisplay *display(void) const { return _display; }

  Visual *visual(void) const { return _visual; }
  Window rootWindow(void) const { return _rootwindow; }
  Colormap colormap(void) const { return _colormap; }

  int depth(void) const { return _depth; }
  int screenNumber(void) const { return _screen; }

  int width(void) const { return _rect.width(); }
  int height(void) const { return _rect.height(); }

  const Rect &rect(void) const { return _rect; }

private:
  BaseDisplay *_display;
  Visual *_visual;
  Window _rootwindow;
  Colormap _colormap;

  int _depth, _screen;
  Rect _rect;

  std::string displaystring;
};

#endif // __BaseDisplay_hh

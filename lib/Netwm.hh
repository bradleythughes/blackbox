// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// netwm.hh for Blackbox - an X11 Window manager
// Copyright (c) 2002 Brad Hughes (bhughes@tcac.net),
//                    Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifndef _blackbox_netwm_hh
#define _blackbox_netwm_hh

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}

#include <string>
#include <vector>

class Netwm {
public:
  explicit Netwm(Display *_display);

  inline Atom utf8String(void) const { return utf8_string; }

  // root window properties
  inline Atom supported(void) const { return net_supported; }
  inline Atom clientList(void) const { return net_client_list; }
  inline Atom numberOfDesktops(void) const { return net_number_of_desktops; }
  inline Atom desktopGeometry(void) const { return net_desktop_geometry; }
  inline Atom currentDesktop(void) const { return net_current_desktop; }
  inline Atom desktopNames(void) const { return net_desktop_names; }
  inline Atom activeWindow(void) const { return net_active_window; }
  inline Atom workarea(void) const { return net_workarea; }
  inline Atom supportingWMCheck(void) const { return net_supporting_wm_check; }

  void setSupported(Window target, Atom atoms[], unsigned int count) const;
  void setClientList(Window target, const Window windows[],
                     unsigned int count) const;
  void setNumberOfDesktops(Window target, unsigned int count) const;
  void setDesktopGeometry(Window target,
                          unsigned int width, unsigned int height) const;
  void setCurrentDesktop(Window target, unsigned int number) const;
  void setDesktopNames(Window target, const std::string& names) const;
  std::vector<std::string> readDesktopNames(Window target) const;
  void setActiveWindow(Window target, Window data) const;
  void setWorkarea(Window target, unsigned long workareas[],
                   unsigned int count) const;
  void setSupportingWMCheck(Window target, Window data) const;

  // other root messages
  inline Atom closeWindow(void) const { return net_close_window; }
  inline Atom moveresizeWindow(void) const { return net_moveresize_window; }

  // application properties
  inline Atom wmName(void) const { return net_wm_name; }
  inline Atom wmIconName(void) const { return net_wm_icon_name; }
  inline Atom wmDesktop(void) const { return net_wm_desktop; }

  void setWMName(Window target, const std::string& name) const;
  bool readWMName(Window target, std::string& name) const;
  bool readWMIconName(Window target, std::string& name) const;
  bool readWMDesktop(Window target, unsigned int& desktop) const;

  // utility
  void removeProperty(Window target, Atom atom) const;

private:
  Netwm(const Netwm&);
  Netwm& operator=(const Netwm&);

  bool getUTF8StringProperty(Window target, Atom property,
                             std::string& value) const;
  bool getCardinalProperty(Window target, Atom property,
                           unsigned int& value) const;

  Display *display;
  Atom utf8_string,
    net_supported, net_client_list, net_number_of_desktops,
    net_desktop_geometry, net_current_desktop, net_desktop_names,
    net_active_window, net_workarea, net_supporting_wm_check,
    net_close_window, net_moveresize_window, net_wm_name, net_wm_icon_name,
    net_wm_desktop;
};

#endif // _blackbox_netwm_hh

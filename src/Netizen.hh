// Netizen.hh for Blackbox - An X11 Window Manager
// Copyright (c) 2001 Sean 'Shaleh' Perry <shaleh@debian.org>
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

#ifndef   __Netizen_hh
#define   __Netizen_hh

#include <X11/Xlib.h>

// forward declaration
class BaseDisplay;
class BScreen;


class Netizen {
private:
    BaseDisplay *display;
    BScreen *screen;
    Window window;
    XEvent event;

protected:

public:
    Netizen(BScreen *, Window);

    inline const Window &windowID(void) const { return window; }

    void sendWorkspaceCount(void);
    void sendCurrentWorkspace(void);

    void sendWindowFocus(Window);
    void sendWindowAdd(Window, unsigned long);
    void sendWindowDel(Window);
    void sendWindowRaise(Window);
    void sendWindowLower(Window);

    void sendConfigNotify(XEvent *);
};


#endif // __Netizen_hh

// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Clientmenu.cc for Blackbox - an X11 Window manager
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

// stupid macros needed to access some functions in version 2 of the GNU C
// library
#ifndef   _GNU_SOURCE
#define   _GNU_SOURCE
#endif // _GNU_SOURCE

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Workspacemenu.hh"


Clientmenu::Clientmenu(Workspace *ws)
  : Basemenu( ws->getScreen()->screen() )
{
  wkspc = ws;
  screen = wkspc->getScreen();
}


void Clientmenu::itemClicked( const Point &, const Item &item, int button )
{
  if (button > 2)
    return;

  BlackboxWindow *win = wkspc->getWindow(item.index());
  if (win) {
    if (button == 1) {
      if (! wkspc->isCurrent()) wkspc->setCurrent();
    } else if (button == 2) {
      if (! wkspc->isCurrent()) win->deiconify(True, False);
    }
    wkspc->raiseWindow(win);
    win->setInputFocus();
  }
}

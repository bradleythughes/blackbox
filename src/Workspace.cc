// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Workspace.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <string.h>
#endif // STDC_HEADERS

#include "i18n.hh"
#include "blackbox.hh"
#include "Clientmenu.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Workspace.hh"
#include "Windowmenu.hh"

using std::string;


Workspace::Workspace(BScreen *scrn, unsigned int i) : screen(scrn) {
  screen = scrn;

  cascade_x = cascade_y = 0;

  id = i;

  clientmenu = new Clientmenu(this, screen->screenNumber());

  lastfocus = (BlackboxWindow *) 0;

  const char *tmp = screen->getNameOfWorkspace(id);
  setName(tmp);
}


Workspace::~Workspace(void) {}


const int Workspace::addWindow(BlackboxWindow *w, Bool place)
{
  if (! w) return -1;

  if (place)
    placeWindow(w);

  w->setWorkspace(id);
  w->setWindowNumber(windowList.size());

  stackingList.push_front(w);
  windowList.push_back(w);

  clientmenu->insert(*w->getTitle());

  screen->updateNetizenWindowAdd(w->getClientWindow(), id);

  raiseWindow(w);

  return w->getWindowNumber();
}

const int Workspace::removeWindow(BlackboxWindow *w)
{
  if (! w)
    return -1;

  stackingList.remove(w);

  if (w->isFocused()) {
    BlackboxWindow *newfocus = 0;
    if (w->isTransient()) newfocus = w->getTransientFor();
    if (! newfocus) newfocus = stackingList.front();
    if (! newfocus || ! newfocus->setInputFocus()) {
      Blackbox::instance()->setFocusedWindow(0);
    }
  }

  if (lastfocus == w)
    lastfocus = (BlackboxWindow *) 0;

  windowList.remove(w);
  clientmenu->remove(w->getWindowNumber());

  screen->updateNetizenWindowDel(w->getClientWindow());

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  for (int i = 0; it != end; ++it, ++i)
    (*it)->setWindowNumber(i);

  return windowList.size();
}


void Workspace::showAll(void) {
  std::for_each(stackingList.begin(), stackingList.end(),
                std::mem_fun(&BlackboxWindow::show));
}


void Workspace::hideAll(void) {
  /* why is the order they are withdrawn important? */

  /* make a temporary list in reverse order */
  BlackboxWindowList lst(stackingList.rbegin(), stackingList.rend());

  BlackboxWindowList::iterator it = lst.begin();
  const BlackboxWindowList::iterator end = lst.end();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (! bw->isStuck())
      bw->withdraw();
  }
}


void Workspace::removeAll(void) {
  while (! windowList.empty())
    windowList.front()->iconify();
}


void Workspace::raiseWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;

  while (bottom->isTransient()) {
    BlackboxWindow *bw = bottom->getTransientFor();
    if (! bw) break;
    bottom = bw;
  }

  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();
    i++;
  }

  Window *nstack = new Window[i], *curr = nstack;
  Workspace *wkspc;

  win = bottom;
  while (True) {
    *(curr++) = win->getFrameWindow();
    screen->updateNetizenWindowRaise(win->getClientWindow());

    if (! win->isIconic()) {
      wkspc = screen->getWorkspace(win->getWorkspaceNumber());
      wkspc->stackingList.remove(win);
      wkspc->stackingList.push_front(win);
    }

    if (! win->hasTransient() || ! win->getTransient())
      break;
    win = win->getTransient();
  }

  screen->raiseWindows(nstack, i);

  delete [] nstack;
}


void Workspace::lowerWindow(BlackboxWindow *w) {
  BlackboxWindow *win = (BlackboxWindow *) 0, *bottom = w;

  while (bottom->isTransient()) {
    BlackboxWindow *bw = bottom->getTransientFor();
    if (! bw) break;
    bottom = bw;
  }

  int i = 1;
  win = bottom;
  while (win->hasTransient() && win->getTransient()) {
    win = win->getTransient();

    i++;
  }

  Window *nstack = new Window[i], *curr = nstack;
  Workspace *wkspc;

  while (True) {
    *(curr++) = win->getFrameWindow();
    screen->updateNetizenWindowLower(win->getClientWindow());

    if (! win->isIconic()) {
      wkspc = screen->getWorkspace(win->getWorkspaceNumber());
      wkspc->stackingList.remove(win);
      wkspc->stackingList.push_back(win);
    }

    win = win->getTransientFor();
    if (! win)
      break;
  }

  XLowerWindow(*BaseDisplay::instance(), *nstack);
  XRestackWindows(*BaseDisplay::instance(), nstack, i);

  delete [] nstack;
}


void Workspace::reconfigure(void) {
  clientmenu->reconfigure();

  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (bw->validateClient())
      bw->reconfigure();
  }
}


void Workspace::updateFocusModel(void) {
  BlackboxWindowList::iterator it = windowList.begin();
  const BlackboxWindowList::iterator end = windowList.end();
  for (; it != end; ++it) {
    BlackboxWindow *bw = *it;
    if (bw->validateClient())
      bw->updateFocusModel();
  }
}


BlackboxWindow *Workspace::getWindow(unsigned int index) const {
  if (index < windowList.size()) {
    BlackboxWindowList::const_iterator it = windowList.begin();
    for(; index > 0; --index, ++it); /* increment to index */
    return *it;
  }
  return 0;
}



void Workspace::changeWindow(BlackboxWindow *window)
{
  clientmenu->change(window->getWindowNumber(), *window->getTitle());
  screen->getToolbar()->redrawWindowLabel(True);
}


Bool Workspace::isCurrent(void) const {
  return (id == screen->getCurrentWorkspaceID());
}


Bool Workspace::isLastWindow(const BlackboxWindow* const w) const {
  return (w == windowList.back());
}

void Workspace::setCurrent(void) {
  screen->changeWorkspaceID(id);
}


void Workspace::setName(const char* const new_name) {
  if (new_name) {
    name = string(new_name);
  } else {
    char default_name[32];
    snprintf(default_name, 32, "Workspace %d", id + 1);
    name = i18n(WorkspaceSet, WorkspaceDefaultNameFormat, default_name);
  }

  clientmenu->setTitle(name);
  clientmenu->showTitle();
}


void Workspace::shutdown(void) {
  while (! windowList.empty()) {
    BlackboxWindow *bw = windowList.front();
    bw->restore();
    delete bw;
    bw = 0;
  }
}

static int calc_possible_x(const Rect &r, int x, int width, int possible)
{
  if (r.right() > x)
    possible = possible < r.right() ? possible : r.right();
  if (r.left() - width > x)
    possible = possible < r.left() - width ? possible : r.left() - width;
  return possible;
}

static int calc_possible_y(const Rect &r, int y, int height, int possible)
{
  if (r.bottom() > y)
    possible = possible < r.bottom() ? possible : r.bottom();
  if (r.top() - height > y)
    possible = possible < r.top() - height ? possible : r.top() - height;
  return possible;
}

void Workspace::placeWindow(BlackboxWindow *win) {
  Rect avail = screen->availableArea();
  int x = avail.x(), y = avail.y(), place_x = x, place_y = y;
  int width = win->getWidth() + screen->style()->borderWidth() * 2,
     height = win->getHeight() + screen->style()->borderWidth() * 2;

  switch (screen->windowPlacement()) {
  case BScreen::SmartRow:
    {
      // when smart placing windows bigger than the available area, constrain them
      // to the size of the available area
      if (width >= avail.width())
        width = avail.width() - 1;
      if (height >= avail.height())
        height = avail.height() - 1;

      int overlap = 0, min = INT_MAX, possible = 0;

      do {
        if (y + height > avail.bottom()) {
          overlap = -1;
        } else if (x + width > avail.right()) {
          overlap = -2;
        } else {
          overlap = 0;
          Rect one(x, y, width, height);
          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect two(c->getXFrame(), c->getYFrame(),
                       c->getWidth() + screen->style()->borderWidth() * 2,
                       h + screen->style()->borderWidth() * 2);
              if (two.intersects(one)) {
                two &= one;
                overlap += (two.right() - two.left()) *
                           (two.bottom() - two.top());
              }
            }
          }
        }

        if (overlap == 0) {
          place_x = x;
          place_y = y;
          break;
        }

        if (overlap >= 0 && overlap < min) {
          min = overlap;
          place_x = x;
          place_y = y;
        }

        if (overlap > 0) {
          possible = avail.right();
          if (possible - width > x)
            possible -= width;

          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect r(c->getXFrame(), c->getYFrame(),
                     c->getWidth() + screen->style()->borderWidth() * 2,
                     h + screen->style()->borderWidth() * 2);

              if ((y < r.bottom()) && (r.top() < y + height)) {
                possible = calc_possible_x(r, x, width, possible);
              }
            }
          }

          x = possible;
        } else if (overlap == -2) {
          x = avail.x();
          possible = avail.bottom();
          if (possible - height > y)
            possible -= height;

          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect r(c->getXFrame(), c->getYFrame(),
                     c->getWidth() + screen->style()->borderWidth() * 2,
                     h + screen->style()->borderWidth() * 2);
              possible = calc_possible_y(r, y, height, possible);
            }
          }

          y = possible;
        }
      } while (overlap != 0 && overlap != -1);
      break;
    }

  case BScreen::SmartColumn:
    {
      // when smart placing windows bigger than the available area, constrain them
      // to the size of the available area
      if (width >= avail.width())
        width = avail.width() - 1;
      if (height >= avail.height())
        height = avail.height() - 1;

      int overlap = 0, min = INT_MAX, possible = 0;

      do {
        if (y + height > avail.bottom()) {
          overlap = -2;
        } else if (x + width > avail.right()) {
          overlap = -1;
        } else {
          overlap = 0;
          Rect one(x, y, width, height);
          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect two(c->getXFrame(), c->getYFrame(),
                       c->getWidth() + screen->style()->borderWidth() * 2,
                       h + screen->style()->borderWidth() * 2);
              if (two.intersects(one)) {
                two &= one;
                overlap += (two.right() - two.left()) *
                           (two.bottom() - two.top());
              }
            }
          }
        }

        if (overlap == 0) {
          place_x = x;
          place_y = y;
          break;
        }

        if (overlap >= 0 && overlap < min) {
          min = overlap;
          place_x = x;
          place_y = y;
        }

        if (overlap > 0) {
          possible = avail.bottom();
          if (possible - height > y)
            possible -= height;

          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect r(c->getXFrame(), c->getYFrame(),
                     c->getWidth() + screen->style()->borderWidth() * 2,
                     h + screen->style()->borderWidth() * 2);

              if ((x < r.right()) && (r.left() < x + width)) {
                possible = calc_possible_y(r, y, height, possible);
              }
            }
          }

          y = possible;
        } else if (overlap == -2) {
          y = avail.y();
          possible = avail.right();
          if (possible - width > x)
            possible -= width;

          BlackboxWindow *c;
          BlackboxWindowList::iterator it = windowList.begin();
          while (it != windowList.end()) {
            c = *it++;
            if (c != win) {
              int h = c->isShaded() ? c->getTitleHeight() : c->getHeight();
              Rect r(c->getXFrame(), c->getYFrame(),
                     c->getWidth() + screen->style()->borderWidth() * 2,
                     h + screen->style()->borderWidth() * 2);
              possible = calc_possible_x(r, x, width, possible);
            }
          }

          x = possible;
        }
      } while (overlap != 0 && overlap != -1);
      break;
    }

  case BScreen::Cascade:
    {
      place_x = cascade_x;
      place_y = cascade_y;

      // if the window is off the edge of the screen, center it
      if (place_x + width > avail.right() ||
          place_y + height > avail.bottom()) {
        place_x = (avail.width() - width) / 2;
        place_y = (avail.height() - height) / 2;
      }

      // go to next cascade placement point
      cascade_x += 32;
      cascade_y += 32;
      // wrap if necessary
      cascade_x %= avail.width();
      cascade_y %= avail.height();
      break;
    }
  }

  win->configure(place_x, place_y, win->getWidth(), win->getHeight());
}

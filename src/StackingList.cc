// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
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

#include "StackingList.hh"
#include "Window.hh"

void StackingList::dump(void) const {
#if 0
  StackingList::const_iterator it = stack.begin(), end = stack.end();
  BlackboxWindow *w;
  fprintf(stderr, "Stack:\n");
  for (; it != end; ++it) {
    w = *it;
    if (w)
      fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
    else
      fprintf(stderr, "zero\n");
  }
  fprintf(stderr, "the layers:\n");
  w = *fullscreen;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *above;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *normal;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *below;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
  w = *desktop;
  if (w)
    fprintf(stderr, "%s: 0x%lx\n", w->getTitle(), w->getClientWindow());
  else
    fprintf(stderr, "zero\n");
#endif
}


StackingList::StackingList(void) {
  desktop = stack.insert(stack.begin(), (BlackboxWindow*) 0);
  below = stack.insert(desktop, (BlackboxWindow*) 0);
  normal = stack.insert(below, (BlackboxWindow*) 0);
  above = stack.insert(normal, (BlackboxWindow*) 0);
  fullscreen = stack.insert(above, (BlackboxWindow*) 0);
}


StackingList::iterator&
StackingList::findLayer(const BlackboxWindow* const w) {
  if (w->getLayer() == BlackboxWindow::LAYER_NORMAL)
    return normal;
  else if (w->getLayer() == BlackboxWindow::LAYER_ABOVE)
    return above;
  else if (w->getLayer() == BlackboxWindow::LAYER_FULLSCREEN)
    return fullscreen;
  else if (w->getLayer() == BlackboxWindow::LAYER_BELOW)
    return below;
  else if (w->getLayer() == BlackboxWindow::LAYER_DESKTOP)
    return desktop;

  assert(0);
  return normal;
}


void StackingList::insert(BlackboxWindow* w) {
  assert(w);

  StackingList::iterator& it = findLayer(w);
  it = stack.insert(it, w);
}


void StackingList::append(BlackboxWindow* w) {
  assert(w);

  StackingList::iterator& it = findLayer(w);
  if (! *it) { // empty layer
    it = stack.insert(it, w);
    return;
  }

  // find the end of the layer (the zero pointer)
  StackingList::iterator tmp = it;
  for (; tmp != stack.end(); ++tmp) {
    if (! *tmp) {
      stack.insert(tmp, w);
      return;
    }
  }
  assert(0);
}


void StackingList::remove(BlackboxWindow* w) {
  assert(w);

  iterator& pos = findLayer(w);
  iterator it = std::find(pos, stack.end(), w);
  assert(it != stack.end());
  if (it == pos)
    ++pos;

  stack.erase(it);
  assert(stack.size() >= 5);
}


BlackboxWindow* StackingList::front(void) const {
  assert(stack.size() > 5);

  if (*fullscreen) return *fullscreen;
  if (*above) return *above;
  if (*normal) return *normal;
  if (*below) return *below;
  // we do not return desktop windows
  assert(0);

  // this point is never reached, but the compiler doesn't know that.
  return (BlackboxWindow*) 0;
}


BlackboxWindow* StackingList::back(void) const {
  assert(stack.size() > 5);

  WindowStack::const_iterator it = desktop, _end = stack.begin();
  for (--it; it != _end; --it) {
    if (*it)
      return *it;
  }
  assert(0);

  return (BlackboxWindow*) 0;
}

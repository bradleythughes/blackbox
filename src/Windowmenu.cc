// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
//
// Blackbox - an X11 window manager
//
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

#include "Windowmenu.hh"
#include "Window.hh"
#include "i18n.hh"


Windowmenu::Windowmenu(bt::Application &app, unsigned int screen,
                       BlackboxWindow *window)
  : bt::Menu(app, screen), _window(window) {

  insertItem(bt::i18n(WindowmenuSet, WindowmenuShade, "Shade"),
             Shade);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuIconify, "Iconify"),
             Iconify);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuMaximize, "Maximize"),
             Maximize);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuRaise, "Raise"),
             Raise);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuLower, "Lower"),
             Lower);
  insertItem("Always on Top", OnTop);
  insertItem("Always on Bottom", OnBottom);
  insertSeparator();
  insertItem(bt::i18n(WindowmenuSet, WindowmenuKillClient, "Kill Client"),
             Kill);
  insertItem(bt::i18n(WindowmenuSet, WindowmenuClose, "Close"),
             Close);
}


void Windowmenu::refresh(void) {
  setItemEnabled(Shade, _window->hasTitlebar());
  setItemChecked(Shade, _window->isShaded());

  setItemEnabled(Maximize, _window->isMaximizable());
  setItemChecked(Maximize, _window->isMaximized());

  setItemEnabled(Iconify, _window->isIconifiable());
  setItemEnabled(Close, _window->isClosable());

  setItemEnabled(OnTop,
                 _window->getLayer() < BlackboxWindow::LAYER_FULLSCREEN);
  setItemChecked(OnTop, _window->getLayer() == BlackboxWindow::LAYER_ABOVE);

  setItemEnabled(OnBottom,
                 _window->getLayer() < BlackboxWindow::LAYER_FULLSCREEN);
  setItemChecked(OnBottom, _window->getLayer() == BlackboxWindow::LAYER_BELOW);
}


void Windowmenu::itemClicked(unsigned int id, unsigned int button) {
  if (button != 1) return;

  switch (id) {
  case Shade:
    _window->shade();
    break;

  case Iconify:
    _window->iconify();
    break;

  case Maximize:
    _window->maximize(button);
    break;

  case Raise:
    _window->getScreen()->raiseWindow(_window);
    break;

  case Lower:
    _window->getScreen()->lowerWindow(_window);
    break;

  case OnTop:
    _window->changeLayer(BlackboxWindow::LAYER_ABOVE);
    break;

  case OnBottom:
    _window->changeLayer(BlackboxWindow::LAYER_BELOW);
    break;

  case Kill:
    XKillClient(_window->getScreen()->getScreenInfo().getDisplay().XDisplay(),
                _window->getClientWindow());
    break;

  case Close:
    _window->close();
    break;

  } // switch
}

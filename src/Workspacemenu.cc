// -*- mode: C++; indent-tabs-mode: nil; -*-
// Workspacemenu.cc for Blackbox - an X11 Window manager
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

#include "i18n.hh"
#include "blackbox.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Workspacemenu.hh"
#include "Workspace.hh"


Workspacemenu::Workspacemenu(BScreen *scrn) : Basemenu(scrn) {
  setInternalMenu();

  setLabel(i18n(WorkspacemenuSet, WorkspacemenuWorkspacesTitle, "Workspaces"));
  insert(i18n(WorkspacemenuSet, WorkspacemenuNewWorkspace, "New Workspace"));
  insert(i18n(WorkspacemenuSet, WorkspacemenuRemoveLast, "Remove Last"));
}


void Workspacemenu::itemSelected(int button, unsigned int index) {
  if (button != 1)
    return;

  if (index == 0) {
    getScreen()->addWorkspace();
  } else if (index == 1) {
    getScreen()->removeLastWorkspace();
  } else {
    index -= 2;
    const Workspace* const wkspc = getScreen()->getCurrentWorkspace();
    if (wkspc->getID() != index && index < getScreen()->getWorkspaceCount())
      getScreen()->changeWorkspaceID(index);
  }
  if (! (getScreen()->getWorkspacemenu()->isTorn() || isTorn()))
    hide();
}

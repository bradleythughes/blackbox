// -*- mode: C++; indent-tabs-mode: nil; -*-
// Rootmenu.cc for Blackbox - an X11 Window manager
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

#ifdef    HAVE_CONFIG_H
#  include "../config.h"
#endif // HAVE_CONFIG_H

#include "blackbox.hh"
#include "Rootmenu.hh"
#include "Screen.hh"

#ifdef    HAVE_STDIO_H
#  include <stdio.h>
#endif // HAVE_STDIO_H

#ifdef    STDC_HEADERS
#  include <stdlib.h>
#  include <string.h>
#endif // STDC_HEADERS

#ifdef    HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif // HAVE_SYS_PARAM_H

#ifndef   MAXPATHLEN
#define   MAXPATHLEN 255
#endif // MAXPATHLEN


Rootmenu::Rootmenu(BScreen *scrn) : Basemenu(scrn) {
  screen = scrn;
  blackbox = screen->getBlackbox();
}


void Rootmenu::itemSelected(int button, int index) {
  if (button != 1)
    return;

  BasemenuItem *item = find(index);

  if (!item->function())
    return;

  switch (item->function()) {
  case BScreen::Execute:
    if (item->exec()) {
#ifndef    __EMX__
      char displaystring[MAXPATHLEN];
      sprintf(displaystring, "DISPLAY=%s",
	      DisplayString(screen->getBaseDisplay()->getXDisplay()));
      sprintf(displaystring + strlen(displaystring) - 1, "%d",
	      screen->getScreenNumber());

      bexec(item->exec(), displaystring);
#else //   __EMX__
      spawnlp(P_NOWAIT, "cmd.exe", "cmd.exe", "/c", item->exec(), NULL);
#endif // !__EMX__
    }
    break;

  case BScreen::Restart:
    blackbox->restart();
    break;

  case BScreen::RestartOther:
    if (item->exec())
      blackbox->restart(item->exec());
    break;

  case BScreen::Exit:
    blackbox->shutdown();
    break;

  case BScreen::SetStyle:
    if (item->exec())
      blackbox->saveStyleFilename(item->exec());

  case BScreen::Reconfigure:
    blackbox->reconfigure();
    return;
  }

  if (! (screen->getRootmenu()->isTorn() || isTorn()) &&
      item->function() != BScreen::Reconfigure &&
      item->function() != BScreen::SetStyle)
    hide();
}

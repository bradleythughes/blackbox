//
// sessionmenu.cc for Blackbox - an X11 Window manager
// Copyright (c) 1997, 1998 by Brad Hughes, bhughes@arn.net
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// (See the included file COPYING / GPL-2.0)
//

#include "blackbox.hh"
#include "session.hh"
#include "menu.hh"

#include <stdlib.h>


SessionMenu::SessionMenu(BlackboxSession *s) : BlackboxMenu(s) {
  session = s;
}


SessionMenu::~SessionMenu(void) {
  for (int i = 0; i < count(); i++)
    remove(0);
}


int SessionMenu::insert(char *l, int f, char *e) {
  return BlackboxMenu::insert(l, f, e);
}

int SessionMenu::insert(char *l, SessionMenu *s) {
  return BlackboxMenu::insert(l, s);
}


int SessionMenu::remove(int index) {
  if (index >= 0 && index < count()) {
    BlackboxMenuItem *itmp = at(index);

    if (itmp->Submenu())
      delete itmp->Submenu();
    if (itmp->Label())
      delete itmp->Label();
    if (itmp->Exec())
      delete itmp->Exec();
    
    return BlackboxMenu::remove(index);
  }

  return -1;
}


void SessionMenu::showMenu(void) {
  BlackboxMenu::showMenu();
}


void SessionMenu::hideMenu(void) {
  BlackboxMenu::hideMenu();
}


void SessionMenu::moveMenu(int x, int y) {
  BlackboxMenu::moveMenu(x, y);
}


void SessionMenu::updateMenu(void) {
  BlackboxMenu::updateMenu();
}


Window SessionMenu::windowID(void) {
  return BlackboxMenu::windowID();
}


void SessionMenu::itemPressed(int button, int item) {
  if (button == 1 && hasSubmenu(item)) {
    drawSubmenu(item);
    XRaiseWindow(session->control(), at(item)->Submenu()->windowID());
  }
}


void SessionMenu::titlePressed(int) {
}


void SessionMenu::titleReleased(int button) {
  if (button == 3)
    if (windowID() == session->rootmenu->windowID())
      hideMenu();
}


void SessionMenu::itemReleased(int button, int index) {
  if (button == 1) {
    BlackboxMenuItem *item = at(index);
    if (item->Function()) {
      switch (item->Function()) {
      case BlackboxSession::B_Reconfigure:
	Session()->Reconfigure();
	break;

      case BlackboxSession::B_Restart:
	Session()->Restart();
	break;

      case BlackboxSession::B_RestartOther:
	blackbox->Restart(item->Exec());
	break;

      case BlackboxSession::B_Exit:
	Session()->Exit();
	break;
      }

      if (! session->rootmenu->userMoved() &&
	  item->Function() != BlackboxSession::B_Reconfigure)
	session->rootmenu->hideMenu();
    } else if (item->Exec()) {
      char *command = new char[strlen(item->Exec()) + 8];
      sprintf(command, "exec %s &", item->Exec());
      system(command);
      delete [] command;
      if (! session->rootmenu->userMoved())
	session->rootmenu->hideMenu();
    }
  }
}


void SessionMenu::drawSubmenu(int index) {
  BlackboxMenu::drawSubmenu(index);
}
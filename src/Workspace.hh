// Workspace.hh for Blackbox - an X11 Window manager
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

#ifndef   __Workspace_hh
#define   __Workspace_hh

#include <X11/Xlib.h>

#include "LinkedList.hh"

class BScreen;
class Clientmenu;
class Workspace;
class BlackboxWindow;

class Workspace {
private:
  BScreen *screen;
  BlackboxWindow *lastfocus;
  Clientmenu *clientmenu;

  LinkedList<BlackboxWindow> *stackingList, *windowList;

  char *name;
  int id, cascade_x, cascade_y;


protected:
  void placeWindow(BlackboxWindow *);


public:
  Workspace(BScreen *, int = 0);
  ~Workspace(void);

  inline BScreen *getScreen(void) { return screen; }

  inline BlackboxWindow *getLastFocusedWindow(void) { return lastfocus; }
  
  inline Clientmenu *getMenu(void) { return clientmenu; }

  inline const char *getName(void) const { return name; }

  inline const int &getWorkspaceID(void) const { return id; }
  
  inline void setLastFocusedWindow(BlackboxWindow *w) { lastfocus = w; }

  BlackboxWindow *getWindow(int);

  Bool isCurrent(void);
  Bool isLastWindow(BlackboxWindow *);
  
  const int addWindow(BlackboxWindow *, Bool = False);
  const int removeWindow(BlackboxWindow *);
  const int getCount(void);

  void showAll(void);
  void hideAll(void);
  void removeAll(void);
  void raiseWindow(BlackboxWindow *);
  void lowerWindow(BlackboxWindow *);
  void reconfigure();
  void update();
  void setCurrent(void);
  void setName(char *);
  void shutdown(void);
};


#endif // __Workspace_hh


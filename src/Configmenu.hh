// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Configmenu.hh for Blackbox - An X11 Window Manager
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

#ifndef   __Configmenu_hh
#define   __Configmenu_hh

#include "Basemenu.hh"

// forward declaration
class Blackbox;
class BScreen;
class Configmenu;

class Configmenu : public Basemenu
{
public:
  Configmenu(int);

  void reconfigure(void);

protected:
  virtual void itemClicked(const Point &, const Item &, int);

private:
  class Focusmenu : public Basemenu {
  private:
    Configmenu *configmenu;

  protected:
    virtual void itemClicked(const Point &, const Item &, int);

  public:
    Focusmenu(Configmenu *);
  };

  class Placementmenu : public Basemenu {
  private:
    Configmenu *configmenu;

  protected:
    virtual void itemClicked(const Point &, const Item &, int);

  public:
    Placementmenu(Configmenu *);
  };

  Blackbox *blackbox;
  BScreen *screen;
  Focusmenu *focusmenu;
  Placementmenu *placementmenu;

  friend class Focusmenu;
  friend class Placementmenu;
};

#endif // __Configmenu_hh

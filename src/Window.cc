// -*- mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; -*-
// Window.cc for Blackbox - an X11 Window manager
// Copyright (c) 2001 - 2002 Sean 'Shaleh' Perry <shaleh at debian.org>
// Copyright (c) 1997 - 2000, 2002 Brad Hughes <bhughes at trolltech.com>
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

extern "C" {
#include <X11/Xatom.h>
#include <X11/keysym.h>

#ifdef HAVE_STRING_H
#  include <string.h>
#endif // HAVE_STRING_H

#ifdef    DEBUG
#  ifdef    HAVE_STDIO_H
#    include <stdio.h>
#  endif // HAVE_STDIO_H
#endif // DEBUG

#ifdef HAVE_STDLIB_H
   #include <stdlib.h>
#endif // HAVE_STDLIB_H

  #include <assert.h>
}

#include "i18n.hh"
#include "blackbox.hh"
#include "GCCache.hh"
#include "Iconmenu.hh"
#include "Image.hh"
#include "Netwm.hh"
#include "Screen.hh"
#include "Toolbar.hh"
#include "Util.hh"
#include "Window.hh"
#include "Windowmenu.hh"
#include "Workspace.hh"


/*
 * Initializes the class with default values/the window's set initial values.
 */
BlackboxWindow::BlackboxWindow(Blackbox *b, Window w, BScreen *s) {
  // fprintf(stderr, "BlackboxWindow size: %d bytes\n",
  // sizeof(BlackboxWindow));

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::BlackboxWindow(): creating 0x%lx\n", w);
#endif // DEBUG

  /*
    set timer to zero... it is initialized properly later, so we check
    if timer is zero in the destructor, and assume that the window is not
    fully constructed if timer is zero...
  */
  timer = 0;
  blackbox = b;
  client.window = w;
  screen = s;

  if (! validateClient()) {
    delete this;
    return;
  }

  // fetch client size and placement
  XWindowAttributes wattrib;
  if (! XGetWindowAttributes(blackbox->getXDisplay(),
                             client.window, &wattrib) ||
      ! wattrib.screen || wattrib.override_redirect) {
#ifdef    DEBUG
    fprintf(stderr,
            "BlackboxWindow::BlackboxWindow(): XGetWindowAttributes failed\n");
#endif // DEBUG

    delete this;
    return;
  }

  // set the eventmask early in the game so that we make sure we get
  // all the events we are interested in
  XSetWindowAttributes attrib_set;
  attrib_set.event_mask = PropertyChangeMask | FocusChangeMask |
                          StructureNotifyMask;
  attrib_set.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask |
                                     ButtonMotionMask;
  XChangeWindowAttributes(blackbox->getXDisplay(), client.window,
                          CWEventMask|CWDontPropagate, &attrib_set);

  flags.moving = flags.resizing = flags.shaded = flags.visible =
    flags.iconic = flags.focused = flags.modal =
    flags.send_focus_message = flags.shaped = False;
  flags.maximized = 0;

  blackbox_attrib.workspace = window_number = BSENTINEL;

  blackbox_attrib.flags = blackbox_attrib.attrib = blackbox_attrib.stack
    = blackbox_attrib.decoration = 0l;
  blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
  blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

  frame.border_w = 1;
  frame.window = frame.plate = frame.title = frame.handle = None;
  frame.close_button = frame.iconify_button = frame.maximize_button = None;
  frame.right_grip = frame.left_grip = None;

  frame.ulabel_pixel = frame.flabel_pixel = frame.utitle_pixel =
  frame.ftitle_pixel = frame.uhandle_pixel = frame.fhandle_pixel =
    frame.ubutton_pixel = frame.fbutton_pixel = frame.pbutton_pixel =
    frame.uborder_pixel = frame.fborder_pixel = frame.ugrip_pixel =
    frame.fgrip_pixel = 0;
  frame.utitle = frame.ftitle = frame.uhandle = frame.fhandle = None;
  frame.ulabel = frame.flabel = frame.ubutton = frame.fbutton = None;
  frame.pbutton = frame.ugrip = frame.fgrip = None;

  decorations = Decor_Titlebar | Decor_Border | Decor_Handle |
                Decor_Iconify | Decor_Maximize;
  functions = Func_Resize | Func_Move | Func_Iconify | Func_Maximize;

  client.normal_hint_flags = 0;
  client.window_group = None;
  client.transient_for = 0;

  current_state = NormalState;

  /*
    set the initial size and location of client window (relative to the
    _root window_). This position is the reference point used with the
    window's gravity to find the window's initial position.
  */
  client.rect.setRect(wattrib.x, wattrib.y, wattrib.width, wattrib.height);
  client.old_bw = wattrib.border_width;

  windowmenu = 0;
  lastButtonPressTime = 0;

  timer = new BTimer(blackbox, this);
  timer->setTimeout(blackbox->getAutoRaiseDelay());

  // get size, aspect, minimum/maximum size and other hints set by the
  // client

  if (! getBlackboxHints())
    getMWMHints();

  getWMProtocols();
  getWMHints();
  getWMNormalHints();

  frame.window = createToplevelWindow();
  blackbox->insertEventHandler(frame.window, this);

  frame.plate = createChildWindow(frame.window, ExposureMask);
  blackbox->insertEventHandler(frame.plate, this);

  // determine if this is a transient window
  getTransientInfo();

  // adjust the window decorations based on transience and window sizes

  if (isTransient()) {
    decorations &= ~(Decor_Maximize | Decor_Handle);
    functions &= ~Func_Maximize;
  }

  if ((client.normal_hint_flags & PMinSize) &&
      (client.normal_hint_flags & PMaxSize) &&
      client.max_width <= client.min_width &&
      client.max_height <= client.min_height) {
    decorations &= ~(Decor_Maximize | Decor_Handle);
    functions &= ~(Func_Resize | Func_Maximize);
  }

  // now that we know what decorations are required, create them

  if (decorations & Decor_Titlebar)
    createTitlebar();

  if (decorations & Decor_Handle)
    createHandle();

  // apply the size and gravity hint to the frame

  upsize();

  bool place_window = True;
  if (blackbox->isStartup() || isTransient() ||
      client.normal_hint_flags & (PPosition|USPosition)) {
    applyGravity(frame.rect);

    if (blackbox->isStartup() || client.rect.intersects(screen->getRect()))
      place_window = False;
  }

  /*
    the server needs to be grabbed here to prevent client's from sending
    events while we are in the process of configuring their window.
    We hold the grab until after we are done moving the window around.
  */

  XGrabServer(blackbox->getXDisplay());

  associateClientWindow();

  blackbox->insertEventHandler(client.window, this);
  blackbox->insertWindow(client.window, this);

  if (blackbox_attrib.workspace >= screen->getWorkspaceCount())
    screen->getCurrentWorkspace()->addWindow(this, place_window);
  else
    screen->getWorkspace(blackbox_attrib.workspace)->
      addWindow(this, place_window);

  if (! place_window) {
    // don't need to call configure if we are letting the workspace
    // place the window
    configure(frame.rect.x(), frame.rect.y(),
              frame.rect.width(), frame.rect.height());
  }

  positionWindows();

  XUngrabServer(blackbox->getXDisplay());

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions() && flags.shaped)
    configureShape();
#endif // SHAPE

  // now that we know where to put the window and what it should look like
  // we apply the decorations
  decorate();

  grabButtons();

  XMapSubwindows(blackbox->getXDisplay(), frame.window);

  // this ensures the title, buttons, and other decor are properly displayed
  redrawWindowFrame();

  // preserve the window's initial state on first map, and its current state
  // across a restart
  unsigned long initial_state = current_state;
  if (! getState())
    current_state = initial_state;

  // the following flags are set by blackbox native apps only
  if (flags.shaded) {
    flags.shaded = False;
    initial_state = current_state;
    shade();

    /*
      At this point in the life of a window, current_state should only be set
      to IconicState if the window was an *icon*, not if it was shaded.
    */
    if (initial_state != IconicState)
      current_state = NormalState;
  }

  if (flags.maximized && (functions & Func_Maximize))
    remaximize();

  // create this last so it only needs to be configured once
  windowmenu = new Windowmenu(this);
}


BlackboxWindow::~BlackboxWindow(void) {
#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::~BlackboxWindow: destroying 0x%lx\n",
          client.window);
#endif // DEBUG

  if (! timer) // window not managed...
    return;

  if (flags.moving || flags.resizing) {
    screen->hideGeometry();
    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  }

  delete timer;

  delete windowmenu;

  if (client.window_group) {
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) group->removeWindow(this);
  }

  // remove ourselves from our transient_for
  if (isTransient()) {
    if (client.transient_for != (BlackboxWindow *) ~0ul)
      client.transient_for->client.transientList.remove(this);

    client.transient_for = (BlackboxWindow*) 0;
  }

  if (! client.transientList.empty()) {
    // reset transient_for for all transients
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      (*it)->client.transient_for = (BlackboxWindow*) 0;
  }

  if (frame.title)
    destroyTitlebar();

  if (frame.handle)
    destroyHandle();

  if (frame.plate) {
    blackbox->removeEventHandler(frame.plate);
    XDestroyWindow(blackbox->getXDisplay(), frame.plate);
  }

  if (frame.window) {
    blackbox->removeEventHandler(frame.window);
    XDestroyWindow(blackbox->getXDisplay(), frame.window);
  }

  blackbox->removeEventHandler(client.window);
  blackbox->removeWindow(client.window);
}


/*
 * Creates a new top level window, with a given location, size, and border
 * width.
 * Returns: the newly created window
 */
Window BlackboxWindow::createToplevelWindow(void) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel | CWColormap |
                              CWOverrideRedirect | CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.colormap = screen->getColormap();
  attrib_create.override_redirect = True;
  attrib_create.event_mask = EnterWindowMask | LeaveWindowMask;

  return XCreateWindow(blackbox->getXDisplay(), screen->getRootWindow(),
                       0, 0, 1, 1, frame.border_w, screen->getDepth(),
                       InputOutput, screen->getVisual(), create_mask,
                       &attrib_create);
}


/*
 * Creates a child window, and optionally associates a given cursor with
 * the new window.
 */
Window BlackboxWindow::createChildWindow(Window parent,
                                         unsigned long event_mask,
                                         Cursor cursor) {
  XSetWindowAttributes attrib_create;
  unsigned long create_mask = CWBackPixmap | CWBorderPixel |
                              CWEventMask;

  attrib_create.background_pixmap = None;
  attrib_create.event_mask = event_mask;

  if (cursor) {
    create_mask |= CWCursor;
    attrib_create.cursor = cursor;
  }

  return XCreateWindow(blackbox->getXDisplay(), parent, 0, 0, 1, 1, 0,
                       screen->getDepth(), InputOutput, screen->getVisual(),
                       create_mask, &attrib_create);
}


void BlackboxWindow::associateClientWindow(void) {
  XSetWindowBorderWidth(blackbox->getXDisplay(), client.window, 0);
  getWMName();
  getWMIconName();

  XChangeSaveSet(blackbox->getXDisplay(), client.window, SetModeInsert);

  XSelectInput(blackbox->getXDisplay(), frame.plate, SubstructureRedirectMask);

  /*
    note we used to grab around this call to XReparentWindow however the
    server is now grabbed before this method is called
  */
  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XSelectInput(blackbox->getXDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XReparentWindow(blackbox->getXDisplay(), client.window, frame.plate, 0, 0);
  XSelectInput(blackbox->getXDisplay(), client.window, event_mask);

  XRaiseWindow(blackbox->getXDisplay(), frame.plate);
  XMapSubwindows(blackbox->getXDisplay(), frame.plate);

#ifdef    SHAPE
  if (blackbox->hasShapeExtensions()) {
    XShapeSelectInput(blackbox->getXDisplay(), client.window,
                      ShapeNotifyMask);

    Bool shaped = False;
    int foo;
    unsigned int ufoo;

    XShapeQueryExtents(blackbox->getXDisplay(), client.window, &shaped,
                       &foo, &foo, &ufoo, &ufoo, &foo, &foo, &foo,
                       &ufoo, &ufoo);
    flags.shaped = shaped;
  }
#endif // SHAPE
}


void BlackboxWindow::decorate(void) {
  BTexture* texture;

  texture = &(screen->getWindowStyle()->b_focus);
  frame.fbutton = texture->render(frame.button_w, frame.button_w,
                                  frame.fbutton);
  if (! frame.fbutton)
    frame.fbutton_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->b_unfocus);
  frame.ubutton = texture->render(frame.button_w, frame.button_w,
                                  frame.ubutton);
  if (! frame.ubutton)
    frame.ubutton_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->b_pressed);
  frame.pbutton = texture->render(frame.button_w, frame.button_w,
                                  frame.pbutton);
  if (! frame.pbutton)
    frame.pbutton_pixel = texture->color().pixel();

  if (decorations & Decor_Titlebar) {
    texture = &(screen->getWindowStyle()->t_focus);
    frame.ftitle = texture->render(frame.inside_w, frame.title_h,
                                   frame.ftitle);
    if (! frame.ftitle)
      frame.ftitle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->t_unfocus);
    frame.utitle = texture->render(frame.inside_w, frame.title_h,
                                   frame.utitle);
    if (! frame.utitle)
      frame.utitle_pixel = texture->color().pixel();

    XSetWindowBorder(blackbox->getXDisplay(), frame.title,
                     screen->getBorderColor()->pixel());

    decorateLabel();
  }

  if (decorations & Decor_Border) {
    frame.fborder_pixel = screen->getWindowStyle()->f_focus.color().pixel();
    frame.uborder_pixel = screen->getWindowStyle()->f_unfocus.color().pixel();
    blackbox_attrib.flags |= AttribDecoration;
    blackbox_attrib.decoration = DecorNormal;
  } else {
    blackbox_attrib.flags |= AttribDecoration;
    blackbox_attrib.decoration = DecorNone;
  }

  if (decorations & Decor_Handle) {
    texture = &(screen->getWindowStyle()->h_focus);
    frame.fhandle = texture->render(frame.inside_w, frame.handle_h,
                                    frame.fhandle);
    if (! frame.fhandle)
      frame.fhandle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->h_unfocus);
    frame.uhandle = texture->render(frame.inside_w, frame.handle_h,
                                    frame.uhandle);
    if (! frame.uhandle)
      frame.uhandle_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->g_focus);
    frame.fgrip = texture->render(frame.grip_w, frame.handle_h, frame.fgrip);
    if (! frame.fgrip)
      frame.fgrip_pixel = texture->color().pixel();

    texture = &(screen->getWindowStyle()->g_unfocus);
    frame.ugrip = texture->render(frame.grip_w, frame.handle_h, frame.ugrip);
    if (! frame.ugrip)
      frame.ugrip_pixel = texture->color().pixel();

    XSetWindowBorder(blackbox->getXDisplay(), frame.handle,
                     screen->getBorderColor()->pixel());
    XSetWindowBorder(blackbox->getXDisplay(), frame.left_grip,
                     screen->getBorderColor()->pixel());
    XSetWindowBorder(blackbox->getXDisplay(), frame.right_grip,
                     screen->getBorderColor()->pixel());
  }

  XSetWindowBorder(blackbox->getXDisplay(), frame.window,
                   screen->getBorderColor()->pixel());
}


void BlackboxWindow::decorateLabel(void) {
  BTexture *texture;

  texture = &(screen->getWindowStyle()->l_focus);
  frame.flabel = texture->render(frame.label_w, frame.label_h, frame.flabel);
  if (! frame.flabel)
    frame.flabel_pixel = texture->color().pixel();

  texture = &(screen->getWindowStyle()->l_unfocus);
  frame.ulabel = texture->render(frame.label_w, frame.label_h, frame.ulabel);
  if (! frame.ulabel)
    frame.ulabel_pixel = texture->color().pixel();
}


void BlackboxWindow::createHandle(void) {
  frame.handle = createChildWindow(frame.window,
                                   ButtonPressMask | ButtonReleaseMask |
                                   ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.handle, this);

  frame.left_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->getLowerLeftAngleCursor());
  blackbox->insertEventHandler(frame.left_grip, this);

  frame.right_grip =
    createChildWindow(frame.handle,
                      ButtonPressMask | ButtonReleaseMask |
                      ButtonMotionMask | ExposureMask,
                      blackbox->getLowerRightAngleCursor());
  blackbox->insertEventHandler(frame.right_grip, this);
}


void BlackboxWindow::destroyHandle(void) {
  if (frame.fhandle)
    screen->getImageControl()->removeImage(frame.fhandle);

  if (frame.uhandle)
    screen->getImageControl()->removeImage(frame.uhandle);

  if (frame.fgrip)
    screen->getImageControl()->removeImage(frame.fgrip);

  if (frame.ugrip)
    screen->getImageControl()->removeImage(frame.ugrip);

  blackbox->removeEventHandler(frame.left_grip);
  blackbox->removeEventHandler(frame.right_grip);

  XDestroyWindow(blackbox->getXDisplay(), frame.left_grip);
  XDestroyWindow(blackbox->getXDisplay(), frame.right_grip);
  frame.left_grip = frame.right_grip = None;

  blackbox->removeEventHandler(frame.handle);
  XDestroyWindow(blackbox->getXDisplay(), frame.handle);
  frame.handle = None;
}


void BlackboxWindow::createTitlebar(void) {
  frame.title = createChildWindow(frame.window,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  frame.label = createChildWindow(frame.title,
                                  ButtonPressMask | ButtonReleaseMask |
                                  ButtonMotionMask | ExposureMask);
  blackbox->insertEventHandler(frame.title, this);
  blackbox->insertEventHandler(frame.label, this);

  if (decorations & Decor_Iconify) createIconifyButton();
  if (decorations & Decor_Maximize) createMaximizeButton();
  if (decorations & Decor_Close) createCloseButton();
}


void BlackboxWindow::destroyTitlebar(void) {
  if (frame.close_button)
    destroyCloseButton();

  if (frame.iconify_button)
    destroyIconifyButton();

  if (frame.maximize_button)
    destroyMaximizeButton();

  if (frame.ftitle)
    screen->getImageControl()->removeImage(frame.ftitle);

  if (frame.utitle)
    screen->getImageControl()->removeImage(frame.utitle);

  if (frame.flabel)
    screen->getImageControl()->removeImage(frame.flabel);

  if( frame.ulabel)
    screen->getImageControl()->removeImage(frame.ulabel);

  if (frame.fbutton)
    screen->getImageControl()->removeImage(frame.fbutton);

  if (frame.ubutton)
    screen->getImageControl()->removeImage(frame.ubutton);

  if (frame.pbutton)
    screen->getImageControl()->removeImage(frame.pbutton);

  blackbox->removeEventHandler(frame.title);
  blackbox->removeEventHandler(frame.label);

  XDestroyWindow(blackbox->getXDisplay(), frame.label);
  XDestroyWindow(blackbox->getXDisplay(), frame.title);
  frame.title = frame.label = None;
}


void BlackboxWindow::createCloseButton(void) {
  if (frame.title != None) {
    frame.close_button = createChildWindow(frame.title,
                                           ButtonPressMask |
                                           ButtonReleaseMask |
                                           ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.close_button, this);
  }
}


void BlackboxWindow::destroyCloseButton(void) {
  blackbox->removeEventHandler(frame.close_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.close_button);
  frame.close_button = None;
}


void BlackboxWindow::createIconifyButton(void) {
  if (frame.title != None) {
    frame.iconify_button = createChildWindow(frame.title,
                                             ButtonPressMask |
                                             ButtonReleaseMask |
                                             ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.iconify_button, this);
  }
}


void BlackboxWindow::destroyIconifyButton(void) {
  blackbox->removeEventHandler(frame.iconify_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.iconify_button);
  frame.iconify_button = None;
}


void BlackboxWindow::createMaximizeButton(void) {
  if (frame.title != None) {
    frame.maximize_button = createChildWindow(frame.title,
                                              ButtonPressMask |
                                              ButtonReleaseMask |
                                              ButtonMotionMask | ExposureMask);
    blackbox->insertEventHandler(frame.maximize_button, this);
  }
}


void BlackboxWindow::destroyMaximizeButton(void) {
  blackbox->removeEventHandler(frame.maximize_button);
  XDestroyWindow(blackbox->getXDisplay(), frame.maximize_button);
  frame.maximize_button = None;
}


void BlackboxWindow::positionButtons(bool redecorate_label) {
  unsigned int bw = frame.button_w + frame.bevel_w + 1,
    by = frame.bevel_w + 1, lx = by, lw = frame.inside_w - by;

  if (decorations & Decor_Iconify) {
    if (frame.iconify_button == None) createIconifyButton();

    XMoveResizeWindow(blackbox->getXDisplay(), frame.iconify_button, by, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->getXDisplay(), frame.iconify_button);
    XClearWindow(blackbox->getXDisplay(), frame.iconify_button);

    lx += bw;
    lw -= bw;
  } else if (frame.iconify_button) {
    destroyIconifyButton();
  }
  int bx = frame.inside_w - bw;

  if (decorations & Decor_Close) {
    if (frame.close_button == None) createCloseButton();

    XMoveResizeWindow(blackbox->getXDisplay(), frame.close_button, bx, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->getXDisplay(), frame.close_button);
    XClearWindow(blackbox->getXDisplay(), frame.close_button);

    bx -= bw;
    lw -= bw;
  } else if (frame.close_button) {
    destroyCloseButton();
  }
  if (decorations & Decor_Maximize) {
    if (frame.maximize_button == None) createMaximizeButton();

    XMoveResizeWindow(blackbox->getXDisplay(), frame.maximize_button, bx, by,
                      frame.button_w, frame.button_w);
    XMapWindow(blackbox->getXDisplay(), frame.maximize_button);
    XClearWindow(blackbox->getXDisplay(), frame.maximize_button);

    lw -= bw;
  } else if (frame.maximize_button) {
    destroyMaximizeButton();
  }
  frame.label_w = lw - by;
  XMoveResizeWindow(blackbox->getXDisplay(), frame.label, lx, frame.bevel_w,
                    frame.label_w, frame.label_h);
  if (redecorate_label) decorateLabel();

  redrawLabel();
  redrawAllButtons();
}


void BlackboxWindow::reconfigure(void) {
  restoreGravity(client.rect);
  upsize();
  applyGravity(frame.rect);
  positionWindows();
  decorate();
  redrawWindowFrame();

  ungrabButtons();
  grabButtons();

  if (windowmenu) {
    windowmenu->move(windowmenu->getX(), frame.rect.y() + frame.title_h);
    windowmenu->reconfigure();
  }
}


void BlackboxWindow::grabButtons(void) {
  if (! screen->isSloppyFocus() || screen->doClickRaise())
    // grab button 1 for changing focus/raising
    blackbox->grabButton(Button1, 0, frame.plate, True, ButtonPressMask,
                         GrabModeSync, GrabModeSync, frame.plate, None,
                         screen->allowScrollLock());

  if (functions & Func_Move)
    blackbox->grabButton(Button1, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         blackbox->getMoveCursor(),
                         screen->allowScrollLock());
  if (functions & Func_Resize)
    blackbox->grabButton(Button3, Mod1Mask, frame.window, True,
                         ButtonReleaseMask | ButtonMotionMask, GrabModeAsync,
                         GrabModeAsync, frame.window,
                         blackbox->getLowerRightAngleCursor(),
                         screen->allowScrollLock());
  // alt+middle lowers the window
  blackbox->grabButton(Button2, Mod1Mask, frame.window, True,
                       ButtonReleaseMask, GrabModeAsync, GrabModeAsync,
                       frame.window, None, screen->allowScrollLock());
}


void BlackboxWindow::ungrabButtons(void) {
  blackbox->ungrabButton(Button1, 0, frame.plate);
  blackbox->ungrabButton(Button1, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button2, Mod1Mask, frame.window);
  blackbox->ungrabButton(Button3, Mod1Mask, frame.window);
}


void BlackboxWindow::positionWindows(void) {
  XMoveResizeWindow(blackbox->getXDisplay(), frame.window,
                    frame.rect.x(), frame.rect.y(), frame.inside_w,
                    (flags.shaded) ? frame.title_h : frame.inside_h);
  XSetWindowBorderWidth(blackbox->getXDisplay(), frame.window,
                        frame.border_w);
  XSetWindowBorderWidth(blackbox->getXDisplay(), frame.plate,
                        frame.mwm_border_w);
  XMoveResizeWindow(blackbox->getXDisplay(), frame.plate,
                    frame.margin.left - frame.mwm_border_w - frame.border_w,
                    frame.margin.top - frame.mwm_border_w - frame.border_w,
                    client.rect.width(), client.rect.height());
  XMoveResizeWindow(blackbox->getXDisplay(), client.window,
                    0, 0, client.rect.width(), client.rect.height());
  // ensure client.rect contains the real location
  client.rect.setPos(frame.rect.left() + frame.margin.left,
                     frame.rect.top() + frame.margin.top);

  if (decorations & Decor_Titlebar) {
    if (frame.title == None) createTitlebar();

    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.title,
                          frame.border_w);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.title, -frame.border_w,
                      -frame.border_w, frame.inside_w, frame.title_h);

    positionButtons();
    XMapSubwindows(blackbox->getXDisplay(), frame.title);
    XMapWindow(blackbox->getXDisplay(), frame.title);
  } else if (frame.title) {
    destroyTitlebar();
  }
  if (decorations & Decor_Handle) {
    if (frame.handle == None) createHandle();
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.handle,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.left_grip,
                          frame.border_w);
    XSetWindowBorderWidth(blackbox->getXDisplay(), frame.right_grip,
                          frame.border_w);

    // use client.rect here so the value is correct even if shaded
    XMoveResizeWindow(blackbox->getXDisplay(), frame.handle,
                      -frame.border_w,
                      client.rect.height() + frame.margin.top +
                      frame.mwm_border_w - frame.border_w,
                      frame.inside_w, frame.handle_h);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.left_grip,
                      -frame.border_w, -frame.border_w,
                      frame.grip_w, frame.handle_h);
    XMoveResizeWindow(blackbox->getXDisplay(), frame.right_grip,
                      frame.inside_w - frame.grip_w - frame.border_w,
                      -frame.border_w, frame.grip_w, frame.handle_h);

    XMapSubwindows(blackbox->getXDisplay(), frame.handle);
    XMapWindow(blackbox->getXDisplay(), frame.handle);
  } else if (frame.handle) {
    destroyHandle();
  }
  XSync(blackbox->getXDisplay(), False);
}


void BlackboxWindow::getWMName(void) {
  XTextProperty text_prop;

  std::string name;

  if (! blackbox->netwm()->readWMName(client.window, name) || name.empty()) {
    if (XGetWMName(blackbox->getXDisplay(), client.window, &text_prop)) {
      name = textPropertyToString(blackbox->getXDisplay(), text_prop);
      XFree((char *) text_prop.value);
    }
  }

  if (! name.empty())
    client.title = name;
  else
    client.title = i18n(WindowSet, WindowUnnamed, "Unnamed");

#ifdef DEBUG_WITH_ID
  // the 16 is the 8 chars of the debug text plus the number
  char *tmp = new char[client.title.length() + 16];
  sprintf(tmp, "%s; id: 0x%lx", client.title.c_str(), client.window);
  client.title = tmp;
  delete tmp;
#endif
}


void BlackboxWindow::getWMIconName(void) {
  XTextProperty text_prop;

  std::string name;

  if (! blackbox->netwm()->readWMIconName(client.window, name) ||
      name.empty()) {
    if (XGetWMIconName(blackbox->getXDisplay(), client.window, &text_prop)) {
      name = textPropertyToString(blackbox->getXDisplay(), text_prop);
      XFree((char *) text_prop.value);
    }
  }

  if (! name.empty())
    client.icon_title = name;
  else
    client.icon_title = client.title;
}


/*
 * Retrieve which WM Protocols are supported by the client window.
 * If the WM_DELETE_WINDOW protocol is supported, add the close button to the
 * window's decorations and allow the close behavior.
 * If the WM_TAKE_FOCUS protocol is supported, save a value that indicates
 * this.
 */
void BlackboxWindow::getWMProtocols(void) {
  Atom *proto;
  int num_return = 0;

  if (XGetWMProtocols(blackbox->getXDisplay(), client.window,
                      &proto, &num_return)) {
    for (int i = 0; i < num_return; ++i) {
      if (proto[i] == blackbox->getWMDeleteAtom()) {
        decorations |= Decor_Close;
        functions |= Func_Close;
      } else if (proto[i] == blackbox->getWMTakeFocusAtom()) {
        flags.send_focus_message = True;
      } else if (proto[i] == blackbox->getBlackboxStructureMessagesAtom()) {
        screen->addNetizen(new Netizen(screen, client.window));
      }
    }

    XFree(proto);
  }
}


/*
 * Gets the value of the WM_HINTS property.
 * If the property is not set, then use a set of default values.
 */
void BlackboxWindow::getWMHints(void) {
  focus_mode = F_Passive;

  // remove from current window group
  if (client.window_group) {
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) group->removeWindow(this);
  }
  client.window_group = None;

  XWMHints *wmhint = XGetWMHints(blackbox->getXDisplay(), client.window);
  if (! wmhint)
    return;

  if (wmhint->flags & InputHint) {
    if (wmhint->input == True) {
      if (flags.send_focus_message)
        focus_mode = F_LocallyActive;
    } else {
      if (flags.send_focus_message)
        focus_mode = F_GloballyActive;
      else
        focus_mode = F_NoInput;
    }
  }

  if (wmhint->flags & StateHint)
    current_state = wmhint->initial_state;

  if (wmhint->flags & WindowGroupHint) {
    client.window_group = wmhint->window_group;

    // add window to the appropriate group
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (! group) { // no group found, create it!
      new BWindowGroup(blackbox, client.window_group);
      group = blackbox->findWindowGroup(client.window_group);
    }
    if (group)
      group->addWindow(this);
  }

  XFree(wmhint);
}


/*
 * Gets the value of the WM_NORMAL_HINTS property.
 * If the property is not set, then use a set of default values.
 */
void BlackboxWindow::getWMNormalHints(void) {
  long icccm_mask;
  XSizeHints sizehint;

  client.min_width = client.min_height =
    client.width_inc = client.height_inc = 1;
  client.base_width = client.base_height = 0;
  client.win_gravity = NorthWestGravity;
#if 0
  client.min_aspect_x = client.min_aspect_y =
    client.max_aspect_x = client.max_aspect_y = 1;
#endif

  /*
    use the full screen, not the strut modified size. otherwise when the
    availableArea changes max_width/height will be incorrect and lead to odd
    rendering bugs.
  */
  const Rect& screen_area = screen->getRect();
  client.max_width = screen_area.width();
  client.max_height = screen_area.height();

  if (! XGetWMNormalHints(blackbox->getXDisplay(), client.window,
                          &sizehint, &icccm_mask))
    return;

  client.normal_hint_flags = sizehint.flags;

  if (sizehint.flags & PMinSize) {
    if (sizehint.min_width >= 0)
      client.min_width = sizehint.min_width;
    if (sizehint.min_height >= 0)
      client.min_height = sizehint.min_height;
  }

  if (sizehint.flags & PMaxSize) {
    if (sizehint.max_width > static_cast<signed>(client.min_width))
      client.max_width = sizehint.max_width;
    else
      client.max_width = client.min_width;

    if (sizehint.max_height > static_cast<signed>(client.min_height))
      client.max_height = sizehint.max_height;
    else
      client.max_height = client.min_height;
  }

  if (sizehint.flags & PResizeInc) {
    client.width_inc = sizehint.width_inc;
    client.height_inc = sizehint.height_inc;
  }

#if 0 // we do not support this at the moment
  if (sizehint.flags & PAspect) {
    client.min_aspect_x = sizehint.min_aspect.x;
    client.min_aspect_y = sizehint.min_aspect.y;
    client.max_aspect_x = sizehint.max_aspect.x;
    client.max_aspect_y = sizehint.max_aspect.y;
  }
#endif

  if (sizehint.flags & PBaseSize) {
    client.base_width = sizehint.base_width;
    client.base_height = sizehint.base_height;
  }

  if (sizehint.flags & PWinGravity)
    client.win_gravity = sizehint.win_gravity;
}


/*
 * Gets the MWM hints for the class' contained window.
 * This is used while initializing the window to its first state, and not
 * thereafter.
 * Returns: true if the MWM hints are successfully retreived and applied;
 * false if they are not.
 */
void BlackboxWindow::getMWMHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;
  MwmHints *mwm_hint = 0;

  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getMotifWMHintsAtom(), 0,
                               PropMwmHintsElements, False,
                               blackbox->getMotifWMHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &mwm_hint);

  if (ret != Success || ! mwm_hint || num != PropMwmHintsElements)
    return;

  if (mwm_hint->flags & MwmHintsDecorations) {
    if (mwm_hint->decorations & MwmDecorAll) {
      decorations = Decor_Titlebar | Decor_Handle | Decor_Border |
                    Decor_Iconify | Decor_Maximize | Decor_Close;
    } else {
      decorations = 0;

      if (mwm_hint->decorations & MwmDecorBorder)
        decorations |= Decor_Border;
      if (mwm_hint->decorations & MwmDecorHandle)
        decorations |= Decor_Handle;
      if (mwm_hint->decorations & MwmDecorTitle)
        decorations |= Decor_Titlebar;
      if (mwm_hint->decorations & MwmDecorIconify)
        decorations |= Decor_Iconify;
      if (mwm_hint->decorations & MwmDecorMaximize)
        decorations |= Decor_Maximize;
    }
  }

  if (mwm_hint->flags & MwmHintsFunctions) {
    if (mwm_hint->functions & MwmFuncAll) {
      functions = Func_Resize | Func_Move | Func_Iconify | Func_Maximize |
                  Func_Close;
    } else {
      functions = 0;

      if (mwm_hint->functions & MwmFuncResize)
        functions |= Func_Resize;
      if (mwm_hint->functions & MwmFuncMove)
        functions |= Func_Move;
      if (mwm_hint->functions & MwmFuncIconify)
        functions |= Func_Iconify;
      if (mwm_hint->functions & MwmFuncMaximize)
        functions |= Func_Maximize;
      if (mwm_hint->functions & MwmFuncClose)
        functions |= Func_Close;
    }
  }
  XFree(mwm_hint);
}


/*
 * Gets the blackbox hints from the class' contained window.
 * This is used while initializing the window to its first state, and not
 * thereafter.
 * Returns: true if the hints are successfully retreived and applied; false if
 * they are not.
 */
bool BlackboxWindow::getBlackboxHints(void) {
  int format;
  Atom atom_return;
  unsigned long num, len;
  BlackboxHints *blackbox_hint = 0;

  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getBlackboxHintsAtom(), 0,
                               PropBlackboxHintsElements, False,
                               blackbox->getBlackboxHintsAtom(), &atom_return,
                               &format, &num, &len,
                               (unsigned char **) &blackbox_hint);
  if (ret != Success || ! blackbox_hint || num != PropBlackboxHintsElements)
    return False;

  if (blackbox_hint->flags & AttribShaded)
    flags.shaded = (blackbox_hint->attrib & AttribShaded);

  if ((blackbox_hint->flags & AttribMaxHoriz) &&
      (blackbox_hint->flags & AttribMaxVert))
    flags.maximized = (blackbox_hint->attrib &
                       (AttribMaxHoriz | AttribMaxVert)) ? 1 : 0;
  else if (blackbox_hint->flags & AttribMaxVert)
    flags.maximized = (blackbox_hint->attrib & AttribMaxVert) ? 2 : 0;
  else if (blackbox_hint->flags & AttribMaxHoriz)
    flags.maximized = (blackbox_hint->attrib & AttribMaxHoriz) ? 3 : 0;

  if (blackbox_hint->flags & AttribWorkspace)
    blackbox_attrib.workspace = blackbox_hint->workspace;

  // if (blackbox_hint->flags & AttribStack)
  //   don't yet have always on top/bottom for blackbox yet... working
  //   on that

  if (blackbox_hint->flags & AttribDecoration) {
    switch (blackbox_hint->decoration) {
    case DecorNone:
      decorations = 0;

      break;

    case DecorTiny:
      decorations |= Decor_Titlebar | Decor_Iconify;
      decorations &= ~(Decor_Border | Decor_Handle | Decor_Maximize);
      functions &= ~(Func_Resize | Func_Maximize);

      break;

    case DecorTool:
      decorations |= Decor_Titlebar;
      decorations &= ~(Decor_Iconify | Decor_Border | Decor_Handle);
      functions &= ~(Func_Resize | Func_Maximize | Func_Iconify);

      break;

    case DecorNormal:
    default:
      decorations |= Decor_Titlebar | Decor_Border | Decor_Handle |
                     Decor_Iconify | Decor_Maximize;
      break;
    }

    reconfigure();
  }
  XFree(blackbox_hint);
  return True;
}


void BlackboxWindow::getTransientInfo(void) {
  if (client.transient_for &&
      client.transient_for != (BlackboxWindow *) ~0ul) {
    // reset transient_for in preparation of looking for a new owner
    client.transient_for->client.transientList.remove(this);
  }

  // we have no transient_for until we find a new one
  client.transient_for = (BlackboxWindow *) 0;

  Window trans_for;
  if (!XGetTransientForHint(blackbox->getXDisplay(), client.window,
                            &trans_for)) {
    // transient_for hint not set
    return;
  }

  if (trans_for == client.window) {
    // wierd client... treat this window as a normal window
    return;
  }

  if (trans_for == None || trans_for == screen->getRootWindow()) {
    // this is an undocumented interpretation of the ICCCM. a transient
    // associated with None/Root/itself is assumed to be a modal root
    // transient.  we don't support the concept of a global transient,
    // so we just associate this transient with nothing, and perhaps
    // we will add support later for global modality.
    client.transient_for = (BlackboxWindow *) ~0ul;
    flags.modal = True;
    return;
  }

  client.transient_for = blackbox->findWindow(trans_for);
  if (! client.transient_for &&
      client.window_group && trans_for == client.window_group) {
    // no direct transient_for, perhaps this is a group transient?
    BWindowGroup *group = blackbox->findWindowGroup(client.window_group);
    if (group) client.transient_for = group->find(screen);
  }

  if (! client.transient_for || client.transient_for == this) {
    // no transient_for found, or we have a wierd client that wants to be
    // a transient for itself, so we treat this window as a normal window
    client.transient_for = (BlackboxWindow*) 0;
    return;
  }

  // Check for a circular transient state: this can lock up Blackbox
  // when it tries to find the non-transient window for a transient.
  BlackboxWindow *w = this;
  while(w->client.transient_for &&
        w->client.transient_for != (BlackboxWindow *) ~0ul) {
    if(w->client.transient_for == this) {
      client.transient_for = (BlackboxWindow*) 0;
      break;
    }
    w = w->client.transient_for;
  }

  if (client.transient_for) {
    // register ourselves with our new transient_for
    client.transient_for->client.transientList.push_back(this);
  }
}


BlackboxWindow *BlackboxWindow::getTransientFor(void) const {
  if (client.transient_for &&
      client.transient_for != (BlackboxWindow*) ~0ul)
    return client.transient_for;
  return 0;
}


/*
 * This function is responsible for updating both the client and the frame
 * rectangles.
 * According to the ICCCM a client message is not sent for a resize, only a
 * move.
 */
void BlackboxWindow::configure(int dx, int dy,
                               unsigned int dw, unsigned int dh) {
  bool send_event = ((frame.rect.x() != dx || frame.rect.y() != dy) &&
                     ! flags.moving);

  if (dw != frame.rect.width() || dh != frame.rect.height()) {
    frame.rect.setRect(dx, dy, dw, dh);
    frame.inside_w = frame.rect.width() - (frame.border_w * 2);
    frame.inside_h = frame.rect.height() - (frame.border_w * 2);

    if (frame.rect.right() <= 0 || frame.rect.bottom() <= 0)
      frame.rect.setPos(0, 0);

    client.rect.setCoords(frame.rect.left() + frame.margin.left,
                          frame.rect.top() + frame.margin.top,
                          frame.rect.right() - frame.margin.right,
                          frame.rect.bottom() - frame.margin.bottom);

#ifdef    SHAPE
    if (blackbox->hasShapeExtensions() && flags.shaped) {
      configureShape();
    }
#endif // SHAPE

    positionWindows();
    decorate();
    redrawWindowFrame();
  } else {
    frame.rect.setPos(dx, dy);

    XMoveWindow(blackbox->getXDisplay(), frame.window,
                frame.rect.x(), frame.rect.y());
    /*
      we may have been called just after an opaque window move, so even though
      the old coords match the new ones no ConfigureNotify has been sent yet.
      There are likely other times when this will be relevant as well.
    */
    if (! flags.moving) send_event = True;
  }

  if (send_event) {
    // if moving, the update and event will occur when the move finishes
    client.rect.setPos(frame.rect.left() + frame.margin.left,
                       frame.rect.top() + frame.margin.top);

    XEvent event;
    event.type = ConfigureNotify;

    event.xconfigure.display = blackbox->getXDisplay();
    event.xconfigure.event = client.window;
    event.xconfigure.window = client.window;
    event.xconfigure.x = client.rect.x();
    event.xconfigure.y = client.rect.y();
    event.xconfigure.width = client.rect.width();
    event.xconfigure.height = client.rect.height();
    event.xconfigure.border_width = client.old_bw;
    event.xconfigure.above = frame.window;
    event.xconfigure.override_redirect = False;

    XSendEvent(blackbox->getXDisplay(), client.window, False,
               StructureNotifyMask, &event);
    screen->updateNetizenConfigNotify(&event);
    XFlush(blackbox->getXDisplay());
  }
}


#ifdef SHAPE
void BlackboxWindow::configureShape(void) {
  XShapeCombineShape(blackbox->getXDisplay(), frame.window, ShapeBounding,
                     frame.margin.left - frame.border_w,
                     frame.margin.top - frame.border_w,
                     client.window, ShapeBounding, ShapeSet);

  int num = 0;
  XRectangle xrect[2];

  if (decorations & Decor_Titlebar) {
    xrect[0].x = xrect[0].y = -frame.border_w;
    xrect[0].width = frame.rect.width();
    xrect[0].height = frame.title_h + (frame.border_w * 2);
    ++num;
  }

  if (decorations & Decor_Handle) {
    xrect[1].x = -frame.border_w;
    xrect[1].y = frame.rect.height() - frame.margin.bottom +
                 frame.mwm_border_w - frame.border_w;
    xrect[1].width = frame.rect.width();
    xrect[1].height = frame.handle_h + (frame.border_w * 2);
    ++num;
  }

  XShapeCombineRectangles(blackbox->getXDisplay(), frame.window,
                          ShapeBounding, 0, 0, xrect, num,
                          ShapeUnion, Unsorted);
}
#endif // SHAPE


bool BlackboxWindow::setInputFocus(void) {
  if (flags.focused) return True;

  // do not give focus to a window that is about to close
  if (! validateClient()) return False;

  assert(! flags.iconic &&
         // window must be on the current workspace
         blackbox_attrib.workspace == screen->getCurrentWorkspaceID());

  if (! frame.rect.intersects(screen->getRect())) {
    // client is outside the screen, move it to the center
    configure((screen->getWidth() - frame.rect.width()) / 2,
              (screen->getHeight() - frame.rect.height()) / 2,
              frame.rect.width(), frame.rect.height());
  }

  if (! client.transientList.empty()) {
    // transfer focus to any modal transients
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      if ((*it)->flags.modal) return (*it)->setInputFocus();
  }

  bool ret = True;
  switch (focus_mode) {
  case F_Passive:
  case F_LocallyActive:
    XSetInputFocus(blackbox->getXDisplay(), client.window,
                   RevertToPointerRoot, CurrentTime);
    blackbox->setFocusedWindow(this);
    break;

  case F_GloballyActive:
  case F_NoInput:
    /*
     * we could set the focus to none, since the window doesn't accept focus,
     * but we shouldn't set focus to nothing since this would surely make
     * someone angry
     */
    ret = False;
    break;
  }

  if (flags.send_focus_message) {
    XEvent ce;
    ce.xclient.type = ClientMessage;
    ce.xclient.message_type = blackbox->getWMProtocolsAtom();
    ce.xclient.display = blackbox->getXDisplay();
    ce.xclient.window = client.window;
    ce.xclient.format = 32;
    ce.xclient.data.l[0] = blackbox->getWMTakeFocusAtom();
    ce.xclient.data.l[1] = blackbox->getLastTime();
    ce.xclient.data.l[2] = 0l;
    ce.xclient.data.l[3] = 0l;
    ce.xclient.data.l[4] = 0l;
    XSendEvent(blackbox->getXDisplay(), client.window, False,
               NoEventMask, &ce);
    XFlush(blackbox->getXDisplay());
  }

  return ret;
}


void BlackboxWindow::iconify(void) {
  // walk up to the topmost transient_for that is not iconified
  if (isTransient() &&
      client.transient_for != (BlackboxWindow *) ~0ul &&
      ! client.transient_for->isIconic()) {

    client.transient_for->iconify();
    return;
  }

  if (flags.iconic) return;

  /*
   * unmap the frame window first, so when all the transients are
   * unmapped, we don't get an enter event in sloppy focus mode
   */
  XUnmapWindow(blackbox->getXDisplay(), frame.window);
  flags.visible = False;
  flags.iconic = True;

  if (windowmenu) windowmenu->hide();

  setState(IconicState);

  // iconify all transients first
  if (! client.transientList.empty()) {
    std::for_each(client.transientList.begin(), client.transientList.end(),
                  std::mem_fun(&BlackboxWindow::iconify));
  }

  /*
   * remove the window from the workspace and add it to the screen's
   * icons *AFTER* we have process all transients.  since we always
   * iconify transients, it's pointless to have focus reverted to one
   * of them (since they are above their transient_for) for a split
   * second
   */
  screen->getWorkspace(blackbox_attrib.workspace)->removeWindow(this);
  screen->addIcon(this);

  /*
   * we don't want this XUnmapWindow call to generate an UnmapNotify event, so
   * we need to clear the event mask on client.window for a split second.
   * HOWEVER, since X11 is asynchronous, the window could be destroyed in that
   * split second, leaving us with a ghost window... so, we need to do this
   * while the X server is grabbed
   */
  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XGrabServer(blackbox->getXDisplay());
  XSelectInput(blackbox->getXDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XUnmapWindow(blackbox->getXDisplay(), client.window);
  XSelectInput(blackbox->getXDisplay(), client.window, event_mask);
  XUngrabServer(blackbox->getXDisplay());
}


void BlackboxWindow::show(void) {
  current_state = (flags.shaded) ? IconicState : NormalState;
  setState(current_state);

  XMapWindow(blackbox->getXDisplay(), client.window);
  XMapSubwindows(blackbox->getXDisplay(), frame.window);
  XMapWindow(blackbox->getXDisplay(), frame.window);

#ifdef DEBUG
  int real_x, real_y;
  Window child;
  XTranslateCoordinates(blackbox->getXDisplay(), client.window,
                        screen->getRootWindow(),
                        0, 0, &real_x, &real_y, &child);
  fprintf(stderr, "%s -- assumed: (%d, %d), real: (%d, %d)\n", getTitle(),
          client.rect.left(), client.rect.top(), real_x, real_y);
  assert(client.rect.left() == real_x && client.rect.top() == real_y);
#endif

  flags.visible = True;
  flags.iconic = False;
}


void BlackboxWindow::deiconify(bool reassoc, bool raise) {
  if (flags.iconic || reassoc)
    screen->reassociateWindow(this, BSENTINEL);
  else if (blackbox_attrib.workspace != screen->getCurrentWorkspaceID())
    return;

  show();

  // reassociate and deiconify all transients
  if (reassoc && ! client.transientList.empty()) {
    BlackboxWindowList::iterator it, end = client.transientList.end();
    for (it = client.transientList.begin(); it != end; ++it)
      (*it)->deiconify(True, False);
  }

  if (raise)
    screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
}


void BlackboxWindow::close(void) {
  XEvent ce;
  ce.xclient.type = ClientMessage;
  ce.xclient.message_type = blackbox->getWMProtocolsAtom();
  ce.xclient.display = blackbox->getXDisplay();
  ce.xclient.window = client.window;
  ce.xclient.format = 32;
  ce.xclient.data.l[0] = blackbox->getWMDeleteAtom();
  ce.xclient.data.l[1] = CurrentTime;
  ce.xclient.data.l[2] = 0l;
  ce.xclient.data.l[3] = 0l;
  ce.xclient.data.l[4] = 0l;
  XSendEvent(blackbox->getXDisplay(), client.window, False, NoEventMask, &ce);
  XFlush(blackbox->getXDisplay());
}


void BlackboxWindow::withdraw(void) {
  setState(current_state);

  flags.visible = False;
  flags.iconic = False;

  XUnmapWindow(blackbox->getXDisplay(), frame.window);

  XGrabServer(blackbox->getXDisplay());

  unsigned long event_mask = PropertyChangeMask | FocusChangeMask |
                             StructureNotifyMask;
  XSelectInput(blackbox->getXDisplay(), client.window,
               event_mask & ~StructureNotifyMask);
  XUnmapWindow(blackbox->getXDisplay(), client.window);
  XSelectInput(blackbox->getXDisplay(), client.window, event_mask);

  XUngrabServer(blackbox->getXDisplay());

  if (windowmenu) windowmenu->hide();
}


void BlackboxWindow::maximize(unsigned int button) {
  // handle case where menu is open then the max button is used instead
  if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

  if (flags.maximized) {
    flags.maximized = 0;

    blackbox_attrib.flags &= ! (AttribMaxHoriz | AttribMaxVert);
    blackbox_attrib.attrib &= ! (AttribMaxHoriz | AttribMaxVert);

    /*
      when a resize is begun, maximize(0) is called to clear any maximization
      flags currently set.  Otherwise it still thinks it is maximized.
      so we do not need to call configure() because resizing will handle it
    */
    if (!flags.resizing)
      configure(blackbox_attrib.premax_x, blackbox_attrib.premax_y,
                blackbox_attrib.premax_w, blackbox_attrib.premax_h);

    blackbox_attrib.premax_x = blackbox_attrib.premax_y = 0;
    blackbox_attrib.premax_w = blackbox_attrib.premax_h = 0;

    redrawAllButtons(); // in case it is not called in configure()
    setState(current_state);
    return;
  }

  blackbox_attrib.premax_x = frame.rect.x();
  blackbox_attrib.premax_y = frame.rect.y();
  blackbox_attrib.premax_w = frame.rect.width();
  // use client.rect so that clients can be restored even if shaded
  blackbox_attrib.premax_h =
    client.rect.height() + frame.margin.top + frame.margin.bottom;

  const Rect &screen_area = screen->availableArea();
  frame.changing = screen_area;

  switch(button) {
  case 1:
    blackbox_attrib.flags |= AttribMaxHoriz | AttribMaxVert;
    blackbox_attrib.attrib |= AttribMaxHoriz | AttribMaxVert;
    break;

  case 2:
    blackbox_attrib.flags |= AttribMaxVert;
    blackbox_attrib.attrib |= AttribMaxVert;

    frame.changing.setX(blackbox_attrib.premax_x);
    frame.changing.setWidth(blackbox_attrib.premax_w);
    break;

  case 3:
    blackbox_attrib.flags |= AttribMaxHoriz;
    blackbox_attrib.attrib |= AttribMaxHoriz;

    frame.changing.setY(blackbox_attrib.premax_y);
    frame.changing.setHeight(blackbox_attrib.premax_h);
    break;
  }

  constrain(TopLeft);

  if (flags.shaded) {
    blackbox_attrib.flags ^= AttribShaded;
    blackbox_attrib.attrib ^= AttribShaded;
    flags.shaded = False;
  }

  flags.maximized = button;

  configure(frame.changing.x(), frame.changing.y(),
            frame.changing.width(), frame.changing.height());
  redrawAllButtons(); // in case it is not called in configure()
  setState(current_state);
}


// re-maximizes the window to take into account availableArea changes
void BlackboxWindow::remaximize(void) {
  if (flags.shaded) {
    // we only update the window's attributes otherwise we lose the shade bit
    switch(flags.maximized) {
    case 1:
      blackbox_attrib.flags |= AttribMaxHoriz | AttribMaxVert;
      blackbox_attrib.attrib |= AttribMaxHoriz | AttribMaxVert;
      break;

    case 2:
      blackbox_attrib.flags |= AttribMaxVert;
      blackbox_attrib.attrib |= AttribMaxVert;
      break;

    case 3:
      blackbox_attrib.flags |= AttribMaxHoriz;
      blackbox_attrib.attrib |= AttribMaxHoriz;
      break;
    }
    return;
  }

  // save the original dimensions because maximize will wipe them out
  int premax_x = blackbox_attrib.premax_x,
    premax_y = blackbox_attrib.premax_y,
    premax_w = blackbox_attrib.premax_w,
    premax_h = blackbox_attrib.premax_h;

  unsigned int button = flags.maximized;
  flags.maximized = 0; // trick maximize() into working
  maximize(button);

  // restore saved values
  blackbox_attrib.premax_x = premax_x;
  blackbox_attrib.premax_y = premax_y;
  blackbox_attrib.premax_w = premax_w;
  blackbox_attrib.premax_h = premax_h;
}


void BlackboxWindow::setWorkspace(unsigned int n) {
  blackbox_attrib.flags |= AttribWorkspace;
  blackbox_attrib.workspace = n;
}


void BlackboxWindow::shade(void) {
  if (flags.shaded) {
    flags.shaded = False;
    blackbox_attrib.flags ^= AttribShaded;
    blackbox_attrib.attrib ^= AttribShaded;

    if (flags.maximized) {
      remaximize();
    } else {
      XResizeWindow(blackbox->getXDisplay(), frame.window,
                    frame.inside_w, frame.inside_h);
      // set the frame rect to the normal size
      frame.rect.setHeight(client.rect.height() + frame.margin.top +
                           frame.margin.bottom);
    }

    setState(NormalState);
  } else {
    if (! (decorations & Decor_Titlebar))
      return; // can't shade it without a titlebar!

    XResizeWindow(blackbox->getXDisplay(), frame.window,
                  frame.inside_w, frame.title_h);
    flags.shaded = True;
    blackbox_attrib.flags |= AttribShaded;
    blackbox_attrib.attrib |= AttribShaded;

    setState(IconicState);

    // set the frame rect to the shaded size
    frame.rect.setHeight(frame.title_h + (frame.border_w * 2));
  }
}


void BlackboxWindow::redrawWindowFrame(void) const {
  if (decorations & Decor_Titlebar) {
    if (flags.focused) {
      if (frame.ftitle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.title, frame.ftitle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.title, frame.ftitle_pixel);
    } else {
      if (frame.utitle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.title, frame.utitle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.title, frame.utitle_pixel);
    }
    XClearWindow(blackbox->getXDisplay(), frame.title);

    redrawLabel();
    redrawAllButtons();
  }

  if (decorations & Decor_Handle) {
    if (flags.focused) {
      if (frame.fhandle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.handle, frame.fhandle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.handle, frame.fhandle_pixel);

      if (frame.fgrip) {
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.left_grip, frame.fgrip);
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.right_grip, frame.fgrip);
      } else {
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.left_grip, frame.fgrip_pixel);
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.right_grip, frame.fgrip_pixel);
      }
    } else {
      if (frame.uhandle)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.handle, frame.uhandle);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.handle, frame.uhandle_pixel);

      if (frame.ugrip) {
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.left_grip, frame.ugrip);
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.right_grip, frame.ugrip);
      } else {
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.left_grip, frame.ugrip_pixel);
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.right_grip, frame.ugrip_pixel);
      }
    }
    XClearWindow(blackbox->getXDisplay(), frame.handle);
    XClearWindow(blackbox->getXDisplay(), frame.left_grip);
    XClearWindow(blackbox->getXDisplay(), frame.right_grip);
  }

  if (decorations & Decor_Border) {
    if (flags.focused)
      XSetWindowBorder(blackbox->getXDisplay(),
                       frame.plate, frame.fborder_pixel);
    else
      XSetWindowBorder(blackbox->getXDisplay(),
                       frame.plate, frame.uborder_pixel);
  }
}


void BlackboxWindow::setFocusFlag(bool focus) {
  // only focus a window if it is visible
  if (focus && ! flags.visible)
    return;

  flags.focused = focus;

  redrawWindowFrame();

  if (flags.focused)
    blackbox->setFocusedWindow(this);
}


void BlackboxWindow::installColormap(bool install) {
  int i = 0, ncmap = 0;
  Colormap *cmaps = XListInstalledColormaps(blackbox->getXDisplay(),
                                            client.window, &ncmap);
  if (cmaps) {
    XWindowAttributes wattrib;
    if (XGetWindowAttributes(blackbox->getXDisplay(),
                             client.window, &wattrib)) {
      if (install) {
        // install the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap)
            // this window is using an installed color map... do not install
            install = False;
        }
        // otherwise, install the window's colormap
        if (install)
          XInstallColormap(blackbox->getXDisplay(), wattrib.colormap);
      } else {
        // uninstall the window's colormap
        for (i = 0; i < ncmap; i++) {
          if (*(cmaps + i) == wattrib.colormap)
            // we found the colormap to uninstall
            XUninstallColormap(blackbox->getXDisplay(), wattrib.colormap);
        }
      }
    }

    XFree(cmaps);
  }
}


void BlackboxWindow::setState(unsigned long new_state) {
  current_state = new_state;

  unsigned long state[2];
  state[0] = current_state;
  state[1] = None;
  XChangeProperty(blackbox->getXDisplay(), client.window,
                  blackbox->getWMStateAtom(), blackbox->getWMStateAtom(), 32,
                  PropModeReplace, (unsigned char *) state, 2);

  XChangeProperty(blackbox->getXDisplay(), client.window,
                  blackbox->getBlackboxAttributesAtom(),
                  blackbox->getBlackboxAttributesAtom(), 32, PropModeReplace,
                  (unsigned char *) &blackbox_attrib,
                  PropBlackboxAttributesElements);
}


bool BlackboxWindow::getState(void) {
  current_state = 0;

  Atom atom_return;
  bool ret = False;
  int foo;
  unsigned long *state, ulfoo, nitems;

  if ((XGetWindowProperty(blackbox->getXDisplay(), client.window,
                          blackbox->getWMStateAtom(),
                          0l, 2l, False, blackbox->getWMStateAtom(),
                          &atom_return, &foo, &nitems, &ulfoo,
                          (unsigned char **) &state) != Success) ||
      (! state)) {
    return False;
  }

  if (nitems >= 1) {
    current_state = static_cast<unsigned long>(state[0]);

    ret = True;
  }

  XFree((void *) state);

  return ret;
}


void BlackboxWindow::restoreAttributes(void) {
  Atom atom_return;
  int foo;
  unsigned long ulfoo, nitems;

  BlackboxAttributes *net;
  int ret = XGetWindowProperty(blackbox->getXDisplay(), client.window,
                               blackbox->getBlackboxAttributesAtom(), 0l,
                               PropBlackboxAttributesElements, False,
                               blackbox->getBlackboxAttributesAtom(),
                               &atom_return, &foo, &nitems, &ulfoo,
                               (unsigned char **) &net);
  if (ret != Success || !net || nitems != PropBlackboxAttributesElements)
    return;

  if (net->flags & AttribShaded && net->attrib & AttribShaded) {
    flags.shaded = False;
    unsigned long orig_state = current_state;
    shade();

    /*
      At this point in the life of a window, current_state should only be set
      to IconicState if the window was an *icon*, not if it was shaded.
    */
    if (orig_state != IconicState)
      current_state = WithdrawnState;
  }

  if (net->workspace != screen->getCurrentWorkspaceID() &&
      net->workspace < screen->getWorkspaceCount()) {
    screen->reassociateWindow(this, net->workspace);

    // set to WithdrawnState so it will be mapped on the new workspace
    if (current_state == NormalState) current_state = WithdrawnState;
  } else if (current_state == WithdrawnState) {
    // the window is on this workspace and is Withdrawn, so it is waiting to
    // be mapped
    current_state = NormalState;
  }

  if (net->flags & AttribMaxHoriz || net->flags & AttribMaxVert) {
    blackbox_attrib.premax_x = net->premax_x;
    blackbox_attrib.premax_y = net->premax_y;
    blackbox_attrib.premax_w = net->premax_w;
    blackbox_attrib.premax_h = net->premax_h;

    flags.maximized = 0;

    if (net->flags & AttribMaxHoriz && net->flags & AttribMaxVert &&
        net->attrib & (AttribMaxHoriz | AttribMaxVert))
      flags.maximized = 1;
    else if (net->flags & AttribMaxVert && net->attrib & AttribMaxVert)
        flags.maximized = 2;
    else if (net->flags & AttribMaxHoriz && net->attrib & AttribMaxHoriz)
        flags.maximized = 3;

    if (flags.maximized) remaximize();
  }

  if (net->flags & AttribDecoration) {
    switch (net->decoration) {
    case DecorNone:
      decorations = 0;

      break;

    default:
    case DecorNormal:
      decorations |= Decor_Titlebar | Decor_Handle | Decor_Border |
        Decor_Iconify | Decor_Maximize;

      break;

    case DecorTiny:
      decorations |= Decor_Titlebar | Decor_Iconify;
      decorations &= ~(Decor_Border | Decor_Handle | Decor_Maximize);

      break;

    case DecorTool:
      decorations |= Decor_Titlebar;
      decorations &= ~(Decor_Iconify | Decor_Border | Decor_Handle);

      break;
    }

    // sanity check the new decor
    if (! (functions & Func_Resize) || isTransient())
      decorations &= ~(Decor_Maximize | Decor_Handle);
    if (! (functions & Func_Maximize))
      decorations &= ~Decor_Maximize;

    if (decorations & Decor_Titlebar) {
      if (functions & Func_Close)   // close button is controlled by function
        decorations |= Decor_Close; // not decor type
    } else {
      if (flags.shaded) // we can not be shaded if we lack a titlebar
        shade();
    }

    if (flags.visible && frame.window) {
      XMapSubwindows(blackbox->getXDisplay(), frame.window);
      XMapWindow(blackbox->getXDisplay(), frame.window);
    }

    reconfigure();
    setState(current_state);
  }

  // with the state set it will then be the map event's job to read the
  // window's state and behave accordingly

  XFree((void *) net);
}


/*
 * Positions the Rect r according the the client window position and
 * window gravity.
 */
void BlackboxWindow::applyGravity(Rect &r) {
  // apply horizontal window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(client.rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(client.rect.x() - (frame.margin.left + frame.margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(client.rect.x() - (frame.margin.left + frame.margin.right) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(client.rect.x() - frame.margin.left);
    break;
  }

  // apply vertical window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(client.rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(client.rect.y() - ((frame.margin.top + frame.margin.bottom) / 2));
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(client.rect.y() - (frame.margin.bottom + frame.margin.top) + 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(client.rect.y() - frame.margin.top);
    break;
  }
}


/*
 * The reverse of the applyGravity function.
 *
 * Positions the Rect r according to the frame window position and
 * window gravity.
 */
void BlackboxWindow::restoreGravity(Rect &r) {
  // restore horizontal window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case SouthWestGravity:
  case WestGravity:
    r.setX(frame.rect.x());
    break;

  case NorthGravity:
  case SouthGravity:
  case CenterGravity:
    r.setX(frame.rect.x() + (frame.margin.left + frame.margin.right) / 2);
    break;

  case NorthEastGravity:
  case SouthEastGravity:
  case EastGravity:
    r.setX(frame.rect.x() + (frame.margin.left + frame.margin.right) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setX(frame.rect.x() + frame.margin.left);
    break;
  }

  // restore vertical window gravity
  switch (client.win_gravity) {
  default:
  case NorthWestGravity:
  case NorthEastGravity:
  case NorthGravity:
    r.setY(frame.rect.y());
    break;

  case CenterGravity:
  case EastGravity:
  case WestGravity:
    r.setY(frame.rect.y() + (frame.margin.top + frame.margin.bottom) / 2);
    break;

  case SouthWestGravity:
  case SouthEastGravity:
  case SouthGravity:
    r.setY(frame.rect.y() + (frame.margin.top + frame.margin.bottom) - 2);
    break;

  case ForgetGravity:
  case StaticGravity:
    r.setY(frame.rect.y() + frame.margin.top);
    break;
  }
}


void BlackboxWindow::redrawLabel(void) const {
  if (flags.focused) {
    if (frame.flabel)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.label, frame.flabel);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.label, frame.flabel_pixel);
  } else {
    if (frame.ulabel)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.label, frame.ulabel);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.label, frame.ulabel_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.label);

  WindowStyle *style = screen->getWindowStyle();

  int pos = frame.bevel_w * 2,
    dlen = style->doJustify(client.title.c_str(), pos, frame.label_w,
                            frame.bevel_w * 4, i18n.multibyte());

  BPen pen((flags.focused) ? style->l_text_focus : style->l_text_unfocus,
           style->font);
  if (i18n.multibyte())
    XmbDrawString(blackbox->getXDisplay(), frame.label, style->fontset,
                  pen.gc(), pos,
                  (1 - style->fontset_extents->max_ink_extent.y),
                  client.title.c_str(), dlen);
  else
    XDrawString(blackbox->getXDisplay(), frame.label, pen.gc(), pos,
                (style->font->ascent + 1), client.title.c_str(), dlen);
}


void BlackboxWindow::redrawAllButtons(void) const {
  if (frame.iconify_button) redrawIconifyButton(False);
  if (frame.maximize_button) redrawMaximizeButton(flags.maximized);
  if (frame.close_button) redrawCloseButton(False);
}


void BlackboxWindow::redrawIconifyButton(bool pressed) const {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.iconify_button, frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(),
                             frame.iconify_button, frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.iconify_button, frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.iconify_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.iconify_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.iconify_button, frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.iconify_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawRectangle(blackbox->getXDisplay(), frame.iconify_button, pen.gc(),
                 2, (frame.button_w - 5), (frame.button_w - 5), 2);
}


void BlackboxWindow::redrawMaximizeButton(bool pressed) const {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.maximize_button, frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                             frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                   frame.maximize_button, frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.maximize_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(), frame.maximize_button,
                           frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.maximize_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawRectangle(blackbox->getXDisplay(), frame.maximize_button, pen.gc(),
                 2, 2, (frame.button_w - 5), (frame.button_w - 5));
  XDrawLine(blackbox->getXDisplay(), frame.maximize_button, pen.gc(),
            2, 3, (frame.button_w - 3), 3);
}


void BlackboxWindow::redrawCloseButton(bool pressed) const {
  if (! pressed) {
    if (flags.focused) {
      if (frame.fbutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(), frame.close_button,
                                   frame.fbutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.close_button,
                             frame.fbutton_pixel);
    } else {
      if (frame.ubutton)
        XSetWindowBackgroundPixmap(blackbox->getXDisplay(), frame.close_button,
                                   frame.ubutton);
      else
        XSetWindowBackground(blackbox->getXDisplay(), frame.close_button,
                             frame.ubutton_pixel);
    }
  } else {
    if (frame.pbutton)
      XSetWindowBackgroundPixmap(blackbox->getXDisplay(),
                                 frame.close_button, frame.pbutton);
    else
      XSetWindowBackground(blackbox->getXDisplay(),
                           frame.close_button, frame.pbutton_pixel);
  }
  XClearWindow(blackbox->getXDisplay(), frame.close_button);

  BPen pen((flags.focused) ? screen->getWindowStyle()->b_pic_focus :
           screen->getWindowStyle()->b_pic_unfocus);
  XDrawLine(blackbox->getXDisplay(), frame.close_button, pen.gc(),
            2, 2, (frame.button_w - 3), (frame.button_w - 3));
  XDrawLine(blackbox->getXDisplay(), frame.close_button, pen.gc(),
            2, (frame.button_w - 3), (frame.button_w - 3), 2);
}


void BlackboxWindow::netwmMoveResize(const XClientMessageEvent* const ce) {
  XConfigureRequestEvent request;
  request.window = ce->window;
  request.x = ce->data.l[1];
  request.y = ce->data.l[2];
  request.width = ce->data.l[3];
  request.height = ce->data.l[4];
  request.value_mask = CWX | CWY | CWWidth | CWHeight;

  const int old_gravity = client.win_gravity;
  if (ce->data.l[0] != 0)
    client.win_gravity = ce->data.l[0];

  configureRequestEvent(&request);

  client.win_gravity = old_gravity;
}


void BlackboxWindow::mapRequestEvent(const XMapRequestEvent* const re) {
  if (re->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::mapRequestEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  switch (current_state) {
  case IconicState:
    iconify();
    break;

  case WithdrawnState:
    withdraw();
    break;

  case NormalState:
  case InactiveState:
  case ZoomState:
  default:
    show();
    screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
    if (! blackbox->isStartup() && (isTransient() || screen->doFocusNew())) {
      XSync(blackbox->getXDisplay(), False); // make sure the frame is mapped..
      setInputFocus();
    }
    break;
  }
}


void BlackboxWindow::unmapNotifyEvent(const XUnmapEvent *ue) {
  if (ue->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::unmapNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  screen->releaseWindow(this, False);
}


void BlackboxWindow::destroyNotifyEvent(const XDestroyWindowEvent *de) {
  if (de->window != client.window)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::destroyNotifyEvent() for 0x%lx\n",
          client.window);
#endif // DEBUG

  screen->releaseWindow(this, False);
}


void BlackboxWindow::reparentNotifyEvent(const XReparentEvent *re) {
  if (re->window != client.window || re->parent == frame.plate)
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::reparentNotifyEvent(): reparent 0x%lx to "
          "0x%lx.\n", client.window, re->parent);
#endif // DEBUG

  XEvent ev;
  ev.xreparent = *re;
  XPutBackEvent(blackbox->getXDisplay(), &ev);
  screen->releaseWindow(this, True);
}


void BlackboxWindow::propertyNotifyEvent(const XPropertyEvent *pe) {
  if (pe->state == PropertyDelete || ! validateClient())
    return;

#ifdef    DEBUG
  fprintf(stderr, "BlackboxWindow::propertyNotifyEvent(): for 0x%lx\n",
          client.window);
#endif

  switch(pe->atom) {
  case XA_WM_CLASS:
  case XA_WM_CLIENT_MACHINE:
  case XA_WM_COMMAND:
    break;

  case XA_WM_TRANSIENT_FOR: {
    // determine if this is a transient window
    getTransientInfo();

    // adjust the window decorations based on transience
    if (isTransient()) {
      decorations &= ~(Decor_Maximize | Decor_Handle);
      functions &= ~Func_Maximize;
    }

    reconfigure();
  }
    break;

  case XA_WM_HINTS:
    getWMHints();
    break;

  case XA_WM_ICON_NAME:
    getWMIconName();
    if (flags.iconic) screen->propagateWindowName(this);
    break;

  case XA_WM_NAME:
    getWMName();

    if (decorations & Decor_Titlebar)
      redrawLabel();

    screen->propagateWindowName(this);
    break;

  case XA_WM_NORMAL_HINTS: {
    getWMNormalHints();

    if ((client.normal_hint_flags & PMinSize) &&
        (client.normal_hint_flags & PMaxSize)) {
      // the window now can/can't resize itself, so the buttons need to be
      // regrabbed.
      ungrabButtons();
      if (client.max_width <= client.min_width &&
          client.max_height <= client.min_height) {
        decorations &= ~(Decor_Maximize | Decor_Handle);
        functions &= ~(Func_Resize | Func_Maximize);
      } else {
        if (! isTransient()) {
          decorations |= Decor_Maximize | Decor_Handle;
          functions |= Func_Maximize;
        }
        functions |= Func_Resize;
      }
      grabButtons();
    }

    Rect old_rect = frame.rect;

    upsize();

    if (old_rect != frame.rect)
      reconfigure();

    break;
  }

  default:
    if (pe->atom == blackbox->getWMProtocolsAtom()) {
      getWMProtocols();

      if (decorations & Decor_Close && ! frame.close_button) {
        createCloseButton();
        if (decorations & Decor_Titlebar) {
          positionButtons(True);
          XMapSubwindows(blackbox->getXDisplay(), frame.title);
        }
        if (windowmenu) windowmenu->reconfigure();
      }
    }

    break;
  }
}


void BlackboxWindow::exposeEvent(const XExposeEvent *ee) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::exposeEvent() for 0x%lx\n", client.window);
#endif

  if (frame.label == ee->window && (decorations & Decor_Titlebar))
    redrawLabel();
  else if (frame.close_button == ee->window)
    redrawCloseButton(False);
  else if (frame.maximize_button == ee->window)
    redrawMaximizeButton(flags.maximized);
  else if (frame.iconify_button == ee->window)
    redrawIconifyButton(False);
}


void BlackboxWindow::configureRequestEvent(const XConfigureRequestEvent *cr) {
  if (cr->window != client.window || flags.iconic)
    return;

  if (cr->value_mask & CWBorderWidth)
    client.old_bw = cr->border_width;

  if (cr->value_mask & (CWX | CWY | CWWidth | CWHeight)) {
    Rect req = frame.rect;

    if (cr->value_mask & (CWX | CWY)) {
      if (cr->value_mask & CWX)
        client.rect.setX(cr->x);
      if (cr->value_mask & CWY)
        client.rect.setY(cr->y);

      applyGravity(req);
    }

    if (cr->value_mask & CWWidth)
      req.setWidth(cr->width + frame.margin.left + frame.margin.right);

    if (cr->value_mask & CWHeight)
      req.setHeight(cr->height + frame.margin.top + frame.margin.bottom);

    configure(req.x(), req.y(), req.width(), req.height());
  }

  if (cr->value_mask & CWStackMode) {
    switch (cr->detail) {
    case Below:
    case BottomIf:
      screen->getWorkspace(blackbox_attrib.workspace)->lowerWindow(this);
      break;

    case Above:
    case TopIf:
    default:
      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
      break;
    }
  }
}


void BlackboxWindow::buttonPressEvent(const XButtonEvent * const be) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonPressEvent() for 0x%lx\n",
          client.window);
#endif

  if (frame.maximize_button == be->window) {
    redrawMaximizeButton(True);
  } else if (be->button == 1 || (be->button == 3 && be->state == Mod1Mask)) {
    if (! flags.focused)
      setInputFocus();

    if (frame.iconify_button == be->window) {
      redrawIconifyButton(True);
    } else if (frame.close_button == be->window) {
      redrawCloseButton(True);
    } else if (frame.plate == be->window) {
      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);

      XAllowEvents(blackbox->getXDisplay(), ReplayPointer, be->time);
    } else {
      if (frame.title == be->window || frame.label == be->window) {
        if (((be->time - lastButtonPressTime) <=
             blackbox->getDoubleClickInterval()) ||
            (be->state == ControlMask)) {
          lastButtonPressTime = 0;
          shade();
        } else {
          lastButtonPressTime = be->time;
        }
      }

      frame.grab_x = be->x_root - frame.rect.x() - frame.border_w;
      frame.grab_y = be->y_root - frame.rect.y() - frame.border_w;

      if (windowmenu && windowmenu->isVisible()) windowmenu->hide();

      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
    }
  } else if (be->button == 2 && (be->window != frame.iconify_button) &&
             (be->window != frame.close_button)) {
    screen->getWorkspace(blackbox_attrib.workspace)->lowerWindow(this);
  } else if (windowmenu && be->button == 3 &&
             (frame.title == be->window || frame.label == be->window ||
              frame.handle == be->window || frame.window == be->window)) {
    if (windowmenu->isVisible()) {
      windowmenu->hide();
    } else {
      int mx = be->x_root - windowmenu->getWidth() / 2,
          my = be->y_root - windowmenu->getHeight() / 2;

      // snap the window menu into a corner/side if necessary
      int left_edge, right_edge, top_edge, bottom_edge;

      /*
         the " + (frame.border_w * 2) - 1" bits are to get the proper width
         and height of the menu, as the sizes returned by it do not include
         the borders.
       */
      left_edge = frame.rect.x();
      right_edge = frame.rect.right() -
        (windowmenu->getWidth() + (frame.border_w * 2) - 1);
      top_edge = client.rect.top() - (frame.border_w + frame.mwm_border_w);
      bottom_edge = client.rect.bottom() -
        (windowmenu->getHeight() + (frame.border_w * 2) - 1) +
        (frame.border_w + frame.mwm_border_w);

      if (mx < left_edge)
        mx = left_edge;
      if (mx > right_edge)
        mx = right_edge;
      if (my < top_edge)
        my = top_edge;
      if (my > bottom_edge)
        my = bottom_edge;

      windowmenu->move(mx, my);
      windowmenu->show();
      XRaiseWindow(blackbox->getXDisplay(), windowmenu->getWindowID());
      XRaiseWindow(blackbox->getXDisplay(),
                   windowmenu->getSendToMenu()->getWindowID());
    }
  }
}


void BlackboxWindow::buttonReleaseEvent(const XButtonEvent * const re) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::buttonReleaseEvent() for 0x%lx\n",
          client.window);
#endif

  if (re->window == frame.maximize_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w))) {
      maximize(re->button);
      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
    } else {
      redrawMaximizeButton(flags.maximized);
    }
  } else if (re->window == frame.iconify_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w))) {
      iconify();
    } else {
      redrawIconifyButton(False);
    }
  } else if (re->window == frame.close_button) {
    if ((re->x >= 0 && re->x <= static_cast<signed>(frame.button_w)) &&
        (re->y >= 0 && re->y <= static_cast<signed>(frame.button_w)))
      close();
    redrawCloseButton(False);
  } else if (flags.moving) {
    flags.moving = False;

    if (! screen->doOpaqueMove()) {
      /* when drawing the rubber band, we need to make sure we only draw inside
       * the frame... frame.changing_* contain the new coords for the window,
       * so we need to subtract 1 from changing_w/changing_h every where we
       * draw the rubber band (for both moving and resizing)
       */
      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);
      XUngrabServer(blackbox->getXDisplay());

      configure(frame.changing.x(), frame.changing.y(),
                frame.changing.width(), frame.changing.height());
    } else {
      configure(frame.rect.x(), frame.rect.y(),
                frame.rect.width(), frame.rect.height());
    }
    screen->hideGeometry();
    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  } else if (flags.resizing) {
    XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                   screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                   frame.changing.width() - 1, frame.changing.height() - 1);
    XUngrabServer(blackbox->getXDisplay());

    screen->hideGeometry();

    constrain((re->window == frame.left_grip) ? TopRight : TopLeft);

    // unset maximized state when resized after fully maximized
    if (flags.maximized == 1)
      maximize(0);
    flags.resizing = False;
    configure(frame.changing.x(), frame.changing.y(),
              frame.changing.width(), frame.changing.height());

    XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  } else if (re->window == frame.window) {
    if (re->button == 2 && re->state == Mod1Mask)
      XUngrabPointer(blackbox->getXDisplay(), CurrentTime);
  }
}


void BlackboxWindow::motionNotifyEvent(const XMotionEvent *me) {
#ifdef DEBUG
  fprintf(stderr, "BlackboxWindow::motionNotifyEvent() for 0x%lx\n",
          client.window);
#endif

  if (!flags.resizing && me->state & Button1Mask && (functions & Func_Move) &&
      (frame.title == me->window || frame.label == me->window ||
       frame.handle == me->window || frame.window == me->window)) {
    if (! flags.moving) {
      XGrabPointer(blackbox->getXDisplay(), me->window, False,
                   Button1MotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync,
                   None, blackbox->getMoveCursor(), CurrentTime);

      if (windowmenu && windowmenu->isVisible())
        windowmenu->hide();

      flags.moving = True;

      if (! screen->doOpaqueMove()) {
        XGrabServer(blackbox->getXDisplay());

        frame.changing = frame.rect;
        screen->showPosition(frame.changing.x(), frame.changing.y());

        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);
      }
    } else {
      int dx = me->x_root - frame.grab_x, dy = me->y_root - frame.grab_y;
      dx -= frame.border_w;
      dy -= frame.border_w;

      const int snap_distance = screen->getEdgeSnapThreshold();

      if (snap_distance) {
        Rect srect = screen->availableArea();
        // window corners
        const int wleft = dx,
                 wright = dx + frame.rect.width() - 1,
                   wtop = dy,
                wbottom = dy + frame.rect.height() - 1;

        int dleft = abs(wleft - srect.left()),
           dright = abs(wright - srect.right()),
             dtop = abs(wtop - srect.top()),
          dbottom = abs(wbottom - srect.bottom());

        // snap left?
        if (dleft < snap_distance && dleft <= dright)
          dx = srect.left();
        // snap right?
        else if (dright < snap_distance)
          dx = srect.right() - frame.rect.width() + 1;

        // snap top?
        if (dtop < snap_distance && dtop <= dbottom)
          dy = srect.top();
        // snap bottom?
        else if (dbottom < snap_distance)
          dy = srect.bottom() - frame.rect.height() + 1;

        if (! screen->doFullMax()) {
          srect = screen->getRect(); // now get the full screen

          dleft = abs(wleft - srect.left()),
             dright = abs(wright - srect.right()),
               dtop = abs(wtop - srect.top()),
            dbottom = abs(wbottom - srect.bottom());

          // snap left?
          if (dleft < snap_distance && dleft <= dright)
            dx = srect.left();
          // snap right?
          else if (dright < snap_distance)
            dx = srect.right() - frame.rect.width() + 1;

          // snap top?
          if (dtop < snap_distance && dtop <= dbottom)
            dy = srect.top();
          // snap bottom?
          else if (dbottom < snap_distance)
            dy = srect.bottom() - frame.rect.height() + 1;
        }
      }

      if (screen->doOpaqueMove()) {
        configure(dx, dy, frame.rect.width(), frame.rect.height());
      } else {
        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);

        frame.changing.setPos(dx, dy);

        XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                       screen->getOpGC(),
                       frame.changing.x(),
                       frame.changing.y(),
                       frame.changing.width() - 1,
                       frame.changing.height() - 1);
      }

      screen->showPosition(dx, dy);
    }
  } else if ((functions & Func_Resize) &&
             (me->state & Button1Mask && (me->window == frame.right_grip ||
                                          me->window == frame.left_grip)) ||
             (me->state & Button3Mask && me->state & Mod1Mask &&
              me->window == frame.window)) {
    bool left = (me->window == frame.left_grip);

    if (! flags.resizing) {
      XGrabServer(blackbox->getXDisplay());
      XGrabPointer(blackbox->getXDisplay(), me->window, False,
                   ButtonMotionMask | ButtonReleaseMask,
                   GrabModeAsync, GrabModeAsync, None,
                   ((left) ? blackbox->getLowerLeftAngleCursor() :
                    blackbox->getLowerRightAngleCursor()),
                   CurrentTime);

      flags.resizing = True;

      unsigned int gw, gh;
      frame.grab_x = me->x;
      frame.grab_y = me->y;
      frame.changing = frame.rect;

      constrain((left) ? TopRight : TopLeft, &gw, &gh);

      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      screen->showGeometry(gw, gh);
    } else {
      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      unsigned int gw, gh;

      Corner anchor;

      if (left) {
        anchor = TopRight;
        frame.changing.setCoords(me->x_root - frame.grab_x, frame.rect.top(),
                                 frame.rect.right(), frame.rect.bottom());
        frame.changing.setHeight(frame.rect.height() + (me->y - frame.grab_y));
      } else {
        anchor = TopLeft;
        frame.changing.setSize(frame.rect.width() + (me->x - frame.grab_x),
                               frame.rect.height() + (me->y - frame.grab_y));
      }

      constrain(anchor, &gw, &gh);

      XDrawRectangle(blackbox->getXDisplay(), screen->getRootWindow(),
                     screen->getOpGC(), frame.changing.x(), frame.changing.y(),
                     frame.changing.width() - 1, frame.changing.height() - 1);

      screen->showGeometry(gw, gh);
    }
  }
}


void BlackboxWindow::enterNotifyEvent(const XCrossingEvent* ce) {
  if (! (screen->isSloppyFocus() && isVisible()))
    return;

  XEvent e;
  bool leave = False, inferior = False;

  while (XCheckTypedWindowEvent(blackbox->getXDisplay(), ce->window,
                                LeaveNotify, &e)) {
    if (e.type == LeaveNotify && e.xcrossing.mode == NotifyNormal) {
      leave = True;
      inferior = (e.xcrossing.detail == NotifyInferior);
    }
  }

  if ((! leave || inferior) && ! isFocused()) {
    bool success = setInputFocus();
    if (success)    // if focus succeeded install the colormap
      installColormap(True); // XXX: shouldnt we honour no install?
  }

  if (screen->doAutoRaise())
    timer->start();
}


void BlackboxWindow::leaveNotifyEvent(const XCrossingEvent*) {
  if (! (screen->isSloppyFocus() && screen->doAutoRaise()))
    return;

  installColormap(False);

  if (timer->isTiming())
    timer->stop();
}


#ifdef    SHAPE
void BlackboxWindow::shapeEvent(const XShapeEvent * const) {
  if (blackbox->hasShapeExtensions() && flags.shaped) {
    configureShape();
  }
}
#endif // SHAPE


bool BlackboxWindow::validateClient(void) const {
  XSync(blackbox->getXDisplay(), False);

  XEvent e;
  if (XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                             DestroyNotify, &e) ||
      XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                             UnmapNotify, &e)) {
    XPutBackEvent(blackbox->getXDisplay(), &e);

    return False;
  }

  return True;
}


void BlackboxWindow::restore(bool remap) {
  XChangeSaveSet(blackbox->getXDisplay(), client.window, SetModeDelete);
  XSelectInput(blackbox->getXDisplay(), client.window, NoEventMask);
  XSelectInput(blackbox->getXDisplay(), frame.plate, NoEventMask);

  // do not leave a shaded window as an icon unless it was an icon
  if (flags.shaded && ! flags.iconic)
    current_state = NormalState;

  setState(current_state);

  restoreGravity(client.rect);

  XGrabServer(blackbox->getXDisplay());

  XUnmapWindow(blackbox->getXDisplay(), frame.window);
  XUnmapWindow(blackbox->getXDisplay(), client.window);

  XSetWindowBorderWidth(blackbox->getXDisplay(), client.window, client.old_bw);
  XMoveWindow(blackbox->getXDisplay(), client.window,
              client.rect.x() - frame.rect.x(),
              client.rect.y() - frame.rect.y());

  XUngrabServer(blackbox->getXDisplay());

  XEvent ev;
  if (XCheckTypedWindowEvent(blackbox->getXDisplay(), client.window,
                             ReparentNotify, &ev)) {
    remap = True;
  } else {
    // according to the ICCCM - if the client doesn't reparent to
    // root, then we have to do it for them
    XReparentWindow(blackbox->getXDisplay(), client.window,
                    screen->getRootWindow(),
                    client.rect.x(), client.rect.y());
  }

  if (remap) XMapWindow(blackbox->getXDisplay(), client.window);
}


// timer for autoraise
void BlackboxWindow::timeout(void) {
  screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
}


void BlackboxWindow::changeBlackboxHints(const BlackboxHints *net) {
  if ((net->flags & AttribShaded) &&
      ((blackbox_attrib.attrib & AttribShaded) !=
       (net->attrib & AttribShaded)))
    shade();

  if (flags.visible && // watch out for requests when we can not be seen
      (net->flags & (AttribMaxVert | AttribMaxHoriz)) &&
      ((blackbox_attrib.attrib & (AttribMaxVert | AttribMaxHoriz)) !=
       (net->attrib & (AttribMaxVert | AttribMaxHoriz)))) {
    if (flags.maximized) {
      maximize(0);
    } else {
      int button = 0;

      if (net->flags & AttribMaxHoriz && net->flags & AttribMaxVert &&
          net->attrib & (AttribMaxHoriz | AttribMaxVert))
        button = 1;
      else if (net->flags & AttribMaxVert && net->attrib & AttribMaxVert)
        button = 2;
      else if (net->flags & AttribMaxHoriz && net->attrib & AttribMaxHoriz)
        button = 3;

      maximize(button);
    }
  }

  if ((net->flags & AttribWorkspace) &&
      (blackbox_attrib.workspace != net->workspace)) {
    screen->reassociateWindow(this, net->workspace);

    if (screen->getCurrentWorkspaceID() != net->workspace) {
      withdraw();
    } else {
      show();
      screen->getWorkspace(blackbox_attrib.workspace)->raiseWindow(this);
    }
  }

  if (net->flags & AttribDecoration) {
    switch (net->decoration) {
    case DecorNone:
      decorations = 0;

      break;

    default:
    case DecorNormal:
      decorations |= Decor_Titlebar | Decor_Handle | Decor_Border |
        Decor_Iconify | Decor_Maximize;

      break;

    case DecorTiny:
      decorations |= Decor_Titlebar | Decor_Iconify;
      decorations &= ~(Decor_Border | Decor_Handle | Decor_Maximize);

      break;

    case DecorTool:
      decorations |= Decor_Titlebar;
      decorations &= ~(Decor_Iconify | Decor_Border | Decor_Handle);

      break;
    }

    // sanity check the new decor
    if (! (functions & Func_Resize) || isTransient())
      decorations &= ~(Decor_Maximize | Decor_Handle);
    if (! (functions & Func_Maximize))
      decorations &= ~Decor_Maximize;
    if (! (functions & Func_Iconify))
      decorations &= ~Decor_Iconify;
    if (decorations & Decor_Titlebar) {
      if (functions & Func_Close)   // close button is controlled by function
        decorations |= Decor_Close; // not decor type
    } else {
      if (flags.shaded) // we can not be shaded if we lack a titlebar
        shade();
    }

    if (flags.visible && frame.window) {
      XMapSubwindows(blackbox->getXDisplay(), frame.window);
      XMapWindow(blackbox->getXDisplay(), frame.window);
    }

    reconfigure();
    setState(current_state);
  }
}


/*
 * Set the sizes of all components of the window frame
 * (the window decorations).
 * These values are based upon the current style settings and the client
 * window's dimensions.
 */
void BlackboxWindow::upsize(void) {
  frame.bevel_w = screen->getBevelWidth();

  if (decorations & Decor_Border) {
    frame.border_w = screen->getBorderWidth();
    if (!isTransient())
      frame.mwm_border_w = screen->getFrameWidth();
    else
      frame.mwm_border_w = 0;
  } else {
    frame.mwm_border_w = frame.border_w = 0;
  }

  if (decorations & Decor_Titlebar) {
    // the height of the titlebar is based upon the height of the font being
    // used to display the window's title
    WindowStyle *style = screen->getWindowStyle();
    if (i18n.multibyte())
      frame.title_h = (style->fontset_extents->max_ink_extent.height +
                       (frame.bevel_w * 2) + 2);
    else
      frame.title_h = (style->font->ascent + style->font->descent +
                       (frame.bevel_w * 2) + 2);

    frame.label_h = frame.title_h - (frame.bevel_w * 2);
    frame.button_w = (frame.label_h - 2);

    // set the top frame margin
    frame.margin.top = frame.border_w + frame.title_h +
                       frame.border_w + frame.mwm_border_w;
  } else {
    frame.title_h = 0;
    frame.label_h = 0;
    frame.button_w = 0;

    // set the top frame margin
    frame.margin.top = frame.border_w + frame.mwm_border_w;
  }

  // set the left/right frame margin
  frame.margin.left = frame.margin.right = frame.border_w + frame.mwm_border_w;

  if (decorations & Decor_Handle) {
    frame.grip_w = frame.button_w * 2;
    frame.handle_h = screen->getHandleWidth();

    // set the bottom frame margin
    frame.margin.bottom = frame.border_w + frame.handle_h +
                          frame.border_w + frame.mwm_border_w;
  } else {
    frame.handle_h = 0;
    frame.grip_w = 0;

    // set the bottom frame margin
    frame.margin.bottom = frame.border_w + frame.mwm_border_w;
  }

  /*
    We first get the normal dimensions and use this to define the inside_w/h
    then we modify the height if shading is in effect.
    If the shade state is not considered then frame.rect gets reset to the
    normal window size on a reconfigure() call resulting in improper
    dimensions appearing in move/resize and other events.
  */
  unsigned int
    height = client.rect.height() + frame.margin.top + frame.margin.bottom,
    width = client.rect.width() + frame.margin.left + frame.margin.right;

  frame.inside_w = width - (frame.border_w * 2);
  frame.inside_h = height - (frame.border_w * 2);

  if (flags.shaded)
    height = frame.title_h + (frame.border_w * 2);
  frame.rect.setSize(width, height);
}


/*
 * Calculate the size of the client window and constrain it to the
 * size specified by the size hints of the client window.
 *
 * The logical width and height are placed into pw and ph, if they
 * are non-zero.  Logical size refers to the users perception of
 * the window size (for example an xterm resizes in cells, not in pixels).
 * pw and ph are then used to display the geometry during window moves, resize,
 * etc.
 *
 * The physical geometry is placed into frame.changing_{x,y,width,height}.
 * Physical geometry refers to the geometry of the window in pixels.
 */
void BlackboxWindow::constrain(Corner anchor,
                               unsigned int *pw, unsigned int *ph) {
  // frame.changing represents the requested frame size, we need to
  // strip the frame margin off and constrain the client size
  frame.changing.setCoords(frame.changing.left() + frame.margin.left,
                           frame.changing.top() + frame.margin.top,
                           frame.changing.right() - frame.margin.right,
                           frame.changing.bottom() - frame.margin.bottom);

  unsigned int dw = frame.changing.width(), dh = frame.changing.height(),
    base_width = (client.base_width) ? client.base_width : client.min_width,
    base_height = (client.base_height) ? client.base_height :
                                         client.min_height;

  // constrain
  if (dw < client.min_width) dw = client.min_width;
  if (dh < client.min_height) dh = client.min_height;
  if (dw > client.max_width) dw = client.max_width;
  if (dh > client.max_height) dh = client.max_height;

  assert(dw >= base_width && dh >= base_height);

  if (client.width_inc > 1) {
    dw -= base_width;
    dw /= client.width_inc;
  }
  if (client.height_inc > 1) {
    dh -= base_height;
    dh /= client.height_inc;
  }

  if (pw)
      *pw = dw;

  if (ph)
      *ph = dh;

  if (client.width_inc > 1) {
    dw *= client.width_inc;
    dw += base_width;
  }
  if (client.height_inc > 1) {
    dh *= client.height_inc;
    dh += base_height;
  }

  frame.changing.setSize(dw, dh);

  // add the frame margin back onto frame.changing
  frame.changing.setCoords(frame.changing.left() - frame.margin.left,
                           frame.changing.top() - frame.margin.top,
                           frame.changing.right() + frame.margin.right,
                           frame.changing.bottom() + frame.margin.bottom);

  // move frame.changing to the specified anchor
  switch (anchor) {
  case TopLeft:
    // nothing to do
    break;

  case TopRight:
    int dx = frame.rect.right() - frame.changing.right();
    frame.changing.setPos(frame.changing.x() + dx, frame.changing.y());
    break;
  }
}


int WindowStyle::doJustify(const char *text, int &start_pos,
                           unsigned int max_length, unsigned int modifier,
                           bool multibyte) const {
  size_t text_len = strlen(text);
  unsigned int length;

  do {
    if (multibyte) {
      XRectangle ink, logical;
      XmbTextExtents(fontset, text, text_len, &ink, &logical);
      length = logical.width;
    } else {
      length = XTextWidth(font, text, text_len);
    }
    length += modifier;
  } while (length > max_length && text_len-- > 0);

  switch (justify) {
  case RightJustify:
    start_pos += max_length - length;
    break;

  case CenterJustify:
    start_pos += (max_length - length) / 2;
    break;

  case LeftJustify:
  default:
    break;
  }

  return text_len;
}


BWindowGroup::BWindowGroup(Blackbox *b, Window _group)
  : blackbox(b), group(_group) {
  XWindowAttributes wattrib;
  if (! XGetWindowAttributes(blackbox->getXDisplay(), group, &wattrib)) {
    // group window doesn't seem to exist anymore
    delete this;
    return;
  }

  XSelectInput(blackbox->getXDisplay(), group,
               PropertyChangeMask | FocusChangeMask | StructureNotifyMask);

  blackbox->insertWindowGroup(group, this);
}


BWindowGroup::~BWindowGroup(void) {
  blackbox->removeWindowGroup(group);
}


BlackboxWindow *
BWindowGroup::find(BScreen *screen, bool allow_transients) const {
  BlackboxWindow *ret = blackbox->getFocusedWindow();

  // does the focus window match (or any transient_fors)?
  for (; ret; ret = ret->getTransientFor()) {
    if (ret->getScreen() == screen && ret->getGroupWindow() == group &&
        (! ret->isTransient() || allow_transients))
      break;
  }

  if (ret) return ret;

  // the focus window didn't match, look in the group's window list
  BlackboxWindowList::const_iterator it, end = windowList.end();
  for (it = windowList.begin(); it != end; ++it) {
    ret = *it;
    if (ret->getScreen() == screen && ret->getGroupWindow() == group &&
        (! ret->isTransient() || allow_transients))
      break;
  }

  return ret;
}

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

#include <list>

class BlackboxWindow;

class StackingList {
public:
  typedef std::list<BlackboxWindow*> WindowStack;
  typedef WindowStack::iterator iterator;
  typedef WindowStack::reverse_iterator reverse_iterator;
  typedef WindowStack::const_iterator const_iterator;
  typedef WindowStack::const_reverse_iterator const_reverse_iterator;

  StackingList(void);
  void insert(BlackboxWindow* w);
  void append(BlackboxWindow* w);
  void remove(BlackboxWindow* w);
  void dump(void) const;
  iterator& findLayer(const BlackboxWindow* const w);

  bool empty(void) const { return (stack.size() == 5); }
  WindowStack::size_type size(void) const { return stack.size() - 5; }
  BlackboxWindow* front(void) const;
  BlackboxWindow* back(void) const;
  iterator begin(void) { return stack.begin(); }
  iterator end(void) { return stack.end(); }
  reverse_iterator rbegin(void) { return stack.rbegin(); }
  reverse_iterator rend(void) { return stack.rend(); }
  const_iterator begin(void) const { return stack.begin(); }
  const_iterator end(void) const { return stack.end(); }
  const_reverse_iterator rbegin(void) const { return stack.rbegin(); }
  const_reverse_iterator rend(void) const { return stack.rend(); }

private:
  WindowStack stack;
  iterator fullscreen, above, normal, below, desktop;
};

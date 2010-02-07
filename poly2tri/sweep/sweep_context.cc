/* 
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "sweep_context.h"
#include <algorithm>
#include <GL/glfw.h>
#include "advancing_front.h"

namespace p2t {

SweepContext::SweepContext(std::vector<Point*> polyline)
{
  basin = Basin();
  edge_event = EdgeEvent();

  points_ = polyline;

  InitEdges(points_);
}

void SweepContext::AddHole(std::vector<Point*> polyline)
{
  InitEdges(polyline);
  for(int i = 0; i < polyline.size(); i++) {
    points_.push_back(polyline[i]);
  }
}

void SweepContext::AddPoint(Point* point) {
  points_.push_back(point);
}

std::vector<Triangle*> SweepContext::GetTriangles()
{
  return triangles_;
}

std::list<Triangle*> SweepContext::GetMap()
{
  return map_;
}

void SweepContext::InitTriangulation()
{
  double xmax(points_[0]->x), xmin(points_[0]->x);
  double ymax(points_[0]->y), ymin(points_[0]->y);

  // Calculate bounds.
  for (int i = 0; i < points_.size(); i++) {
    Point& p = *points_[i];
    if (p.x > xmax)
      xmax = p.x;
    if (p.x < xmin)
      xmin = p.x;
    if (p.y > ymax)
      ymax = p.y;
    if (p.y < ymin)
      ymin = p.y;
  }

  double dx = kAlpha * (xmax - xmin);
  double dy = kAlpha * (ymax - ymin);
  head_ = new Point(xmax + dx, ymin - dy);
  tail_ = new Point(xmin - dx, ymin - dy);

  // Sort points along y-axis
  std::sort(points_.begin(), points_.end(), cmp);
  
}

void SweepContext::InitEdges(std::vector<Point*> polyline)
{
  int num_points = polyline.size();
  for (int i = 0; i < num_points; i++) {
    int j = i < num_points - 1 ? i + 1 : 0;
    edge_list.push_back(new Edge(*polyline[i], *polyline[j]));
  }
}

Point* SweepContext::GetPoint(const int& index)
{
  return points_[index];
}

void SweepContext::AddToMap(Triangle* triangle)
{
  map_.push_back(triangle);
}

Node& SweepContext::LocateNode(Point& point)
{
  // TODO implement search tree
  return *front_->LocateNode(point.x);
}

void SweepContext::CreateAdvancingFront()
{
  Node *head, *middle, *tail;
  // Initial triangle
  Triangle* triangle = new Triangle(*points_[0], *tail_, *head_);

  map_.push_back(triangle);

  head = new Node(*triangle->GetPoint(1), *triangle);
  middle = new Node(*triangle->GetPoint(0), *triangle);
  tail = new Node(*triangle->GetPoint(2));

  front_ = new AdvancingFront(*head, *tail); 
  
  // TODO: More intuitive if head is middles next and not previous?
  //       so swap head and tail
  head->next = middle;
  middle->next = tail;
  middle->prev = head;
  tail->prev = middle;
}

void SweepContext::RemoveNode(Node* node)
{
  delete node;
}

void SweepContext::MapTriangleToNodes(Triangle& t)
{
  for (int i = 0; i < 3; i++) {
    if (!t.GetNeighbor(i)) {
      Node* n = front_->LocatePoint(t.PointCW(*t.GetPoint(i)));
      if (n)
        n->triangle = &t;
    }
  }
}

void SweepContext::RemoveFromMap(Triangle* triangle)
{
  map_.remove(triangle);
}

void SweepContext::MeshClean(Triangle& triangle)
{
  if (&triangle != NULL && !triangle.IsInterior()) {
    triangle.IsInterior(true);
    triangles_.push_back(&triangle);
    for (int i = 0; i < 3; i++) {
      if (!triangle.constrained_edge[i])
        MeshClean(*triangle.GetNeighbor(i));
    }
  }
}

SweepContext::~SweepContext()
{
  delete head_;
  delete tail_;
  delete front_;
}

}
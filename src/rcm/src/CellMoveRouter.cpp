#include "rcm/CellMoveRouter.h"
#include "odb/db.h"
#include "gui/gui.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "grt/GlobalRouter.h"

#include <vector>
#include <iostream>

namespace rcm {

class RectangleRender : public gui::Renderer
{
public:
  virtual void drawObjects(gui::Painter& /* painter */) override;

  void addRectangle(odb::Rect rect){ rectangles_.push_back(rect); };
  void clear_rectangles();
  
private:
  std::vector<odb::Rect> rectangles_;
};

void
RectangleRender::drawObjects(gui::Painter &painter)
{
  for(int i; i < rectangles_.size();i++)
  {
    if (i == rectangles_.size()-1)
    {
      painter.setBrush(gui::Painter::dark_green);// Try other colors
      painter.drawRect(rectangles_[i]);
    } else
    {
      painter.setBrush(gui::Painter::dark_red);// Try other colors
      painter.drawRect(rectangles_[i]);
    }
  }
}

void
RectangleRender::clear_rectangles()
{
  rectangles_.clear();
}


CellMoveRouter::CellMoveRouter():
  db_{ord::OpenRoad::openRoad()->getDb()},
  grt_{ord::OpenRoad::openRoad()->getGlobalRouter()},
  logger_{ord::OpenRoad::openRoad()->getLogger()}
{
}

void
CellMoveRouter::helloWorld()
{
  logger_->report("Hello World!");
}

void
CellMoveRouter::drawRectangle(int x1, int y1, int x2, int y2)
{
  gui::Gui* gui = gui::Gui::get();
  if (rectangleRender_ == nullptr)
  {
    rectangleRender_ = std::make_unique<RectangleRender>();
    gui->registerRenderer(rectangleRender_.get());
  }
  odb::Rect rect{x1, y1, x2, y2};
  rectangleRender_->addRectangle(rect);
  gui->redraw();
}

void
CellMoveRouter::ShowFirstNetRout() {
  auto block = db_->getChip()->getBlock();
  auto nets = block->getNets();
  //creates a list of all nets
  std::vector<odb::dbNet*>  net_list;
  net_list.reserve(nets.size());
  for(auto cell : nets)
    net_list.push_back(cell); //TODO should check if a cell is a macro

  //makes the global routing
  grt_->globalRoute();

  //gets the segment for the first net
  grt::NetRouteMap routs = grt_->getRoutes();
  odb::dbNet* net1 = net_list[100];
  grt::GRoute route = routs[net1];

  for (auto segment : route)
  {
    logger_->report("{:6d} {:6d} {:2d} -> {:6d} {:6d} {:2d}",
                    segment.init_x,
                    segment.init_y,
                    segment.init_layer,
                    segment.final_x,
                    segment.final_y,
                    segment.final_layer);
  }
}
void
CellMoveRouter::Random_Cell_Rerout(){

  auto block = db_->getChip()->getBlock();
  auto cells = block->getInsts();

  //creates a list of all nets
  std::vector<odb::dbInst*>  cell_list;
  cell_list.reserve(cells.size());
  for(auto cell : cells)
    cell_list.push_back(cell);

  odb::dbInst* cell1 = cell_list[std::rand() % (cells.size()-1)];
  Swap_and_Rerout(cell1);

}

void
CellMoveRouter::Swap_and_Rerout(odb::dbInst * moving_cell) {
  // Inital Global Rout by OpenROAD
  grt_->globalRoute();

  gui::Gui* gui = gui::Gui::get();
  if (rectangleRender_ == nullptr)
  {
    rectangleRender_ = std::make_unique<RectangleRender>();
    gui->registerRenderer(rectangleRender_.get());
  }

  rectangleRender_->clear_rectangles();
  // Initializing incremental router
  auto block = db_->getChip()->getBlock();
  grt::IncrementalGRoute IncrementalRouter = grt::IncrementalGRoute(grt_, block);


  // Divding Cells by size and shape
  auto cells = block->getInsts();
  std::map<int, std::vector<odb::dbInst *>> cell_length;

  for (auto cell : cells)
  {
    int length = cell->getBBox()->getLength();
    auto vec = cell_length[length];
    if (!vec.empty())
    {
      vec.push_back(cell);
      cell_length[length] = vec;
    }
    else
    {
      std::vector<odb::dbInst *> new_vector;
      new_vector.push_back(cell);
      cell_length[length] = new_vector;
    }
  }

  /*// chosing a random cell with same size and shape to swap
  // Possible better soution using Median as an metric to chose a swapping cell insted of reandom chosing
  auto same_length = cell_length[moving_cell->getBBox()->getLength()];
  int random_swap_cell_index = std::rand() % (same_length.size() - 1);
  auto swap_cell = same_length[random_swap_cell_index];
  
  if (swap_cell == moving_cell)
  {
    swap_cell = same_length[std::rand() % (same_length.size() - 1)];
  }*/

  /*//swapping cells
  swapCells(swap_cell, moving_cell); */

  std::vector<odb::dbNet*>  affected_nets;
  std::vector<int>  nets_Bbox_Xs;
  std::vector<int>  nets_Bbox_Ys;
  for(auto pin : moving_cell->getITerms())
  {
    auto net = pin->getNet();
    if(net != NULL){

      int xll = std::numeric_limits<int>::max();
      int yll = std::numeric_limits<int>::max();
      int xur = std::numeric_limits<int>::min();
      int yur = std::numeric_limits<int>::min();
      for(auto iterm : net->getITerms())
      {
        int x=0, y=0;
        //Using Cell location (fast)
        odb::dbInst* inst = iterm->getInst();
        if(inst && (inst != moving_cell))// is connected
        {
          inst->getLocation(x, y);
          xur = std::max(xur, x);
          yur = std::max(yur, y);
          xll = std::min(xll, x);
          yll = std::min(yll, y);
        }
      }
      nets_Bbox_Xs.push_back(xur);
      nets_Bbox_Xs.push_back(xll);
      nets_Bbox_Ys.push_back(yur);
      nets_Bbox_Ys.push_back(yll);

      rectangleRender_->addRectangle(odb::Rect(xll, yll,xur, yur));

      grt_->addDirtyNet(net);
      affected_nets.push_back(net);
    }
  }

  odb::Rect Optimal_Region = nets_Bboxes_median(nets_Bbox_Xs, nets_Bbox_Ys);

  gui->redraw();

  /*//adding affected nets to re-rout
  for(auto pin : swap_cell->getITerms())
  {
    auto net = pin->getNet();
    if(net != NULL){
      grt_->addDirtyNet(net);
      affected_nets.push_back(net);
    }
  }
  // wirelength before re-routing
  for (auto net : affected_nets) {
    grt_->reportNetWireLength(net, true, false, false, "before_wl");
  }
  // re-routing
  IncrementalRouter.updateRoutes();
  // wirelength after re-routing
  for (auto net : affected_nets) {
    grt_->reportNetWireLength(net, true, false, false, "after_wl");
  }*/

}

odb::Rect
CellMoveRouter::nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys) {

  int median_pos_X = std::floor(Xs.size()/2);
  std::sort(Xs.begin(), Xs.end());

  int median_pos_Y = std::floor(Ys.size()/2);
  std::sort(Ys.begin(), Ys.end());

  int xll = Xs[median_pos_X - 1];
  int xur = Xs[median_pos_X];
  int yll = Ys[median_pos_Y - 1];
  int yur = Ys[median_pos_Y];

  if (xll == xur) xur = Xs[median_pos_X + 1];
  if (yll == yur) yur = Ys[median_pos_Y + 1];

  std::cout<<"Optimal Region bounding box:\nxll : " << xll << "; xur : " << xur << " \nyll : " << yll << "; yur : "<< yur<<"\n ";

  //drawRectangle(xll, yll, xur, yur);

  rectangleRender_->addRectangle(odb::Rect(xll, yll,xur, yur));

  return odb::Rect(xll, yll,xur, yur);
}

}

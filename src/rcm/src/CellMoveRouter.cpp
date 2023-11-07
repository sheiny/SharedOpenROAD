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
CellMoveRouter::InitCellTree(){
  cellrTree_ = std::make_unique<CRTree>();

  auto block = db_->getChip()->getBlock();
  auto cells = block->getInsts();

  for (auto cell : cells) {
    auto lx = cell->getBBox()->xMin();
    auto rx = cell->getBBox()->xMax();
    auto ly = cell->getBBox()->yMin();
    auto uy = cell->getBBox()->yMax();

    box_t cell_box({lx, ly}, {rx, uy});
    CellElement el = std::pair(cell_box, cell);
    cellrTree_->insert(el);
  }
}

void
CellMoveRouter::InitGCellTree() {
  gcellTree_ = std::make_unique<GRTree>();

  auto block = db_->getChip()->getBlock();
  auto ggrid = block->getGCellGrid();

  std::vector<int> gridX, gridY;
  ggrid->getGridX(gridX);
  ggrid->getGridY(gridY);

  for (int i = 1; i < gridX.size(); i++) {
    auto lx = gridX[i - 1];
    auto rx = gridX[i];
    for(int j = 1; j < gridY.size(); j++) {
      auto ly = gridY[j - 1];
      auto uy = gridY[j];

      box_t gcell_box({lx, ly}, {rx, uy});

      odb::Rect Bbox = odb::Rect(lx, ly, rx, uy);
      GCellElement el = std::pair(gcell_box, Bbox);
      gcellTree_->insert(el);
      rectangleRender_->addRectangle(Bbox);
    }
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
  auto block = db_->getChip()->getBlock();
  std::map<int, std::vector<odb::dbInst *>> cell_length;
  std::vector<odb::dbNet*>  affected_nets;
  std::vector<int>  nets_Bbox_Xs;
  std::vector<int>  nets_Bbox_Ys;
  gui::Gui* gui = gui::Gui::get();
  if (rectangleRender_ == nullptr)
  {
    rectangleRender_ = std::make_unique<RectangleRender>();
    gui->registerRenderer(rectangleRender_.get());
  }
  
  //Initalize Rtrees
  InitCellTree();
  InitGCellTree();
  
  // Inital Global Rout by OpenROAD
  grt_->globalRoute();

  rectangleRender_->clear_rectangles();

  // Initializing incremental router
  grt::IncrementalGRoute IncrementalRouter = grt::IncrementalGRoute(grt_, block);


  /* Divding Cells by size and shape

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
  }*/

  //Finding the cell's nets bounding boxes
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

      grt_->addDirtyNet(net);
      affected_nets.push_back(net);
    }
  }

  point_t p;
  point_t Optimal_Region = nets_Bboxes_median(nets_Bbox_Xs, nets_Bbox_Ys);
  std::vector<GCellElement> result;
  gcellTree_->query(bgi::intersects(Optimal_Region), std::back_inserter(result));
  for(auto gcell : result) {
    
    std::cout<<"gcell Box: ("<<gcell.second.xMin()<<", "<<gcell.second.yMin()<<"), ";
    std::cout<<"("<<gcell.second.xMax()<<", "<<gcell.second.yMax()<<")\n";
    rectangleRender_->addRectangle(gcell.second);
  }
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

point_t CellMoveRouter::nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys) {

  int median_pos_X = std::floor(Xs.size()/2);
  std::sort(Xs.begin(), Xs.end());

  int median_pos_Y = std::floor(Ys.size()/2);
  std::sort(Ys.begin(), Ys.end());

  int xll = Xs[median_pos_X - 1];
  int xur = Xs[median_pos_X];
  int yll = Ys[median_pos_Y - 1];
  int yur = Ys[median_pos_Y];

  int x = (xll + xur)/2;
  int y = (yll + yur)/2; 

  std::cout<<"Optimal Region bounding box:\nxll : " << xll << "; xur : " << xur << " \nyll : " << yll << "; yur : "<< yur<<"\n ";
  std::cout<<"Optimal Point: x: " << x << ", y: " << y << "\n";

  //drawRectangle(x, y, x+100, y+100);

  rectangleRender_->addRectangle(odb::Rect(x, y, x+100, y+100));

  return point_t(x, y);
}

}

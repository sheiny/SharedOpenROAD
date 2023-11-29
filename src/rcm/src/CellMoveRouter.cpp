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
  //std::cout<<"Initializing Cell rtree..."<<std::endl;
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
  //std::cout<<"Initializing GCell rtree..."<<std::endl;
  gcellTree_ = std::make_unique<GRTree>();

  auto block = db_->getChip()->getBlock();
  auto ggrid = block->getGCellGrid();

  std::vector<int> gridX, gridY;
  ggrid->getGridX(gridX);
  ggrid->getGridY(gridY);

  auto prev_y = *gridY.begin();
  auto prev_x = *gridX.begin();

  for(auto y_it = std::next(gridY.begin()); y_it != gridY.end(); y_it++)
  {
    int yll = prev_y;
    int yur = *y_it;
    for(auto x_it = std::next(gridX.begin()); x_it != gridX.end(); x_it++)
    {
      int xll = prev_x;
      int xur = *x_it;
      box_t gcell_box({xll, yll}, {xur, yur});

      odb::Rect Bbox = odb::Rect(xll, yll, xur, yur);
      GCellElement el = std::pair(gcell_box, Bbox);
      gcellTree_->insert(el);
      prev_x = *x_it;
    }
    prev_x = *gridX.begin();
    prev_y = *y_it;
  }
}

void
CellMoveRouter::Cell_Move_Rerout(){

  auto block = db_->getChip()->getBlock();
  auto cells = block->getInsts();

  icr_grt_ = new grt::IncrementalGRoute(grt_, block);


  // Inital Global Rout by OpenROAD
  grt_->globalRoute();

  long init_wl = grt_->computeWirelength();
  std::cout<<"initial wl  "<<init_wl<<std::endl;

  InitCellsWeight();

  int n_move_cells = std::floor(cells_weight.size() * 1/100);

  //Initalize Rtrees
  InitCellTree();
  InitGCellTree();
  
  int n_cells = cells_weight.size();
  std::cout<<"Celulas a serem movidas  "<<n_move_cells<<std::endl;
  for(int i = cells_weight.size() - 1; i >=0; i--) {
    if(i < n_cells - n_move_cells) {
      break;
    }
    cells_to_move.push_back(cells_weight[i].second);
  }

  int cont = 0;
  while(!cells_to_move.empty()) {
    bool complete = Swap_and_Rerout(cells_to_move[0]);
    if(complete) {
      cont++;
    }
  }

  std::cout<<"moved cells  "<<cont<<std::endl;
  long after_wl = grt_->computeWirelength();
  std::cout<<"final wl  "<<after_wl<<std::endl;

  delete icr_grt_;
  icr_grt_ = nullptr;

}

bool
CellMoveRouter::Swap_and_Rerout(odb::dbInst * moving_cell) {
  auto block = db_->getChip()->getBlock();
  std::map<int, std::vector<odb::dbInst *>> cell_length;
  std::vector<odb::dbNet*>  affected_nets;
  std::vector<int>  nets_Bbox_Xs;
  std::vector<int>  nets_Bbox_Ys;
  gui::Gui* gui = gui::Gui::get();

  //Finding the cell's nets bounding boxes
  int before_hwpl = 0;
  for(auto pin : moving_cell->getITerms())
  {
    auto net = pin->getNet();
    if(net != NULL){
      if (net->getSigType() == odb::dbSigType::POWER ||
          net->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }
      before_hwpl += getNetHPWLFast(net);

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

  //Get median cell Point
  //std::cout<<"Computing cell median point"<<std::endl;
  std::pair<int, int> Optimal_Region = nets_Bboxes_median(nets_Bbox_Xs, nets_Bbox_Ys);
  
  //move cell to median point
  int cx, cy, xur, yur, xll, yll;
  moving_cell->setLocation(Optimal_Region.first, Optimal_Region.second);
  moving_cell->getLocation(cx,cy);
  /*std::cout<<"nome celula: "<<moving_cell->getName()<<"\n";
  std::cout<<"Cell pos before Abacus: ("<<cx<<", "<<cy<<")"<<std::endl;*/
  
  //Find median Gcell
  std::vector<GCellElement> result;
  gcellTree_->query(bgi::intersects(point_t(Optimal_Region.first, Optimal_Region.second)), std::back_inserter(result));
  /*std::cout<<"Optimal Gcell: ("<<result[0].second.xMin()<<", "<<result[0].second.yMin()<<"), ";
  std::cout<<"("<<result[0].second.xMax()<<", "<<result[0].second.yMax()<<")\n";
  std::cout<<std::endl;*/
  
  //Expanding legalization Area
  std::vector<GCellElement> result2;
  gcellTree_->query(bgi::intersects(box_t({result[0].second.xMin(), result[0].second.yMin()}, {result[0].second.xMax(), result[0].second.yMax()})), std::back_inserter(result2));
  for(auto gcell : result2) {
    xur = std::max(xur, gcell.second.xMax());
    yur = std::max(yur, gcell.second.yMax());
    xll = std::min(xll, gcell.second.xMin());
    yll = std::min(yll, gcell.second.yMin());
  }
  bool regect = false;
  int after_hwpl = 0;
  for(auto pin : moving_cell->getITerms())
  {
    auto net = pin->getNet();
    if(net != NULL){
      if (net->getSigType() == odb::dbSigType::POWER ||
          net->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }
      after_hwpl += getNetHPWLFast(net);
    }
  }

  if(after_hwpl > before_hwpl) {
    return false;
  }


  //Call abacus for legalization area
  /*TODO return information of moved cells during abacus legalizarion*/
  auto changed_cells = abacus_.abacus(xll, yll, xur, yur);
  moving_cell->getLocation(cx,cy);
  //std::cout<<"Cell pos before Abacus: ("<<cx<<", "<<cy<<")"<<std::endl;

  for(auto cell : changed_cells) {
    if(cell == moving_cell) {
      continue;
    }

    for(auto pin : cell->getITerms())
    {
      auto affected_net = pin->getNet();
      if(affected_net != NULL){
        if (affected_net->getSigType() == odb::dbSigType::POWER ||
            affected_net->getSigType() == odb::dbSigType::GROUND) {
          continue;
        }
        grt_->addDirtyNet(affected_net);
      }
    }
  }

  for(auto pin : moving_cell->getITerms())
  {
    auto net = pin->getNet();
    if(net != NULL){
      if (net->getSigType() == odb::dbSigType::POWER ||
          net->getSigType() == odb::dbSigType::GROUND) {
        continue;
      }
      for (auto iterm : net->getITerms()) {
        auto inst = iterm->getInst();
        auto erase = std::find(cells_to_move.begin(), cells_to_move.end(), inst);
        if(erase != cells_to_move.end()) {
          cells_to_move.erase(erase);
        }
      }
    }
  }

  /*TODO chamar o incremental router para as nets afetadas*/
  //std::cout<<"Reroteando nets afetadas....."<<std::endl;
  icr_grt_->updateRoutes();
  if(!grt_->getDirtyNets().empty()) {
    grt_->clearDirtyNets();
  }
  //std::cout<<"nets afetadas reroteadas..."<<std::endl;
  return true;
}

std::pair<int, int> CellMoveRouter::nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys) {

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

  //std::cout<<"Optimal Region bounding box:\nxll : " << xll << "; xur : " << xur << " \nyll : " << yll << "; yur : "<< yur<<"\n ";
  //std::cout<<"Optimal Point: x: " << x << ", y: " << y << "\n";

  //drawRectangle(x, y, x+100, y+100);

  rectangleRender_->addRectangle(odb::Rect(x, y, x+100, y+100));

  return std::pair<int, int> (x, y);
}

void
CellMoveRouter::InitCellsWeight()
{

  odb::dbBlock *block = db_->getChip()->getBlock(); //pega o bloco
  auto cellNumber = block->getInsts().size();
  
  std::map <std::string, int> netDeltaLookup; //mapa de nets e hpwl,stwl

  for (auto net: block->getNets()){ //cálculo do delta hpwl-wl de uma net
    
    auto netName = net->getName(); //pega o nome desta net

    int hpwl=0, routing_wl=0;
  
    hpwl = getNetHPWLFast(net);

    routing_wl = grt_->computeNetWirelength(net); //transformei esse metodo pra public - WL da net

    int netDelta = routing_wl - hpwl; //calculo do delta da net/pino
    netDeltaLookup[netName] = netDelta;
  }

  odb::dbInst* test;

  for(auto cell : block->getInsts()) {
    int delta_sum = 0;
    if(cell->isFixed()) {
      cells_weight.push_back(std::make_pair(delta_sum, cell));
      continue;
    }
    for (auto pin : cell->getITerms()) {
      auto net = pin->getNet();
      if(net != nullptr) {
        if (net->getSigType() == odb::dbSigType::POWER ||
            net->getSigType() == odb::dbSigType::GROUND) {
          continue;
        }
        delta_sum += netDeltaLookup[net->getName()];
      }
    }
    cells_weight.push_back(std::make_pair(delta_sum, cell));
    test = cell;
  }
  std::sort(cells_weight.begin(),cells_weight.end());
  
}

int
CellMoveRouter::getNetHPWLFast(odb::dbNet * net) const
{
  int xll = std::numeric_limits<int>::max();
  int yll = std::numeric_limits<int>::max();
  int xur = std::numeric_limits<int>::min();
  int yur = std::numeric_limits<int>::min();
  for(auto iterm : net->getITerms())
  {
    int x=0, y=0;
    //Using Cell LL location (fast)
    odb::dbInst* inst = iterm->getInst();
    if(inst)// is connected
    {
      inst->getLocation(x, y);
      xur = std::max(xur, x);
      yur = std::max(yur, y);
      xll = std::min(xll, x);
      yll = std::min(yll, y);
    }
  }
  const int width = std::abs(xur-xll);
  const int height = std::abs(yur-yll);
  int hpwl = width + height;
  return hpwl;
}

}

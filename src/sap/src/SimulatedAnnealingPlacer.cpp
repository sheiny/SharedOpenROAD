#include "sap/SimulatedAnnealingPlacer.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "grt/GlobalRouter.h"
#include "stt/SteinerTreeBuilder.h"
#include "utl/Logger.h"

#include <iostream>

struct Segment
{
  Segment(int xs, int xe, int ys, int ye):
    x1(xs),
    x2(xe),
    y1(ys),
    y2(ye)
  {};
  int x1, x2, y1, y2;
};

//This function returns the horizontal/vertical segments
//from a given stt::Tree
int
getTreeWl(const stt::Tree &tree)
{
  //std::vector<Segment> result;
  int treeWl = 0;
  
  for(int i = 0; i < tree.branchCount(); ++i)
  {
    const stt::Branch& branch = tree.branch[i];
    if(i == branch.n)
      continue;
    const int x1 = branch.x;
    const int y1 = branch.y;
    const stt::Branch& neighbor = tree.branch[branch.n];
    const int x2 = neighbor.x;
    const int y2 = neighbor.y;
    treeWl += abs(x1 - x2) + abs(y1 - y2);
    /*if(horizontal)
    {
      result.push_back({x1, x2, y1, y1});
      result.push_back({x1, x2, y2, y2});
    }
    else
    {
      result.push_back({x1, x1, y1, y2});
      result.push_back({x2, x2, y1, y2});
    }*/

  }
  return treeWl;
}


namespace sap {

SimulatedAnnealingPlacer::SimulatedAnnealingPlacer() :
  db_{ord::OpenRoad::openRoad()->getDb()},
  logger_{ord::OpenRoad::openRoad()->getLogger()},
  stt_{ord::OpenRoad::openRoad()->getSteinerTreeBuilder()},
  grt_{ord::OpenRoad::openRoad()->getGlobalRouter()},
  gui_{gui::Gui::get()}
{
}

SimulatedAnnealingPlacer::~SimulatedAnnealingPlacer()
{
  //clear();
}

void
SimulatedAnnealingPlacer::generateInitialRandomPlacement()
{
  auto block = db_->getChip()->getBlock();
  odb::Rect rect;
  block->getCoreArea(rect);
  int xCoreMin = rect.xMin();
  int xCoreMax = rect.xMax();
  int yCoreMin = rect.yMin();
  int yCoreMax = rect.yMax();

  for(auto inst : block->getInsts())
  {
    int randX = std::rand()%(xCoreMax-xCoreMin + 1) + xCoreMin;
    int randY = std::rand()%(yCoreMax-yCoreMin + 1) + yCoreMin;
    inst->setLocation(randX, randY);
  }
}

int
SimulatedAnnealingPlacer::getNetHPWL(odb::dbNet * net) const
{
  int xll = std::numeric_limits<int>::max();
  int yll = std::numeric_limits<int>::max();
  int xur = std::numeric_limits<int>::min();
  int yur = std::numeric_limits<int>::min();
  for(auto iterm : net->getITerms())
  {
    int x=0, y=0;
    const bool pinExist = iterm->getAvgXY(&x, &y);
    if(pinExist)
    {
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

int
SimulatedAnnealingPlacer::getNetHPWLFast(odb::dbNet * net) const
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

int
SimulatedAnnealingPlacer::totalWirelength()
{
  auto block = db_->getChip()->getBlock();
  int totalWireLength = 0;
  for(auto net : block->getNets())
  {
    //int hpwl = getNetHPWL(net);
    int hpwl = getNetHPWLFast(net);
    totalWireLength += hpwl;
  }
  return totalWireLength;
}

void
SimulatedAnnealingPlacer::swapCells(odb::dbInst* cell1, odb::dbInst* cell2)
{
  int cell1X, cell1Y;
  int cell2X, cell2Y;
  cell1->getLocation(cell1X, cell1Y);
  cell2->getLocation(cell2X, cell2Y);

  cell1->setLocation(cell2X, cell2Y);
  cell2->setLocation(cell1X, cell1Y);
}

void
SimulatedAnnealingPlacer::placeCells()
{
  generateInitialRandomPlacement();

  auto block = db_->getChip()->getBlock();
  auto instances = block->getInsts();

  int M = 1;
  int numCellInsts = instances.size();
  
  std::vector<odb::dbInst*>  cells;
  cells.reserve(numCellInsts);

  for(auto cell : instances)
    cells.push_back(cell); //TODO should check if a cell is a macro

  int currentTotalHPWL = totalWirelength();
  double temperature = currentTotalHPWL;
  bool frozen = false;
  int stoppingCriteria = 0.01;
  int itrCount = 0;
  while(!frozen)
  {
    int beforeTotalHPWL = currentTotalHPWL;
    for(int s=0; s<M*numCellInsts;s++)// where M could be a big number for example 1000
    {
      int hpwlBefore = 0, hpwlAfter = 0;
      int cell1Index = std::rand() % (numCellInsts - 1);
      int cell2Index = std::rand() % (numCellInsts - 1);
      
      odb::dbInst* cell1 = cells[cell1Index];
      odb::dbInst* cell2 = cells[cell2Index];

      for(auto pin : cell1->getITerms())
      {
        auto net = pin->getNet();
        if(net != NULL)
          hpwlBefore += getNetHPWLFast(net);
      }

      for(auto pin : cell2->getITerms())
      {
        auto net = pin->getNet();
        if(net != NULL)
          hpwlBefore += getNetHPWLFast(net);
      }
     
      swapCells(cell1, cell2);

      for(auto pin : cell1->getITerms())
      {
        auto net = pin->getNet();
        if(net != NULL)
          hpwlAfter += getNetHPWLFast(net);
      }

      for(auto pin : cell2->getITerms())
      {
        auto net = pin->getNet();
        if(net != NULL)
          hpwlAfter += getNetHPWLFast(net);
      }

      double deltaHPWL = hpwlAfter - hpwlBefore;
      if (deltaHPWL < 0)// Good swap
        currentTotalHPWL += deltaHPWL;
      else
      {
        int randomUniform = std::rand();
        if (randomUniform < exp(-deltaHPWL/temperature))
          currentTotalHPWL += deltaHPWL;
        else
          swapCells(cell1, cell2);
      }

      //after every 1k swaps prints the current progress
      if(s %1000==0)
      {
        continue;
      }
    }

    std::cout<<"Iteration count: "<<itrCount<<" deltaHPWL: "<<(double)currentTotalHPWL/beforeTotalHPWL<<std::endl;

    if ((double)currentTotalHPWL/beforeTotalHPWL > stoppingCriteria)
    {
      temperature *= 0.9; // cool down the temperature
    }
    else
    {
      frozen = true; // reached a minimum local, let's stop
    }
  }
}

stt::Tree
SimulatedAnnealingPlacer::buildSteinerTree(odb::dbNet * net)
{
  //std::cout<<"Branch Count :\n";
  //skip PG Nets (double check for clock?)
  if ((net->getSigType() == odb::dbSigType::GROUND)
      || (net->getSigType() == odb::dbSigType::POWER))
    return stt::Tree{};

  const int driverID = net->getDrivingITerm();
  //std::cout<<"driver id: "<<driverID<<"\n";
  if(driverID == 0 || driverID == -1)
    return stt::Tree{}; //throw std::logic_error("Error, net without a driver (should we skip it?).");

  // Get pin coords and driver
  std::vector<int> xcoords, ycoords;
  int rootIndex;
  for(auto dbITerm : net->getITerms())
  {
    int x, y;
    const bool pinExist = dbITerm->getAvgXY(&x, &y);
    if(pinExist)
    {
      if(driverID == dbITerm->getId())
      {
        rootIndex = xcoords.size();
      }
      xcoords.push_back(x);
      ycoords.push_back(y);
    }
  }
  // Build Steiner Tree
  const stt::Tree tree = stt_->makeSteinerTree(xcoords, ycoords, rootIndex);
  return tree;
}

void
SimulatedAnnealingPlacer::getSteinerTrees()
{
  auto block = db_->getChip()->getBlock();
  block->setDrivingItermsforNets();
  for(auto net : block->getNets())
  {
    stt::Tree tree = buildSteinerTree(net);
    tree.printTree(logger_);
    //std::cout<<"length tree: "<<getTreeWl(tree)<<" \n";
    //std::cout<<"length HPWL: "<<getNetHPWLFast(net)<<" \n\n";
  }
}

void
SimulatedAnnealingPlacer::ShowFirstNetRout() {
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
SimulatedAnnealingPlacer::Random_Cell_Rerout(){

  auto block = db_->getChip()->getBlock();
  auto cells = block->getInsts();

  //creates a list of all nets
  std::vector<odb::dbInst*>  cell_list;
  cell_list.reserve(cells.size());
  for(auto cell : cells)
    cell_list.push_back(cell);

  odb::dbInst* cell1 = cell_list[1000];
  Swap_and_Rerout(cell1);

}

void
SimulatedAnnealingPlacer::Swap_and_Rerout(odb::dbInst * moving_cell) {
  // Inital Global Rout by OpenROAD
  grt_->globalRoute();

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
        //Using Cell LL location (fast)
        odb::dbInst* inst = iterm->getInst();
        if(inst && (inst != moving_cell))// is connected
        {
          inst->getLocation(x, y);
          xur = std::max(xur, x);
          yur = std::max(yur, y);
          xll = std::min(xll, x);
          yll = std::min(yll, y);
        }
        else {
          std::cout<<"oi\n";
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

  odb::Rect Optimal_Region = nets_Bboxes_median(nets_Bbox_Xs, nets_Bbox_Ys);

  gui::Painter painter = gui::Painter();

  painter.drawGeomShape(&Optimal_Region);

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
SimulatedAnnealingPlacer::nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys) {

  int median_pos_X = std::floor(Xs.size()/2);
  std::nth_element(Xs.begin(), Xs.begin() + median_pos_X, Xs.end());
  int median_pos_Y = std::floor(Ys.size()/2);
  std::nth_element(Ys.begin(), Ys.begin() + median_pos_Y, Ys.end());

  int xll = Xs[median_pos_X];
  int xur = Xs[median_pos_X + 1];
  int yll = Ys[median_pos_Y];
  int yur = Ys[median_pos_Y + 1];

  std::cout<<"xll : " << xll << "; xur : " << xur << " \n yll : " << yll << "; yur : "<< yur<<"\n tamanho dos Xs: "<<Xs.size()<<"\n";

  return odb::Rect(xll, yll,xur, yur);
}


}
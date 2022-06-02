#include "sap/SimulatedAnnealingPlacer.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
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
  stt_{ord::OpenRoad::openRoad()->getSteinerTreeBuilder()}
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
      if(s%1000==0)
      {
        std::cout<<"Temp: "<<temperature<<" currentIt: "<<s
                 <<" maxIt: "<<M*numCellInsts
                 <<" HPWL: "<<currentTotalHPWL<<std::endl;
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
  std::cout<<"entrou 1: \n";
  auto block = db_->getChip()->getBlock();
  block->setDrivingItermsforNets();
  for(auto net : block->getNets())
  {
    stt::Tree tree = buildSteinerTree(net);
    //tree.printTree(logger_);
    std::cout<<"length tree: "<<getTreeWl(tree)<<" \n";
    std::cout<<"length HPWL: "<<getNetHPWLFast(net)<<" \n\n";
  }
  std::cout<<"entrou 3: \n";
}
}
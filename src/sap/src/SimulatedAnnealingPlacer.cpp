#include "sap/SimulatedAnnealingPlacer.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

#include <iostream>

namespace sap {

SimulatedAnnealingPlacer::SimulatedAnnealingPlacer() :
  db_{ord::OpenRoad::openRoad()->getDb()},
  logger_{ord::OpenRoad::openRoad()->getLogger()}
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
		inst->setOrigin(randX, randY);
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
SimulatedAnnealingPlacer::total_wirelength()
{
  auto block = db_->getChip()->getBlock();
  int tota_wire_lenght = 0;
  for(auto net : block->getNets())
  {
    int hpwl = getNetHPWL(net);
    tota_wire_lenght += hpwl;
  }
  return tota_wire_lenght;
}

void
SimulatedAnnealingPlacer::swap_cells(odb::dbInst* cell_1, odb::dbInst* cell_2)
{
  int cell_1_x, cell_1__y;
  int cell_2_x, cell_2__y;
  cell_1->getLocation(cell_1_x, cell_1__y);
  cell_2->getLocation(cell_2_x, cell_2__y);

  cell_1->setLocation(cell_2_x, cell_2__y);
  cell_2->setLocation(cell_1_x, cell_1__y);
}
void
SimulatedAnnealingPlacer::placeCells()
{

  generateInitialRandomPlacement();

  auto block = db_->getChip()->getBlock();
  auto cells = block->getInsts();

  int M = 1;
  int NumCellInsts = cells.size();
  
  std::vector<odb::dbInst*>  cells_list;
  cells_list.reserve(NumCellInsts);

  for(auto cell : block->getInsts()){
    cells_list.push_back(cell);
  }

  double temperature = total_wirelength();
  bool frozen = false;
  while(!frozen)
  {
    int hpwl_emprovement = 0;
    for(int s=0; s<M*NumCellInsts;s++)// where M could be a big number for example 1000
    {
      int hpwl_before_OPTM = 0, hpwl_after_OPT = 0;
      int cell_1_number, cell_2_number;


      cell_1_number = std::rand() % (NumCellInsts - 1);
      cell_2_number = std::rand() % (NumCellInsts - 1);
      
      odb::dbInst* cell_1 = cells_list[cell_1_number];
      odb::dbInst* cell_2 = cells_list[cell_2_number];

      for (auto pin : cell_1->getITerms()) {
        auto net = pin->getNet();
        if (net != NULL) {
          hpwl_before_OPTM += getNetHPWL(net);
        }
      }
      for (auto pin : cell_2->getITerms()) {
        auto net = pin->getNet();
        if (net != NULL) {
          hpwl_before_OPTM += getNetHPWL(net);
        }
      }
     
      swap_cells(cell_1, cell_2);

      for (auto pin : cell_1->getITerms()) {
        auto net = pin->getNet();
        if (net != NULL) {
          hpwl_after_OPT += getNetHPWL(net);
        }
      }

      for (auto pin : cell_2->getITerms()) {
        auto net = pin->getNet();
        if (net != NULL) {
          hpwl_after_OPT += getNetHPWL(net);
        }
      }

      double deltaWL = hpwl_after_OPT - hpwl_before_OPTM;
      if (deltaWL < 0)// Good swap
      {
        hpwl_emprovement += deltaWL;
        continue;
      }
      else
      {
        int random_uniform = std::rand() ;
        if (random_uniform < exp(-deltaWL/temperature))
        {
          hpwl_emprovement += deltaWL;
          continue;
        }
        else
        {
          swap_cells(cell_1, cell_2);
        }
      }
    }

    std::cout<<"emprovement : " << hpwl_emprovement << "\n";
    if (hpwl_emprovement < -1000)
    {
      temperature *= 0.9; // cool down the temperature
    }
    else
    {
      frozen = true; // reached a minimum local, let's stop
    }
  }
  std::cout<<"concluido \n";
}

}


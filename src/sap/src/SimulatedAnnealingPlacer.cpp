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

void
SimulatedAnnealingPlacer::placeCells()
{
  generateInitialRandomPlacement();
#if 0
  Pseudo Code for Simulated Annealing
      source: https://www.youtube.com/watch?v=nKDqmTfTbAU

  Start with a random placement
  Initialize the temperature using the total wirelength of this random placement.
  int temperature = total wirelength;
  bool frozen = false;
  while(!frozen)
  {
    for(s=0; s<M*NumCellInsts;s++)// where M could be a big number for example 1000
    {
      swap 2 random cell insts;
      compute delta wirelength
      if (delta < 0)// Good swap
      {
        //keep the movement (swap)
      }
      else
      {
        if (random_uniform < exp(-deltaWL/T)) //uphill climb
        {
          //accept bad movement
        }
        else
        {
          //undo movement (swap)
        }
      }
    }
    if (WL is still deacreasing)
    {
      temperature *= 0.9; // cool down the temperature
    }
    else
    {
      frozen = true; // reached a minimum local, let's stop
    }
  }
#endif
}

}


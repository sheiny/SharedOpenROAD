#include "tut/Tutorial.h"
#include "odb/db.h"
#include <iostream>
#include "ord/OpenRoad.hh"

namespace tut {

Tutorial::Tutorial(odb::dbDatabase* db) :
  db_(db)
{
}

//TODO implement a hello world
void
Tutorial::printHello()
{
  std::cout<<"Hello world"<<std::endl;

  // utl::Logger *logger = ord::OpenRoad::openRoad()->getLogger();
  // logger->report("heloo tiago");
}

//TODO print all cell names
void
Tutorial::printCells()
{
  std::cout<<"Printing all cell names:"<<std::endl;
  //First lets get the circuit block
  auto block = db_->getChip()->getBlock();
  for(auto inst : block->getInsts())
    std::cout<<inst->getName()<<std::endl;
}


//TODO print all net names
void
Tutorial::printNets()
{
  std::cout<<"Printing all net names:"<<std::endl;
  auto block = db_->getChip()->getBlock();
  for(auto inst : block->getNets())
    std::cout<<inst->getName()<<std::endl;
}

//TODO print all pin names
void
Tutorial::printPins()
{
  std::cout<<"Printing all pins names:"<<std::endl;
  auto block = db_->getChip()->getBlock();
  for(auto net : block->getNets())
  {
    std::cout<<"Net: "<<net->getName()<<std::endl;
    for(auto iterm : net->getITerms())
    {
      auto cell = iterm->getInst();
      auto cellName = cell->getName();
      auto std_pin = iterm->getMTerm();
      auto pinName = std_pin->getName();
      // std::cout<<"    PinName: "<< cellName << " : "<< pinName << std::endl;
      int x=0, y=0;
      iterm->getAvgXY(&x, &y);
      std::cout<<"    PinName: "<< cellName << " : "<< pinName << " Position: ( " << x << " , "<< y << " )"<< std::endl;
    }

    // calculo do hpwl
  }
}

//Challenge: Traverse all nets printing the total HPWL
void
Tutorial::printHPWLs()
{
  //Challenge :)
}

Tutorial::~Tutorial()
{
  //clear();
}

}

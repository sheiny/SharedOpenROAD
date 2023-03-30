#include "tut/Tutorial.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

#include <iostream>

namespace tut {

Tutorial::Tutorial() :
  db_{ord::OpenRoad::openRoad()->getDb()},
  logger_{ord::OpenRoad::openRoad()->getLogger()}
{
}

void
Tutorial::printHello()
{
  logger_->report("olha o terminal");
  std::cout<<"Listing HPWL:"<<std::endl;
  auto block = db_->getChip()->getBlock(); //pegar o bloco
  for(auto net : block->getNets()) //percorrer net por net do bloco
  {
    auto xmax = std::numeric_limits<int>::min();
    auto xmin = std::numeric_limits<int>::max();
    auto ymax = std::numeric_limits<int>::min();
    auto ymin = std::numeric_limits<int>::max();
    auto HPWL = 0;
    
    std::cout<<net->getName()<<std::endl; //nome da net
    for(auto iterm : net->getITerms()) //iterm(instance terminal) eh o pino da instancia da standard cell que ta ligada na net
    {
      auto cell = iterm->getInst(); // instancia da standard cell onde a net esta ligada
      auto cellName = cell->getName(); //pega o nome desta celula
      auto std_pin = iterm->getMTerm(); //master terminal da standard cell - o pino da standard cell global
      auto pinName = std_pin->getName();//nome do pino
      // std::cout<<"    PinName: "<< cellName << " : "<< pinName << std::endl;
      int x=0, y=0;
      iterm->getAvgXY(&x, &y);
      //xmin = std::numeric_limits<int>::lowest();
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;
      if (x < xmin) xmin = x;
      if (y < ymin) ymin = y;
      //std::cout<<"xmax: "<<xmax<<" xmin: "<<xmin<<" ymax: "<<ymax<<" ymin: "<<ymin<<std::endl;

      std::cout<<"    PinName: "<< cellName << " : "<< pinName << " Position: ( " << x << " , "<< y << " )"<< std::endl;
    }
  std::cout<<"xmax: "<<xmax<<" xmin: "<<xmin<<" ymax: "<<ymax<<" ymin: "<<ymin<<std::endl;

  HPWL = (xmax - xmin) + (ymax - ymin);
  std::cout<<"HPWL: "<<HPWL<<std::endl;

  }


}

void
Tutorial::printCells()
{
  
  /*std::cout<<"Printing all cell names:"<<std::endl;
  auto block = db_->getChip()->getBlock();
  for(auto inst : block->getInsts())
    std::cout<<inst->getName()<<std::endl;*/
}

void
Tutorial::printNets()
{
  std::cout<<"Printing all net names:"<<std::endl;
  auto block = db_->getChip()->getBlock();
  for(auto inst : block->getNets())
    std::cout<<inst->getName()<<std::endl;
}

void
Tutorial::printPins()
{
  std::cout<<"Printing all pins names:"<<std::endl;
  auto block = db_->getChip()->getBlock();
  for(auto net : block->getNets())
  {
    net->get
    std::cout<<"Net: "<<net->getName()<<std::endl;
    for(auto iterm : net->getITerms()) //instance terminals são os pinos da instancia?
    {
      iterm->
      auto cell = iterm->getInst();
      auto cellName = cell->getName();
      auto std_pin = iterm->getMTerm(); //o que seria master terminal?
      auto pinName = std_pin->getName();
      // std::cout<<"    PinName: "<< cellName << " : "<< pinName << std::endl;
      int x=0, y=0;
      iterm->getAvgXY(&x, &y); //isso é um vetor? 
      std::cout<<"    PinName: "<< cellName << " : "<< pinName << " Position: ( " << x << " , "<< y << " )"<< std::endl;
    }

    // calculo do hpwl
  }
}

//Challenge: Traverse all nets printing the total HPWL
void
Tutorial::printHPWLs()
{
  odb::dbBlock *block = db_->getChip()->getBlock();
  for(auto net : block->getNets())
  {
    logger_->report("Net: "+net->getName());
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
        const std::string pin_str = "Pin: "+iterm->getInst()->getName()+
                                    ":"+iterm->getMTerm()->getName()+
                                    " x:"+std::to_string(x)+" y:"+std::to_string(y);
        logger_->report(pin_str);
        xur = std::max(xur, x);
        yur = std::max(yur, y);
        xll = std::min(xll, x);
        yll = std::min(yll, y);
      }
    }
    const int width = std::abs(xur-xll);
    const int height = std::abs(yur-yll);
    const int hpwl = width + height;
    logger_->report("HPWL: "+std::to_string(hpwl));
  }
}

Tutorial::~Tutorial()
{
  //clear();
}

}

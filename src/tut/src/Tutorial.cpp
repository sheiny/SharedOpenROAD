#include "tut/Tutorial.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"
#include "grt/GlobalRouter.h"
#include "stt/SteinerTreeBuilder.h"


#include <iostream>

namespace tut {

Tutorial::Tutorial() :
  db_{ord::OpenRoad::openRoad()->getDb()},
  //block_{ord::OpenRoad::openRoad()->getDb()->getChip()->getBlock()},
  logger_{ord::OpenRoad::openRoad()->getLogger()},
  grt_{ord::OpenRoad::openRoad()->getGlobalRouter()}
{
}

void
Tutorial::printHello()
{

  odb::dbBlock *block = db_->getChip()->getBlock(); //pega o bloco
  auto cellNumber = db_->getChip()->getBlock()->getInsts().size();
  //int blockDelta = 0;

  std::cout<<"No. of cells in block: "<< cellNumber << std::endl;
  //int delta[cellNumber];


  std::map <std::string, int> mapNetDeltas; //mapa de nets e deltas
  
  std::map <std::string, std::pair<int,int>> netWlLookup; //mapa de nets e hpwl,stwl
  //netWlLookup[net->getName()] = std::make_pair(hpwl,stwl);
  //netWlLookup[net->getName()].first; //retorna hpwl
  //netWlLookup[net->getName()].second; //retorna stwl

  /* sugerido na reuniao:
  std::vector<std::pair<int,std::string>> aa; 
  
  aa.push_back(std::make_pair(10,"oioioi"));
  aa.push_back(std::make_pair(5,"aiaia"));

  

  std::sort(aa.begin(),aa.end());
  for(auto data: aa){
    auto inteiro = data.first;
    auto text = data.second;
  }*/
  std::vector<std::pair<int,std::string>> delta;

  for (auto net: block->getNets()){ //desenvolver a partir daqui

      //std::cout<< "cells map size: " << netWlLookup.size() << std::endl; //
      //std::cout<< "delta map size: " << mapNetDeltas.size() << std::endl;

      auto netName = net->getName(); //pega o nome desta celula
      std::cout<<"      NetName: "<< netName << std::endl;

      int hpwl, stwl;

      netWlLookup[netName] = std::make_pair(hpwl,stwl);
    
      hpwl = calc_HPWL(net);
      netWlLookup[netName].first = hpwl; //insere hpwl no pair

      stwl = grt_->computeNetWirelength(net); //transformei esse metodo pra public - WL da net
      std::cout<<"               WL: "<< stwl << std::endl; //calculo do wirelength da net
      netWlLookup[netName].second = stwl; //insere stwl no pair

      //logger_->report("HPWL: "+std::to_string(hpwl));  

      int netDelta = stwl - hpwl; //calculo do delta da net/pino
      std::cout<<"        Net Delta: "<< netDelta << std::endl;
      mapNetDeltas[netName] = netDelta;


      delta.push_back(std::make_pair(netDelta,netName));

      std::cout<< "            cell map size: " << netWlLookup.size() << std::endl;
      std::cout<< "           delta map size: " << mapNetDeltas.size() << std::endl;

  }

  std::sort(delta.begin(),delta.end());

  for(auto data: delta){
    auto inteiro = data.first;
    auto text = data.second;
    std::cout<<" Delta: "<< inteiro <<" Net name: " << text <<std::endl;
  }



//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if(0)
  for(auto cell: block->getInsts()) //pega a cell
  {
    auto cellName = cell->getName(); //pega o nome deste pino
    std::cout<<"CellName: "<< cellName << std::endl;
    
    int cellDelta = 0;

    for(auto pin : cell->getITerms()) //pega o pino da cell
    {
      auto pinName = pin->getMTerm()->getName(); //pega o nome desta celula
      std::cout<<"  PinName: "<< pinName << std::endl;

      auto net = pin->getNet(); //pega a net ligada ao pino

      if(pinName != "VDD" && pinName != "VSS") //se o pino nao eh VDD ou VSS
      {
        if(net) //se existe uma net ligada no pino
          {
            if( !(netWlLookup.count(net->getName()) )) //se a net ainda nao foi contabilizada e inserida no mapa
            { 
            //std::cout<< "cells map size: " << netWlLookup.size() << std::endl; //
            //std::cout<< "delta map size: " << mapNetDeltas.size() << std::endl;

            auto netName = net->getName(); //pega o nome desta celula
            std::cout<<"      NetName: "<< netName << std::endl;

            int hpwl, stwl;

            netWlLookup[netName] = std::make_pair(hpwl,stwl);
          
            hpwl = calc_HPWL(net);
            netWlLookup[netName].first = hpwl; //insere hpwl no pair

            stwl = grt_->computeNetWirelength(net); //transformei esse metodo pra public - WL da net
            std::cout<<"               WL: "<< stwl << std::endl; //calculo do wirelength da net
            netWlLookup[netName].second = stwl; //insere stwl no pair

            //logger_->report("HPWL: "+std::to_string(hpwl));  

            int netDelta = stwl - hpwl; //calculo do delta da net/pino
            std::cout<<"        Net Delta: "<< netDelta << std::endl;
            mapNetDeltas[netName] = netDelta;

            cellDelta += netDelta;

            std::cout<< "            cell map size: " << netWlLookup.size() << std::endl;
            std::cout<< "           delta map size: " << mapNetDeltas.size() << std::endl;

          } //if not mapped

          else std::cout<<"      Net "<< net->getName()<< " already mapped"<<std::endl; // net ja mapeada

        } //if net

        else std::cout<<"      No Net connected"<<std::endl; // pino sem net
      } //if VDD

      //else std::cout<<"      VDD/VSS"<<std::endl; // VDD/VSS

    } //for pin
  blockDelta += cellDelta;
  //mapNetDeltas[cellDelta] = cell;

  } //for cell
  
  std::cout<<"Total Block Delta: "<< blockDelta << std::endl;

  std::cout<< "cells map size: " << netWlLookup.size() << std::endl;
  std::cout<< "delta map size: " << mapNetDeltas.size() << std::endl;


  for(auto pair:netWlLookup){
    auto net = pair.first;
    auto data = pair.second;//pair

    auto hpwl = data.first;
    auto stwl = data.second;

  }
#endif
  //std::cout<<mapNetDeltas[0] << std::endl;

  //for (auto i = mapNetDeltas.begin(); i != mapNetDeltas.end(); i++){
  // std::cout<< "delta map pos "<< i <<": " << mapNetDeltas[i] << std::endl;
  //}
  

  //for(int i; i > mapNetDeltas.end() ; i++){
  //  std::cout << mapNetDeltas[i] <<" , "<< ;

} //metodo
  //grt_->reportNetWireLength(net,true,false,false,""); metodo void, retorna no logger

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int
Tutorial::calc_HPWL(odb::dbNet* net)
{
  int xll = std::numeric_limits<int>::max();
  int yll = std::numeric_limits<int>::max();
  int xur = std::numeric_limits<int>::min();
  int yur = std::numeric_limits<int>::min();
  for(auto iterm : net->getITerms())
  {
    int x=0, y=0;
    iterm->getAvgXY(&x, &y);
            
    xur = std::max(xur, x);
    yur = std::max(yur, y);
    xll = std::min(xll, x);
    yll = std::min(yll, y);
  }
  int width = std::abs(xur-xll);
  int height = std::abs(yur-yll);
          
  int hpwl = width + height; //calculo do hpwl da net/pino
  std::cout<<"             HPWL: "<< hpwl << std::endl;
  return hpwl;

}

void
Tutorial::printCells()
{
  
  
  
  
  
  
  
  
  
  /*
  std::cout<<"Printing all cell names:"<<std::endl;
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
    std::cout<<"Net: "<<net->getName()<<std::endl;
    for(auto iterm : net->getITerms()) //instance terminals são os pinos da instancia?
    {
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


          #if(0)
          // calculo do hpwl (substituir por uso de metodo depois)
          //logger_->report("Net: "+net->getName());
          //logger_->report("HPWL: "+std::to_string(hpwl));  

          
          int xll = std::numeric_limits<int>::max();
          int yll = std::numeric_limits<int>::max();
          int xur = std::numeric_limits<int>::min();
          int yur = std::numeric_limits<int>::min();
          for(auto iterm : net->getITerms())
          {
            int x=0, y=0;
            iterm->getAvgXY(&x, &y);
            
            xur = std::max(xur, x);
            yur = std::max(yur, y);
            xll = std::min(xll, x);
            yll = std::min(yll, y);
          }
          int width = std::abs(xur-xll);
          int height = std::abs(yur-yll);
          
          int hpwl = width + height; //calculo do hpwl da net/pino
          std::cout<<"             HPWL: "<< hpwl << std::endl;
          #endif

/*
void
Tutorial::printHello() //meu primeiro hpwl
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
*/
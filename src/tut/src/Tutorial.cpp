#include "tut/Tutorial.h"
#include "odb/db.h"

namespace tut {

Tutorial::Tutorial(odb::dbDatabase* db) :
  db_(db)
{
}

//TODO implement a hello world
void
Tutorial::printHello()
{
}

//TODO print all cell names
void
Tutorial::printCells()
{
}


//TODO print all net names
void
Tutorial::printNets()
{
}

//TODO print all pin names
void
Tutorial::printPins()
{
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

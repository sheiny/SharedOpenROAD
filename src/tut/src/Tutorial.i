%{
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "tut/Tutorial.h"

namespace ord {
tut::Tutorial* getTutorial(); // Defined in OpenRoad.i
OpenRoad *getOpenRoad(); // Defined in OpenRoad.i
}  // namespace ord

using ord::getTutorial;
using tut::Tutorial;
%}

%inline %{

namespace tut {

void
print()
{
  Tutorial* tutorial = getTutorial();
  tutorial->printHello();
}

void
printCells()
{
  Tutorial* tutorial = getTutorial();
  tutorial->printCells();
}

void
printNets()
{
  Tutorial* tutorial = getTutorial();
  tutorial->printNets();
}

void
printPins()
{
  Tutorial* tutorial = getTutorial();
  tutorial->printPins();
}

void
printHPWLs()
{
  Tutorial* tutorial = getTutorial();
  tutorial->printHPWLs();
}

} // namespace

%} // inline

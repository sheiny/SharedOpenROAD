%{
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "sap/SimulatedAnnealingPlacer.h"

namespace ord {
sap::SimulatedAnnealingPlacer* getSimulatedAnnealingPlacer(); // Defined in OpenRoad.i
OpenRoad *getOpenRoad(); // Defined in OpenRoad.i
}  // namespace ord

using ord::getSimulatedAnnealingPlacer;
using sap::SimulatedAnnealingPlacer;
%}

%inline %{

namespace sap {

void
printHello()
{
  SimulatedAnnealingPlacer* sa_placer = getSimulatedAnnealingPlacer();
  sa_placer->printHello();
}

} // namespace

%} // inline


#include "sap/MakeSimulatedAnnealingPlacer.h"
#include "sap/SimulatedAnnealingPlacer.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* sap_tcl_inits[];
}  // namespace sta

extern "C" {
extern int Sap_Init(Tcl_Interp* interp);
}

//All these three functions are being used in /src/OpenRoad.cc
namespace ord {

sap::SimulatedAnnealingPlacer * makeSimulatedAnnealingPlacer()
{
  return new sap::SimulatedAnnealingPlacer();
}

//This funcion will bind the calls between .tcl and .i files
void
initSimulatedAnnealingPlacer(OpenRoad *openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Sap_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::sap_tcl_inits);
}

void
deleteSimulatedAnnealingPlacer(sap::SimulatedAnnealingPlacer *sa_placer)
{
  delete sa_placer;
}

}


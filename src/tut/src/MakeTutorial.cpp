#include "tut/MakeTutorial.h"
#include "tut/Tutorial.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* tut_tcl_inits[];
}  // namespace sta

//Rule: Tutorial class -> Tutorial_Init
//So, the module name in .i have to equal the Class name,
//although .i is considered case insensitive.
extern "C" {
extern int Tut_Init(Tcl_Interp* interp);
}

//All these three functions are being used in /src/OpenRoad.cc
namespace ord {

tut::Tutorial * makeTutorial(odb::dbDatabase* db)
{
  return new tut::Tutorial(db);
}

//This funcion will bind the calls between .tcl and .i files
void
initTutorial(OpenRoad *openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Tut_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::tut_tcl_inits);
}

void
deleteTutorial(tut::Tutorial *tutorial)
{
  delete tutorial;
}

}

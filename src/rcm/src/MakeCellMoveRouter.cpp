#include "rcm/MakeCellMoveRouter.h"
#include "rcm/CellMoveRouter.h"
#include "ord/OpenRoad.hh"
#include "sta/StaMain.hh"

namespace sta {
// Tcl files encoded into strings.
extern const char* rcm_tcl_inits[];
}  // namespace rcm

//Rule: CellMoveRouter class -> CellMoveRouter_Init
//So, the module name in .i have to equal the class name,
//although .i is considered case insensitive.
extern "C" {
extern int Rcm_Init(Tcl_Interp* interp);
}

//All these three functions are being used in /src/OpenRoad.cc
namespace ord {

rcm::CellMoveRouter * makeCellMoveRouter()
{
  return new rcm::CellMoveRouter();
}

//This function will bind the calls between .tcl and .i files
void
initCellMoveRouter(OpenRoad *openroad)
{
  Tcl_Interp* tcl_interp = openroad->tclInterp();
  // Define swig TCL commands.
  Rcm_Init(tcl_interp);
  sta::evalTclInit(tcl_interp, sta::rcm_tcl_inits);
}
}

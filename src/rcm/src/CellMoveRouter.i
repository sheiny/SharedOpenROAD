%{
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "rcm/CellMoveRouter.h"

namespace ord {
rcm::CellMoveRouter* getCellMoveRouter(); // Defined in OpenRoad.i
OpenRoad *getOpenRoad(); // Defined in OpenRoad.i
}  // namespace ord

using ord::getCellMoveRouter;
using rcm::CellMoveRouter;
%}

%inline %{

namespace rcm {

void
helloWorld()
{
  CellMoveRouter* cellMoveRouter = getCellMoveRouter();
  cellMoveRouter->helloWorld();
}


} // namespace

%} // inline


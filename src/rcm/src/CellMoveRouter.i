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

void
drawRect(int x1, int y1, int x2, int y2)
{
  CellMoveRouter* cellMoveRouter = getCellMoveRouter();
  cellMoveRouter->drawRectangle(x1, y1, x2, y2);
}


} // namespace

%} // inline


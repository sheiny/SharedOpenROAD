#include "rcm/CellMoveRouter.h"
#include "odb/db.h"
#include "ord/OpenRoad.hh"
#include "utl/Logger.h"

namespace rcm {

CellMoveRouter::CellMoveRouter():
  db_{ord::OpenRoad::openRoad()->getDb()},
  logger_{ord::OpenRoad::openRoad()->getLogger()}
{
}

void
CellMoveRouter::helloWorld()
{
  logger_->report("Hello World!");
}

}

#pragma once

namespace rcm{
class CellMoveRouter;
}

namespace odb{
class dbDatabase;
}

namespace ord {

class OpenRoad;

rcm::CellMoveRouter* makeCellMoveRouter();

void initCellMoveRouter(OpenRoad *openroad);

//void deleteCellMoveRouter(rcm::CellMoveRouter *cellMoveRouter);
}  // namespace ord

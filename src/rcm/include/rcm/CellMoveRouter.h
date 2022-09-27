#pragma once

#include <memory>

namespace odb {
  class dbDatabase;
}

namespace utl {
  class Logger;
}


namespace rcm {
class RectangleRender;

class CellMoveRouter {
  public:
    CellMoveRouter();

    void helloWorld();

    void drawRectangle(int x1, int y1, int x2, int y2);

  private:
    odb::dbDatabase* db_;
    utl::Logger* logger_;
    std::unique_ptr<RectangleRender> rectangleRender_;
};
}

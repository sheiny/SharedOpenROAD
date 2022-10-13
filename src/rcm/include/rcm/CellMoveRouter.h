#pragma once

#include <memory>
#include <vector>

namespace odb {
  class dbDatabase;
  class dbNet;
  class dbInst;
  class Rect;
}

namespace utl {
  class Logger;
}

namespace grt {
  class GlobalRouter;
  class IncrementalGRoute;
  struct GSegment;
}


namespace rcm {
class RectangleRender;

class CellMoveRouter {
  public:
    CellMoveRouter();

    void helloWorld();

    void drawRectangle(int x1, int y1, int x2, int y2);

    void ShowFirstNetRout();

    void Random_Cell_Rerout();

  private:
  
    void Swap_and_Rerout(odb::dbInst * moving_cell);

    odb::Rect nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys);

    odb::dbDatabase* db_;
    utl::Logger* logger_;
    std::unique_ptr<RectangleRender> rectangleRender_;
    grt::GlobalRouter *grt_;
};
}

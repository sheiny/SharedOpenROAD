#pragma once

#include <memory>
#include <vector>
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/graph/grid_graph.hpp>

namespace odb {
  class dbDatabase;
  class dbNet;
  class dbInst;
  class Rect;
  class Point;
}

namespace utl {
  class Logger;
}

namespace grt {
  class GlobalRouter;
  class IncrementalGRoute;
  struct GSegment;
}

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
typedef bg::model::point<int64_t, 2, bg::cs::cartesian> point_t;

namespace rcm {
class RectangleRender;

class CellMoveRouter {
  private:
    // Define a 2D cartesian point using geometry box of DBUs.
    typedef bg::model::box<point_t> box_t;
    // Define RTree type of DBU box using R-Star algorithm.
    typedef std::pair<box_t, odb::dbInst *> CellElement;
    typedef std::pair<box_t, odb::Rect> GCellElement;
    typedef bgi::rtree<CellElement, bgi::rstar<16>> CRTree;
    typedef bgi::rtree<GCellElement, bgi::rstar<16>> GRTree;
  public:
    CellMoveRouter();

    void helloWorld();

    void drawRectangle(int x1, int y1, int x2, int y2);

    void ShowFirstNetRout();

    void Random_Cell_Rerout();

  private:

    void InitCellTree();

    void InitGCellTree();
  
    void Swap_and_Rerout(odb::dbInst * moving_cell);

    point_t nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys);

    odb::dbDatabase* db_;
    utl::Logger* logger_;
    std::unique_ptr<RectangleRender> rectangleRender_;
    grt::GlobalRouter *grt_;
    std::unique_ptr<CRTree> cellrTree_;
    std::unique_ptr<GRTree> gcellTree_;
};
}

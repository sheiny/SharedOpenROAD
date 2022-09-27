#pragma once
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

namespace stt {
  class SteinerTreeBuilder;
  class Tree;
}

namespace grt {
  class GlobalRouter;
  class IncrementalGRoute;
  struct GSegment;
}

namespace gui{
  class Gui;
  class Painter;
}

namespace sap {
class SimulatedAnnealingPlacer {
public:
  SimulatedAnnealingPlacer();
  ~SimulatedAnnealingPlacer();

  void placeCells();

  void getSteinerTrees();

  void ShowFirstNetRout();

  void Random_Cell_Rerout();

  odb::Rect nets_Bboxes_median(std::vector<int> Xs, std::vector<int> Ys);

private:
  void generateInitialRandomPlacement();

  int getNetHPWL(odb::dbNet * net) const;

  int getNetHPWLFast(odb::dbNet * net) const;

  int totalWirelength();

  void swapCells(odb::dbInst* cell1, odb::dbInst* cell2);

  void Swap_and_Rerout(odb::dbInst * moving_cell);

  

  stt::Tree buildSteinerTree(odb::dbNet * net);

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder *stt_;
  grt::GlobalRouter *grt_;
  gui::Gui* gui_;
};
}

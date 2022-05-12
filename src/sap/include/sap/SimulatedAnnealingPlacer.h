#pragma once

namespace odb {
  class dbDatabase;
  class dbNet;
  class dbInst;
}

namespace utl {
  class Logger;
}

namespace stt {
  class SteinerTreeBuilder;
  class Tree;
}

namespace sap {
class SimulatedAnnealingPlacer {
public:
  SimulatedAnnealingPlacer();
  ~SimulatedAnnealingPlacer();

  void placeCells();

private:
  void generateInitialRandomPlacement();

  int getNetHPWL(odb::dbNet * net) const;

  int getNetHPWLFast(odb::dbNet * net) const;

  int totalWirelength();

  void swapCells(odb::dbInst* cell1, odb::dbInst* cell2);

  stt::Tree buildSteinerTree(odb::dbNet * net);

  odb::dbDatabase* db_;
  utl::Logger* logger_;
  stt::SteinerTreeBuilder *stt_;
};
}

#pragma once

namespace odb {
  class dbDatabase;
  class dbNet;
}

namespace utl {
  class Logger;
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

  int total_wirelength();

  void swap_cells(odb::dbInst * cell_1, odb::dbInst * cell_2);

  odb::dbDatabase* db_;
  utl::Logger* logger_;
};
}

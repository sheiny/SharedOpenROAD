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
  int getNetHPWL(odb::dbNet * net) const;

  odb::dbDatabase* db_;
  utl::Logger* logger_;
};
}

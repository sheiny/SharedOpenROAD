#pragma once

namespace odb {
  class dbDatabase;
}

namespace utl {
  class Logger;
}

namespace sap {
class SimulatedAnnealingPlacer {
public:
  SimulatedAnnealingPlacer();
  ~SimulatedAnnealingPlacer();

void printHello();

private:
  odb::dbDatabase* db_;
  utl::Logger* logger_;
};
}

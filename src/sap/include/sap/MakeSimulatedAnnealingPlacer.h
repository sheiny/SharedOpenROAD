#pragma once

namespace sap{
class SimulatedAnnealingPlacer;
}

namespace odb{
class dbDatabase;
}

namespace ord {

class OpenRoad;

sap::SimulatedAnnealingPlacer* makeSimulatedAnnealingPlacer();

void initSimulatedAnnealingPlacer(OpenRoad *openroad);

void deleteSimulatedAnnealingPlacer(sap::SimulatedAnnealingPlacer *sa_placer);

}  // namespace ord

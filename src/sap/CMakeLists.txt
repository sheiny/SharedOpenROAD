include("openroad")

project(sap)

swig_lib(NAME      sap
         NAMESPACE sap
         I_FILE    src/SimulatedAnnealingPlacer.i
         SCRIPTS   src/SimulatedAnnealingPlacer.tcl
)

target_sources(sap
  PRIVATE
    src/SimulatedAnnealingPlacer.cpp
    src/MakeSimulatedAnnealingPlacer.cpp
)

target_include_directories(sap
  PUBLIC
    include
)

target_link_libraries(sap
    utl
    odb
    OpenSTA
)

messages(
  TARGET sap
)

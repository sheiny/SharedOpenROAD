include("openroad")

project(rcm)

swig_lib(NAME       rcm
         NAMESPACE  rcm
	 I_FILE     src/CellMoveRouter.i
	 SCRIPTS    src/CellMoveRouter.tcl
)

target_sources(rcm
  PRIVATE
    src/CellMoveRouter.cpp
    src/MakeCellMoveRouter.cpp
)

target_include_directories(rcm
  PUBLIC
    include
)

target_link_libraries(rcm
  utl
  odb
  OpenSTA
)

messages(
  TARGET rcm
)

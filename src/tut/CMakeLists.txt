include("openroad")

project(tut)

swig_lib(NAME      tut
         NAMESPACE tut
         I_FILE    src/Tutorial.i
         SCRIPTS   src/Tutorial.tcl
)

target_sources(tut
  PRIVATE
    src/Tutorial.cpp
    src/MakeTutorial.cpp
)

target_include_directories(tut
  PUBLIC
    include
)

target_link_libraries(tut
    utl
    odb
    OpenSTA
    grt
    stt
)

messages(
  TARGET tut
)

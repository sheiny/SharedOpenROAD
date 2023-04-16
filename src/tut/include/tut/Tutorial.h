#pragma once

namespace odb {
  class dbDatabase;
  class dbNet;
}

namespace utl {
  class Logger;
}

namespace grt {
  class GlobalRouter;
  class IncrementalGRoute;
  struct GSegment;
}

namespace tut {

class Tutorial {
  public:
    Tutorial();
    ~Tutorial();

    //Print Hello World
    void printHello();

    //Print all cell names
    void printCells();

    //Print all net names
    void printNets();

    //Print all pin names
    void printPins();

    //Traverse all nets printing the total HPWL
    void printHPWLs();

    int calc_HPWL(odb::dbNet* net);

  private:
    odb::dbDatabase* db_;
    utl::Logger* logger_;
    grt::GlobalRouter *grt_;
    //odb::dbBlock* block_;
  };
}


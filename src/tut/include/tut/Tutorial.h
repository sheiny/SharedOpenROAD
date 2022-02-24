#pragma once

namespace odb {
  class dbDatabase;
}

namespace utl {
  class Logger;
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

  private:
    odb::dbDatabase* db_;
    utl::Logger* logger_;
  };
}


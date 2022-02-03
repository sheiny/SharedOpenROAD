#pragma once

namespace odb {
  class dbDatabase;
}

namespace tut {

class Tutorial {
  public:
    Tutorial(odb::dbDatabase* db);
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
  };
}


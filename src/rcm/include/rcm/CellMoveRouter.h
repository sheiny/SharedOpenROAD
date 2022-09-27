#pragma once

namespace odb {
  class dbDatabase;
}

namespace utl {
  class Logger;
}

namespace rcm {

class CellMoveRouter {
  public:
    CellMoveRouter();

    void helloWorld();

  private:
    odb::dbDatabase* db_;
    utl::Logger* logger_;
};
}

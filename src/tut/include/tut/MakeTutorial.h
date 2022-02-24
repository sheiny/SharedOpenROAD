#pragma once

namespace tut{
class Tutorial;
}

namespace odb{
class dbDatabase;
}

namespace ord {

class OpenRoad;

tut::Tutorial* makeTutorial();

void initTutorial(OpenRoad *openroad);

void deleteTutorial(tut::Tutorial *tutorial);

}  // namespace ord

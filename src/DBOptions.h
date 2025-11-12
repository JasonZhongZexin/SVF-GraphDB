#pragma once
#include "Util/Options.h"

namespace SVF {

extern const Option<bool> ReadFromDBOpt;
extern const Option<bool> Write2DBOpt;

bool ReadFromDB();
bool Write2DB();

} // namespace SVF

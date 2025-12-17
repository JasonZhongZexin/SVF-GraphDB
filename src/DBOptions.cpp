#include "DBOptions.h"

namespace SVF {

const Option<bool> ReadFromDBOpt("read-from-db",
                                 "Read IR/graphs from GraphDB",
                                 false);

const Option<bool> Write2DBOpt("write2db",
                               "Write analysis/results to GraphDB",
                               false);

bool ReadFromDB() { return ReadFromDBOpt(); }
bool Write2DB()   { return Write2DBOpt(); }

} // namespace SVF

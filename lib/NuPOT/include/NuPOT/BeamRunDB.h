#ifndef NU_POT_BEAM_RUN_DB_H
#define NU_POT_BEAM_RUN_DB_H

#include <sqlite3.h>

#include <string>
#include <vector>

#include "NuIO/StageResult.h"

namespace nupot {

class BeamRunDB {
public:
  explicit BeamRunDB(std::string path);
  ~BeamRunDB();

  BeamRunDB(const BeamRunDB&) = delete;
  BeamRunDB& operator=(const BeamRunDB&) = delete;

  nuio::DBSums SumRuninfoForSelection(const std::vector<nuio::RunSubrun>& pairs) const;

private:
  void Exec(const std::string& sql) const;
  void Prepare(const std::string& sql, sqlite3_stmt** stmt) const;

  std::string db_path_;
  sqlite3* db_ = nullptr;
};

}  // namespace nupot

#endif  // NU_POT_BEAM_RUN_DB_H

#ifndef NU_IO_STAGE_RESULT_IO_H
#define NU_IO_STAGE_RESULT_IO_H

#include <stdexcept>
#include <string>
#include <vector>

#include <TDirectory.h>
#include <TParameter.h>

#include "NuIO/StageResult.h"

namespace nuio {

class StageResultIO {
public:
  static void Write(const StageResult& r, const std::string& outFile);
  static StageResult Read(const std::string& inFile);

private:
  static std::string ReadNamedString(TDirectory* d, const char* key);

  template <typename T>
  static T ReadParam(TDirectory* d, const char* key) {
    TObject* obj = d->Get(key);
    auto* param = dynamic_cast<TParameter<T>*>(obj);
    if (!param) {
      throw std::runtime_error("Missing TParameter for key: " + std::string(key));
    }
    return param->GetVal();
  }

  static std::vector<std::string> ReadInputFiles(TDirectory* d);
  static std::vector<RunSubrun> ReadRunSubrunPairs(TDirectory* d);
};

}  // namespace nuio

#endif  // NU_IO_STAGE_RESULT_IO_H

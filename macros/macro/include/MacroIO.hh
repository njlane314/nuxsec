/* -- C++ -- */
/// \file macros/macro/include/MacroIO.hh
/// \brief Shared I/O helpers for ROOT macros.

#ifndef heron_macro_MACROIO_H
#define heron_macro_MACROIO_H

#include <iostream>
#include <memory>
#include <string>

#include <TFile.h>

namespace heron {
namespace macro {

/// \brief Check whether a path looks like a ROOT file used by macros.
inline bool looks_like_root_file(const std::string &path)
{
  return path.size() >= 5 && path.substr(path.size() - 5) == ".root";
}

/// \brief Open a ROOT file in read mode with consistent error reporting.
inline std::unique_ptr<TFile> open_root_file_read(const std::string &path, const std::string &macro_name)
{
  std::unique_ptr<TFile> file(TFile::Open(path.c_str(), "READ"));
  if (!file || file->IsZombie()) {
    std::cerr << "[" << macro_name << "] failed to open ROOT file: " << path << "\n";
    return std::unique_ptr<TFile>();
  }
  return file;
}

/// \brief Validate that an input path is a ROOT file and log a uniform message on failure.
inline bool validate_root_input_path(const std::string &path, const std::string &macro_name,
  const std::string &label = "input")
{
  if (looks_like_root_file(path)) {
    return true;
  }

  std::cerr << "[" << macro_name << "] " << label << " is not a ROOT file: " << path << "\n";
  return false;
}

} // namespace macro
} // namespace heron

#endif

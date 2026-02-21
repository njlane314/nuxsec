/* -- C++ -- */
/// \file macros/macro/include/MacroColumns.hh
/// \brief Shared column-validation helpers for ROOT dataframe macros.

#ifndef heron_macro_MACROCOLUMNS_H
#define heron_macro_MACROCOLUMNS_H

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace heron {
namespace macro {

/// \brief Return true if a column name exists in the provided list.
inline bool has_column(const std::vector<std::string> &columns, const std::string &name)
{
  return std::find(columns.begin(), columns.end(), name) != columns.end();
}

/// \brief Compute missing required columns from available/required sets.
inline std::vector<std::string> missing_required_columns(const std::vector<std::string> &available,
  const std::vector<std::string> &required)
{
  std::vector<std::string> missing;
  for (std::vector<std::string>::const_iterator it = required.begin(); it != required.end(); ++it) {
    if (!has_column(available, *it)) {
      missing.push_back(*it);
    }
  }
  return missing;
}

/// \brief Print a consistent missing-column report.
inline void print_missing_columns(const std::string &macro_name, const std::vector<std::string> &missing,
  std::ostream &stream = std::cerr)
{
  if (missing.empty()) {
    return;
  }

  stream << "[" << macro_name << "] missing required columns:\n";
  for (std::vector<std::string>::const_iterator it = missing.begin(); it != missing.end(); ++it) {
    stream << "  - " << *it << "\n";
  }
}

} // namespace macro
} // namespace heron

#endif

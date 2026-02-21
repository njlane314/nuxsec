/* -- C++ -- */
/// \file macros/macro/include/MacroGuard.hh
/// \brief Shared helpers to provide a consistent exception guard for ROOT macros.

#ifndef heron_macro_MACROGUARD_H
#define heron_macro_MACROGUARD_H

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <utility>

namespace heron {
namespace macro {

namespace detail {

inline void log_exception(std::ostream &stream, const std::string &macro_name, const std::exception &ex)
{
  stream << "[" << macro_name << "] exception: " << ex.what() << "\n";
}

inline void log_unknown_exception(std::ostream &stream, const std::string &macro_name)
{
  stream << "[" << macro_name << "] unknown exception\n";
}

} // namespace detail

/// \brief Execute a callable under a standard macro exception guard.
/// \param macro_name Name used in error messages.
/// \param callable Callable object executed within the guard.
/// \param stream Destination stream for error messages (defaults to std::cerr).
/// \returns The callable return value.
template <typename Callable>
inline decltype(auto) run_with_guard(const std::string &macro_name, Callable &&callable, std::ostream &stream = std::cerr)
{
  try {
    return std::invoke(std::forward<Callable>(callable));
  } catch (const std::exception &ex) {
    detail::log_exception(stream, macro_name, ex);
    throw;
  } catch (...) {
    detail::log_unknown_exception(stream, macro_name);
    throw;
  }
}

/// \brief Execute a callable under the macro exception guard and suppress rethrow.
/// \returns true on success and false if an exception was caught.
template <typename Callable>
inline bool run_with_guard_no_throw(const std::string &macro_name, Callable &&callable,
  std::ostream &stream = std::cerr)
{
  try {
    std::invoke(std::forward<Callable>(callable));
    return true;
  } catch (const std::exception &ex) {
    detail::log_exception(stream, macro_name, ex);
    return false;
  } catch (...) {
    detail::log_unknown_exception(stream, macro_name);
    return false;
  }
}

} // namespace macro
} // namespace heron

#endif

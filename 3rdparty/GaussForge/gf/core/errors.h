#pragma once

#include "3rdparty/optional.hpp"
#include <string>
#include <utility>

#ifdef _MSC_VER
#ifndef __restrict__
#define __restrict__ __restrict
#endif
#endif

namespace gf {


struct Error {
  std::string message;
};

inline Error MakeError(std::string msg) { return Error{std::move(msg)}; }

template <typename T> class Expected {
public:
  Expected(const T &value) : value_(value) {}
  Expected(T &&value) : value_(std::move(value)) {}
  Expected(const Error &error) : error_(error) {}
  Expected(Error &&error) : error_(std::move(error)) {}

  bool ok() const { return value_.has_value(); }
  explicit operator bool() const { return ok(); }

  const T &value() const { return *value_; }
  T &value() { return *value_; }
  const Error &error() const { return *error_; }

private:
  nonstd::optional<T> value_;
  nonstd::optional<Error> error_;
};

} // namespace gf

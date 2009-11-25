// Copyright (c) 2009, Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#ifdef _MSC_VER
# pragma once
#endif
#ifndef TYPES_H
#define TYPES_H

namespace QuickFAST{
  /// @brief Unsigned character. AKA: a byte
  typedef unsigned char uchar;
  /// @brief Signed 8 bit integer. AKA: char on some, but not all platforms.
  typedef boost::int8_t int8;
  /// @brief 16 bit signed integer
  typedef boost::int16_t int16;
  /// @brief 16 bit unsigned integer
  typedef boost::uint16_t uint16;
  /// @brief 32 bit signed integer
  typedef boost::int32_t int32;
  /// @brief 32 bit unsigned integer
  typedef boost::uint32_t uint32;
  /// @brief 64 bit signed integer
  typedef boost::int64_t int64;
  /// @brief 64 bit unsigned integer
  typedef boost::uint64_t uint64;
  /// @brief the mantissa portion of a Decimal
  typedef int64 mantissa_t;
  /// @brief the exponent portion of a Decimal
  typedef int8 exponent_t;

  class Decimal;
  /// @brief an alias for Decimal
  typedef Decimal decimal_t;
  /// @brief the type used to store a template Id.
  typedef uint32 template_id_t;
  /// @brief type type used to store a field Id.
  typedef std::string field_id_t;
}
#endif // TYPES_H

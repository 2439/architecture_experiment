/*BEGIN_LEGAL 
Copyright 2002-2020 Intel Corporation.

This software and the related documents are Intel copyrighted materials, and your
use of them is governed by the express license under which they were provided to
you ("License"). Unless the License provides otherwise, you may not use, modify,
copy, publish, distribute, disclose or transmit this software or the related
documents without Intel's prior written permission.

This software and the related documents are provided as is, with no express or
implied warranties, other than those that are expressly stated in the License.
END_LEGAL */
/// @file xed-syntax-enum.h

// This file was automatically generated.
// Do not edit this file.

#if !defined(XED_SYNTAX_ENUM_H)
# define XED_SYNTAX_ENUM_H
#include "xed-common-hdrs.h"
#define XED_SYNTAX_INVALID_DEFINED 1
#define XED_SYNTAX_XED_DEFINED 1
#define XED_SYNTAX_ATT_DEFINED 1
#define XED_SYNTAX_INTEL_DEFINED 1
#define XED_SYNTAX_LAST_DEFINED 1
typedef enum {
  XED_SYNTAX_INVALID,
  XED_SYNTAX_XED, ///< XED disassembly syntax
  XED_SYNTAX_ATT, ///< ATT SYSV disassembly syntax
  XED_SYNTAX_INTEL, ///< Intel disassembly syntax
  XED_SYNTAX_LAST
} xed_syntax_enum_t;

/// This converts strings to #xed_syntax_enum_t types.
/// @param s A C-string.
/// @return #xed_syntax_enum_t
/// @ingroup ENUM
XED_DLL_EXPORT xed_syntax_enum_t str2xed_syntax_enum_t(const char* s);
/// This converts strings to #xed_syntax_enum_t types.
/// @param p An enumeration element of type xed_syntax_enum_t.
/// @return string
/// @ingroup ENUM
XED_DLL_EXPORT const char* xed_syntax_enum_t2str(const xed_syntax_enum_t p);

/// Returns the last element of the enumeration
/// @return xed_syntax_enum_t The last element of the enumeration.
/// @ingroup ENUM
XED_DLL_EXPORT xed_syntax_enum_t xed_syntax_enum_t_last(void);
#endif

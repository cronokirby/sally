#pragma once

/// Represents all of the builtin commands we have
typedef enum Builtin {
  // A builtin which prints the current directory
  BUILTIN_PWD,
  // A builtin command which changes the current directory
  BUILTIN_CD
} Builtin;

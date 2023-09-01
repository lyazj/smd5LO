#pragma once

#include <string>
#include <vector>
#include <stdint.h>

// Information extracted from MG5_aMC@NLO output.
struct Mg5Run {
  std::string base;
  std::string path;
  std::string host;
  uint64_t time;
  uint64_t nano_time;
  double cs;
  double cs_err;
  std::string cs_unit;
  size_t nevt;

  Mg5Run();
  bool parse(const std::string &line);
  std::vector<std::string> selected() const;
};

// Split string `str` into several segments with delimiters in `delims`.
std::vector<std::string> split(const std::string &str, const std::string &delims="\x09\x0a\x0b\x0c\x0d");

// Fetch MG5_aMC@NLO running information in directory `path`.
std::vector<Mg5Run> list_run(std::string path = ".");

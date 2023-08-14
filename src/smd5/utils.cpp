#include "smd5/utils.h"
#include <stdio.h>

using namespace std;

Mg5Run::Mg5Run()
{
  time = 0;
  nano_time = 0;
  cs = 0.0;
  cs_err = 0.0;
  nevt = 0;
}

bool Mg5Run::parse(const std::string &line)
{
  // LINE ::= (without spaces) PATH \t CS \t CS_ERR \t CS_UNIT \t NEVT
  vector<string> elems = split(line, "\t");
  if(elems.size() != 5) return false;
  try {
    path = elems[0];
    cs = stod(elems[1]);
    cs_err = stod(elems[2]);
    cs_unit = elems[3];
    nevt = stoll(elems[4]);
  } catch(...) { return false; }

  // PATH ::= (without spaces) run / HOST - TIME_NANO_TIME
  vector<string> path_elems = split(path, "/-");
  if(path_elems.size() != 3 || path_elems[0] != "run") return false;
  try {
    host = path_elems[1];
  } catch(...) { return false; }
  const string &time_nano_time = path_elems[2];

  // TIME_NANO_TIME ::= (without spaces) TIME . NANO_TIME
  vector<string> time_elems = split(time_nano_time, ".");
  if(time_elems.size() != 2) return false;
  try {
    time = stoll(time_elems[0]);
    nano_time = stoll(time_elems[1]);
  } catch(...) { return false; }

  return true;
}

vector<string> split(const string &str, const string &delims)
{
  vector<string> elems;
  size_t b = 0, e;
  for(;;) {
    e = str.find_first_of(delims, b);
    if(e == str.npos) {
      elems.emplace_back(str, b);
      return elems;
    }
    elems.emplace_back(str, b, e - b);
    b = str.find_first_not_of(delims, e + 1);
  }
}

vector<Mg5Run> list_run(string path)
{
  // Launch `list-run` program in directory `path`.
  char buf[BUFSIZ];
  vector<Mg5Run> runs;
  string cmd = "cd '" + path + "' && ./list-run 2>/dev/null";
  FILE *pp = popen(cmd.c_str(), "r");

  // Read and parse output lines from stdout stream.
  Mg5Run run;
  while(fgets(buf, sizeof buf, pp)) {
    string line = buf;
    if(line.empty() || line.back() != '\n') {
      fprintf(stderr, "ERROR: expect new line\n");
      return { };
    }
    line.pop_back();
    if(!run.parse(line)) {
      fprintf(stderr, "ERROR: unrecognized line: %s\n", line.c_str());
      continue;
    }
    runs.push_back(run);
  }
  return runs;
}

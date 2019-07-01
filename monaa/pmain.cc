#include <iostream>
#include <boost/program_options.hpp>

#include "../libmonaa/parametric_monaa.hh"
#include "parametric_timed_automaton_parser.hh"
#include "config.hh"

using namespace boost::program_options;

int main(int argc, char *argv[])
{
  const auto programName = "pmonaa";
  const auto errorHeader = "pmonaa: ";

  std::cin.tie(0);
  std::ios::sync_with_stdio(false);

  const auto die = [&errorHeader] (const char* message, int status) {
    std::cerr << errorHeader << message << std::endl;
    exit(status);
  };

  // visible options
  options_description visible("description of options");
  std::string timedWordFileName;
  std::string timedAutomatonFileName;
  std::string skipMode;
  bool isBinary = false;
  visible.add_options()
    ("help,h", "help")
    ("quiet,q", "quiet")
    ("skip,s", value<std::string>(&skipMode)->default_value("parametric"), "Specify which skipping is used. It should be one of `parametric` (default), `non-parametric`, and `none`.")
    ("ascii,a", "ascii mode (default)")
    ("binary,b", "binary mode")
    ("version,V", "version")
    ("input,i", value<std::string>(&timedWordFileName)->default_value("stdin"),"input timedWordFile of Timed Words")
    ("rational,r", "use rational number of GMP (default)")
    ("float,F", "use floating point number")
    ("automaton,f", value<std::string>(&timedAutomatonFileName)->default_value(""),"input timedWordFile of Parametric Timed Automaton");

  command_line_parser parser(argc, argv);
  parser.options(visible);
  variables_map vm;
  const auto parseResult = parser.run();
  store(parseResult, vm);
  notify(vm);

  if (timedAutomatonFileName.empty() || vm.count("help")) {
    std::cout << programName << " [OPTIONS] -f FILE [FILE]\n" 
              << visible << std::endl;
    return 0;
  }
  if (vm.count("version")) {
    std::cout << "ParamMONAA (a PARAMetric MONitoring tool Acceralated by Automata) " << PROJECT_VER
              << "\n"
              << visible << std::endl;
    return 0;
  }
  if ((vm.count("ascii") && vm.count("binary"))) {
    die("conflicting input formats specified", 1);
  }
  if (vm.count("binary")) {
    isBinary = true;
  } else if (vm.count("ascii")) {
    isBinary = false;
  }

  if (skipMode == "parametric") {
    parametric_monaa::enable_quick_search = true;
    parametric_monaa::enable_parametric_kmp = true;
    parametric_monaa::enable_kmp = false;
  } else if (skipMode == "non-parametric") {
    parametric_monaa::enable_quick_search = true;
    parametric_monaa::enable_parametric_kmp = false;
    parametric_monaa::enable_kmp = true;
  } else if (skipMode == "none") {
    parametric_monaa::enable_quick_search = false;
    parametric_monaa::enable_parametric_kmp = false;
    parametric_monaa::enable_kmp = false;
  } else {
    std::cerr << "Unknown skip type is specified: " << skipMode << "\n";
    return 1;
  }

  ParametricTimedAutomaton TA;

  // parse PTA
  std::ifstream taStream(timedAutomatonFileName);
  BoostParametricTimedAutomaton BoostTA;
  parseBoostTA(taStream, BoostTA);
  convBoostTA(BoostTA, TA);

  FILE* timedWordFile;

  if (timedWordFileName == "stdin") {
    timedWordFile = stdin;
  } else {
    timedWordFile = fopen(timedWordFileName.c_str(), "r");
  }
  AnsPolyhedronPrinter ans(vm.count("quiet"));

  if(vm.count("float")) {
    WordLazyDeque w(timedWordFile, isBinary);
    parametricMonaa(w, TA, ans);
  } else {
    WordLazyDequeMPQ w(timedWordFile, isBinary);
    parametricMonaa(w, TA, ans);
  }
  return 0;
}

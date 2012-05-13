#include <iostream>
#include <fstream>
#include <string>

#include <rans.hpp>

DEFINE_string(f, "", "Obtain patterns from FILE.");
DEFINE_bool(size, false, "Print DFA's size.");
DEFINE_bool(repl, false, "Start REPL.");
DEFINE_string(text, "", "Print value of given text on ANS.");
DEFINE_string(value, "", "Print text of given value on ANS.");
DEFINE_bool(verbose, false, "Report additional informations.");

int main(int argc, char* argv[])
{
  google::SetUsageMessage("This program is RANS sample driver.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  std::string regex;
  if (FLAGS_f != "") {
    std::ifstream ifs(FLAGS_f.data());
    ifs >> regex;
  } else if (argc > 1) {
    regex = argv[1];
  } else {
    exit(0);
  }

  RANS r(regex);
  if (FLAGS_size) std::cout << "dfa.size() = " << r.dfa().size() << std::endl;

  if (FLAGS_value != "") {
    try {
      std::cout << r.rep(mpz_class(FLAGS_value)) << std::endl;
    } catch (const char *msg) {
      std::cout << msg << std::endl;
    }
  } else if (FLAGS_text != "") {
    try {
      std::cout << r.val(FLAGS_text) << std::endl;
    } catch (const char *msg) {
      std::cout << msg << std::endl;
    }
  } else if (FLAGS_repl) {
    std::string text;
    mpz_class value;

    while (std::cin >> text) {
      try {
        std::cout << r.val(text, value) << std::endl;
        std::cout << r.rep(value, text) << std::endl;
      } catch (const char *msg) {
        std:: cout << msg << std::endl;
      }
    }
  }
  
  return 0;
}

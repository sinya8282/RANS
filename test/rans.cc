#include <rans.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <gflags/gflags.h>

DEFINE_string(f, "", "Obtain patterns from FILE.");
DEFINE_bool(size, false, "Print DFA's size.");
DEFINE_bool(repl, false, "Start REPL.");
DEFINE_string(text, "", "Print value of given text on ANS.");
DEFINE_string(value, "", "Print text of given value on ANS.");
DEFINE_bool(verbose, false, "Report additional informations.");
DEFINE_bool(syntax, false, "Print RANS regular expression syntax.");
DEFINE_string(encode, "", "Encode given file.");
DEFINE_string(decode, "", "Decode given file");
DEFINE_string(out, "", "Output file name.");

void set_filename(const std::string& src, std::string& dst)
{
  if (dst != "") return;

  const std::size_t suffix_len = 5; // suffix = ".rans"

  if (src.length() > suffix_len &&
      src.substr(src.length() - suffix_len, std::string::npos) == ".rans") {
    dst = src.substr(0, src.length() - suffix_len);
  } else {
    std::string input = "dummy";
    do {
      switch (input[0]) {
        case 'y': case 'A': dst = src; return;
        case 'n': case 'N': return;
        case 'r':
          std::cout << "new name: ";
          std::cin >> dst;
          return;
        default:
          std::cout << "replace hoge? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
      }
    } while (std::cin >> input);
  }
}

int main(int argc, char* argv[])
{
  google::SetUsageMessage(
      "Usage: rans REGEX [OPTION]...\n"
      "You can check RANS extended regular expression syntax by '--syntax' option.\n\n"
      "OPTIONS:");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_syntax) {
    std::cout << rans::SYNTAX;
    return 0;
  }
  
  std::string regex;
  if (FLAGS_f != "") {
    std::ifstream ifs(FLAGS_f.data());
    ifs >> regex;
  } else if (argc > 1) {
    regex = argv[1];
  } else {
    exit(0);
  }

  try {
    RANS r(regex);
    if (FLAGS_encode != "") {
      std::string text; RANS::Value value;
      std::ifstream ifs(FLAGS_encode.data());
      ifs >> text;
      if (FLAGS_out == "") FLAGS_out = FLAGS_encode;
      FLAGS_out += ".rans";
      RANS::write(FLAGS_out, r(text, value));
    } else if (FLAGS_decode != "") {
      std::string text; RANS::Value value;
      RANS::read(FLAGS_decode, value);
      set_filename(FLAGS_decode, FLAGS_out);
      if (FLAGS_out == "") exit(0);
      std::ofstream ofs(FLAGS_out.data());
      ofs << r(value, text);
    } else if (FLAGS_size) {
      std::cout << "Size of DFA: " << r.dfa().size() << std::endl;
    } else if (FLAGS_value != "") {
      try {
        std::cout << r(RANS::Value(FLAGS_value)) << std::endl;
      } catch (const char *msg) {
        std::cout << msg << std::endl;
      }
    } else if (FLAGS_text != "") {
      try {
        std::cout << r(FLAGS_text) << std::endl;
      } catch (const char *msg) {
        std::cout << msg << std::endl;
      }
    } else if (FLAGS_repl) {
      std::string text;
      RANS::Value value;

      while (std::cin >> text) {
        try {
          std::cout << r(text, value) << std::endl;
          std::cout << r(value, text) << std::endl;
        } catch (const char *msg) {
          std:: cout << msg << std::endl;
        }
      }
    }
  } catch (const char* msg) {
    std::cout << msg << std::endl;
    exit(0);
  }
  
  return 0;
}

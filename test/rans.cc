#include <gflags/gflags.h>
#include <rans.hpp>

#include <iostream>
#include <fstream>
#include <string>

DEFINE_string(f, "", "obtain patterns from FILE.");
DEFINE_string(text, "", "print value of given text on ANS.");
DEFINE_string(quick_check, "", "check wheter given text is acceptable or not.");
DEFINE_string(value, "", "print text of given value on ANS.");
DEFINE_bool(verbose, false, "report additional informations.");
DEFINE_bool(syntax, false, "print RANS regular expression syntax.");
DEFINE_bool(utf8, false, "use utf8 as internal encoding.");
DEFINE_string(convert_from, "", "convert given value base from given expression.");
DEFINE_string(convert_to, "", "convert given value base to given expression.");
DEFINE_string(compress, "", "compress given file (create '.rans' file, by default).");
DEFINE_string(decompress, "", "decompress given file");
DEFINE_string(out, "", "output file name.");
DEFINE_bool(size, false, "print DFA's size.");
DEFINE_bool(repl, false, "start REPL.");

const std::string version = ": "
#ifdef RANS_DEBUG
    "DEBUG"
#else
    "DEVELOP"
#endif    
    ", matrix lib = "
#ifdef RANS_USE_UBLAS
    "uBLAS"
#else
    "SELF"
#endif
    ;

void set_filename(const std::string&, std::string&);

int main(int argc, char* argv[])
{
  google::SetVersionString(version);
  google::SetUsageMessage(
      "RANS driver program.\n"
      "Usage: rans REGEX [OPTIONS ...] \n"
      "You can check RANS extended regular expression syntax by '--syntax' option.\n\n"
      "OPTIONS:");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (FLAGS_syntax) {
    std::cout << rans::SYNTAX;
    return 0;
  }
  
  std::string regex;
  if (!FLAGS_f.empty()) {
    std::ifstream ifs(FLAGS_f.data());
    ifs >> regex;
  } else if (argc > 1) {
    regex = argv[1];
  } else if (FLAGS_convert_from.empty() || FLAGS_convert_to.empty()) {
    exit(0);
  }

  RANS::Encoding enc = FLAGS_utf8 ? RANS::UTF8 : RANS::ASCII;
  if (!FLAGS_convert_from.empty() && !FLAGS_convert_to.empty()) {
    RANS from(FLAGS_convert_from), to(FLAGS_convert_to);
    if (!from.ok() || !to.ok()) {
      std::cout << from.error() << std::endl << to.error() << std::endl;
      exit(0);
    }
    // try without error check ;-p
    if (!FLAGS_text.empty()) {
      std::cout << to(from(FLAGS_text)) << std::endl;
    }
    return 0;
  }

  RANS r(regex, enc);
  if (!r.ok()) {
    std::cout << r.error() << std::endl;
    exit(0);
  }

  if (!FLAGS_quick_check.empty()) {
    if (r.dfa().is_acceptable(FLAGS_quick_check)) {
      std::cout << "text is acceptable." << std::endl;
    } else {
      std::cout << "text is not acceptable." << std::endl;
    }
  } else if (!FLAGS_compress.empty()) {
    std::string text;
    std::ifstream ifs(FLAGS_compress.data(), std::ios::in | std::ios::binary);
    if (ifs.fail()) {
      std::cout << FLAGS_decompress + " does not exists." << std::endl;
      exit(0);
    }
    ifs >> text;

    if (FLAGS_out.empty()) {
      FLAGS_out = FLAGS_compress;
      FLAGS_out += ".rans";
    }

    std::ofstream ofs(FLAGS_out.data(), std::ios::out | std::ios::binary);
    ofs << r.compress(text);
  } else if (!FLAGS_decompress.empty()) {
    std::ifstream ifs(FLAGS_decompress.data(), std::ios::in | std::ios::binary);
    if (ifs.fail()) {
      std::cout << FLAGS_decompress + " does not exists." << std::endl;
      exit(0);
    }

    std::istreambuf_iterator<char> first(ifs);
    std::istreambuf_iterator<char> last;
    std::string text(first, last);

    set_filename(FLAGS_decompress, FLAGS_out);
    if (FLAGS_out.empty()) exit(0);
    std::ofstream ofs(FLAGS_out.data(), std::ios::out | std::ios::binary);
    ofs << r.decompress(text);
  } else if (FLAGS_size) {
    std::cout << "size of DFA: " << r.dfa().size() << std::endl;
  } else if (!FLAGS_value.empty()) {
    std::cout << r(RANS::Value(FLAGS_value)) << std::endl;
  } else if (!FLAGS_text.empty()) {
    std::cout << r(FLAGS_text) << std::endl;
  } else if (FLAGS_repl) {
    std::string text;
    RANS::Value value;

    while (std::cin >> text) {
      if (r.is_acceptable(text)) {
        std::cout << "text is not acceptable" << std::endl;
        continue;
      }
      std::cout << r(text, value) << std::endl;
      std::cout << r(value, text) << std::endl;
    }
  }
  
  return 0;
}

void set_filename(const std::string& src, std::string& dst)
{
  if (!dst.empty()) return;

  static const std::string suffix = ".rans";

  if (src.length() > suffix.length() &&
      src.substr(src.length() - suffix.length(), std::string::npos) == suffix) {
    dst = src.substr(0, src.length() - suffix.length() - 1);
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
          std::cout << "replace " << src << "? [y]es, [n]o, [A]ll, [N]one, [r]ename: ";
      }
    } while (std::cin >> input);
  }
}

#include <gflags/gflags.h>
#include <rans.hpp>

#include <iostream>
#include <fstream>
#include <string>

DEFINE_bool(dump_dfa, false, "dump DFA as dot language.");
DEFINE_bool(dump_matrix, false, "dump Matrix.");
DEFINE_bool(dump_exmatrix, false, "dump Extended Matrix.");
DEFINE_bool(dump_scc, false, "dump Strongly-connected-components of DFA.");
DEFINE_string(f, "", "obtain patterns from FILE.");
DEFINE_bool(i, false, "ignore case distinctions in both the REGEX and the input files..");
DEFINE_string(text, "", "print the value of given text on ANS.");
DEFINE_string(quick_check, "", "check wheter given text is acceptable or not.");
DEFINE_string(value, "", "print the text of given value on ANS.");
DEFINE_bool(verbose, false, "report additional informations.");
DEFINE_bool(syntax, false, "print RANS regular expression syntax.");
DEFINE_bool(utf8, false, "use utf8 as internal encoding.");
DEFINE_string(convert_from, "", "convert the given value base from the given expression.");
DEFINE_string(convert_to, "", "convert the given value base to the given expression.");
DEFINE_string(compress, "", "compress the given file (create '.rans' file, by default).");
DEFINE_string(decompress, "", "decompress the given file");
DEFINE_string(out, "", "output file name.");
DEFINE_bool(size, false, "print the size of the DFA.");
DEFINE_bool(repl, false, "start REPL.");
DEFINE_bool(amount, false, "print number of acceptable strings that has less than '--value' characters in length.");
DEFINE_int64(count, -1, "print number of acceptable strings that just has specified characters in length.");
DEFINE_bool(compression_ratio, false, "print asymptotic compression ratio [%].");
DEFINE_bool(frobenius_root, false, "print frobenius root of adjacency matrix.");
DEFINE_bool(frobenius_root2, false, "print frobenius root of adjacency matrix without linear algebraic optimization.");
DEFINE_bool(factorial, false, "make langauge as a factorial");

void dispatch(const RANS&);
void set_filename(const std::string&, std::string&);

int main(int argc, char* argv[])
{
  google::SetUsageMessage(
      "RANS command line tool.\n"
      "Usage: rans REGEX [Flags ...] \n"
      "You can check RANS extended regular expression syntax via '--syntax' option."
                          );
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
    std::cout << google::ProgramUsage() << std::endl;
    return 0;
  }

  RANS::Encoding enc = FLAGS_utf8 ? RANS::UTF8 : RANS::ASCII;
  if (!FLAGS_convert_from.empty() && !FLAGS_convert_to.empty()) {
    RANS from(FLAGS_convert_from, enc, FLAGS_factorial, FLAGS_i),
        to(FLAGS_convert_to, enc, FLAGS_factorial, FLAGS_i);
    if (!from.ok() || !to.ok()) {
      std::cout << from.error() << std::endl << to.error() << std::endl;
      exit(0);
    }

    if (FLAGS_compression_ratio) {
      std::cout << from.compression_ratio(FLAGS_count, to) << std::endl;
    } else if (!FLAGS_text.empty()) {
      try {
        std::cout << to(from(FLAGS_text)) << std::endl;
      } catch (RANS::Exception& e) {
        std::cout << e.what() << std::endl;
      }
    }
    return 0;
  }

  RANS r(regex, enc, FLAGS_factorial, FLAGS_i);
  if (!r.ok()) {
    std::cout << r.error() << std::endl;
    exit(0);
  }

  if (FLAGS_dump_dfa) std::cout << r.dfa();
  if (FLAGS_dump_matrix) std::cout << r.adjacency_matrix();
  if (FLAGS_dump_exmatrix) std::cout << r.extended_adjacency_matrix();
  if (FLAGS_dump_scc) {
    for (std::size_t i = 0; i < r.scc().size(); i++) {
      for (std::set<std::size_t>::iterator iter = r.scc()[i].begin();
           iter != r.scc()[i].end(); ++iter) {
        std::cout << *iter << ", ";
      }
      std::cout << std::endl;
    }
  }
  
  try {
    dispatch(r);
  } catch (RANS::Exception& e) {
    std::cout << e.what() << std::endl;
    exit(0);
  }
  
  return 0;
}

void dispatch(const RANS& r) {
  if (FLAGS_frobenius_root) {
    std::cout << r.spectrum().root << std::endl;
  } else if (FLAGS_frobenius_root2) {
    std::cout << r.adjacency_matrix().frobenius_root() << std::endl;
  } else if (FLAGS_compression_ratio) {
    std::cout << r.compression_ratio(FLAGS_count) << std::endl;
  } else if (FLAGS_amount) {
    if (FLAGS_count < 0) {
      if (r.finite()) {
        std::cout << r.amount() << std::endl;
      } else {
        std::cout << "there exists infinite acceptable strings." << std::endl;
      }
    } else {
      std::cout << r.amount(FLAGS_count) << std::endl;
    }
  } else if (FLAGS_count >= 0) {
    std::cout << r.count(FLAGS_count) << std::endl;
  } else if (!FLAGS_quick_check.empty()) {
    if (r.dfa().accept(FLAGS_quick_check)) {
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
    try {
      std::string dst;
      ofs << r.compress(text, dst);
    } catch (const RANS::Exception &e) {
      std::cout << e.what() << std::endl;
    }
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
    try {
      std::string dst;
      ofs << r.decompress(text, dst);
    } catch (const RANS::Exception &e) {
      std::cout << e.what() << std::endl;
    }
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
      try {
        std::cout << r(text, value) << std::endl;
        std::cout << r(value, text) << std::endl;
      } catch(const RANS::Exception& e) {
        std::cout << e.what() << std::endl;
      }
    }
  }
}

void set_filename(const std::string& src, std::string& dst)
{
  if (!dst.empty()) return;

  static const std::string suffix = ".rans";

  if (src.length() > suffix.length() &&
      src.substr(src.length() - suffix.length(), std::string::npos) == suffix) {
    dst = src.substr(0, src.length() - suffix.length());
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

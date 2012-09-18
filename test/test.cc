#include <gtest/gtest.h>
#include <rans.hpp>
#include <map>
#include <fstream>
#include <string>

TEST(ELEMENTAL_TEST, DFA_MINIMIZE) {
  std::map<std::string, std::size_t> tests;
  tests["a*"] = 1;
  tests["a"] = 2;
  tests["a|b"] = 2;
  tests["(a|b)*c"] = 2;
  tests["[ab]*[ac][abc]{1}"] = 7;
  tests["[ab]*[ac][abc]{2}"] = 15;
  tests["[ab]*[ac][abc]{3}"] = 31;
  tests["[ab]*[ac][abc]{4}"] = 63;

  for (std::map<std::string, std::size_t>::iterator iter = tests.begin();
       iter != tests.end(); ++iter) {
    rans::DFA d(iter->first);
    ASSERT_EQ(iter->second, d.size())
        << "DFA(" << iter->first << ").size() != " << iter->second
        << "(" << d.size() << ")";
  }
}

TEST(ELEMENTAL_TEST, DFA_ACCEPT) {
  struct testcase {
    testcase(std::string regex_, std::string text_, bool result_): regex(regex_), text(text_), result(result_) {}
    std::string regex;
    std::string text;
    bool result;
  };
  testcase accept_test[] = {
    testcase("abc", "abc", true),
    testcase("abc", "xbc", false),
    testcase("abc", "axc", false),
    testcase("abc", "abx", false),
    testcase(".*abc.*", "xabcy", true),
    testcase(".*abc", "ababc", true),
    testcase("ab*c", "abc", true),
    testcase("ab*bc", "abc", true),
    testcase("ab*bc", "abbc", true),
    testcase("ab*bc", "abbbbc", true),
    testcase("ab+bc", "abbc", true),
    testcase("ab+bc", "abc", false),
    testcase("ab+bc", "abq", false),
    testcase("ab+bc", "abbbbc", true),
    testcase("ab?bc", "abbc", true),
    testcase("ab?bc", "abc", true),
    testcase("ab?bc", "abbbbc", false),
    testcase("ab?c", "abc", true),
    testcase("a.c", "abc", true),
    testcase("a.c", "axc", true),
    testcase("a.*c", "axyzc", true),
    testcase("a.*c", "axyzd", false),
    testcase("a[bc]d", "abc", false),
    testcase("a[bc]d", "abd", true),
    testcase("a[b-d]e", "abd", false),
    testcase("a[b-d]e", "ace", true),
    testcase(".*a[b-d]", "aac", true),
    testcase("a[-b]", "a-", true),
    testcase("a[b-]", "a-", true),
    testcase("a]", "a]", true),
    testcase("a[]]b", "a]b", true),
    testcase("a[^bc]d", "aed", true),
    testcase("a[^bc]d", "abd", false),
    testcase("a[^-b]c", "adc", true),
    testcase("a[^-b]c", "a-c", false),
    testcase("a[^]b]c", "a]c", false),
    testcase("a[^]b]c", "adc", true),
    testcase("ab|cd", "abc", false),
    testcase("ab|cd", "abcd", false),
    testcase("$b", "b", false),
    testcase("a\\(b", "a(b", true),
    testcase("a\\(*b", "ab", true),
    testcase("a\\(*b", "a((b", true),
    testcase("a\\x", "a\\x", false),
    testcase("((a))", "a", true),
    testcase("(a)b(c)", "abc", true),
    testcase("a+b+c", "aabbbc", true),
    testcase("a**", "", true),
    testcase("a*?", "", true),
    testcase("(a*)*", "", true),
    testcase("(a*)+", "", true),
    testcase("(a*|b)*", "", true),
    testcase("(a+|b)*", "ab", true),
    testcase("(a+|b)+", "ab", true),
    testcase(".*(a+|b)?", "ab", true),
    testcase("[^ab]*", "cde", true),
    testcase("abc", "", false),
    testcase("abc", "", false),
    testcase("a*", "", true),
    testcase("([abc])*d", "abbbcd", true),
    testcase("([abc])*bcd", "abcd", true),
    testcase("a|b|c|d|e", "e", true),
    testcase("(a|b|c|d|e)f", "ef", true),
    testcase("((a*|b))*", "", true),
    testcase("abcd*efg", "abcdefg", true),
    testcase("ab*", "xabyabbbz", false),
    testcase("ab*", "xayabbbz", false),
    testcase(".*(ab|cd)e", "abcde", true),
    testcase("[abhgefdc]ij", "hij", true),
    testcase(".*(a|b)c*d", "abcd", true),
    testcase("(ab|ab*)bc", "abc", true),
    testcase("a([bc]*)c*", "abc", true),
    testcase("a([bc]*)(c*d)", "abcd", true),
    testcase("a([bc]+)(c*d)", "abcd", true),
    testcase("a([bc]*)(c+d)", "abcd", true),
    testcase("a[bcd]*dcdcde", "adcdcde", true),
    testcase("a[bcd]+dcdcde", "adcdcde", false),
    testcase("(ab|a)b*c", "abc", true),
    testcase("((a)(b)c)(d)", "abcd", true),
    testcase("[A-Za-z_][A-Za-z1-9_]*", "alpha", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "effgz", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "ij", true),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "effg", false),
    testcase("(bc+d$|ef*g.|h?i(j|k))", "bcdd", false),
    testcase(".*(bc+d$|ef*g.|h?i(j|k))", "reffgz", true),
    testcase("((((((((((a))))))))))", "-", false),
    testcase("(((((((((a)))))))))", "a", true),
    testcase("multiple words of text", "uh-uh", false),
    testcase("multiple words.*", "multiple words, yeah", true),
    testcase("(.*)c(.*)", "abcde", true),
    testcase("\\((.*),", "(.*)\\)", false),
    testcase("[k]", "ab", false),
    testcase("abcd", "abcd", true),
    testcase("a(bc)d", "abcd", true),
    testcase("a[-]?c", "ac", true)
  };
  const std::size_t num_of_test = sizeof(accept_test) / sizeof(testcase);
  for (std::size_t i = 0; i < num_of_test; i++) {
    testcase &t = accept_test[i];
    try {
      rans::DFA d(t.regex);
      ASSERT_EQ(t.result, d.accept(t.text))
          << "regex: " << t.regex << ", text: " << t.text;
    } catch(const char* msg) {
      std::cout << "regex: " << t.regex
                << ", text: " << t.text
                << std::endl << msg << std::endl;
    }
  }
}

TEST(COUNTING_TEST, RANS_COUNT_AND_AMOUNT) {
  struct testcase {
    testcase(std::string regex_, int amount_, int count_ = 0, std::size_t length_ = 0):
        regex(regex_), amount(amount_), count(count_), length(length_) {}
    std::string regex;
    int amount;
    int count;
    std::size_t length;
  };

  testcase total_amount_test[] = {
    testcase("a*b*|b*c*", -1),
    testcase("a?", 2),
    testcase("[12345]", 5),
    testcase("[^12345]", 256-5),
    testcase("...", 256*256*256),
    testcase("", 1),
    testcase("there exist just one string!", 1),
    testcase("or infinite strings!*", -1),
    testcase("[ab][cde][efgh][ijklm][opqrst][uvwxyz]", 2*3*4*5*6*6)
  };
  const std::size_t num_of_test = sizeof(total_amount_test) / sizeof(testcase);

  for (std::size_t i = 0; i < num_of_test; i++) {
    testcase& t = total_amount_test[i];
    RANS r(t.regex);
    ASSERT_EQ(t.amount, r.amount()) << "regex: " << t.regex;
  }


  testcase amount_and_count_test[] = {
    testcase("a*b*|b*c*", 4, 3, 1),
    testcase("a?", 2, 1, 1),
    testcase("a?", 1, 1, 0),
    testcase("", 1, 0, 100),
    testcase("there exist just one string!", 0, 0, 27),
    testcase("there exist just one string!", 1, 1, 28),
    testcase("there exist just one string!", 1, 0, 29),
    testcase("a*(b*|c*)", 1, 1, 0),
    testcase("a*(b*|c*)", 1+3, 3, 1),
    testcase("a*(b*|c*)", 1+3+5, 5, 2),
  };
  const std::size_t num_of_test_ = sizeof(amount_and_count_test) / sizeof(testcase);

  for (std::size_t i = 0; i < num_of_test_; i++) {
    testcase& t = amount_and_count_test[i];
    RANS r(t.regex);
    ASSERT_EQ(t.amount, r.amount(t.length))
        << "regex: " << t.regex << ", length = " << t.length;
    ASSERT_EQ(t.count, r.count(t.length))
        << "regex: " << t.regex << ", length = " << t.length;
  }
}

// Theorem(Eilenberg): The set of squares { 1, 4, 9,.., n^2, .. }
// is never recognizable in any integer base system.
// 
// But this theorem doesn't hold in ANS on L(a*b*|a*c*) !!
// -> val("a") = 1, val("aa") = 4,,, val("a"*n) = n^2

TEST(EILENBERG_TEST, RANS_VAL) {
  RANS r("a*b*|a*c*");
  std::string text;
  for (std::size_t i = 0; i < 10; i++) {
    ASSERT_EQ(i * i, r(text));
    text += "a";
  }
}

TEST(EILENBERG_TEST, RANS_REP) {
  RANS r("a*b*|a*c*");
  std::string text;
  for (std::size_t i = 0; i < 10; i++) {
    ASSERT_EQ(text, r(i * i));
    text += "a";
  }
}

// Googol = 10^100
const RANS::Value googol("10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");

RANS base2("0|1[01]*");
const std::string googol_base2 = "100100100100110101101001001011001010011000011011111001110101100001011001001111000010011000100110011100000101111110011100010101100111001000000100011100010000100011010011111001010101010110010010000110000100010101000001011101000111100010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";

RANS base3("0|[12][012]*");
const std::string googol_base3 = "122012210112120112111212010011100001101211222101110010100012001010011011021010111212020100220020021122002200200010101000112122102122010002012010000000120120022011020201122101010221121011200012121021202022020101";

RANS base16("0|[1-9A-F][0-9A-F]*");
const std::string googol_base16 = "1249AD2594C37CEB0B2784C4CE0BF38ACE408E211A7CAAB24308A82E8F10000000000000000000000000";

RANS baseACGT("[ACGT]+");
const std::string googol_baseACGT = "TATTCACCCTTCAAATTTCGTGAGCTGCCCGTCCTCAGAGTTCGTTCTGAGTCGGCTGATCCCTGATGGTATGATAGTTACCAGCTCCCCCCTATGTCGTATCCAGTCGCATGCGTGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGTA";

TEST(GOOGOL_BASE_TEST, RANS_VAL) {
  ASSERT_EQ(googol, base2(googol_base2));
  ASSERT_EQ(googol, base3(googol_base3));
  ASSERT_EQ(googol, base16(googol_base16));
  ASSERT_EQ(googol, baseACGT(googol_baseACGT));
}

TEST(GOOGOL_BASE_TEST, RANS_REP) {
  ASSERT_EQ(googol_base2, base2(googol));
  ASSERT_EQ(googol_base3, base3(googol));
  ASSERT_EQ(googol_base16, base16(googol));
  ASSERT_EQ(googol_baseACGT, baseACGT(googol));
}


// Complex regular language test.
// Formal URI can be defined by regular expression.
// #see RFC2396/3986 Uniform Resource Identifiers (URI): Generic Syntax
// -> http://www.ietf.org/rfc/rfc2396.txt, http://www.ietf.org/rfc/rfc3986.txt
//

RANS base_uri2396("([a-z][\\x2b\\x2d\\x2e0-9a-z]*:((//((((%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-;=_a-z~])*@)?((([0-9a-z]|[0-9a-z][\\x2d0-9a-z]*[0-9a-z])\\x2e)*([a-z]|[a-z][\\x2d0-9a-z]*[0-9a-z])\\x2e?|\\d+\\x2e\\d+\\x2e\\d+\\x2e\\d+)(:\\d*)?)?|(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-;=@_a-z~])+)(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*)*)?|/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*)*)(\\x3f([!\\x24&-;=\\x3f@_a-z~]|%[0-9a-f][0-9a-f])*)?|(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-;=\\x3f@_a-z~])([!\\x24&-;=\\x3f@_a-z~]|%[0-9a-f][0-9a-f])*)|(//((((%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-;=_a-z~])*@)?((([0-9a-z]|[0-9a-z][\\x2d0-9a-z]*[0-9a-z])\\x2e)*([a-z]|[a-z][\\x2d0-9a-z]*[0-9a-z])\\x2e?|\\d+\\x2e\\d+\\x2e\\d+\\x2e\\d+)(:\\d*)?)?|(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-;=@_a-z~])+)(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*)*)?|/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*)*|(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-9;=@_a-z~])+(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*(/(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*(;(%[0-9a-f][0-9a-f]|[!\\x24&-\\x2e0-:=@_a-z~])*)*)*)?)(\\x3f([!\\x24&-;=\\x3f@_a-z~]|%[0-9a-f][0-9a-f])*)?)?(\\x23([!\\x24&-;=\\x3f@_a-z~]|%[0-9a-f][0-9a-f])*)?");
RANS base_uri3986("[a-z][\\x2b\\x2d\\x2e0-9a-z]*:(//(([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=])*@)?(\\x5b(([0-9a-f]{1,4}:){6}([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|::([0-9a-f]{1,4}:){5}([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|([0-9a-f]{1,4})?::([0-9a-f]{1,4}:){4}([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|(([0-9a-f]{1,4}:)?[0-9a-f]{1,4})?::([0-9a-f]{1,4}:){3}([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|(([0-9a-f]{1,4}:){0,2}[0-9a-f]{1,4})?::([0-9a-f]{1,4}:){2}([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|(([0-9a-f]{1,4}:){0,3}[0-9a-f]{1,4})?::[0-9a-f]{1,4}:([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|(([0-9a-f]{1,4}:){0,4}[0-9a-f]{1,4})?::([0-9a-f]{1,4}:[0-9a-f]{1,4}|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5]))|(([0-9a-f]{1,4}:){0,5}[0-9a-f]{1,4})?::[0-9a-f]{1,4}|(([0-9a-f]{1,4}:){0,6}[0-9a-f]{1,4})?::|v[0-9a-f]+\\x2e[!\\x24&-\\x2e0-;=_a-z~]+)\\x5d|(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])\\x2e(\\d|[1-9]\\d|1\\d{2}|2[0-4]\\d|25[0-5])|([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,;=])*)(:\\d*)?(/([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=@])*)*|/(([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=@])+(/([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=@])*)*)?|([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=@])+(/([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,:;=@])*)*)?(\\x3f([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,/:;=\\x3f@])*)?(\\x23([\\x2d\\x2e0-9_a-z~]|%[0-9a-f][0-9a-f]|[!\\x24&-,/:;=\\x3f@])*)?");

TEST(URI_TEST, DFA_MINIMIZE) {
  ASSERT_EQ(12, base_uri2396.size());
  ASSERT_EQ(179, base_uri3986.size());
}

const std::string homepage_url("http://swatmac.info/");
const RANS::Value homepage_val_rfc2396("47707318137249109960017513245316138");
const RANS::Value homepage_val_rfc3986("846983527258207777892746259575925");

TEST(URI_TEST, RANS_VAL) {
  ASSERT_EQ(homepage_val_rfc2396, base_uri2396(homepage_url));
  ASSERT_EQ(homepage_val_rfc3986, base_uri3986(homepage_url));
}

TEST(URI_TEST, RANS_REP) {
  ASSERT_EQ(homepage_url, base_uri2396(homepage_val_rfc2396));
  // ASSERT_EQ(homepage_url, base_uri3986(homepage_val_rfc3986));
}

TEST(URI_TEST, RANS_FINITE) {
  ASSERT_TRUE(base_uri2396.infinite());
  // ASSERT_TRUE(base_uri3986.infinite());
}

TEST(URI_TEST, RANS_COMPRESSION_RATIO) {
  ASSERT_GT(base_uri2396.spectrum().root, 55.0);
  ASSERT_LT(base_uri2396.spectrum().root, 55.1);
  ASSERT_EQ(base_uri2396.spectrum().root, base_uri3986.spectrum().root);
  ASSERT_EQ(1.0, base_uri3986.compression_ratio(-1, base_uri2396)); // same compression ratio
}

#include <rans.hpp>
#include <gtest/gtest.h>
#include <map>
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
    d.minimize();
    ASSERT_EQ(iter->second, d.size())
        << "DFA(" << iter->first << ").size() != " << iter->second
        << "(" << d.size() << ")";
  }
}

struct testcase {
  testcase(std::string regex_, std::string text_, bool result_): regex(regex_), text(text_), result(result_) {}
  std::string regex;
  std::string text;
  bool result;
};

TEST(ELEMENTAL_TEST, DFA_IS_ACCEPTABLE) {
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
    try {
      rans::DFA d(accept_test[i].regex);
      ASSERT_EQ(accept_test[i].result, d.is_acceptable(accept_test[i].text))
          << "regex: " << accept_test[i].regex << ", text: " << accept_test[i].text;
    } catch(const char* msg) {
      std::cout << "regex: " << accept_test[i].regex
                << ", text: " << accept_test[i].text
                << std::endl << msg << std::endl;
    }
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
// Formal HTTP URL(URI) can be defined by regular expression.
// #see RFC2396 Uniform Resource Identifiers (URI): Generic Syntax
// -> http://www.ietf.org/rfc/rfc2396.txt
//
// That is below. (my handwritten, might is wrong :-p)
const std::string http_url_regex = "http://((([a-zA-Z0-9]|[a-zA-Z0-9][-a-zA-Z0-9]*[a-zA-Z0-9])\\.)*([a-zA-Z]|[a-zA-Z][-a-zA-Z0-9]*[a-zA-Z0-9])\\.?|[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+)(:[0-9]*)?(/([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*(;([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)*(/([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*(;([-_.!~*'()a-zA-Z0-9:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)*)*(\\?([-_.!~*'()a-zA-Z0-9;/?:@&=+$,]|%[0-9A-Fa-f][0-9A-Fa-f])*)?)?";

TEST(HTTP_URL_TEST, DFA_MINIMIZE) {
  rans::DFA orig(http_url_regex);
  rans::DFA mini = orig;
  mini.minimize();

  const std::size_t dfa_size_of_http_url_regex = 25;
  ASSERT_EQ(dfa_size_of_http_url_regex, mini.size());
  ASSERT_EQ(orig, mini);
}

RANS base_http(http_url_regex);
const std::string homepage_url("http://swatmac.info/");
const RANS::Value homepage_val("418454636203975692482093");

const std::string googol_url("http://A/Yq~s3G7F.3uK:M5r_swBtJW~7ZmRGX1mO:,I6lB2YT$(BZriW@&.");

TEST(HTTP_URL_TEST, RANS_VAL) {
  ASSERT_EQ(homepage_val, base_http(homepage_url));
  ASSERT_EQ(googol, base_http(googol_url));
}

TEST(HTTP_URL_TEST, RANS_REP) {
  ASSERT_EQ(homepage_url, base_http(homepage_val));
  ASSERT_EQ(googol_url, base_http(googol));
}

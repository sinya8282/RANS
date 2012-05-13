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

// Theorem(Eilenberg): The set of squares { 1, 4, 9,.., n^2, .. }
// is never recognizable in any integer base system.
// 
// But this theorem doesn't hold in ANS on L(a*b*|a*c*) !!
// -> val("a") = 1, val("aa") = 4,,, val("a"*n) = n^2
// See Prof.Michel Rigo's great publications about ANS, for more theoretical asspects.

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

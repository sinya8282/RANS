#ifndef RANS_H_
#define RANS_H_

// Copyright Ryoma Sin'ya, 2012. All Rights Reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.
//
/*
    _______          _       ____  _____   ______
    |_  __ \        / \     |_   \|_   _|.' ____ \
     | |__) |      / _ \      |   \ | |  | (___ \_|
     |  __ /      / ___ \     | |\ \| |   \_____`.
    _| |  \ \_  _/ /   \ \_  _| |_\   |_ | \____) |
   |____| |___||____| |____||_____|\____| \______.'

*/
// RANS - Implementation of Abstract-Numeration-System(ANS) on a regular language.
// This header file consists of mainly three parts described below.
//
// rans::DFA (and Parse) is simple DFA implementation.
// Parse recognizes simplified extended regular expression.
//
// rans::MPMatrix (and MPVector) is Multi-Precision Integer Matrix and Vector
// implementation. it's necessary to calculate both value and representation
// on ANS. I implemented ANS by using a adjacency matrix of DFA.
//
// rans::RANS, which has essential function val() and rep(), is main class.
// - val(text) returns a number N that means 'given text is Nth acceptable string
//   in regular languages'
// - rep(value), which is inverse function of val(text), returns a correspoinding
//   string.
// examples: rans::RANS r1("(ab)*"); rans::RANS r2("a*(b*|c*)");
//           r1.rep(3)=="ababab"; r1.val("")==0; r1.val("ab")==1;
//           r2.rep(1)=="a"; r2.rep(4)=="aa"; r2.val("aaa")==9;
//
// This class would be exported (using rans::RANS), so users can use simply
// just like: 'RANS r(regex); RANS::Value value = r(text);'.
//
// To see more detail and usage, you could check RANS/test/rans.cc, which is
// RANS simple program, and RANS/test/test.cc, which contains some theortical
// notes. and project page of RANS - http://sinya8282.github.com/RANS
//
// Also, you can get more interesting, thoretical aspects by Berthé and Rigo's
// great book - "Combinatorics, Automata and Number Theory".

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <bitset>
#include <deque>
#include <queue>
#include <set>
#include <map>
#include <algorithm>
#include <exception>

// External libraries: gmp(gmpxx), (optional: gflags, Boost.uBLAS)
#include <gmpxx.h>

#ifdef DEFINE_bool // if gflags is enabled
DEFINE_bool(dump_expr, false, "dump Expr-tree.");
DEFINE_bool(dump_dfa, false, "dump DFA as dot language.");
DEFINE_bool(dump_matrix, false, "dump Matrix.");
DEFINE_bool(dump_exmatrix, false, "dump Extended Matrix.");
#else
#define FLAGS_dump_expr false
#define FLAGS_dump_dfa false
#define FLAGS_dump_matrix false
#define FLAGS_dump_exmatrix false
#endif

namespace rans {

const std::string SYNTAX = 
"RANS \"simplified\" extended regular expression syntax:      \n"
"  regex      ::= union* EOP                                  \n"
"  union      ::= concat ('|' concat)*                        \n"
"  concat     ::= repetition+                                 \n"
"  repetition ::= atom quantifier*                            \n"
"  quantifier ::= [*+?] | '{' (\\d+ | \\d* ',' \\d* ) '}'     \n"
"  atom       ::= literal | dot | charclass | '(' union ')'   \n"
"                 utf8char # optional (--utf8)                \n"
"  charclass  ::= '[' ']'? [^]]* ']'                          \n"
"  literal    ::= [^*+?[\\]|]                                 \n"
"  dot        ::= '.' # NOTE: dot matchs also newline('\\n')  \n"
"  utf8char   ::= [\\x00-\\x7f] | [\\xC0-\\xDF][\\x80-\\xBF]  \n"
"               | [\\xE0-\\xEF][\\x80-\\xBF]{2}               \n"
"               | [\\xF0-\\xF7][\\x80-\\xBF]{3}               \n";

enum Encoding {
  ASCII = 0, // default
  UTF8 = 1
};

std::size_t utf8_byte_length(const unsigned char c)
{
  static const std::size_t len[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    4,4,4,4,4,4,4,4,
    0,0,0,0,0,0,0,0
  };

  return len[c];
}

bool is_valid_utf8_sequence(const unsigned char *s)
{
  std::size_t len = utf8_byte_length(*s);
  if (len == 0) return false;

  for (std::size_t i = 1; i < len; i++) {
    if (!(0x80 <= *(s+i) && *(s+i) <= 0xBF)) return false;
  }

  return true;
}

class Parser {
 public:
  enum ExprType {
    kLiteral = 0, kDot, kCharClass, kUTF8,
    kConcat, kUnion,
    kStar, kPlus, kRepetition, kQmark,
    kEOP, kLpar, kRpar, kByteRange, kEpsilon,
    kBadExpr
  };
  struct Expr {
    Expr() { type = kEpsilon; lhs = rhs = 0; }
    Expr(ExprType t, Expr* lhs = NULL, Expr* rhs = NULL) { init(t, lhs, rhs); }
    void init(ExprType, Expr*, Expr*);
    void set_union(std::set<Expr*> &src1, std::set<Expr*> &src2, std::set<Expr*> &dst)
    { dst.insert(src1.begin(), src1.end()); dst.insert(src2.begin(), src2.end()); }
    const char* type_name();

    // fiesds
    ExprType type;
    bool nullable;
    unsigned char literal;
    std::bitset<256> cc_table;
    Expr* lhs;
    Expr* rhs;
    std::size_t id;
    std::set<Expr*> follow;
    std::set<Expr*> first;
    std::set<Expr*> last;
    void dump(std::size_t tab);
    friend std::ostream& operator<<(std::ostream&, Expr&);
  };
  
  Parser(const std::string &, Encoding);
  Expr* expr_tree() { return _expr_root; }
  Expr* expr(std::size_t);
  Expr* new_expr(ExprType, Expr*, Expr*);
  Expr* clone_expr(Expr*);
  bool ok() const { return _ok; }
  const std::string& error() const { return _error; }
 private:
  ExprType consume();
  ExprType lex();
  std::size_t offset() const { return _regex_end - _regex_ptr; }
  unsigned char consume_char();
  int consume_int();
  bool lex_is_int() { return '0' <= lex_char() && lex_char() <= '9'; }
  unsigned char lex_char();
  bool regex_empty() { return _regex_ptr == _regex_end; }
  bool lex_is_atom();
  bool lex_is_quantifier();
  ExprType consume_repetition();
  ExprType consume_metachar();

  void parse();
  Expr* parse_union();
  Expr* parse_concat();
  Expr* parse_repetition();
  Expr* parse_atom();
  Expr* parse_charclass();

  void fill_transition(Expr *);
  void connect(std::set<Expr*>, std::set<Expr*>);

  // fields
  static const int repeat_infinitely = -1;
  bool _ok;
  std::string _error;
  std::string _regex;
  Encoding _encoding;
  const unsigned char* _regex_begin;
  const unsigned char* _regex_end;
  const unsigned char* _regex_ptr;
  std::deque<Expr> _expr_tree;
  Expr* _expr_root;
  std::bitset<256> _cc_table;
  unsigned char _literal;
  int _repeat_min, _repeat_max;
  ExprType _token;
};

Parser::Expr* Parser::expr(std::size_t index)
{
  return index < _expr_tree.size() ? &_expr_tree[index] : NULL;
}

void Parser::Expr::init(ExprType t, Expr* lhs_ = NULL, Expr* rhs_ = NULL)
{
  type = t;
  lhs = lhs_;
  rhs = rhs_;

  switch (type) {
    case kLiteral: case kDot: case kCharClass: case kEOP: {
      nullable = false;
      first.insert(this);
      last.insert(this);
      break;
    }
    case kUnion: {
      nullable = lhs->nullable || rhs->nullable;
      set_union(lhs->first, rhs->first, first);
      set_union(lhs->last, rhs->last, last);
      break;
    }
    case kConcat: {
      nullable = lhs->nullable && rhs->nullable;
      if (lhs->nullable) set_union(lhs->first, rhs->first, first);
      else first = lhs->first;
      if (rhs->nullable) set_union(lhs->last, rhs->last, last);
      else last = rhs->last;
      break;
    }
    case kStar: case kQmark: case kPlus: {
      nullable = (type == kPlus || type == kEOP) ? lhs->nullable : true;
      first = lhs->first; last = lhs->last;
      break;
    }
    case kEpsilon: nullable = true; break;
    default: throw "can't handle the type";
  }
}

std::ostream& operator<<(std::ostream& stream, const std::set<Parser::Expr*>& expr_set)
{
  for (std::set<Parser::Expr*>::iterator iter = expr_set.begin();
       iter != expr_set.end(); ++iter) {
    stream << (*iter)->id << ", ";
  }
  return stream;
}

void dump(const std::set<Parser::Expr*> &expr_set)
{
  std::cout << expr_set << std::endl;
}

void Parser::Expr::dump(std::size_t tab = 0)
{
  static std::string tabs = "    ";
  for (std::size_t i = 0; i < tab; i++) std::cout << tabs;
  switch (type) {
    case Parser::kUnion: case Parser::kConcat: {
      std::cout << *this << std::endl;
      lhs->dump(tab + 1);
      rhs->dump(tab + 1);
      break;
    }
    case Parser::kStar: case Parser::kPlus: case Parser::kQmark: {
      std::cout << *this << std::endl;
      lhs->dump(tab + 1);
      break;
    }
    default: std::cout << *this << std::endl;
  }
}

std::ostream& operator<<(std::ostream& stream, Parser::Expr& expr)
{
  stream << expr.type_name() << ": " << "id = " << expr.id
         << ", nullable = " << expr.nullable << ", literal = "
         << static_cast<int>(expr.literal) << ", #cc_table = " << expr.cc_table.count()
         << ", follow = " << expr.follow.size();
  return stream;
}

const char* Parser::Expr::type_name()
{
  static const char* type_name_[] = {
    "Literal", "Dot", "CharClass", "UTF8",
    "Concat", "Union", 
    "kStar", "kPlus", "kRepetition", "kQmark",
    "kEOP", "kLpar", "kRpar", "kByteRange", "kEpsilon",
    "kBadExpr"
  };

  return type_name_[type];
}

Parser::Expr* Parser::new_expr(ExprType t = kEpsilon, Expr* lhs = NULL, Expr* rhs = NULL)
{
  _expr_tree.resize(_expr_tree.size() + 1);
  _expr_tree.back().init(t, lhs, rhs);
  _expr_tree.back().id = _expr_tree.size() - 1;
  return &_expr_tree.back();
}

Parser::Expr* Parser::clone_expr(Expr* orig)
{
  if (orig == NULL) return NULL;

  Expr* clone = new_expr(orig->type, clone_expr(orig->lhs), clone_expr(orig->rhs));

  switch (orig->type) {
    case kLiteral:
      clone->literal = orig->literal;
      break;
    case kCharClass:
      clone->cc_table = orig->cc_table;
      break;
    default: break;
  }

  return clone;
}

Parser::Parser(const std::string& regex, Encoding enc): _ok(true), _regex(regex), _encoding(enc)
{
  _regex_begin = _regex_ptr = reinterpret_cast<const unsigned char*>(_regex.data());
  _regex_end = reinterpret_cast<const unsigned char*>(_regex.data()) + _regex.length();
  try {
    parse();
  } catch (const char* error) {
    _ok = false;
    _error = "regular expression parse error: ";
    _error += error;
  }
}

Parser::ExprType Parser::consume()
{
  if (regex_empty()) {
    _literal = '\0';
    _token = kEOP;
    return _token;
  }

  switch (_literal = lex_char()) {
    case '[': _token = kCharClass; break;
    case '.': _token = kDot;   break;
    case '|': _token = kUnion; break;
    case '?': _token = kQmark; break;
    case '+': _token = kPlus;  break;
    case '*': _token = kStar;  break;
    case '(': _token = kLpar;  break;
    case ')': _token = kRpar;  break;
    case '{': consume_char(); _token = consume_repetition(); break;
    case '\\': consume_char(); _token = consume_metachar();  break;
    default:
      if (_encoding == UTF8 && utf8_byte_length(_literal) != 1) {
        if (!is_valid_utf8_sequence(_regex_ptr)) throw "invalid utf8 sequence";
        _token = kUTF8;
        return _token;
      } else {
        _token = kLiteral;
      }
      break;
  }

  consume_char();
  
  return _token;
}

Parser::ExprType Parser::lex()
{
  return _token;
}

unsigned char Parser::consume_char()
{
  if (!regex_empty()) _regex_ptr++;

  return lex_char();
}

unsigned char Parser::lex_char()
{
  return *_regex_ptr;
}

bool Parser::lex_is_atom()
{
  switch (lex()) {
    case kLiteral: case kCharClass: case kDot:
    case kByteRange: case kEpsilon: case kLpar:
    case kUTF8:
      return true;
    default: return false;
  }
}

bool Parser::lex_is_quantifier()
{
  switch (lex()) {
    case kStar: case kPlus: case kQmark:
    case kRepetition:
      return true;
    default: return false;
  }
}

int Parser::consume_int()
{
  int val = 0;
  
  while (lex_is_int()) {
    val *= 10;
    val += lex_char() - '0';
    consume_char();
  }

  return val;
}

Parser::ExprType Parser::consume_repetition()
{
  ExprType token;

  if (lex_char() == '}') throw "bad repetition";

  _repeat_min = _repeat_max = consume_int();
  if (lex_char() == ',') {
    if (consume_char() == '}') {
      _repeat_max = repeat_infinitely;
    } else {
      _repeat_max = consume_int();
    }
  }

  if ((_repeat_max != repeat_infinitely && (_repeat_min > _repeat_max))
      || lex_char() != '}') throw "bad repetition";

  if (_repeat_min == 0 && _repeat_max == repeat_infinitely) token = kStar;
  else if (_repeat_min == 1 && _repeat_max == repeat_infinitely) token = kPlus;
  else if (_repeat_min == 0 && _repeat_max == repeat_infinitely) token = kQmark;
  else token = kRepetition;

  return token;
}

Parser::ExprType Parser::consume_metachar()
{
  ExprType token;

  switch (lex_char()) {
    case '\0': throw "bad '\\'";
    case 'a': /* bell */
      _literal = '\a';
      token = kLiteral;
      break;
    case 'd': /*digits /[0-9]/  */
    case 'D': // or not
      _cc_table.reset();
      for (unsigned char c = '0'; c <= '9'; c++) _cc_table.set(c);
      if (_literal == 'D') _cc_table.flip();
      token = kByteRange;
      break;
    case 'f': /* form feed */
      _literal = '\f';
      token = kLiteral;
      break;
    case 'n': /* new line */
      _literal = '\n';
      token = kLiteral;
      break;
    case 'r': /* carriage retur */
      _literal = '\r';
      token = kLiteral;
      break;
    case 's': /* whitespace [\t\n\f\r ] */
    case 'S': // or not
      _cc_table.reset();
      _cc_table.set('\t'); _cc_table.set('\n'); _cc_table.set('\f');
      _cc_table.set('\r'); _cc_table.set(' ');
      if (_literal == 'S') _cc_table.flip();
      token = kByteRange;
      break;
    case 't': /* horizontal tab */
      _literal = '\t';
      token = kLiteral;
      break;
    case 'v': /* vertical tab */
      _literal = '\v';
      token = kLiteral;
      break;
    case 'w': /* word characters [0-9A-Za-z_] */
    case 'W': // or not
      _cc_table.reset();
      for (unsigned char c = '0'; c <= '9'; c++) _cc_table.set(c);
      for (unsigned char c = 'A'; c <= 'Z'; c++) _cc_table.set(c);
      for (unsigned char c = 'a'; c <= 'z'; c++) _cc_table.set(c);
      _cc_table.set('_');
      if (_literal == 'W') _cc_table.flip();
      token = kByteRange;
      break;
    case 'x': {
      unsigned char hex = 0;
      for (int i = 0; i < 2; i++) {
        _literal = consume_char();
        hex <<= 4;
        if ('A' <= _literal && _literal <= 'F') {
          hex += _literal - 'A' + 10;
        } else if ('a' <= _literal && _literal <= 'f') {
          hex += _literal - 'a' + 10;
        } else if ('0' <= _literal && _literal <= '9') {
          hex += _literal - '0';
        } else {
          if (i == 0) {
            hex = 0;
          } else {
            hex >>= 4;
          }
          _regex_ptr--;
          break;
        }
      }
      token = kLiteral;
      _literal = hex;
      break;
    }
    default:
      token = kLiteral;
      _literal = lex_char();
      break;
  }

  return token;
}

void Parser::parse()
{
  consume();

  if (lex() == kEOP) {
    _expr_root = new_expr(kEOP);
  } else {
    Expr* expr = parse_union();
    if (lex() != kEOP) throw "bad EOP";
    Expr* eop = new_expr(kEOP);
    _expr_root = new_expr(kConcat, expr, eop);
  }

  fill_transition(_expr_root);

  if (FLAGS_dump_expr) _expr_root->dump();
}

Parser::Expr* Parser::parse_union()
{
  Expr* e = parse_concat();

  while (lex() == kUnion) {
    consume();
    Expr* f = parse_concat();
    Expr* g = new_expr(kUnion, e, f);
    e = g;
  }

  return e;
}

Parser::Expr* Parser::parse_concat()
{
  Expr* e = parse_repetition();

  while (lex_is_atom()) {
    Expr* f = parse_repetition();
    Expr* g = new_expr(kConcat, e, f);
    e = g;
  }

  return e;
}

Parser::Expr* Parser::parse_repetition()
{
  Expr* e = parse_atom();

  while (lex_is_quantifier()) {
    switch (lex()) {
      case kStar: case kPlus: case kQmark: {
        Expr* f = new_expr(lex(), e);
        e = f;
        break;
      }
      case kRepetition: {
        Expr* orig = e;
        if (_repeat_min == 0) {
          if (_repeat_max == 0) {
            e = new_expr(kEpsilon, e);
          } else {
            e = new_expr(kQmark, e);
            _repeat_min = 1;
          }
        }
        for (int i = 1; i < _repeat_min; i++) {
          Expr* f = clone_expr(orig);
          e = new_expr(kConcat, e, f);
        }
        if (_repeat_max == repeat_infinitely) {
          Expr* f = clone_expr(orig);
          f = new_expr(kStar, f);
          e = new_expr(kConcat, e, f);          
        } else {
          for (int i = _repeat_min; i < _repeat_max; i++) {
            Expr* f = clone_expr(orig);
            f = new_expr(kQmark, f);
            e = new_expr(kConcat, e, f);
          }
        }
        break;
      }
      default: throw "can't handle the type";
    }
    consume();
  }

  return e;
}

Parser::Expr* Parser::parse_atom()
{
  Expr* e; 

  switch (lex()) {
    case kLiteral: {
      e = new_expr(kLiteral);
      e->literal = _literal;
      break;
    }
    case kCharClass: {
      e = parse_charclass();
      break;
    }
    case kDot: {
      e = new_expr(kDot);
      break;
    }
    case kByteRange: {
      e = new_expr(kCharClass);
      e->cc_table = _cc_table;
      break;
    }
    case kEpsilon: {
      e = new_expr(kEpsilon);
      break;
    }
    case kUTF8: {
      unsigned char top = _literal;
      e = new_expr(kLiteral);
      e->literal = top;
      for (std::size_t i = 1; i < utf8_byte_length(top); i++) {
        Expr* f = new_expr(kLiteral);
        f->literal = consume_char();
        Expr* g = new_expr(kConcat, e, f);
        e = g;
      }
      consume_char();
      break;
    }
    case kLpar: {
      consume();
      e = parse_union();
      if (lex() != kRpar) throw "bad parentheses";
      break;
    }
    default: throw "can't handle the type ";
  }

  consume();
  
  return e;
}

Parser::Expr* Parser::parse_charclass()
{
  Expr* cc = new_expr(kCharClass);
  bool range = false;
  bool negative = false;
  unsigned char last = '\0';

  consume();
  
  if (_literal == '^') {
    consume();
    negative = true;
  }
  if (_literal == '-' ||
      _literal == ']') {
    cc->cc_table.set(_literal);
    last = _literal;
    consume();
  }

  for (; lex() != kEOP && _literal != ']'; consume()) {
    if (!range && _literal == '-') {
      range = true;
      continue;
    }

    if (lex() == kByteRange) cc->cc_table |= _cc_table;
    else cc->cc_table.set(_literal);

    if (range) {
      for (std::size_t c = last; c <= _literal; c++) cc->cc_table.set(c);
      range = false;
    }

    last = _literal;
  }

  if (lex() == kEOP) throw "invalid character class";
  if (range) cc->cc_table.set('-');
  if (negative) cc->cc_table.flip();
  if (cc->cc_table.count() == 1) {
    cc->type = kLiteral;
    for (std::size_t c = 0; c < 256; c++) {
      if (cc->cc_table[c]) {
        cc->literal = c;
        break;
      }
    }
    cc->cc_table.reset();
  }

  return cc;
}

void Parser::fill_transition(Expr *expr)
{
  switch (expr->type) {
    case kLiteral: case kDot: case kCharClass:
    case kEOP: case kEpsilon:
      break;
    case kConcat:
      connect(expr->lhs->last, expr->rhs->first);
      // fall through
    case kUnion:
      fill_transition(expr->lhs); fill_transition(expr->rhs);
      break;
    case kStar: case kPlus:
      connect(expr->lhs->last, expr->lhs->first);
      // fall through
    case kQmark:
      fill_transition(expr->lhs);
      break;
    default: throw "can't handle the type";
  }
}

void Parser::connect(std::set<Expr*> src, std::set<Expr*> dst)
{
  for (std::set<Expr*>::iterator iter = src.begin(); iter != src.end(); ++iter) {
    (*iter)->follow.insert(dst.begin(), dst.end());
  }
}

class DFA {
 public:
  enum State_t { REJECT = -1, START = 0 };
  typedef std::set<Parser::Expr*> Subset;
  struct State {
    int t[256];
    int id;
    bool accept;
    int operator[](std::size_t i) const { return t[i]; }
    int& operator[](std::size_t i) { return t[i]; }
  };
  DFA(const std::string&, Encoding, bool);
  bool ok() const { return _ok; }
  const std::string& error() const { return _error; }
  std::size_t size() const { return _states.size(); }
  const State& state(std::size_t i) const { return _states[i]; }
  State& state(std::size_t i) { return _states[i]; }
  bool accept(int state) const { return state != REJECT && _states[state].accept; }
  bool accept(const std::string&) const;
  const State& operator[](std::size_t i) const { return _states[i]; }
  State& operator[](std::size_t i) { return _states[i]; }
  void minimize();
  bool operator==(const DFA&) const;
  friend std::ostream& operator<<(std::ostream& stream, const DFA& dfa);
 private:
  void construct(Parser::Expr* expr);
  void fill_transition(Parser::Expr*, std::vector<Subset>&);
  State& new_state();
  static std::string& pretty(unsigned char, std::string &);

  //fields
  bool _ok;
  std::string _error;
  std::deque<State> _states;
};

bool DFA::operator==(const DFA& lhs) const
{
  if (state(START).accept != lhs.state(START).accept) return false;
  
  std::set<std::pair<int, int> > equivalent_states;
  equivalent_states.insert(std::pair<int, int>(REJECT, REJECT));
  equivalent_states.insert(std::pair<int, int>(START, START));
  std::queue<std::pair<int, int> > queue;
  queue.push(std::pair<int, int>(START, START));

  while (!queue.empty()) {
    std::pair<int, int> pair = queue.front();
    queue.pop();

    const State& s1 = state(pair.first);
    const State& s2 = lhs.state(pair.second);
    if (s1.accept != s2.accept) return false;

    for (std::size_t c = 0; c < 256; c++) {
      std::pair<int, int> next(s1[c], s2[c]);

      if (equivalent_states.find(next) == equivalent_states.end()) {
        if (next.first == REJECT || next.second == REJECT) return false;

        equivalent_states.insert(next);
        queue.push(next);
      }
    }
  }
  
  return true;
}

std::ostream& operator<<(std::ostream& stream, const DFA& dfa)
{
  static const char* const state_circle = "circle";
  static const char* const accept_circle = "doublecircle";
  static const char* const style  = "fillcolor=lightsteelblue1, style=filled, color = navyblue ";

  stream << "digraph DFA {\n  rankdir=\"LR\"" << std::endl;
  for (std::size_t i = 0; i < dfa.size(); i++) {
    stream << "  " << i << " [shape= " << (dfa[i].accept ? accept_circle : state_circle)
           << ", " << style << "]" << std::endl;
  }
  stream << "  start [shape=point]\n  start -> " << DFA::START << std::endl;

  std::string label;
  for (std::size_t i = 0; i < dfa.size(); i++) {
    bool range = false; unsigned char last;
    for (unsigned int c = 0; c < 256; c++) {
      if (dfa[i][c] != DFA::REJECT) {
        if (range && !(c + 1 < 256 && dfa[i][c] == dfa[i][c+1])) {
          stream << DFA::pretty(c, label) << "]\"]" << std::endl;
          range = false;
        } else if (c + 1 < 256 && dfa[i][c] == dfa[i][c+1]) {
          if (range) continue;
          range = true;
          last = c;
          stream << "  " << i << " -> " << dfa[i][c]
                 << " [label=\"[" << DFA::pretty(c, label) << "-";
        } else {
          stream << "  " << i << " -> " << dfa[i][c]
                 << " [label=\"" << DFA::pretty(c, label) << "\"]" << std::endl;
        }
      }
    }
  }
  stream << "}" << std::endl;
  
  return stream;
}

std::string& rans::DFA::pretty(unsigned char c, std::string &label)
{
  std::stringstream label_;
  if (' ' <= c && c <= '~') {
    if (c == '"' || c == '\\') label_ << '\\';
    label_ << c;
  } else {
    label_ << std::showbase << std::hex << static_cast<unsigned int>(c);
  }

  label_ >> label;
  return label;
}

DFA::DFA(const std::string &regex, Encoding enc = ASCII, bool do_minimize = true): _ok(true)
{
  Parser p(regex, enc);
  if (!p.ok()) {
    _ok = false;
    _error = p.error();
    return;
  }

  try {
    construct(p.expr_tree());
  } catch (const char* error) {
    _ok = false;
    _error = "dfa construct error: ";
    _error += error;
    return;
   }

  try {
    if (do_minimize) minimize();
  } catch (const char* error) {
    _ok = false;
    _error = "dfa minimize error: ";
    _error += error;
  }
}

void DFA::construct(Parser::Expr* expr_tree)
{
  int state_num = 0;
  std::vector<Subset> transition(256);
  std::queue<Subset> queue;
  queue.push(expr_tree->first);
  std::map<Subset, int> subset_to_state;
  subset_to_state[expr_tree->first] = state_num++;

  while (!queue.empty()) {
    bool accept = false;
    Subset &subset = queue.front();
    std::fill(transition.begin(), transition.end(), Subset());

    for (Subset::iterator iter = subset.begin(); iter != subset.end(); ++iter) {
      accept |= (*iter)->type == Parser::kEOP;
      fill_transition(*iter, transition);
    }
    queue.pop();

    State& state = new_state();
    state.accept = accept;

    for (std::size_t c = 0; c < 256; c++) {
      Subset& next = transition[c];

      if (next.empty()) continue;

      if (subset_to_state.find(next) == subset_to_state.end()) {
        subset_to_state[next] = state_num++;
        queue.push(next);
      }

      state[c] = subset_to_state[next];
    }
  }
}

void DFA::fill_transition(Parser::Expr* expr, std::vector<DFA::Subset>& transition)
{
  switch (expr->type) {
    case Parser::kLiteral: {
      unsigned char index = expr->literal;
      transition[index].insert(expr->follow.begin(), expr->follow.end());
      break;
    }
    case Parser::kCharClass: {
      for (std::size_t c = 0; c < 256; c++) {
        if (expr->cc_table[c]) {
          transition[c].insert(expr->follow.begin(), expr->follow.end());
        }
      }
      break;
    }
    case Parser::kDot: {
      for (std::size_t c = 0; c < 256; c++) {
        transition[c].insert(expr->follow.begin(), expr->follow.end());
      }
      break;
    }
    case Parser::kEOP: break;
    default: throw "can't handle the type";
  }
}

DFA::State& DFA::new_state()
{
  _states.resize(_states.size() + 1);
  State& state = _states.back();
  std::fill(state.t, state.t+256, REJECT);
  state.id = _states.size() - 1;
  return state;
}

void DFA::minimize()
{
  std::vector<std::vector<bool> > distinction_table;
  distinction_table.resize(size()-1);
  for (std::size_t i = 0; i < size()-1; i++) {
    distinction_table[i].resize(size()-i-1);
    for (std::size_t j = i+1; j < size(); j++) {
      distinction_table[i][size()-j-1] = _states[i].accept != _states[j].accept;
    }
  }

  bool distinction_flag = true;
  while (distinction_flag) {
    distinction_flag = false;
    for (std::size_t i = 0; i < size()-1; i++) {
      for (std::size_t j = i+1; j < size(); j++) {
        if (!distinction_table[i][size()-j-1]) {
          for (std::size_t input = 0; input < 256; input++) {
            int n1, n2;
            n1 = _states[i][input];
            n2 = _states[j][input];
            if (n1 != n2) {
              if (n1 > n2) std::swap(n1, n2);
              if ((n1 == REJECT || n2 == REJECT) ||
                  distinction_table[n1][size()-n2-1]) {
                distinction_flag = true;
                distinction_table[i][size()-j-1] = true;
                break;
              }
            }
          }
        }
      }
    }
  }

  std::map<int, int> swap_map;
  for (std::size_t i = 0; i < size()-1; i++) {
    for (std::size_t j = i+1; j < size(); j++) {
      if (swap_map.find(j) == swap_map.end()) {
        if (!distinction_table[i][size()-j-1]) {
          swap_map[j] = i;
        }
      }
    }
  }

  if (swap_map.empty()) return;

  std::size_t minimum_size = size() - swap_map.size();
  std::vector<std::size_t> replace_map(size());
  for (std::size_t s = 0, d = 0; s < size(); s++) {
    if (swap_map.find(s) == swap_map.end()) {
      replace_map[s] = d++;
      if (s != replace_map[s]) {
        _states[replace_map[s]] = _states[s];
        _states[replace_map[s]].id = replace_map[s];
      }
    } else {
      replace_map[s] = replace_map[swap_map[s]];
    }
  }

  for (std::size_t i = 0; _states[i].id < static_cast<int>(minimum_size); i++) {
    for (std::size_t input = 0; input < 256; input++) {
      int n = _states[i][input];
      if (n != REJECT) _states[i][input] = replace_map[n];
    }
  }

  _states.resize(minimum_size);
}

bool DFA::accept(const std::string& text) const
{
  int state = START;
  for (std::size_t i = 0; i < text.length(); i++) {
    state = _states[state][static_cast<unsigned char>(text[i])];
    if (state == REJECT) return false;
  }

  return accept(state);
}

// TODO: advanced optimization for Power of Matrix.
// Does anyone know good (multi-precision) matrix library?

typedef mpz_class Value;

#ifdef RANS_USE_UBLAS
// You can choose uBLAS as Matrix/Vector classes, but
// it seems slower (both compilation time and peformance) than
// self-implementation. (I'm not sure how to use uBLAS effectively,,,)
// Therefore RANS_USE_UBLAS macro is *UNDEFINED* as a default in Makefile.
} // escape namespace rans
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/operation.hpp>
namespace rans { // re-enter namespace rans
typedef boost::numeric::ublas::matrix<Value> MPMatrix;
typedef boost::numeric::ublas::identity_matrix<Value> MPIdentityMatrix;
typedef boost::numeric::ublas::vector<Value> MPVector;
typedef boost::numeric::ublas::zero_vector<Value> MPZeroVector;

MPMatrix& prod(MPMatrix& X, const MPMatrix& Y)
{
  return X = boost::numeric::ublas::prod(X, Y);
}

MPMatrix& prod(const MPMatrix& X, const MPMatrix& Y, MPMatrix& Z)
{
  return Z = boost::numeric::ublas::prod(X, Y);
}

MPVector& prod(MPVector& V, const MPMatrix& X)
{
  return V = boost::numeric::ublas::prod(V, X);
}

MPVector& prod(const MPVector& V, const MPMatrix& X, MPVector& W)
{
  return W = boost::numeric::ublas::prod(V, X);
}

Value& inner_prod(const MPVector& V, const MPVector& W, Value& v)
{
  return v = boost::numeric::ublas::inner_prod(V, W);
}

#else
class MPMatrix {
 public:
  MPMatrix(std::size_t i = 0): _size(i), m(_size*_size) {}
  MPMatrix(std::size_t row, std::size_t col): _size(row), m(_size*_size) {}
  MPMatrix(const MPMatrix &M) { *this = M; }
  void resize(std::size_t n) { resize(n, n); }
  void resize(std::size_t row, std::size_t col) { _size = row; m.resize(_size*_size); }
  friend std::ostream& operator<<(std::ostream& stream, const MPMatrix& matrix);
  std::size_t size() const { return _size; }
  std::size_t size1() const { return size(); } // alias
  void clear();
  void init();
  void swap(MPMatrix &M) { m.swap(M.m); }
  Value& operator()(std::size_t i, std::size_t j) { return m[i*_size+j]; }
  const Value& operator()(std::size_t i, std::size_t j) const { return m[i*_size+j]; }
  MPMatrix& operator*=(const MPMatrix&);
 private:
  // fields
  std::size_t _size;
  std::vector<Value> m;
};

class MPIdentityMatrix: public MPMatrix {
 public:
  MPIdentityMatrix(std::size_t n): MPMatrix(n, n)
  {
    for (std::size_t i = 0; i < n; i++) (*this)(i, i) = 1;
  }
};

MPMatrix& MPMatrix::operator*=(const MPMatrix &M)
{
  MPMatrix tmp(size(), size());
  for (std::size_t i = 0; i < size(); i++) {
    for (std::size_t j = 0; j < size(); j++) {
      for (std::size_t k = 0; k < size(); k++) {
        tmp(i, j) += (*this)(i, k) * M(k, j);
      }
    }
  }
  swap(tmp);
  return *this;
}

void MPMatrix::clear()
{
  for (std::size_t i = 0; i < size(); i++) {
    for (std::size_t j = 0; j < size(); j++) {
      (*this)(i, j) = 0;
    }
  }
}

void MPMatrix::init()
{
  for (std::size_t i = 0; i < size(); i++) {
    for (std::size_t j = 0; j < size(); j++) {
      (*this)(i, j) = i == j ? 1 : 0;
    }
  }
}

std::ostream& operator<<(std::ostream& stream, const MPMatrix& matrix)
{
  for (std::size_t i = 0; i < matrix.size(); i++) {
    stream << "{";
    for (std::size_t j = 0; j < matrix.size(); j++) {
      stream << matrix(i, j) << ", ";
    }
    stream << "}," << std::endl;
  }
  return stream;
}

class MPVector {
 public:
  MPVector(std::size_t size = 0): _v(size) {}
  friend std::ostream& operator<<(std::ostream& stream, const MPMatrix& matrix);  
  void resize(std::size_t size) { _v.resize(size); }
  std::size_t size() const { return _v.size(); }
  Value& operator[](std::size_t i) { return _v[i]; }
  const Value& operator[](std::size_t i) const { return _v[i]; }
  void clear() { for (std::size_t i = 0; i < size(); i++) _v[i] = 0; }
  MPVector& operator*=(const MPMatrix &X);
 private:
  // fields
  std::vector<Value> _v;
};

MPVector& MPVector::operator*=(const MPMatrix &X)
{
  std::vector<Value> v(size());
  for (std::size_t i = 0; i < size(); i++) {
    for (std::size_t j = 0; j < size(); j++) {
      v[i] += _v[j] * X(j, i);
    }
  }
  v.swap(_v);
  
  return *this;
}

std::ostream& operator<<(std::ostream& stream, const MPVector& vector)
{
  stream << "{";
  for (std::size_t i = 0; i < vector.size(); i++) {
    stream << vector[i] << ", ";
  }
  stream << "}" << std::endl;
  return stream;
}

MPMatrix& prod(MPMatrix& X, const MPMatrix& Y)
{
  return X *= Y;
}

MPMatrix& prod(const MPMatrix& X, const MPMatrix& Y, MPMatrix& Z)
{
  for (std::size_t i = 0; i < X.size(); i++) {
    for (std::size_t j = 0; j < X.size(); j++) {
      Z(i, j) = 0;
      for (std::size_t k = 0; k < X.size(); k++) {
        Z(i, j) += X(i, k) * Y(k, j);
      }
    }
  }

  return Z;
}

MPVector& prod(MPVector& V, const MPMatrix& Y)
{
  return V *= Y;
}

MPVector& prod(const MPVector& V, const MPMatrix& X, MPVector& W)
{
  for (std::size_t i = 0; i < V.size(); i++) {
    W[i] = 0;
    for (std::size_t j = 0; j < X.size(); j++) {
      W[i] += V[j] * X(j, i);
    }
  }
  return W;
}

Value& inner_prod(const MPVector& V, const MPVector& W, Value& v)
{
  for (std::size_t i = 0; i < V.size(); i++) v += V[i] * W[i];
  return v;
}
#endif // RANS_USE_UBLAS

// returns Y = X^n, with O(|X|^3 log n)-s factor-wise multiplications.
// TODO: It seems more optimizable using linear algebraic techniques.
MPMatrix& power(const MPMatrix& X, std::size_t n, MPMatrix& Y)
{
  if (n == 0) Y = MPIdentityMatrix(X.size1());
  else Y = X;

  std::size_t i = 1;
  while ((i *= 2) <= n) Y = prod(Y, Y);
  for (i /= 2; i < n; i++) Y = prod(Y, X);

  return Y;
}

class RANS {
 public:
  class Exception: public std::out_of_range {
   public:
    Exception(const std::string& error): std::out_of_range(error) {}
  };
  enum Encoding { ASCII = 0, UTF8 = 1 };
  typedef rans::Value Value;
  RANS(const std::string&, Encoding);
  bool ok() const { return _ok; }
  const std::string& error() const { return _error; }
  bool accept(const std::string& text) const { return _dfa.accept(text); }
  Value& val(const std::string&, Value&) const;
  Value val(const std::string& text) const { Value value; return val(text, value); }
  std::string& rep(const Value&, std::string &) const;
  std::string rep(const Value& value) const { std::string text; return rep(value, text); }
  const DFA& dfa() const { return _dfa; }
  std::size_t size() const { return _dfa.size(); }
  Value amount() const;
  bool finite() const { return amount() != -1; }
  bool infinite() const { return !finite(); }
  Value amount(std::size_t length) const { return count(length, true); }
  Value count(std::size_t length) const { return count(length, false); }
  // useful aliases for val & rep
  Value& operator()(const std::string& text, Value& value) const { return val(text, value); }
  Value operator()(const std::string& text) const { return val(text); }
  std::string& operator()(const Value& value, std::string& text) const { return rep(value, text); }
  std::string operator()(const Value& value) const { return rep(value); }
  std::string compress(const std::string& text) const { std::string dst; return compress(text, dst); }
  std::string decompress(const std::string& text) const { std::string dst; return decompress(text, dst); }
  std::string& compress(const std::string&, std::string&) const;
  std::string& decompress(const std::string&, std::string&) const;
  static const RANS baseBYTE;
 private:
  //DISALLOW COPY AND ASSIGN
  RANS(const RANS&);
  void operator=(const RANS&);
  int floor(Value&) const;
  Value count(std::size_t length, bool amount) const;
  // fields
  bool _ok;
  std::string _error;
  DFA _dfa;
  const int _match_epsilon;
  MPMatrix _adjacency_matrix;
  MPMatrix _extended_adjacency_matrix;
  const int _extended_state;
  MPVector _start_vector;
  MPVector _accept_vector;
};

RANS::RANS(const std::string &regex, Encoding enc = ASCII):
    _ok(true), _dfa(regex, rans::Encoding(enc)),
    _match_epsilon(_dfa.accept(DFA::START) ? 1 : 0),
    _extended_state(_dfa.size())
{
  if (!_dfa.ok()) {
    _ok = false;
    _error = _dfa.error();
    return;
  }

  _adjacency_matrix.resize(size(), size());
  _extended_adjacency_matrix.resize(size()+1, size()+1);
  _start_vector.resize(size());
  _start_vector[DFA::START] = 1;
  _accept_vector.resize(size());
  
  for (std::size_t i = 0; i < size(); i++) {
    if (_dfa.accept(i)) _accept_vector[i] = 1;

    for (std::size_t c = 0; c < 256; c++) {
      if (_dfa[i][c] != DFA::REJECT) {
        _adjacency_matrix(i, _dfa[i][c])++;
        _extended_adjacency_matrix(i, _dfa[i][c])++;
        if (_dfa.accept(_dfa[i][c])) {
          _extended_adjacency_matrix(i, _extended_state)++;
        }
      }
    }
  }
  _extended_adjacency_matrix(_extended_state, _extended_state) = 1;

  if (FLAGS_dump_dfa) std::cout << _dfa << std::endl;
  if (FLAGS_dump_matrix) std::cout << _adjacency_matrix << std::endl;
  if (FLAGS_dump_exmatrix) std::cout << _extended_adjacency_matrix << std::endl;
}

// val(), which caliculates the value corresponds given text, is fundamental function of ANS.
// val() is bijection: L -> N where L is set of acceptable string defined by
// regular expression (DFA), and N is natural number (include 0).
// (inverse function of this is rep())
//
// This implementation runs in time O(n * |D|^2) where n is the length of the text
// and |D| is the size of(number of states of) the DFA.
//
// val() throws exception when text is not acceptable.
// Therefore caller should assure that text is acceptable when calling this function.
// caller could check like as: "if(accept(text)) val(text, value);".
RANS::Value& RANS::val(const std::string& text, Value& value) const
{
  int state = DFA::START;
  value = 0;
  MPVector paths(size());

  for (std::size_t i = 0; state != DFA::REJECT && i < text.length(); i++) {
    paths[DFA::START]++;
    for (std::size_t c = 0; c < static_cast<unsigned char>(text[i]); c++) {
      int next = _dfa[state][c];
      if (next != DFA::REJECT) paths[next]++;
    }
    state = _dfa[state][static_cast<unsigned char>(text[i])];
    if (i < text.length() - 1) prod(paths, _adjacency_matrix);
  }

  if (!_dfa.accept(state)) throw Exception("invalid text: text is not acceptable.");

  inner_prod(paths, _accept_vector, value);
  
  return value;
}

// rep(), which caliculates the text corresponds given value, is fundamental function of ANS.
// rep() is bijection: N -> L where and N is natural number (include 0) and L is
// set of acceptable string defined by regular expression (DFA).
// (inverse function of this is val())
//
// This implementation runs in time roughly O(n log n |D|^3) where n is the length
// of text and |D| is the size of(number of states of) DFA.
//
// This will Throw exception when there exists no text s.t. val(text) == value.
// Therefore caller should assure that there exists a correspoding text when
// calling this function. but don't worry, this condition is always true if caller
// got the value via val() just like: "value = val(text)"
std::string& RANS::rep(const Value& value, std::string& text) const
{
  if (value < 0) throw Exception("invalid value: correspoinding text does not exists.");
  
  MPMatrix tmpM(size(), size());
  int state = DFA::START;
  Value value_ = value, val, val_;
  text = "";

  for (int len = floor(value_); len > 0; len--, val = val_ = 0) {
    power(_adjacency_matrix, len - 1, tmpM);

    for (std::size_t c = 0; c < 256; c++) {
      int next = _dfa[state][c];
      if (next == DFA::REJECT) continue;
      
      val_ = val;
      for (std::size_t i = 0; i < size(); i++) {
        if (_dfa.accept(i)) val += tmpM(next, i);
      }
      
      if (val > value_) {
        text.append(1, c);
        state = next;
        value_ -= val_;
        break;
      }
    }
  }

  return text;
}

int RANS::floor(Value& value) const
{
  if (value < _match_epsilon) return 0;

  MPMatrix tmpM(size()+1, size()+1), tmpM_(size()+1, size()+1);
  tmpM = _extended_adjacency_matrix;

  if (tmpM(DFA::START, _extended_state) + _match_epsilon > value) {
    value -= _match_epsilon;
    return 1;
  }
  
  std::size_t length = 1;
  // exponential search
  do {
    tmpM_ = tmpM;
    prod(tmpM, tmpM);
    if (length > size() &&
        tmpM(DFA::START, _extended_state) == tmpM_(DFA::START, _extended_state)) {
      // correspoding text does not exists. (theoretical judgment via Pumping lemma)
      throw Exception("invalid value: correspoinding text does not exists.");
    }
    length *= 2;
  } while (tmpM(DFA::START, _extended_state) + _match_epsilon <= value);

  // linear search
  tmpM = tmpM_; length /= 2;
  do {
    tmpM_ = tmpM;
    prod(tmpM, _extended_adjacency_matrix);
    length++;
  } while (tmpM(DFA::START, _extended_state) + _match_epsilon <= value);

  value -= tmpM_(DFA::START, _extended_state) + _match_epsilon;
  return length;
}

// Return the number of all acceptable strings.
// if it's infinite, then return -1;
Value RANS::amount() const
{
  MPMatrix tmpM(size()+1, size()+1);
  tmpM = _extended_adjacency_matrix;

  std::size_t length = 1;
  Value amount_ = _match_epsilon;

  do {
    amount_ = tmpM(DFA::START, _extended_state);
    prod(tmpM, tmpM);
    length *= 2;
  } while (length < 2 * size());

  if (amount_ != tmpM(DFA::START, _extended_state)) {
    return -1; // there exists infinite acceptable strings.
  } else {
    return amount_ + _match_epsilon;
  }
}

// Return the number of all acceptable strings of
// (less than, if amount is true) 'length' characters in length.
Value RANS::count(std::size_t length, bool amount) const
{
  const std::size_t size_ = amount ? size()+1 : size();
  int accept_state =  _extended_state;
  MPMatrix tmpM(size_, size_);

  power(amount ? _extended_adjacency_matrix : _adjacency_matrix, length, tmpM);

  if (amount) {
    return tmpM(DFA::START, _extended_state) + _match_epsilon;
  } else {
    Value count_;
    for (std::size_t i = 0; i < size(); i++) {
      if (_dfa.accept(i)) count_ += tmpM(DFA::START, i);
    }
    return count_;
  }
}

const RANS RANS::baseBYTE(".*");

std::string& RANS::compress(const std::string& text, std::string& dst) const
{
  Value value;
  return baseBYTE(val(text, value), dst);
}

std::string& RANS::decompress(const std::string& text, std::string& dst) const
{
  Value value;
  return rep(baseBYTE(text, value), dst);
}

} // namespace rans

using rans::RANS; // export

#ifdef RANS_DEBUG // wrappers for gdb
void dump(rans::DFA &v) { std::cout << v << std::endl; }
void dump(rans::Value& v) { std::cout << v << std::endl; }
void dump(rans::MPVector& v) { std::cout << v << std::endl; }
void dump(rans::MPMatrix& v) { std::cout << v << std::endl; }
#endif // RANS_DEBUG

#endif // RANS_H_

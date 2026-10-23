//===- llvm-vbc.cpp - Experimental classic Visual Basic to LLVM IR --------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// Deliberately small, fail-closed VB6/VBA-style standard module subset.
// This translates typed Long expressions; it does not implement VB runtimes.

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool identifierStart(char C) {
  return (C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z');
}
bool identifierBody(char C) {
  return identifierStart(C) || (C >= '0' && C <= '9') || C == '_';
}
std::string lower(std::string S) {
  for (char &C : S)
    if (C >= 'A' && C <= 'Z')
      C = static_cast<char>(C - 'A' + 'a');
  return S;
}
bool number(const std::string &S) {
  if (S.empty())
    return false;
  for (char C : S)
    if (C < '0' || C > '9')
      return false;
  return true;
}
bool ident(const std::string &S) {
  if (S.empty() || !identifierStart(S[0]))
    return false;
  for (char C : S)
    if (!identifierBody(C))
      return false;
  return true;
}

class Compiler {
  std::ostringstream Body;
  std::map<std::string, std::string> Vars;
  std::string FunctionName;
  unsigned Line = 0;
  unsigned Temp = 0;
  bool OptionExplicit = false;
  bool Started = false;
  bool Ended = false;
  bool Returned = false;

  [[noreturn]] void fail(const std::string &Message) const {
    throw std::runtime_error("line " + std::to_string(Line) + ": " + Message);
  }
  std::string temporary() { return "%t" + std::to_string(Temp++); }
  std::vector<std::string> lex(const std::string &Text) {
    std::vector<std::string> Tokens;
    for (size_t I = 0; I < Text.size();) {
      unsigned char C = static_cast<unsigned char>(Text[I]);
      if (C == ' ' || C == '\t' || C == '\r') { ++I; continue; }
      if (C == '\'')
        break;
      if (C >= 128)
        fail("non-ASCII source is not yet supported");
      if (identifierStart(C)) {
        size_t Start = I++;
        while (I < Text.size() && identifierBody(Text[I])) ++I;
        std::string Token = lower(Text.substr(Start, I - Start));
        if (Tokens.empty() && Token == "rem")
          break;
        Tokens.push_back(Token);
        continue;
      }
      if (C >= '0' && C <= '9') {
        size_t Start = I++;
        while (I < Text.size() && Text[I] >= '0' && Text[I] <= '9') ++I;
        Tokens.push_back(Text.substr(Start, I - Start));
        continue;
      }
      if (C == '(' || C == ')' || C == '+' || C == '-' || C == '*' ||
          C == '=' || C == ',') {
        Tokens.emplace_back(1, static_cast<char>(C));
        ++I;
        continue;
      }
      fail("unsupported character or operator");
    }
    return Tokens;
  }
  std::string checkedOp(const char *Intrinsic, std::string A, std::string B) {
    const std::string Pair = temporary(), Value = temporary();
    const std::string Bad = temporary(), Tag = std::to_string(Temp++);
    Body << "  " << Pair << " = call { i32, i1 } @llvm." << Intrinsic
         << ".with.overflow.i32(i32 " << A << ", i32 " << B << ")\n";
    Body << "  " << Value << " = extractvalue { i32, i1 } " << Pair << ", 0\n";
    Body << "  " << Bad << " = extractvalue { i32, i1 } " << Pair << ", 1\n";
    Body << "  br i1 " << Bad << ", label %overflow" << Tag
         << ", label %ok" << Tag << "\n";
    Body << "overflow" << Tag << ":\n  call void @llvm.trap()\n  unreachable\n";
    Body << "ok" << Tag << ":\n";
    return Value;
  }
  std::string parseAtom(const std::vector<std::string> &Tokens, size_t &I) {
    if (I == Tokens.size()) fail("expected expression");
    const std::string T = Tokens[I++];
    if (T == "(") {
      std::string Expr = parseExpr(Tokens, I);
      if (I == Tokens.size() || Tokens[I++] != ")")
        fail("expected ')'");
      return Expr;
    }
    if (number(T)) {
      uint64_t Value = 0;
      for (char C : T) {
        unsigned Digit = C - '0';
        if (Value > (2147483647u - Digit) / 10u)
          fail("Long integer literal exceeds 32-bit signed range");
        Value = Value * 10 + Digit;
      }
      return std::to_string(Value);
    }
    if (ident(T)) {
      auto V = Vars.find(T);
      if (V == Vars.end()) fail("undeclared or unsupported identifier '" + T + "'");
      const std::string Result = temporary();
      Body << "  " << Result << " = load i32, ptr " << V->second
           << ", align 4\n";
      return Result;
    }
    fail("expected Long literal, declared variable, or parenthesized expression");
  }
  std::string parseUnary(const std::vector<std::string> &Tokens, size_t &I) {
    if (I < Tokens.size() && Tokens[I] == "+") { ++I; return parseUnary(Tokens, I); }
    if (I < Tokens.size() && Tokens[I] == "-") {
      ++I;
      // -2147483648 is a valid signed Long, although its magnitude is not.
      if (I < Tokens.size() && number(Tokens[I])) {
        const std::string S = Tokens[I++];
        uint64_t Value = 0;
        for (char C : S) {
          const unsigned Digit = C - '0';
          if (Value > (2147483648ULL - Digit) / 10ULL)
            fail("Long integer literal exceeds 32-bit signed range");
          Value = Value * 10 + Digit;
        }
        return "-" + std::to_string(Value);
      }
      return checkedOp("ssub", "0", parseUnary(Tokens, I));
    }
    return parseAtom(Tokens, I);
  }
  std::string parseMul(const std::vector<std::string> &Tokens, size_t &I) {
    std::string Left = parseUnary(Tokens, I);
    while (I < Tokens.size() && Tokens[I] == "*") {
      ++I;
      std::string Right = parseUnary(Tokens, I);
      Left = checkedOp("smul", Left, Right);
    }
    return Left;
  }
  std::string parseExpr(const std::vector<std::string> &Tokens, size_t &I) {
    std::string Left = parseMul(Tokens, I);
    while (I < Tokens.size() && (Tokens[I] == "+" || Tokens[I] == "-")) {
      const bool Add = Tokens[I++] == "+";
      std::string Right = parseMul(Tokens, I);
      Left = checkedOp(Add ? "sadd" : "ssub", Left, Right);
    }
    return Left;
  }
  void store(const std::string &Name, const std::vector<std::string> &Tokens) {
    auto Var = Vars.find(Name);
    if (Var == Vars.end()) fail("undeclared variable '" + Name + "'");
    size_t I = 2;
    std::string Value = parseExpr(Tokens, I);
    if (I != Tokens.size()) fail("unexpected token after expression");
    Body << "  store i32 " << Value << ", ptr " << Var->second
         << ", align 4\n";
  }
  void parse(const std::vector<std::string> &T) {
    if (T.empty()) return;
    if (!Started) {
      if (T == std::vector<std::string>{"option", "explicit"} && !OptionExplicit) {
        OptionExplicit = true;
        return;
      }
      if (!OptionExplicit)
        fail("Option Explicit required: implicit Variant is unsupported");
      size_t I = 0;
      if (T[I] == "public") ++I;
      if (I >= T.size() || T[I++] != "function" || I >= T.size() ||
          !ident(T[I]))
        fail("expected [Public] Function name() As Long");
      FunctionName = T[I++];
      if (I + 3 >= T.size() || T[I++] != "(" || T[I++] != ")" ||
          T[I++] != "as" || T[I++] != "long" || I != T.size())
        fail("only argument-free As Long functions are supported");
      Started = true;
      Vars.emplace(FunctionName, "%result");
      Body << "  %result = alloca i32, align 4\n"
           << "  store i32 0, ptr %result, align 4\n";
      return;
    }
    if (Ended) fail("only one Function per module is supported");
    if (T == std::vector<std::string>{"end", "function"}) {
      if (!Returned) {
        std::string Val = temporary();
        Body << "  " << Val << " = load i32, ptr %result, align 4\n"
             << "  ret i32 " << Val << "\n";
      }
      Ended = true;
      return;
    }
    if (Returned)
      fail("statements after Exit Function are unsupported");
    if (T == std::vector<std::string>{"exit", "function"}) {
      std::string Val = temporary();
      Body << "  " << Val << " = load i32, ptr %result, align 4\n"
           << "  ret i32 " << Val << "\n";
      Returned = true;
      return;
    }
    if (T[0] == "dim") {
      if (T.size() != 4 || !ident(T[1]) || T[2] != "as" || T[3] != "long")
        fail("only 'Dim name As Long' declarations are supported");
      if (Vars.count(T[1])) fail("duplicate declaration '" + T[1] + "'");
      const std::string Ptr = "%var" + std::to_string(Vars.size());
      Vars.emplace(T[1], Ptr);
      Body << "  " << Ptr << " = alloca i32, align 4\n"
           << "  store i32 0, ptr " << Ptr << ", align 4\n";
      return;
    }
    if (T.size() >= 3 && ident(T[0]) && T[1] == "=") {
      store(T[0], T);
      return;
    }
    fail("unsupported Visual Basic statement");
  }

public:
  std::string compile(const std::string &Text) {
    std::istringstream Input(Text);
    std::string SourceLine;
    while (std::getline(Input, SourceLine)) {
      ++Line;
      parse(lex(SourceLine));
    }
    if (!Ended) fail("missing End Function");
    std::ostringstream IR;
    IR << "; Experimental classic VB Long frontend; not a VB6 runtime/compiler\n"
       << "declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32)\n"
       << "declare { i32, i1 } @llvm.ssub.with.overflow.i32(i32, i32)\n"
       << "declare { i32, i1 } @llvm.smul.with.overflow.i32(i32, i32)\n"
       << "declare void @llvm.trap()\n\n"
       << "define i32 @vb_" << FunctionName << "() {\nentry:\n"
       << Body.str() << "}\n";
    return IR.str();
  }
};

} // namespace

int main(int argc, char **argv) {
  if (argc == 2 && std::string(argv[1]) == "--help") {
    std::cout << "usage: llvm-vbc input.bas [-o output.ll]\n"
              << "Experimental VB6/VBA-style Long-only source-to-LLVM-IR frontend\n";
    return 0;
  }
  if (argc != 2 && argc != 4) {
    std::cerr << "usage: llvm-vbc input.bas [-o output.ll]\n";
    return 2;
  }
  if (argc == 4 && std::string(argv[2]) != "-o") {
    std::cerr << "llvm-vbc: expected -o output.ll\n";
    return 2;
  }
  std::ifstream File(argv[1], std::ios::binary);
  if (!File) {
    std::cerr << "llvm-vbc: cannot open input file\n";
    return 1;
  }
  std::ostringstream Source;
  Source << File.rdbuf();
  if (!File.good() && !File.eof()) {
    std::cerr << "llvm-vbc: cannot read input file\n";
    return 1;
  }
  try {
    const std::string IR = Compiler().compile(Source.str());
    if (argc == 4) {
      std::ofstream Output(argv[3], std::ios::binary | std::ios::trunc);
      if (!Output) throw std::runtime_error("cannot open output file");
      Output << IR;
      if (!Output) throw std::runtime_error("cannot write output file");
    } else {
      std::cout << IR;
    }
  } catch (const std::exception &E) {
    std::cerr << "llvm-vbc: " << E.what() << '\n';
    return 1;
  }
  return 0;
}

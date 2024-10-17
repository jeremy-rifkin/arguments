#include <arguments.hpp>

#include <algorithm>
#include <iostream>
#include <print>
#include <ranges>

int main() {
  #ifdef _WIN32
  auto& out = std::wcout;
  #else
  auto& out = std::cout;
  std::println("{}", std::arguments{}[0]);
  #endif

  std::println("---------------- simple iteration with .c_str()");
  for(const auto& arg : std::arguments{}) {
    out << arg.c_str() << std::endl;
  }

  std::arguments args;

  std::println("---------------- simple iteration with .native()");
  for(const auto& arg : args) {
    out << arg.native() << std::endl;
  }

  std::println("---------------- simple iteration with .native_string()");
  for(const auto& arg : args) {
    out << arg.native_string() << std::endl;
  }

  std::println("---------------- reverse adapter");
  for(const auto& arg : args | std::views::reverse) {
    out << arg.native() << std::endl;
  }

  std::println("---------------- drop(1) | reverse");
  for(const auto& arg : args | std::views::drop(1) | std::views::reverse) {
    out << arg.native() << std::endl;
  }

  std::println("---------------- member access and printing");
  // args[0].c_str()[0] = ' ';
  // args[0].native()[0] = ' ';
  out << args[0].native() << std::endl;
  out << args[1] << std::endl;
  out << args.at(1) << std::endl;
  out << args[2] << std::endl;

  std::println("---------------- iterators behave properly");
  auto it = args.begin();
  out << (*it).c_str() << std::endl;
  out << (++it)->c_str() << std::endl;
  out << it++->c_str() << std::endl;
  out << it->c_str() << std::endl;

  std::println("---------------- encoding stuff");
  #ifdef _WIN32
  #define ARG(str) L##str
  #else
  #define ARG(str) str
  #endif
  if(args.at(1).native() == ARG("--help")) {
    std::println("arguments[1] is --help, using ARG macro");
  }
  if(args.at(1).string() == "--help") {
    std::println("arguments[1] is --help, using .string()");
  }
  if(args.at(1).wstring() == L"--help") {
    std::println("arguments[1] is --help, using .wstring()");
  }
  if(args.at(1).u8string() == u8"--help") {
    std::println("arguments[1] is --help, using .u8string()");
  }
  if(args.at(1).u16string() == u"--help") {
    std::println("arguments[1] is --help, using .u16string()");
  }
  if(args.at(1).u32string() == U"--help") {
    std::println("arguments[1] is --help, using .u32string()");
  }
}

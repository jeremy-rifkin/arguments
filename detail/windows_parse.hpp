#ifndef WINDOWS_PARSE_HPP
#define WINDOWS_PARSE_HPP

#include <string>
#include <string_view>
#include <vector>

namespace std::arg_detail {

  #ifdef _WIN32
  // https://github.com/huangqinjin/ucrt/blob/d6e817a4cc90f6f1fe54f8a0aa4af4fff0bb647d/startup/argv_parsing.cpp#L95
  // https://stdrs.dev/nightly/x86_64-pc-windows-gnu/src/std/sys/windows/args.rs.html#68
  std::vector<std::wstring> parse_command_line(const wchar_t* command_line) {
    std::vector<std::wstring> args;

    const wchar_t* cursor = command_line;

    if(!cursor) {
      return {};
    }

    // parse executable name, quoting rules are different than normal args
    bool in_quotes = false;
    std::wstring current;
    while(*cursor) {
      if(*cursor == L'"') {
        in_quotes = !in_quotes;
      } else if((*cursor == L' ' || *cursor == L'\t') && !in_quotes) {
        break;
      } else {
        current += *cursor;
      }
      cursor++;
    }
    args.push_back(std::move(current));

    // skip whitespace
    while(*cursor == L' ' || *cursor == L'\t') {
      cursor++;
    }

    // Basic rules:
    // - Outside of quotes, whitespace separate arguments (consecutive whitespace is ignored)
    // - Backslashes work as follows:
    //     2N     backslashes   + " ==> N backslashes and begin/end quote
    //     2N + 1 backslashes   + " ==> N backslashes + literal "
    //     N      backslashes       ==> N backslashes
    // - Inside quotes, quotes escape quotes ("" ==> ")
    in_quotes = false;
    current.clear();
    while(*cursor) {
      if((*cursor == L' ' || *cursor == L'\t') && !in_quotes) {
        // whitespace outside quotes delimit an argument
        args.push_back(std::move(current));
        current.clear();
        // skip whitespace
        while(*cursor == L' ' || *cursor == L'\t') {
          cursor++;
        }
      } else if(*cursor == L'\\') {
        std::size_t count = 1;
        while(*cursor && *cursor == L'\\') {
          count++;
          cursor++;
        }
        if(*cursor && *cursor == L'"') {
          current.append(count / 2, L'\\');
          if(count % 2 == 1) {
            current += L'"';
            cursor++;
          }
        } else {
          current.append(count, L'\\');
        }
        if(*cursor) {
          cursor++;
        }
      } else if(*cursor == L'"') {
        if(in_quotes) {
          if(*(cursor + 1) == L'"') { // consecutive "" inside a quote
            current += L'"';
            cursor++;
          } else {
            in_quotes = false;
          }
        } else {
          in_quotes = true;
        }
        if(*cursor) {
          cursor++;
        }
      } else {
        current += *cursor;
        cursor++;
      }
    }

    if(!current.empty() || in_quotes) { // could have an empty current if breaking while in_quotes
      args.push_back(std::move(current));
    }

    return args;
  }
  #endif

}

#endif

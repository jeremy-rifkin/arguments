#ifndef ARGUMENTS_HPP
#define ARGUMENTS_HPP

#include <codecvt>
#include <compare>
#include <cstddef>
#include <format>
#include <iterator>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <detail/locale_conv.hpp>
#include <detail/windows_parse.hpp>

#ifdef _WIN32
#include <windows.h>
#pragma warning(disable : 4996 4244 4702) // awful, but hacking for now
#endif

namespace std {
  class argument;
  template<class Allocator = allocator<argument>> class arguments;
}

namespace std::arg_detail {
  #ifndef _WIN32
  int __argc;
  char** __argv;
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wprio-ctor-dtor"
  __attribute__((constructor(0))) void __init_argv_argc(int argc, char** argv) {
    __argc = argc;
    __argv = argv;
  }
  #pragma GCC diagnostic pop
  #else
  // #error "Windows not implemented just yet"
  #endif

  template<typename _EcharT> struct Codecvt;
  // copied from https://github.com/gcc-mirror/gcc/blob/a8b4ea1bcc10b5253992f4b932aec6862aef32fa/libstdc%2B%2B-v3/include/bits/fs_path.h#L868C1-L892C9
  // path::_Codecvt<C> Performs conversions between C and path::string_type.
  // The native encoding of char strings is the OS-dependent current
  // encoding for pathnames. FIXME: We assume this is UTF-8 everywhere,
  // but should use a Windows API to query it.

  // Converts between native pathname encoding and char16_t or char32_t.
  template<typename _EcharT>
      struct Codecvt
      // Need derived class here because std::codecvt has protected destructor.
      : std::codecvt<_EcharT, char, mbstate_t>
      { };

  // Converts between native pathname encoding and native wide encoding.
  // The native encoding for wide strings is the execution wide-character
  // set encoding. FIXME: We assume that this is either UTF-32 or UTF-16
  // (depending on the width of wchar_t). That matches GCC's default,
  // but can be changed with -fwide-exec-charset.
  // We need a custom codecvt converting the native pathname encoding
  // to/from the native wide encoding.
  template<>
      struct Codecvt<wchar_t>
      : std::conditional_t<sizeof(wchar_t) == sizeof(char32_t),
              std::codecvt_utf8<wchar_t>,       // UTF-8 <-> UTF-32
              std::codecvt_utf8_utf16<wchar_t>> // UTF-8 <-> UTF-16
      { };
}

namespace std {
  class argument {
  public:
    #ifndef _WIN32
    using value_type  = char;
    #else
    using value_type  = wchar_t;
    #endif
    using string_type = basic_string<value_type>;
    using string_view_type = basic_string_view<value_type>;

    // [arguments.argument.native], native observers
    const string_view_type native() const noexcept {
      return {arg};
    }
    const string_type native_string() const {
      return {arg};
    }
    const value_type* c_str() const noexcept {
      return arg;
    }
    explicit operator string_type() const {
      return {arg};
    }
    explicit operator string_view_type() const noexcept {
      return {arg};
    }

    // [arguments.argument.obs], converting observers
    template<class EcharT, class traits = char_traits<EcharT>,
              class Allocator = allocator<EcharT>>
      basic_string<EcharT, traits, Allocator>
        string(const Allocator& a = Allocator()) const {
          // Following what was done in libstdc++'s filesystem::path implementation
          // https://github.com/gcc-mirror/gcc/blob/a8b4ea1bcc10b5253992f4b932aec6862aef32fa/libstdc%2B%2B-v3/include/bits/fs_path.h#L1109
          // TODO: For now hacky and inefficient
          using String = basic_string<EcharT, traits, Allocator>;
          if(!arg || !*arg) {
            return String(a);
          }
          #ifndef _WIN32
          string_view u8_string = arg;
          #else
          // First convert native string from UTF-16 to to UTF-8.
          // XXX This assumes that the execution wide-character set is UTF-16.
          std::codecvt_utf8_utf16<value_type> u8cvt;

          using alloc_traits = std::allocator_traits<Allocator>;
          using char_alloc = alloc_traits::template rebind_alloc<char>;
          using U8String = basic_string<char, char_traits<char>, char_alloc>;
          U8String u8_string{char_alloc{a}};
          const value_type* __wfirst = arg;
          const value_type* __wlast = __wfirst + std::char_traits<value_type>::length(arg);
          if(!__str_codecvt_out_all(__wfirst, __wlast, u8_string, u8cvt)) {
            throw std::runtime_error("Conversion failed for argument");
          }
          if constexpr (is_same_v<EcharT, char>) {
            return u8_string; // XXX assumes native ordinary encoding is UTF-8.
          }
          #endif
          const char* first = u8_string.data();
          const char* last = first + u8_string.size();

          // Convert UTF-8 string to requested format.
          if constexpr(is_same_v<EcharT, char8_t>) {
            return String(first, last, a);
          } else {
            // Convert UTF-8 to wide string.
            String wstr(a);
            arg_detail::Codecvt<EcharT> target_cvt;
            if (__str_codecvt_in_all(first, last, wstr, target_cvt)) {
              return wstr;
            }
          }
          throw std::runtime_error("Conversion failed for argument");
        }
    std::string    string() const {
      return string<char>();
    }
    std::wstring   wstring() const {
        return string<wchar_t>();
    }
    std::u8string  u8string() const {
        return string<char8_t>();
    }
    std::u16string u16string() const {
        return string<char16_t>();
    }
    std::u32string u32string() const {
        return string<char32_t>();
    }

    // [arguments.argument.compare], comparison
    friend bool operator==(const argument& lhs, const argument& rhs) noexcept;
    friend strong_ordering operator<=>(const argument& lhs, const argument& rhs) noexcept;

    // [arguments.argument.ins], inserter
    template<class charT, class traits>
      friend basic_ostream<charT, traits>&
        operator<<(basic_ostream<charT, traits>& os, const argument& a) {
          return os << a.string<charT, traits>();
        }

  private:
    const value_type* arg;
    argument(const value_type* arg) : arg(arg) {}
    argument() {}
    template<class Allocator> friend class arguments;
  };

  // [arguments.argument.fmt], formatter
  template<typename charT> struct formatter<argument, charT> : formatter<argument::string_view_type, charT> {
    template<class FormatContext>
      typename FormatContext::iterator
        format(const argument& argument, FormatContext& ctx) const {
          return std::formatter<argument::string_view_type>::format(argument.string<charT, char_traits<charT>>(), ctx);
        }
  };
}

namespace std {
  class arguments_iterator {
  public:
    using value_type = const argument;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = value_type&;

    arguments_iterator() = default;

    reference operator*() const {
      return *arg;
    }
    pointer operator->() const {
      return arg;
    }

    arguments_iterator& operator++() {
      arg++;
      return *this;
    }
    arguments_iterator operator++(int) {
      auto copy = *this;
      arg++;
      return copy;
    }

    arguments_iterator& operator--() {
      arg--;
      return *this;
    }
    arguments_iterator operator--(int) {
      auto copy = *this;
      arg--;
      return copy;
    }

    friend arguments_iterator operator+(arguments_iterator it, size_type i) {
      return {it.arg + i};
    }
    friend arguments_iterator operator+(size_type i, arguments_iterator it) {
      return {it.arg + i};
    }
    friend arguments_iterator operator-(arguments_iterator it, size_type i) {
      return {it.arg - i};
    }
    friend difference_type operator-(arguments_iterator a, arguments_iterator b) {
      return a.arg - b.arg;
    }

    arguments_iterator& operator+=(size_type i) {
      arg += i;
      return *this;
    }
    arguments_iterator& operator-=(size_type i) {
      arg -= i;
      return *this;
    }

    reference operator[](size_type i) const {
      return arg[i];
    }

    bool operator==(arguments_iterator other) const {
      return arg == other.arg;
    }
    bool operator!=(arguments_iterator other) const {
      return arg != other.arg;
    }
    auto operator<=>(arguments_iterator other) const {
      return arg <=> other.arg;
    }

  private:
    pointer arg = nullptr;
    arguments_iterator(pointer arg) : arg(arg) {}
    template<class Allocator> friend class arguments;
  };

  template<class Allocator/* = allocator<argument>*/>
  class arguments {
  public:
    using value_type = const argument;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = value_type*;
    using const_pointer = value_type*;
    using reference = value_type&;
    using const_reference = value_type&;
    using const_iterator = arguments_iterator; // see [arguments.view.iterators]
    using iterator = const_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using reverse_iterator = const_reverse_iterator;

    // [arguments.view.cons], constructors
    arguments() noexcept(noexcept(Allocator())) : arguments(Allocator()) {}
    arguments(const Allocator&) {
      #ifndef _WIN32
      args.reserve(arg_detail::__argc);
      for(const auto& arg : std::span{arg_detail::__argv, arg_detail::__argv + arg_detail::__argc}) {
        args.push_back(argument(arg));
      }
      #else
      parsed_args = arg_detail::parse_command_line(GetCommandLineW());
      for(const auto& arg : parsed_args) {
        args.push_back(argument(arg.c_str()));
      }
      #endif
    }

    // [arguments.view.access], access
    reference operator[](size_type index) const noexcept {
      return {args[index]};
    }
    reference at(size_type index) const {
      if(index >= size()) {
        throw out_of_range(std::format("Attempt to access argument {} of arguments, argc = {}", index, size()));
      }
      return operator[](index);
    }

    // [arguments.view.obs], observers
    size_type size() const noexcept {
      return args.size();
    }
    bool empty() const noexcept {
      return args.empty();
    }

    // [arguments.view.iterators], iterators
    const_iterator begin() const noexcept {
      return {args.data()};
    }
    const_iterator end() const noexcept {
      return {args.data() + args.size()};
    }

    const_iterator cbegin() const noexcept {
      return begin();
    }
    const_iterator cend() const noexcept {
      return end();
    }

    const_reverse_iterator rbegin() const noexcept {
      return const_reverse_iterator{begin()};
    }
    const_reverse_iterator rend() const noexcept {
      return const_reverse_iterator{end()};
    }

    const_reverse_iterator crbegin() const noexcept {
      return const_reverse_iterator{begin()};
    }
    const_reverse_iterator crend() const noexcept {
      return const_reverse_iterator{end()};
    }

  private:
    #ifdef _WIN32
    std::vector<std::wstring> parsed_args; // just easy for a simple implementation
    #endif
    std::vector<argument> args; // just easy for a simple implementation
  };
}

#endif

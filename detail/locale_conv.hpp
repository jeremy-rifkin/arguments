// wstring_convert internals from libstdc++
// https://github.com/gcc-mirror/gcc/blob/a8b4ea1bcc10b5253992f4b932aec6862aef32fa/libstdc%2B%2B-v3/include/bits/locale_conv.h#L180

// wstring_convert implementation -*- C++ -*-

// Copyright (C) 2015-2024 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file bits/locale_conv.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{locale}
 */

#ifndef _LOCALE_CONV_H
#define _LOCALE_CONV_H 1

#include <locale>
#include <streambuf>

namespace std
{
  /**
   * @addtogroup locales
   * @{
   */

  template<typename _OutStr, typename _InChar, typename _Codecvt,
	   typename _State, typename _Fn>
    bool
    __do_str_codecvt(const _InChar* __first, const _InChar* __last,
		     _OutStr& __outstr, const _Codecvt& __cvt, _State& __state,
		     size_t& __count, _Fn __fn)
    {
      if (__first == __last)
	{
	  __outstr.clear();
	  __count = 0;
	  return true;
	}

      size_t __outchars = 0;
      auto __next = __first;
      const auto __maxlen = __cvt.max_length() + 1;

      codecvt_base::result __result;
      do
	{
	  __outstr.resize(__outstr.size() + (__last - __next) * __maxlen);
	  auto __outnext = &__outstr.front() + __outchars;
	  auto const __outlast = &__outstr.back() + 1;
	  __result = (__cvt.*__fn)(__state, __next, __last, __next,
					__outnext, __outlast, __outnext);
	  __outchars = __outnext - &__outstr.front();
	}
      while (__result == codecvt_base::partial && __next != __last
	     && ptrdiff_t(__outstr.size() - __outchars) < __maxlen);

      if (__result == codecvt_base::error)
	{
	  __count = __next - __first;
	  return false;
	}

      // The codecvt facet will only return noconv when the types are
      // the same, so avoid instantiating basic_string::assign otherwise
      if (is_same<typename _Codecvt::intern_type,
				       typename _Codecvt::extern_type>())
	if (__result == codecvt_base::noconv)
	  {
	    __outstr.assign(__first, __last);
	    __count = __last - __first;
	    return true;
	  }

      __outstr.resize(__outchars);
      __count = __next - __first;
      return true;
    }

  // Convert narrow character string to wide.
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_in(const char* __first, const char* __last,
		     basic_string<_CharT, _Traits, _Alloc>& __outstr,
		     const codecvt<_CharT, char, _State>& __cvt,
		     _State& __state, size_t& __count)
    {
      using _Codecvt = codecvt<_CharT, char, _State>;
      using _ConvFn
	= codecvt_base::result
	  (_Codecvt::*)(_State&, const char*, const char*, const char*&,
			_CharT*, _CharT*, _CharT*&) const;
      _ConvFn __fn = &codecvt<_CharT, char, _State>::in;
      return __do_str_codecvt(__first, __last, __outstr, __cvt, __state,
			      __count, __fn);
    }

  // As above, but with no __count parameter
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_in(const char* __first, const char* __last,
		     basic_string<_CharT, _Traits, _Alloc>& __outstr,
		     const codecvt<_CharT, char, _State>& __cvt)
    {
      _State __state = {};
      size_t __n;
      return __str_codecvt_in(__first, __last, __outstr, __cvt, __state, __n);
    }

  // As above, but returns false for partial conversion
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_in_all(const char* __first, const char* __last,
			 basic_string<_CharT, _Traits, _Alloc>& __outstr,
			 const codecvt<_CharT, char, _State>& __cvt)
    {
      _State __state = {};
      size_t __n;
      return __str_codecvt_in(__first, __last, __outstr, __cvt, __state, __n)
	&& (__n == size_t(__last - __first));
    }

  // Convert wide character string to narrow.
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_out(const _CharT* __first, const _CharT* __last,
		      basic_string<char, _Traits, _Alloc>& __outstr,
		      const codecvt<_CharT, char, _State>& __cvt,
		      _State& __state, size_t& __count)
    {
      using _Codecvt = codecvt<_CharT, char, _State>;
      using _ConvFn
	= codecvt_base::result
	  (_Codecvt::*)(_State&, const _CharT*, const _CharT*, const _CharT*&,
			char*, char*, char*&) const;
      _ConvFn __fn = &codecvt<_CharT, char, _State>::out;
      return __do_str_codecvt(__first, __last, __outstr, __cvt, __state,
			      __count, __fn);
    }

  // As above, but with no __count parameter
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_out(const _CharT* __first, const _CharT* __last,
		      basic_string<char, _Traits, _Alloc>& __outstr,
		      const codecvt<_CharT, char, _State>& __cvt)
    {
      _State __state = {};
      size_t __n;
      return __str_codecvt_out(__first, __last, __outstr, __cvt, __state, __n);
    }

  // As above, but returns false for partial conversions
  template<typename _CharT, typename _Traits, typename _Alloc, typename _State>
    inline bool
    __str_codecvt_out_all(const _CharT* __first, const _CharT* __last,
			  basic_string<char, _Traits, _Alloc>& __outstr,
			  const codecvt<_CharT, char, _State>& __cvt)
    {
      _State __state = {};
      size_t __n;
      return __str_codecvt_out(__first, __last, __outstr, __cvt, __state, __n)
	&& (__n == size_t(__last - __first));
    }
}

#endif

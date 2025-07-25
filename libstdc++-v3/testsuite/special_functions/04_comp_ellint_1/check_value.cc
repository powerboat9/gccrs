// { dg-do run { target c++11 } }
// { dg-options "-D__STDCPP_WANT_MATH_SPEC_FUNCS__" }
//
// Copyright (C) 2016-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

//  comp_ellint_1

//  Compare against values generated by the GNU Scientific Library.
//  The GSL can be found on the web: http://www.gnu.org/software/gsl/
#include <limits>
#include <cmath>
#if defined(__TEST_DEBUG)
#  include <iostream>
#  define VERIFY(A) \
  if (!(A)) \
    { \
      std::cout << "line " << __LINE__ \
	<< "  max_abs_frac = " << max_abs_frac \
	<< std::endl; \
    }
#else
#  include <testsuite_hooks.h>
#endif
#include <specfun_testcase.h>

// Test data.
// max(|f - f_Boost|): 4.4408920985006262e-16 at index 18
// max(|f - f_Boost| / |f_Boost|): 1.9472906870017937e-16
// mean(f - f_Boost): -1.1686558153949016e-17
// variance(f - f_Boost): 1.2181788466954587e-32
// stddev(f - f_Boost): 1.1037113964689586e-16
const testcase_comp_ellint_1<double>
data001[19] =
{
  { 2.2805491384227703, -0.90000000000000002, 0.0 },
  { 1.9953027776647294, -0.80000000000000004, 0.0 },
  { 1.8456939983747234, -0.69999999999999996, 0.0 },
  { 1.7507538029157526, -0.59999999999999998, 0.0 },
  { 1.6857503548125961, -0.50000000000000000, 0.0 },
  { 1.6399998658645112, -0.39999999999999991, 0.0 },
  { 1.6080486199305128, -0.29999999999999993, 0.0 },
  { 1.5868678474541662, -0.19999999999999996, 0.0 },
  { 1.5747455615173560, -0.099999999999999978, 0.0 },
  { 1.5707963267948966, 0.0000000000000000, 0.0 },
  { 1.5747455615173560, 0.10000000000000009, 0.0 },
  { 1.5868678474541662, 0.20000000000000018, 0.0 },
  { 1.6080486199305128, 0.30000000000000004, 0.0 },
  { 1.6399998658645112, 0.40000000000000013, 0.0 },
  { 1.6857503548125961, 0.50000000000000000, 0.0 },
  { 1.7507538029157526, 0.60000000000000009, 0.0 },
  { 1.8456939983747238, 0.70000000000000018, 0.0 },
  { 1.9953027776647294, 0.80000000000000004, 0.0 },
  { 2.2805491384227707, 0.90000000000000013, 0.0 },
};
const double toler001 = 2.5000000000000020e-13;

template<typename Ret, unsigned int Num>
  void
  test(const testcase_comp_ellint_1<Ret> (&data)[Num], Ret toler)
  {
    const Ret eps = std::numeric_limits<Ret>::epsilon();
    Ret max_abs_diff = -Ret(1);
    Ret max_abs_frac = -Ret(1);
    unsigned int num_datum = Num;
    for (unsigned int i = 0; i < num_datum; ++i)
      {
	const Ret f = std::comp_ellint_1(data[i].k);
	const Ret f0 = data[i].f0;
	const Ret diff = f - f0;
	if (std::abs(diff) > max_abs_diff)
	  max_abs_diff = std::abs(diff);
	if (std::abs(f0) > Ret(10) * eps
	 && std::abs(f) > Ret(10) * eps)
	  {
	    const Ret frac = diff / f0;
	    if (std::abs(frac) > max_abs_frac)
	      max_abs_frac = std::abs(frac);
	  }
      }
    VERIFY(max_abs_frac < toler);
  }

int
main()
{
  test(data001, toler001);
  return 0;
}

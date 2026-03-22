// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifdef __cplusplus

#include "version_gen.h"

#ifdef _MSC_VER
#include <sdkddkver.h>
#elif __APPLE__
#include "TargetConditionals.h"
#endif// _MSC_VER

////////////////////////////////////////////////////////////////////////////////
//settings for all files
#ifdef _MSC_VER
#define _SILENCE_NONFLOATING_COMPLEX_DEPRECATION_WARNING
//#pragma warning(disable : 4505)//unreferenced local function has been removed
//#pragma warning(disable : 4996)//This function or variable may be unsafe. Consider using (_CRT_SECURE_NO_WARNINGS) ...
//#pragma warning(disable : 4714)//function 'function' marked as __forceinline not inlined
//#pragma warning(disable : 4503)//decorated name length exceeded, name was truncated
//#pragma warning(disable : 4592)//symbol will be dynamically initialized (implementation limitation)
//#pragma warning(disable : 4201)//nonstandard extension used : nameless struct/union
//#pragma warning(disable : 4239)//nonstandard extension used : 'token' : conversion from 'type' to 'type'
#elif defined(__clang__)
//#pragma clang diagnostic ignored "-Wlogical-op-parentheses"
#endif// _MSC_VER

////////////////////////////////////////////////////////////////////////////////
//settings for this file only
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 5054)//[Eigen] operator '&': deprecated between enumerations of different types
#else
#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif//_MSC_VER


////////////////////////////////////////////////////////////////////////////////
//Standard library
#include <memory>
#include <cmath>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <locale>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <bitset>
#include <cstdlib>
#include <complex>
#include <initializer_list>
#include <type_traits>
#include <chrono>
#include <bit>
#include <span>

////////////////////////////////////////////////////////////////////////////////
//Boost library
#define BOOST_ALLOW_DEPRECATED_HEADERS
//exceptions are allowed, but we use our own implementation
#define BOOST_NO_EXCEPTIONS
#define BOOST_EXCEPTION_DISABLE

//https://github.com/boostorg/container_hash/issues/22
#define BOOST_NO_CXX98_FUNCTION_BASE

//removing unnecessary information from binaries
#define BOOST_DISABLE_CURRENT_FUNCTION
#define BOOST_DISABLE_CURRENT_LOCATION
#define BOOST_NO_RTTI
#define BOOST_NO_TYPEID
#define BOOST_NO_STD_TYPEINFO
#include <boost/core/typeinfo.hpp>
#undef BOOST_CORE_TYPEID
#define BOOST_CORE_TYPEID(T) (*(boost::core::typeinfo*)nullptr)

//also, we can modify boost/type_index/detail/compile_time_type_info.hpp

#include <boost/predef.h>
#include <boost/core/ignore_unused.hpp>
#include <boost/throw_exception.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/hex.hpp>
#include <boost/functional/hash.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/deque.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/unordered/unordered_set.hpp>
#define EIGEN_MPL2_ONLY
#include <boost/multiprecision/eigen.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/multiprecision/integer.hpp>
#include <boost/multiprecision/cpp_bin_float.hpp>
#include <boost/multiprecision/debug_adaptor.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/math/tools/polynomial.hpp>
#include <boost/math/tools/roots.hpp>
#include <boost/rational.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/integer/extended_euclidean.hpp>
#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <boost/graph/strong_components.hpp>
////////////////////////////////////////////////////////////////////////////////

#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <Eigen/LU>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <Eigen/QR>

//#include <Spectra/GenEigsSolver.h>
//#include <Spectra/MatOp/SparseGenMatProd.h>

////////////////////////////////////////////////////////////////////////////////
#include "unordered_dense.h"
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "fmt/ostream.h"
////////////////////////////////////////////////////////////////////////////////
//return the warning level to the standard one
#ifdef _MSC_VER
#pragma warning(pop)
#else
#pragma clang diagnostic pop
#endif//_MSC_VER
////////////////////////////////////////////////////////////////////////////////
//restrict
#ifdef _MSC_VER
#define restrict_var __restrict;
#elif defined(__GNUC__) || defined(__clang__)
#define restrict_var __restrict__
#endif//_MSC_VER
////////////////////////////////////////////////////////////////////////////////
//assume
#ifdef NDEBUG
#if defined(__clang__)
#define ASSUME(expr) __builtin_assume(expr)
#elif defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ >= 13
#define ASSUME(expr) [[assume(expr)]]
#else
#define ASSUME(expr) do { if (!(expr)) __builtin_unreachable(); } while (0)
#endif
#elif defined(_MSC_VER)
#define ASSUME(expr) __assume(expr)
#else
#define ASSUME(expr) ((void)0)
#endif
#else
#define ASSUME(X) assert(X)
#endif//NDEBUG
////////////////////////////////////////////////////////////////////////////////
#define UNUSED(x) (void)(x)
////////////////////////////////////////////////////////////////////////////////
#define let const auto
#define ims_global //globals
#define ims_static static//file-scope globals
#define ims_func_static static//func-scope globals
////////////////////////////////////////////////////////////////////////////////
#define CATMACRO_(a, b) a ## b
#define CATMACRO(a, b) CATMACRO_(a, b)
#define VARLINE(Var) CATMACRO(Var, __LINE__)

#ifdef NDEBUG
#define UNAMESPACE boost
#else	
//#define UNAMESPACE std
#define UNAMESPACE boost
#endif

////////////////////////////////////////////////////////////////////////////////
#include "ims_print.h"
#include "test_alloc_hook.h"
#include "ims_number.h"

////////////////////////////////////////////////////////////////////////////////

static constexpr size_t ims_max = std::numeric_limits<size_t>::max();

//block identifier
using block_id_t = uint32_t;
static constexpr block_id_t block_id_max = std::numeric_limits<block_id_t>::max();

//true if the current thread needs to stop quickly
//works quickly under normal conditions (one TLS request + several flags)
//can suspend the thread upon user request
//always returns false on the main thread
bool ims_need_stop();

////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct ims_opaque_deleter {
	void operator()(T* it)
	{
		void ims_opaque_deleter_hook(T*);
		ims_opaque_deleter_hook(it);
	}
};
template<typename T>
using ims_unique_ptr = std::unique_ptr<T, ims_opaque_deleter<T>>;
#define IMS_DEFINE_OPAQUE_DELETER(T)  void ims_opaque_deleter_hook(T *it) { delete it; }
////////////////////////////////////////////////////////////////////////////////
template<typename F>
struct ims_scope_dont_use
{
	F func;
	ims_scope_dont_use(F&& f) : func(std::forward<F>(f)) {}
	~ims_scope_dont_use() { func(); }
};
template<typename F> ims_scope_dont_use(F&&) -> ims_scope_dont_use<F>;
#define IMS_SCOPE(lambda) ims_scope_dont_use VARLINE(_scope_)(lambda)
////////////////////////////////////////////////////////////////////////////////
template<typename T, typename V> [[nodiscard]]
T ims_clamp(T x, V Min, V Max)
{
	return (x < (T)Min) ? (T)Min : (x > (T)Max) ? (T)Max : x;
}
template<typename Container, typename Pred>
void ims_erase(Container& t, Pred pred)
{
	t.erase(std::remove_if(t.begin(), t.end(), pred), t.end());
};
template <typename Container>
void ims_resize(Container& t, size_t sz)
{
	t.clear();
	t.resize(sz);
}
//reserve space for additional num elements
template<typename Vector>
void ims_geometric_reserve(Vector& v, size_t num)
{
	let new_cap = v.size() + num;
	if (new_cap > v.capacity()) {
		v.reserve(std::max(new_cap, v.capacity() * 3 / 2));
	}
}
////////////////////////////////////////////////////////////////////////////////
#else
#include <stddef.h>
#endif //__cplusplus

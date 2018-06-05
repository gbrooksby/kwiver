/*ckwg +29
 * Copyright 2018 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VITAL_RANGE_DEFS_H
#define VITAL_RANGE_DEFS_H

#include <iterator>
#include <tuple>

namespace kwiver {
namespace vital {
namespace range {

/**
 * \file
 * \brief core types and macros for implementing range utilities.
 */

/// \cond Internal

#define KWIVER_UNPACK_TOKENS(...) __VA_ARGS__

// ----------------------------------------------------------------------------
#define KWIVER_RANGE_ADAPTER_TEMPLATE( name, args, arg_names ) \
  template < KWIVER_UNPACK_TOKENS args > \
  struct name##_view_adapter_t \
  { \
    template < typename Range > \
    static name##_view< KWIVER_UNPACK_TOKENS arg_names, Range > \
    adapt( Range const& range ) \
    { return { range }; } \
  }; \
  \
  template < KWIVER_UNPACK_TOKENS args > inline constexpr \
  range_adapter_t< name##_view_adapter_t< KWIVER_UNPACK_TOKENS arg_names > > \
  name() { return {}; }

// ----------------------------------------------------------------------------
#define KWIVER_RANGE_ADAPTER_FUNCTION( name ) \
  template < typename Functor > \
  struct name##_view_adapter_t \
  { \
    template < typename Range > \
    name##_view< Functor, Range > \
    adapt( Range const& range ) const \
    { return { range, m_func }; } \
    \
    Functor m_func; \
  }; \
  \
  template < typename... Args > \
  constexpr name##_view_adapter_t< Args... > \
  name( Args... args ) \
  { return { args... }; } \
  \
  template < typename Range, typename... Args > \
  auto \
  operator|( \
    Range const& range, \
    name##_view_adapter_t< Args... > const& adapter ) \
  -> decltype( adapter.adapt( range ) ) \
  { \
    return adapter.adapt( range ); \
  }

// ----------------------------------------------------------------------------
struct generic_view {};

// ----------------------------------------------------------------------------
template < typename GenericAdapter >
struct range_adapter_t {};

/// \endcond

#ifdef DOXYGEN

// ----------------------------------------------------------------------------
/**
 * Apply a range adapter to a range.
 */
template < typename Range, typename Adapter >
auto
operator|( Range const&, Adapter );

#else

// ----------------------------------------------------------------------------
template < typename Range, typename Adapter >
auto
operator|(
  Range const& range,
  range_adapter_t< Adapter >(*)() )
-> decltype( Adapter::adapt( range ) )
{
  return Adapter::adapt( range );
}

// ----------------------------------------------------------------------------
template < typename Range, typename Adapter >
auto
operator|(
  Range const& range,
  range_adapter_t< Adapter > )
-> decltype( Adapter::adapt( range ) )
{
  return Adapter::adapt( range );
}

#endif

/// \cond Internal

// ----------------------------------------------------------------------------
template < typename Functor >
struct function_detail : function_detail< decltype( &Functor::operator() ) >
{};

// ----------------------------------------------------------------------------
template < typename ReturnType, typename... ArgsType >
struct function_detail< ReturnType (&)( ArgsType... ) >
{
  using arg_type_t = std::tuple< ArgsType... >;
  using return_type_t = ReturnType;
};

// ----------------------------------------------------------------------------
template < typename ReturnType, typename... ArgsType >
struct function_detail< ReturnType (*)( ArgsType... ) >
{
  using arg_type_t = std::tuple< ArgsType... >;
  using return_type_t = ReturnType;
};

// ----------------------------------------------------------------------------
template < typename Object, typename ReturnType, typename... ArgsType >
struct function_detail< ReturnType ( Object::* )( ArgsType... ) const >
{
  using arg_type_t = std::tuple< ArgsType... >;
  using return_type_t = ReturnType;
};

// ----------------------------------------------------------------------------
namespace range_detail {
  using namespace std;

  template < typename Range >
  struct range_helper
  {
    static auto begin_helper( Range& range )
    -> decltype( begin( range ) )
    {
      return begin( range );
    }

    static auto end_helper( Range& range )
    -> decltype( end( range ) )
    {
      return end( range );
    }

    using iterator_t = decltype( begin_helper( std::declval< Range >() ) );
  };
}

// ----------------------------------------------------------------------------
template < typename Range,
           bool = std::is_base_of< generic_view, Range >::value >
class range_ref : range_detail::range_helper< Range >
{
public:
  using iterator_t = typename range_detail::range_helper< Range >::iterator_t;
  using value_ref_t = decltype( *( std::declval< iterator_t >() ) );
  using value_t = typename std::remove_reference< value_ref_t >::type;

  range_ref( Range& range ) : m_range( range ) {}
  range_ref( range_ref const& ) = default;

  iterator_t begin() const { return detail::begin_helper( m_range ); }
  iterator_t end() const{ return detail::end_helper( m_range ); }

protected:
  using detail = range_detail::range_helper< Range >;

  Range& m_range;
};

/// \endcond

/// \cond DoxygenSuppress

// ----------------------------------------------------------------------------
template < typename Range >
class range_ref< Range, true >
{
public:
  using iterator_t = typename Range::iterator;
  using value_ref_t = decltype( *( std::declval< iterator_t >() ) );
  using value_t = typename std::remove_reference< value_ref_t >::type;

  range_ref( Range const& range ) : m_range( range ) {}
  range_ref( range_ref const& ) = default;

  iterator_t begin() const { return m_range.begin(); }
  iterator_t end() const { return m_range.end(); }

protected:
  typename std::remove_const< Range >::type m_range;
};

/// \endcond

} } } // end namespace

#endif

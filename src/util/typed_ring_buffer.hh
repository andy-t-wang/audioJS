#pragma once

#include "exception.hh"
#include "ring_buffer.hh"
#include "spans.hh"

#include <algorithm>

template<typename T>
class TypedRingStorage : public RingStorage
{
  static constexpr auto elem_size_ = sizeof( T );

protected:
  span<T> storage( const size_t index ) { return span_view<T> { RingStorage::storage( index * elem_size_ ) }; }
  span_view<T> storage( const size_t index ) const { return { RingStorage::storage( index * elem_size_ ) }; }

public:
  explicit TypedRingStorage( const size_t capacity )
    : RingStorage( capacity * elem_size_ )
  {}

  size_t capacity() const { return RingStorage::capacity() / elem_size_; }
};

template<typename T>
class EndlessBuffer : TypedRingStorage<T>
{
  size_t num_popped_ = 0;

  void check_bounds( const size_t pos, const size_t count ) const
  {
    if ( pos < range_begin() ) {
      throw std::out_of_range( std::to_string( pos ) + " < " + std::to_string( range_begin() ) );
    }

    if ( pos + count > range_end() ) {
      throw std::out_of_range( std::to_string( pos ) + " + " + std::to_string( count ) + " > "
                               + std::to_string( range_end() ) );
    }
  }

  size_t next_index_to_read() const { return num_popped_ % TypedRingStorage<T>::capacity(); }

  span_view<T> readable_region() const { return TypedRingStorage<T>::storage( next_index_to_read() ); }
  span<T> readable_region() { return TypedRingStorage<T>::storage( next_index_to_read() ); }

public:
  using TypedRingStorage<T>::TypedRingStorage;

  void pop( const size_t num_elems )
  {
    span<T> region_to_erase { readable_region().substr( 0, num_elems ) };
    std::fill( region_to_erase.begin(), region_to_erase.end(), T {} );
    num_popped_ += num_elems;
  }

  size_t range_begin() const { return num_popped_; }
  size_t range_end() const { return range_begin() + TypedRingStorage<T>::capacity(); }

  span<T> region( const size_t pos, const size_t count )
  {
    check_bounds( pos, count );
    return readable_region().substr( pos - range_begin(), count );
  }

  span_view<T> region( const size_t pos, const size_t count ) const
  {
    check_bounds( pos, count );
    return readable_region().substr( pos - range_begin(), count );
  }
};

template<typename T>
class SafeEndlessBuffer : public EndlessBuffer<T>
{
public:
  using EndlessBuffer<T>::EndlessBuffer;

  T safe_get( const size_t pos ) const
  {
    if ( pos < EndlessBuffer<T>::range_begin() or pos >= EndlessBuffer<T>::range_end() ) {
      return {};
    }

    return EndlessBuffer<T>::region( pos, 1 )[0];
  }

  void safe_set( const size_t pos, const T& val )
  {
    if ( pos < EndlessBuffer<T>::range_begin() or pos >= EndlessBuffer<T>::range_end() ) {
      return;
    }

    EndlessBuffer<T>::region( pos, 1 )[0] = val;
  }
};

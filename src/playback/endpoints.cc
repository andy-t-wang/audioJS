#include "endpoints.hh"
#include "timestamp.hh"

using namespace std;

NetworkClient::NetworkClient( const uint8_t node_id,
                              const Address& server,
                              const Base64Key& send_key,
                              const Base64Key& receive_key,
                              shared_ptr<OpusEncoderProcess> source,
                              shared_ptr<AudioDeviceTask> dest,
                              EventLoop& loop )
  : NetworkConnection( node_id, 0, send_key, receive_key, server )
  , socket_()
  , source_( source )
  , dest_( dest )
  , next_cursor_sample_( Timer::timestamp_ns() + cursor_sample_interval )
{
  socket_.set_blocking( false );

  loop.add_rule(
    "network transmit",
    [&] {
      push_frame( *source_ );
      send_packet( socket_ );
    },
    [&] { return source_->has_frame(); } );

  loop.add_rule( "network receive", socket_, Direction::In, [&] {
    Address src { nullptr, 0 };
    Ciphertext ciphertext;
    socket_.recv( src, ciphertext.data );
    receive_packet( src, ciphertext );
  } );

  /*
  loop.add_rule(
    "sample cursors",
    [&] {
      const uint64_t now = Timer::timestamp_ns();
      next_cursor_sample_ = now + cursor_sample_interval;

      cursor_.sample( *this, now, dest_->cursor() );

      if ( cursor_.sample_index().has_value() ) {
        const size_t amount_to_copy = cursor_.sample_index().value() - cursor_.output().ch1().range_begin();
        if ( amount_to_copy > 0 ) {
          const span_view<float> source1
            = cursor_.output().ch1().region( cursor_.output().ch1().range_begin(), amount_to_copy );
          const span_view<float> source2
            = cursor_.output().ch2().region( cursor_.output().ch2().range_begin(), amount_to_copy );
          span<float> dest1 = dest_->playback().ch1().region( dest_->cursor(), amount_to_copy );
          span<float> dest2 = dest_->playback().ch2().region( dest_->cursor(), amount_to_copy );

          memcpy( dest1.mutable_data(), source1.data(), source1.byte_size() );
          memcpy( dest2.mutable_data(), source2.data(), source1.byte_size() );

          cursor_.output().pop( amount_to_copy );
        }
      }
    },
    [&] { return Timer::timestamp_ns() >= next_cursor_sample_; } );
  */
}

NetworkSingleServer::NetworkSingleServer( EventLoop& loop, const Base64Key& send_key, const Base64Key& receive_key )
  : NetworkConnection( 0, 1, send_key, receive_key )
  , socket_ {}
  , global_ns_timestamp_at_creation_( Timer::timestamp_ns() )
  , last_server_clock_sample_( server_clock() )
  , peer_clock_( last_server_clock_sample_ )
{
  socket_.set_blocking( false );
  socket_.bind( { "0", 0 } );
  cout << "Port " << socket_.local_address().port() << " " << receive_key.printable_key().as_string_view() << " "
       << send_key.printable_key().as_string_view() << endl;

  loop.add_rule( "network receive", socket_, Direction::In, [&] {
    Address src { nullptr, 0 };
    Ciphertext ciphertext;
    socket_.recv( src, ciphertext.data );
    receive_packet( src, ciphertext );
    pop_frames( next_frame_needed() - frames().range_begin() );
    last_server_clock_sample_ = server_clock();
    peer_clock_.new_sample( last_server_clock_sample_,
                            unreceived_beyond_this_frame_index() * opus_frame::NUM_SAMPLES );
    send_packet( socket_ );
  } );

  loop.add_rule(
    "time passes",
    [&] {
      last_server_clock_sample_ = server_clock();
      peer_clock_.time_passes( last_server_clock_sample_ );
    },
    [&] { return server_clock() > last_server_clock_sample_ + opus_frame::NUM_SAMPLES; } );
}

NetworkSingleServer::NetworkSingleServer( EventLoop& loop )
  : NetworkSingleServer( loop, {}, {} )
{}

uint64_t NetworkSingleServer::server_clock() const
{
  return ( Timer::timestamp_ns() - global_ns_timestamp_at_creation_ ) * 48 / 1000000;
}

void NetworkSingleServer::summary( ostream& out ) const
{
  out << "Server clock: ";
  pp_samples( out, server_clock() );
  out << "\n";
  out << "Peer: ";
  peer_clock_.summary( out );

  NetworkConnection::summary( out );
}
#include <stdexcept>
#include <cstdlib>
#include <iostream>

#include "device_claim_util.hh"
#include "exception.hh"
using namespace std;

pair<string, string> find_device( const string_view expected_description )
{
  ALSADevices devices;
  bool found = false;

  string name, interface_name;

  for ( const auto& dev : devices.list() ) {
    for ( const auto& interface : dev.interfaces ) {
      if ( interface.second == expected_description ) {
        if ( found ) {
          throw runtime_error( "Multiple devices matching description" );
        } else {
          found = true;
          name = dev.name;
          interface_name = interface.first;
        }
      }
    }
  }

  if ( not found ) {
    throw runtime_error( "Device \"" + string( expected_description ) + "\" not found" );
  }

  return { name, interface_name };
}

#ifndef NDBUS
optional<AudioDeviceClaim> try_claim_ownership( const string_view name )
{
  try {
    AudioDeviceClaim ownership { name };

    cout << "Claimed ownership of " << name;
    if ( ownership.claimed_from() ) {
      cout << " from " << ownership.claimed_from().value();
    }
    cout << endl;

    return ownership;
  } catch ( const exception& e ) {
    cout << "Failed to claim ownership: " << e.what() << "\n";
    return {};
  }
}
#endif
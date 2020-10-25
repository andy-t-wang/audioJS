
#include <alsa/asoundlib.h>
#include <dbus/dbus.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class ALSADevices
{
public:
  struct Device
  {
    std::string name;
    std::vector<std::pair<std::string, std::string>> interfaces;
  };

  static std::vector<Device> list();
};

class AudioDeviceClaim
{
  class DBusConnectionWrapper
  {
    struct DBusConnection_deleter
    {
      void operator()( DBusConnection* x ) const;
    };
    std::unique_ptr<DBusConnection, DBusConnection_deleter> connection_;

  public:
    DBusConnectionWrapper( const DBusBusType type );
    operator DBusConnection*();
  };

  DBusConnectionWrapper connection_;

  std::optional<std::string> claimed_from_ {};

public:
  AudioDeviceClaim( const std::string_view name );

  const std::optional<std::string>& claimed_from() const { return claimed_from_; }
};

class AudioInterface
{
  std::string interface_name_, annotation_;
  snd_pcm_t* pcm_;

  void check_state( const snd_pcm_state_t expected_state );

  snd_pcm_sframes_t avail_ {}, delay_ {};

  unsigned int recoveries_ {};

  void initialize();

public:
  class Buffer
  {
    snd_pcm_t* pcm_;
    const snd_pcm_channel_area_t* areas_;
    unsigned int frame_count_;
    snd_pcm_uframes_t offset_;

  public:
    Buffer( AudioInterface& interface, const unsigned int sample_count );
    void commit( const unsigned int num_frames );
    void commit() { commit( frame_count_ ); }
    ~Buffer();

    int32_t& sample( const bool right_channel, const unsigned int sample_num )
    {
      return *( static_cast<int32_t*>( areas_[0].addr ) + right_channel + 2 * ( offset_ + sample_num ) );
    }

    /* can't copy or assign */
    Buffer( const Buffer& other ) = delete;
    Buffer& operator=( const Buffer& other ) = delete;

    unsigned int frame_count() const { return frame_count_; }
  };

  AudioInterface( const std::string_view interface_name,
                  const std::string_view annotation,
                  const snd_pcm_stream_t stream );

  void start();
  void prepare();
  void drop();
  void drain();
  void loopback_to( AudioInterface& other );
  void write_silence( const unsigned int sample_count );
  snd_pcm_state_t state() const;
  void recover();

  bool update();

  unsigned int avail() const { return avail_; }
  unsigned int delay() const { return delay_; }
  unsigned int recoveries() const { return recoveries_; }

  std::string name() const;

  ~AudioInterface();

  void link_with( AudioInterface& other );

  operator snd_pcm_t*() { return pcm_; }

  /* can't copy or assign */
  AudioInterface( const AudioInterface& other ) = delete;
  AudioInterface& operator=( const AudioInterface& other ) = delete;
};

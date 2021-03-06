#pragma once
#if __has_include(<jack/jack.h>) && !defined(__EMSCRIPTEN__)

#include <ossia/audio/audio_protocol.hpp>

#include <weak_libjack.h>

#if defined(_WIN32)
#include <TlHelp32.h>
#endif

#include <string_view>

#define USE_WEAK_JACK 1
#define NO_JACK_METADATA 1
#define OSSIA_AUDIO_JACK 1

namespace ossia
{

#if defined(_WIN32)
inline bool has_jackd_process()
{
  auto plist = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (plist == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32 entry;
  entry.dwSize = sizeof(PROCESSENTRY32);

  if (!Process32First(plist, &entry))
  {
    CloseHandle(plist);
    return false;
  }

  do
  {
    using namespace std::literals;

    const auto* name = entry.szExeFile;
#if !defined(UNICODE)
    if (name == std::string("jackd.exe"))
      return true;
#else
    if (name == std::wstring(L"jackd.exe"))
      return true;
#endif
  } while (Process32Next(plist, &entry));

  CloseHandle(plist);
  return false;
}
#endif

struct jack_client
{
  jack_client(std::string name) noexcept
  {
    client = jack_client_open(name.c_str(), JackNoStartServer, nullptr);
  }

  ~jack_client()
  {
    jack_client_close(client);
  }
  operator jack_client_t*() const noexcept { return client; }

  jack_client_t* client{};
};

class jack_engine final : public audio_engine
{
public:
  jack_engine(std::shared_ptr<jack_client> clt, int inputs, int outputs)
    : m_client{clt}
    , stopped{true}
  {
    if (!m_client)
    {
      std::cerr << "JACK server not running?" << std::endl;
      throw std::runtime_error("Audio error: no JACK server");
    }
    jack_client_t* client = *m_client;
    jack_set_process_callback(client, process, this);
    jack_set_sample_rate_callback(client, [] (jack_nframes_t nframes, void *arg) -> int { return 0; }, this);
    jack_on_shutdown(client, JackShutdownCallback{}, this);
    jack_set_error_function([](const char* str) {
      std::cerr << "JACK ERROR: " << str << std::endl;
    });
    jack_set_info_function([](const char* str) {
      // std::cerr << "JACK INFO: " << str << std::endl;
    });

    for (int i = 0; i < inputs; i++)
    {
      auto in = jack_port_register(
          client, ("in_" + std::to_string(i + 1)).c_str(),
          JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
      if(!in)
      {
        jack_deactivate(client);
        jack_client_close(client);
        throw std::runtime_error("Audio error: cannot register JACK input");
      }
      input_ports.push_back(in);
    }
    for (int i = 0; i < outputs; i++)
    {
      auto out = jack_port_register(
          client, ("out_" + std::to_string(i + 1)).c_str(),
          JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
      if(!out)
      {
        jack_deactivate(client);
        jack_client_close(client);
        throw std::runtime_error("Audio error: cannot register JACK output");
      }
      output_ports.push_back(out);
    }

    std::cerr << "=== stream start ===\n";

    int err = jack_activate(client);
    if (err != 0)
    {
      jack_deactivate(client);
      jack_client_close(client);
      std::cerr << "JACK error: " << err << std::endl;
      throw std::runtime_error("Audio error: JACK cannot activate");
    }

    {
      auto ports = jack_get_ports(
          client, nullptr, JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsPhysical | JackPortIsOutput);
      if (ports)
      {
        for (std::size_t i = 0; i < input_ports.size() && ports[i]; i++)
          jack_connect(client, ports[i], jack_port_name(input_ports[i]));

        jack_free(ports);
      }
    }
    {
      auto ports = jack_get_ports(
          client, nullptr, JACK_DEFAULT_AUDIO_TYPE,
          JackPortIsPhysical | JackPortIsInput);
      if (ports)
      {
        for (std::size_t i = 0; i < output_ports.size() && ports[i]; i++)
          jack_connect(client, jack_port_name(output_ports[i]), ports[i]);

        jack_free(ports);
      }
    }

    stopped = false;
  }

  ~jack_engine() override
  {
    stop();
    if (protocol)
      protocol.load()->engine = nullptr;

    if(m_client)
    {
      jack_client_t* client = *m_client;
      jack_deactivate(client);
    }
  }

  void reload(audio_protocol* p) override
  {
    stop();
    this->protocol = p;

    if (!p)
      return;
    auto& proto = *p;
    proto.engine = this;
    std::cerr << "=== STARTING PROCESS ==" << std::endl;

    proto.setup_tree((int)input_ports.size(), (int)output_ports.size());

    stop_processing = false;
  }
  void stop() override
  {
    std::cerr << "=== STOPPED PROCESS ==" << std::endl;
    if (this->protocol)
      this->protocol.load()->engine = nullptr;
    stop_processing = true;
    protocol = nullptr;
    while (processing)
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    stopped = true;
  }

  bool running() const override
  {
    return !stopped;
  }

private:
  static int clearBuffers(jack_engine& self, jack_nframes_t nframes, std::size_t outputs)
  {
    for (std::size_t i = 0; i < outputs; i++)
    {
      auto chan = (jack_default_audio_sample_t*)jack_port_get_buffer(
          self.output_ports[i], nframes);
      for (std::size_t j = 0; j < nframes; j++)
        chan[j] = 0.f;
    }

    return 0;
  }
  static int process(jack_nframes_t nframes, void* arg)
  {
    auto& self = *static_cast<jack_engine*>(arg);
    const auto inputs = self.input_ports.size();
    const auto outputs = self.output_ports.size();
    auto proto = self.protocol.load();
    if (self.stop_processing)
    {
      return clearBuffers(self, nframes, outputs);
    }

    if (proto)
    {
      self.processing = true;
      auto float_input = (float**)alloca(sizeof(float*) * inputs);
      auto float_output = (float**)alloca(sizeof(float*) * outputs);
      for (std::size_t i = 0; i < inputs; i++)
      {
        float_input[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(
            self.input_ports[i], nframes);
      }
      for (std::size_t i = 0; i < outputs; i++)
      {
        float_output[i] = (jack_default_audio_sample_t*)jack_port_get_buffer(
            self.output_ports[i], nframes);
      }

      proto->process_generic(
          *proto, float_input, float_output, (int)inputs, (int)outputs,
          nframes);
      proto->audio_tick(nframes, 0);

      // Run a tick
      if (proto->replace_tick)
      {
        proto->audio_tick = std::move(proto->ui_tick);
        proto->ui_tick = {};
        proto->replace_tick = false;
      }
      self.processing = false;
    }
    else
    {
      return clearBuffers(self, nframes, outputs);
    }
    return 0;
  }

  std::shared_ptr<jack_client> m_client{};
  std::vector<jack_port_t*> input_ports;
  std::vector<jack_port_t*> output_ports;
  std::atomic_bool stopped{};
};
}

#endif

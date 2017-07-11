#pragma once
#include <Device/Protocol/ProtocolFactoryInterface.hpp>
#include <Engine/Protocols/OSSIADevice.hpp>
#include <Device/Protocol/ProtocolSettingsWidget.hpp>
#include <Device/Protocol/DeviceSettings.hpp>
#include <ossia/network/base/protocol.hpp>
#include <portaudio.h>
#include <ossia/dataflow/audio_address.hpp>
class QLineEdit;
template<typename Address>
auto create_address(ossia::net::node_base& root, std::string name)
{
  auto& node = ossia::net::create_node(root, name);
  auto addr = new Address(node);
  node.set_address(std::unique_ptr<Address>(addr));
  return addr;
}
namespace Pd
{
class AudioProtocolFactory final : public Device::ProtocolFactory
{
    ISCORE_CONCRETE("2835e6da-9b55-4029-9802-e1c817acbdc1")
    QString prettyName() const override;

    Device::DeviceInterface* makeDevice(
        const Device::DeviceSettings& settings,
        const iscore::DocumentContext& ctx) override;
    const Device::DeviceSettings& defaultSettings() const override;

    Device::ProtocolSettingsWidget* makeSettingsWidget() override;

    QVariant
    makeProtocolSpecificSettings(const VisitorVariant& visitor) const override;

    void serializeProtocolSpecificSettings(
        const QVariant& data, const VisitorVariant& visitor) const override;

    bool checkCompatibility(
        const Device::DeviceSettings& a,
        const Device::DeviceSettings& b) const override;
};

class AudioDevice final : public Engine::Network::OSSIADevice
{
    Q_OBJECT
  public:
    AudioDevice(const Device::DeviceSettings& settings, const iscore::DocumentContext& c);

    bool reconnect() override;
    void recreate(const Device::Node& n) override;
    ossia::net::device_base* getDevice() const override
    {
      return &m_dev;
    }

  private:
    Device::Node refresh() override;
    void disconnect() override;
    ossia::net::device_base& m_dev;
};


class AudioSettingsWidget final
    : public Device::ProtocolSettingsWidget
{
  public:
    AudioSettingsWidget(QWidget* parent = nullptr);

    Device::DeviceSettings getSettings() const override;

    void setSettings(const Device::DeviceSettings& settings) override;

  private:
    void setDefaults();
    QLineEdit* m_deviceNameEdit{};
};

struct AudioSpecificSettings
{
    QString card;
    int rate{44100};
    int bufferSize{64};
};

class audio_protocol : public ossia::net::protocol_base
{
  public:
    int rate{44100};
    int bufferSize{64};
    int inputs{};
    int outputs{};

    std::atomic_bool replace_tick{false};
    std::function<void(unsigned long)> ui_tick;
    std::function<void(unsigned long)> audio_tick;
    static int PortAudioCallback(
        const void *input, void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userData)
    {
      using idx_t = gsl::span<float>::index_type;
      const idx_t fc = frameCount;
      auto& self = *static_cast<audio_protocol*>(userData);

      auto float_input = ((float **) input);
      auto float_output = ((float **) output);

      // Prepare audio inputs
      const int n_in_channels = self.audio_ins.size();
      for(int i = 0; i < n_in_channels; i++)
      {
        self.audio_ins[i]->audio = {float_input[i], fc};
      }

      // Prepare audio outputs
      const int n_out_channels = self.audio_outs.size();
      for(int i = 0; i < n_out_channels; i++)
      {
        self.audio_outs[i]->audio = {float_output[i], fc};

        for(int j = 0; j < (int)frameCount; j++)
        {
          float_output[i][j] = 0;
       }
      }

      // Run a tick
      if(self.replace_tick)
      {
        self.audio_tick = std::move(self.ui_tick);
        self.ui_tick = {};
        self.replace_tick = false;
      }
      if(self.audio_tick)
      {
        self.audio_tick(frameCount);
      }
      return paContinue;
    }

    audio_protocol()
    {
      if(Pa_Initialize() != paNoError)
        throw std::runtime_error("Audio error");
    }

    ~audio_protocol()
    {
      stop();
      Pa_Terminate();
    }

    bool pull(ossia::net::address_base&) override
    {
      return false;
    }
    bool push(const ossia::net::address_base&) override
    {
      return false;
    }
    bool observe(ossia::net::address_base&, bool) override
    {
      return false;
    }
    bool update(ossia::net::node_base& node_base) override
    {
      return false;
    }
    void set_device(ossia::net::device_base& dev) override
    {
      m_dev = &dev;
    }

    void stop()
    {
      if(m_stream)
      {
        Pa_StopStream(m_stream);
        m_stream = nullptr;
      }
    }

    void reload()
    {
      auto inputInfo = Pa_GetDeviceInfo(Pa_GetDefaultInputDevice());
      auto outputInfo = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice());
      inputs = inputInfo->maxInputChannels;
      outputs = outputInfo->maxOutputChannels;

      audio_ins.clear();
      audio_outs.clear();
      m_dev->get_root_node().clear_children();

      for(int i = 0; i < inputs; i++)
      {
        audio_ins.push_back(create_address<ossia::audio_address>(m_dev->get_root_node(), "/in/" + std::to_string(i)));
      }
      for(int i = 0; i < outputs; i++)
      {
        audio_outs.push_back(create_address<ossia::audio_address>(m_dev->get_root_node(), "/out/" + std::to_string(i)));
      }

      PaStreamParameters inputParameters;
      inputParameters.device = Pa_GetDefaultInputDevice();
      inputParameters.channelCount = inputs;
      inputParameters.sampleFormat = paFloat32 | paNonInterleaved;
      inputParameters.suggestedLatency = 0.01;
      inputParameters.hostApiSpecificStreamInfo = nullptr;

      PaStreamParameters outputParameters;
      outputParameters.device = Pa_GetDefaultOutputDevice();
      outputParameters.channelCount = outputs;
      outputParameters.sampleFormat = paFloat32 | paNonInterleaved;
      outputParameters.suggestedLatency = 0.01;
      outputParameters.hostApiSpecificStreamInfo = nullptr;

      auto ec = Pa_OpenStream(&m_stream,
                              &inputParameters,
                              &outputParameters,
                              rate,
                              bufferSize,
                              paNoFlag,
                              &PortAudioCallback,
                              this);
      if(ec == PaErrorCode::paNoError)
        Pa_StartStream( m_stream );
      else
        std::cerr << "Error while opening audio stream: " << ec << std::endl;

    }

    PaStream* stream() { return m_stream; }
  private:
    ossia::net::device_base* m_dev{};
    PaStream* m_stream{};

    std::vector<ossia::audio_address*> audio_ins;
    std::vector<ossia::audio_address*> audio_outs;
};
}
Q_DECLARE_METATYPE(Pd::AudioSpecificSettings)

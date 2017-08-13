#include "AudioDevice.hpp"
#include <ossia/network/generic/generic_address.hpp>
#include <ossia/network/generic/generic_device.hpp>
#include <State/Widgets/AddressFragmentLineEdit.hpp>
#include <portaudio.h>
#include <QLineEdit>
#include <QFormLayout>
#include <Pd/DocumentPlugin.hpp>
namespace Pd
{
AudioDevice::AudioDevice(const Device::DeviceSettings& settings, const iscore::DocumentContext& c)
  : OSSIADevice{settings}
  , m_dev{c.plugin<Dataflow::DocumentPlugin>().audio_dev}
{
  m_capas.canAddNode = false;
  m_capas.canRemoveNode = false;
  m_capas.canRenameNode = false;
  m_capas.canSetProperties = false;
  m_capas.canRefreshTree = true;
  m_capas.canRefreshValue = false;
  m_capas.hasCallbacks = false;
  m_capas.canListen = false;
  m_capas.canSerialize = false;

  reconnect();
}


void AudioDevice::disconnect()
{
  // TODO handle listening ??
  setLogging_impl(false);
}

bool AudioDevice::reconnect()
{
  disconnect();

  try
  {
    AudioSpecificSettings stgs
        = settings().deviceSpecificSettings.value<AudioSpecificSettings>();

    auto& proto = static_cast<audio_protocol&>(m_dev.get_protocol());
    proto.rate = stgs.rate;
    proto.bufferSize = stgs.bufferSize;
    proto.reload();

    setLogging_impl(isLogging());
  }
  catch (std::exception& e)
  {
    qDebug() << "Could not connect: " << e.what();
  }
  catch (...)
  {
    // TODO save the reason of the non-connection.
  }

  return connected();
}

void AudioDevice::recreate(const Device::Node& n)
{
  for(auto& child : n)
  {
    addNode(child);
  }
}

Device::Node AudioDevice::refresh()
{
  return simple_refresh();
}

QString AudioProtocolFactory::prettyName() const
{
  return QObject::tr("Audio");
}

Device::DeviceInterface* AudioProtocolFactory::makeDevice(
    const Device::DeviceSettings& settings, const iscore::DocumentContext& ctx)
{
  return new AudioDevice{settings, ctx};
}

const Device::DeviceSettings& AudioProtocolFactory::defaultSettings() const
{
  static const Device::DeviceSettings settings = [&]() {
    Device::DeviceSettings s;
    s.protocol = concreteKey();
    s.name = "Audio";
    AudioSpecificSettings specif;
    s.deviceSpecificSettings = QVariant::fromValue(specif);
    return s;
  }();
  return settings;
}

Device::ProtocolSettingsWidget* AudioProtocolFactory::makeSettingsWidget()
{
  return new AudioSettingsWidget;
}

QVariant AudioProtocolFactory::makeProtocolSpecificSettings(
    const VisitorVariant& visitor) const
{
  return makeProtocolSpecificSettings_T<AudioSpecificSettings>(visitor);
}

void AudioProtocolFactory::serializeProtocolSpecificSettings(
    const QVariant& data, const VisitorVariant& visitor) const
{
  serializeProtocolSpecificSettings_T<AudioSpecificSettings>(data, visitor);
}

bool AudioProtocolFactory::checkCompatibility(
    const Device::DeviceSettings& a, const Device::DeviceSettings& b) const
{
  return a.name != b.name;
}


AudioSettingsWidget::AudioSettingsWidget(QWidget* parent)
  : ProtocolSettingsWidget(parent)
{
  m_deviceNameEdit = new State::AddressFragmentLineEdit{this};

  auto layout = new QFormLayout;
  layout->addRow(tr("Device Name"), m_deviceNameEdit);

  setLayout(layout);

  setDefaults();
}

void AudioSettingsWidget::setDefaults()
{
  m_deviceNameEdit->setText("audio");
}

Device::DeviceSettings AudioSettingsWidget::getSettings() const
{
  Device::DeviceSettings s;
  s.name = m_deviceNameEdit->text();

  AudioSpecificSettings audio;
  audio.card = "default";
  audio.rate = 44100;
  audio.bufferSize = 64;

  s.deviceSpecificSettings = QVariant::fromValue(audio);
  return s;
}

void AudioSettingsWidget::setSettings(
    const Device::DeviceSettings& settings)
{
  m_deviceNameEdit->setText(settings.name);
  AudioSpecificSettings audio;
  if (settings.deviceSpecificSettings
      .canConvert<AudioSpecificSettings>())
  {
    audio = settings.deviceSpecificSettings
            .value<AudioSpecificSettings>();
  }
}

int audio_protocol::PortAudioCallback(
    const void* input,
    void* output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
  using idx_t = gsl::span<float>::index_type;
  const idx_t fc = frameCount;
  auto& self = *static_cast<audio_protocol*>(userData);

  auto float_input = ((float **) input);
  auto float_output = ((float **) output);

  // Prepare audio inputs
  const int n_in_channels = self.audio_ins.size();
  self.main_audio_in->audio.resize(n_in_channels);
  for(int i = 0; i < n_in_channels; i++)
  {
    self.main_audio_in->audio[i] = {float_input[i], fc};

    self.audio_ins[i]->audio.resize(1);
    self.audio_ins[i]->audio[0] = {float_input[i], fc};
  }

  // Prepare audio outputs
  const int n_out_channels = self.audio_outs.size();
  self.main_audio_out->audio.resize(n_out_channels);
  for(int i = 0; i < n_out_channels; i++)
  {
    self.main_audio_out->audio[i] = {float_input[i], fc};
    self.audio_outs[i]->audio.resize(1);
    self.audio_outs[i]->audio[0] = {float_output[i], fc};

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

}


template <>
void DataStreamReader::read(
    const Pd::AudioSpecificSettings& n)
{
  m_stream << n.card << n.bufferSize << n.rate;
  insertDelimiter();
}


template <>
void DataStreamWriter::write(
    Pd::AudioSpecificSettings& n)
{
  m_stream >> n.card >> n.bufferSize >> n.rate;
  checkDelimiter();
}


template <>
void JSONObjectReader::read(
    const Pd::AudioSpecificSettings& n)
{
  obj["Card"] = n.card;
  obj["BufferSize"] = n.bufferSize;
  obj["Rate"] = n.rate;
}


template <>
void JSONObjectWriter::write(
    Pd::AudioSpecificSettings& n)
{
  n.card = obj["Card"].toString();
  n.bufferSize = obj["BufferSize"].toInt();
  n.rate = obj["Rate"].toInt();
}


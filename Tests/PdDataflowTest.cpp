#include <QtTest/QTest>
#include <ossia/dataflow/dataflow.hpp>
#include <z_libpd.h>
#include <ossia/network/generic/generic_device.hpp>
#include <ossia/network/local/local.hpp>
#include <ossia/editor/scenario/scenario.hpp>
#include <ossia/editor/scenario/time_node.hpp>
#include <ossia/editor/scenario/time_constraint.hpp>
#include <ossia/editor/scenario/time_event.hpp>
#include <ossia/editor/scenario/time_value.hpp>
#include <sndfile.hh>
#include <thread>

struct libpd_list_wrapper
{
  t_atom* impl{};
  int size{};

  template<typename Functor>
  void iterate(const Functor& f) const {
    for(auto it = impl; it; it = libpd_next_atom(it))
    {
      if(libpd_is_float(it))
        f(libpd_get_float(it));
      else if(libpd_is_symbol(it))
        f(libpd_get_symbol(it));
    }
  }

  struct value_visitor
  {
    std::vector<ossia::value>& v;
    void operator()(float f) const
    { v.push_back(f); }
    void operator()(const char* s) const
    { v.push_back(std::string(s)); }
  };

  std::vector<ossia::value> to_tuple()
  {
    std::vector<ossia::value> vals;
    vals.reserve(size);
    iterate(value_visitor{vals});
    return vals;
  }
};

class pd_graph_node final :
    public ossia::graph_node
{
public:
  pd_graph_node(
      ossia::string_view folder,
      ossia::string_view file,
      std::size_t inputs,
      std::size_t outputs,
      std::vector<std::string> in_val,
      std::vector<std::string> out_val,
      bool midi_in = true,
      bool midi_out = true
      )
    : m_inputs{inputs}
    , m_outputs{outputs}
    , m_inmess{std::move(in_val)}
    , m_outmess{std::move(out_val)}
    , m_file{file}
  {
    m_currentInstance = nullptr;

    // Set-up buffers
    const std::size_t bs = libpd_blocksize();
    m_inbuf.resize(m_inputs * bs);
    m_outbuf.resize(m_outputs * bs);

    // Create instance
    m_instance = pdinstance_new();
    pd_setinstance(m_instance);

    // Enable audio
    libpd_start_message(1);
    libpd_add_float(1.0f);
    libpd_finish_message("pd", "dsp");

    // Open
    auto handle = libpd_openfile(file.data(), folder.data());
    m_dollarzero = libpd_getdollarzero(handle);

    for(auto& mess : m_inmess)
      add_dzero(mess);
    for(auto& mess : m_outmess)
      add_dzero(mess);

    // Create correct I/Os
    m_inlets.reserve(inputs + m_inmess.size() + int(midi_in));
    for(std::size_t i = 0U; i < inputs ; i++)
      m_inlets.push_back(ossia::make_inlet<ossia::audio_port>());
    for(auto& str : m_inmess)
    {
      m_inlets.push_back(ossia::make_inlet<ossia::value_port>());
    }

    m_outlets.reserve(outputs + m_outmess.size()+ int(midi_out));
    for(std::size_t i = 0U; i < outputs ; i++)
      m_outlets.push_back(ossia::make_outlet<ossia::audio_port>());
    for(auto& mess : m_outmess)
    {
      libpd_bind(mess.c_str());
      m_outlets.push_back(ossia::make_outlet<ossia::value_port>());
    }

    if(midi_in)
    {
      auto mid = ossia::make_inlet<ossia::midi_port>();
      m_midi_inlet = mid->data.target<ossia::midi_port>();
      m_inlets.push_back(std::move(mid));
    }
    if(midi_out)
    {
      auto mid = ossia::make_outlet<ossia::midi_port>();
      m_midi_outlet = mid->data.target<ossia::midi_port>();
      m_outlets.push_back(std::move(mid));
    }

    // Set-up message callbacks
    libpd_set_floathook([] (const char *recv, float f) {
      if(auto v = m_currentInstance->get_value_port(recv))
      {
        v->data.push_back(f);
      }
    });
    libpd_set_banghook([] (const char *recv) {
      if(auto v = m_currentInstance->get_value_port(recv))
      {
        v->data.push_back(ossia::impulse{});
      }
    });
    libpd_set_symbolhook([] (const char *recv, const char *sym) {
      if(auto v = m_currentInstance->get_value_port(recv))
      {
        v->data.push_back(std::string(sym));
      }
    });

    libpd_set_listhook([] (const char *recv, int argc, t_atom *argv) {
      if(auto v = m_currentInstance->get_value_port(recv))
      {
        v->data.push_back(libpd_list_wrapper{argv, argc}.to_tuple());
      }
    });
    libpd_set_messagehook([] (const char *recv, const char *msg, int argc, t_atom *argv) {
      if(auto v = m_currentInstance->get_value_port(recv))
      {
        v->data.push_back(libpd_list_wrapper{argv, argc}.to_tuple());
      }
    });

    // TODO
    libpd_set_noteonhook([] (int channel, int pitch, int velocity) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(
              (velocity > 0)
              ? mm::MakeNoteOn(channel, pitch, velocity)
              : mm::MakeNoteOff(channel, pitch, velocity)
                );
      }
    });
    libpd_set_controlchangehook([] (int channel, int controller, int value) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(mm::MakeControlChange(channel, controller, value + 8192));
      }
    });

    libpd_set_programchangehook([] (int channel, int value) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(mm::MakeProgramChange(channel, value));
      }
    });
    libpd_set_pitchbendhook([] (int channel, int value) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(mm::MakePitchBend(channel, value));
      }
    });
    libpd_set_aftertouchhook([] (int channel, int value) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(mm::MakeAftertouch(channel, value));
      }
    });
    libpd_set_polyaftertouchhook([] (int channel, int pitch, int value) {
      if(auto v = m_currentInstance->get_midi_out())
      {
        v->messages.push_back(mm::MakePolyPressure(channel, pitch, value));
      }
    });
    libpd_set_midibytehook([] (int port, int byte) {
      // TODO
    });


  }

  ~pd_graph_node()
  {
    pdinstance_free(m_instance);
  }

private:
  ossia::outlet* get_outlet(const char* str) const
  {
    ossia::string_view s = str;
    auto it = ossia::find(m_outmess, s);
    if(it != m_outmess.end())
      return m_outlets[std::distance(m_outmess.begin(), it) + m_outputs].get();
    return nullptr;
  }

  ossia::value_port* get_value_port(const char* str) const
  {
    return get_outlet(str)->data.target<ossia::value_port>();
  }

  ossia::midi_port* get_midi_in() const
  {
    return m_midi_inlet;
  }
  ossia::midi_port* get_midi_out() const
  {
    return m_midi_outlet;
  }


  struct ossia_to_pd_value
  {
    const char* mess{};
    void operator()() const { }
    template<typename T>
    void operator()(const T&) const { }

    void operator()(float f) const { libpd_float(mess, f); }
    void operator()(int f) const { libpd_float(mess, f); }
    void operator()(const std::string& f) const { libpd_symbol(mess, f.c_str()); }
    void operator()(const ossia::impulse& f) const { libpd_bang(mess); }

    // TODO convert other types
  };

  void run(ossia::execution_state& e) override
  {
    // Setup
    pd_setinstance(m_instance);
    m_currentInstance = this;
    libpd_init_audio(m_inputs, m_outputs, 44100);

    const auto bs = libpd_blocksize();

    // Copy audio inputs
    for(std::size_t i = 0U; i < m_inputs; i++)
    {
      auto ap = m_inlets[i]->data.target<ossia::audio_port>();
      const auto pos = i * bs;
      if(!ap->samples.empty())
      {
        auto& arr = ap->samples.back();
        std::copy_n(arr.begin(), bs, m_inbuf.begin() + pos);
      }
    }

    // Copy midi inputs
    if(m_midi_inlet)
    {
      auto& dat = m_midi_inlet->messages;
      for(const auto& mess : dat)
      {
        switch(mess.getMessageType())
        {
          case mm::MessageType::NOTE_OFF:
            libpd_noteon(mess.getChannel(), mess.data[1], 0);
            break;
          case mm::MessageType::NOTE_ON:
            libpd_noteon(mess.getChannel(), mess.data[1], mess.data[2]);
            break;
          case mm::MessageType::POLY_PRESSURE:
            libpd_polyaftertouch(mess.getChannel(), mess.data[1], mess.data[2]);
            break;
          case mm::MessageType::CONTROL_CHANGE:
            libpd_controlchange(mess.getChannel(), mess.data[1], mess.data[2]);
            break;
          case mm::MessageType::PROGRAM_CHANGE:
            libpd_programchange(mess.getChannel(), mess.data[1]);
            break;
          case mm::MessageType::AFTERTOUCH:
            libpd_aftertouch(mess.getChannel(), mess.data[1]);
            break;
          case mm::MessageType::PITCH_BEND:
            libpd_pitchbend(mess.getChannel(), mess.data[1] - 8192);
            break;
          case mm::MessageType::INVALID:
          default:
            break;
        }
      }

      dat.clear();
    }

    // Copy message inputs
    for(std::size_t i = m_inputs; i < m_inlets.size(); ++i)
    {
      auto& dat = m_inlets[i]->data.target<ossia::value_port>()->data;

      auto mess = m_inmess[i - m_inputs].c_str();
      for(const auto& val : dat)
      {
        val.apply(ossia_to_pd_value{mess});
      }
      dat.clear();
    }

    // Process
    libpd_process_raw(m_inbuf.data(), m_outbuf.data());

    // Copy audio outputs. Message inputs are copied in callbacks.
    for(std::size_t i = 0U; i < m_outputs; ++i)
    {
      auto ap = m_outlets[i]->data.target<ossia::audio_port>();
      ap->samples.resize(ap->samples.size() + 1);

      std::copy_n(m_outbuf.begin() + i * bs, bs, ap->samples.back().begin());
    }

    // Teardown
    m_currentInstance = nullptr;
  }

  void add_dzero(std::string& s) const
  {
    s = std::to_string(m_dollarzero) + "-" + s;
  }

  static pd_graph_node* m_currentInstance;

  t_pdinstance * m_instance{};
  int m_dollarzero = 0;

  std::size_t m_inputs{}, m_outputs{};
  std::vector<std::string> m_inmess, m_outmess;
  std::vector<float> m_inbuf, m_outbuf;
  ossia::midi_port* m_midi_inlet{};
  ossia::midi_port* m_midi_outlet{};
  std::string m_file;

};

pd_graph_node* pd_graph_node::m_currentInstance;

class PdDataflowTest : public QObject
{
  Q_OBJECT

public:
  PdDataflowTest()
  {
    libpd_init();
    libpd_set_printhook([] (const char *s) { qDebug() << "string: " << s; });
  }

private slots:
  void test_pd()
  {
    using namespace ossia; using namespace ossia::net;
    generic_device dev{std::make_unique<local_protocol>(), ""};
    auto& filt_node = create_node(dev.getRootNode(), "/filter");
    auto filt_addr = filt_node.createAddress(ossia::val_type::FLOAT);
    filt_addr->pushValue(100);
    auto& vol_node = create_node(dev.getRootNode(), "/param");
    auto vol_addr = vol_node.createAddress(ossia::val_type::FLOAT);
    vol_addr->pushValue(0.5);
    auto& note_node = create_node(dev.getRootNode(), "/note");
    auto note_addr = note_node.createAddress(ossia::val_type::FLOAT);
    note_addr->pushValue(220.);
    auto& l_node = create_node(dev.getRootNode(), "/l");
    auto l_addr = l_node.createAddress(ossia::val_type::IMPULSE);
    auto& r_node = create_node(dev.getRootNode(), "/r");
    auto r_addr = r_node.createAddress(ossia::val_type::IMPULSE);

    auto graph = std::make_shared<ossia::graph>();
    ossia::graph& g = *graph;
    using string_vec = std::vector<std::string>;
    auto cwd = getcwd(NULL, 0);
    auto f1 = std::make_shared<pd_graph_node>(cwd, "gen1.pd", 0, 0, string_vec{}, string_vec{"out-1"});
    auto f1_2 = std::make_shared<pd_graph_node>(cwd, "gen1.pd", 0, 0, string_vec{}, string_vec{"out-1"});
    auto f2 = std::make_shared<pd_graph_node>(cwd, "gen2.pd", 0, 0, string_vec{"in-1"}, string_vec{"out-1"});
    auto f3 = std::make_shared<pd_graph_node>(cwd, "gen3.pd", 0, 2, string_vec{"in-1", "in-2"}, string_vec{});
    auto f4 = std::make_shared<pd_graph_node>(cwd, "gen4.pd", 2, 2, string_vec{"in-2"}, string_vec{});
    auto f5 = std::make_shared<pd_graph_node>(cwd, "gen5.pd", 2, 2, string_vec{"in-2"}, string_vec{});
    auto f5_2 = std::make_shared<pd_graph_node>(cwd, "gen5.pd", 2, 2, string_vec{"in-2"}, string_vec{});

    g.add_node(f1);
    g.add_node(f1_2);
    g.add_node(f2);
    g.add_node(f3);
    g.add_node(f4);
    g.add_node(f5);
    g.add_node(f5_2);

    f1->outputs()[0]->address = note_addr;
    f1_2->outputs()[0]->address = note_addr;
    f2->outputs()[0]->address = note_addr;

    f3->inputs()[0]->address = note_addr;
    f3->inputs()[1]->address = vol_addr;
    f3->outputs()[0]->address = l_addr;
    f3->outputs()[1]->address = r_addr;

    f4->inputs()[0]->address = l_addr;
    f4->inputs()[1]->address = r_addr;
    f4->inputs()[2]->address = filt_addr;
    f4->outputs()[0]->address = l_addr;
    f4->outputs()[1]->address = r_addr;

    f5->inputs()[0]->address = l_addr;
    f5->inputs()[1]->address = r_addr;
    f5->inputs()[2]->address = filt_addr;
    f5->outputs()[0]->address = l_addr;
    f5->outputs()[1]->address = r_addr;

    f5_2->inputs()[0]->address = l_addr;
    f5_2->inputs()[1]->address = r_addr;
    f5_2->inputs()[2]->address = filt_addr;
    f5_2->outputs()[0]->address = l_addr;
    f5_2->outputs()[1]->address = r_addr;


    g.connect(make_edge(immediate_strict_connection{}, f3->outputs()[0], f4->inputs()[0], f3, f4));
    g.connect(make_edge(immediate_strict_connection{}, f3->outputs()[1], f4->inputs()[1], f3, f4));

    g.connect(make_edge(immediate_glutton_connection{}, f4->outputs()[0], f5->inputs()[0], f4, f5));
    g.connect(make_edge(immediate_glutton_connection{}, f4->outputs()[1], f5->inputs()[1], f4, f5));

    g.connect(make_edge(delayed_strict_connection{}, f4->outputs()[0], f5_2->inputs()[0], f4, f5_2));
    g.connect(make_edge(delayed_strict_connection{}, f4->outputs()[1], f5_2->inputs()[1], f4, f5_2));

    std::vector<float> samples;

    execution_state st;
    auto copy_samples = [&] {
      auto it_l = st.localState.find(l_addr);
      auto it_r = st.localState.find(r_addr);
      if(it_l == st.localState.end() || it_r == st.localState.end())
      {
        // Just copy silence
        samples.resize(samples.size() + 128);
        return;
      }

      auto audio_l = it_l->second.target<audio_port>();
      auto audio_r = it_r->second.target<audio_port>();
      QVERIFY(audio_l);
      QVERIFY(!audio_l->samples.empty());
      QVERIFY(audio_r);
      QVERIFY(!audio_r->samples.empty());

      auto pos = samples.size();
      samples.resize(samples.size() + 128);
      for(auto& arr : audio_l->samples)
      {
        for(int i = 0; i < 64; i++)
          samples[pos + i * 2] += arr[i];
      }
      for(auto& arr : audio_r->samples)
      {
        for(int i = 0; i < 64; i++)
          samples[pos + i * 2 + 1] += arr[i];
      }
    };

    // Create an ossia scenario
    auto main_start_node = std::make_shared<time_node>();
    auto main_end_node = std::make_shared<time_node>();

    // create time_events inside TimeNodes and make them interactive to the /play address
    auto main_start_event = *(main_start_node->emplace(main_start_node->timeEvents().begin(), {}));
    auto main_end_event = *(main_end_node->emplace(main_end_node->timeEvents().begin(), {}));

    const time_value granul{1000. * 64. / 44100.};
    // create the main time_constraint

    auto cb = [] (time_value t0, time_value, const state&) {
    };

    auto cb_2 = [] (time_value t0, time_value, const state&) {
    };
    auto main_constraint = std::make_shared<time_constraint>(
          cb,
          *main_start_event,
          *main_end_event,
          20000_tv,
          20000_tv,
          20000_tv);

    auto main_scenario_ptr = std::make_unique<scenario>();
    scenario& main_scenario = *main_scenario_ptr;
    main_constraint->addTimeProcess(std::move(main_scenario_ptr));

    auto make_constraint = [&] (auto time, auto s, auto e)
    {
      time_value tv{(double)time};
      auto cst = std::make_shared<time_constraint>(cb_2, *s, *e, tv, tv, tv);
      cst->setGranularity(granul);
      s->nextTimeConstraints().push_back(cst);
      e->previousTimeConstraints().push_back(cst);
      main_scenario.addTimeConstraint(cst);
      return cst;
    };

    std::vector<std::shared_ptr<time_node>> t(15); std::generate(t.begin(), t.end(), [&] {
      auto tn = std::make_shared<time_node>();
      main_scenario.addTimeNode(tn);
      return tn;
    });

    t[0] = main_scenario.getStartTimeNode();

    std::vector<std::shared_ptr<time_event>> e(15);
    for(int i = 0; i < t.size(); i++) e[i] = *t[i]->emplace(t[i]->timeEvents().begin(), {});

    std::vector<std::shared_ptr<time_constraint>> c(14);
    c[0] = make_constraint(1000, e[0], e[1]);
    c[1] = make_constraint(2000, e[1], e[2]);
    c[2] = make_constraint(1500, e[2], e[3]);
    c[3] = make_constraint(1000, e[3], e[4]);
    c[4] = make_constraint(500, e[4], e[5]);
    c[5] = make_constraint(3000, e[5], e[6]);

    c[6] = make_constraint(500, e[0], e[7]);
    c[7] = make_constraint(10000, e[7], e[8]);

    c[8] = make_constraint(3000, e[0], e[9]);
    c[9] = make_constraint(4000, e[9], e[10]);

    c[10] = make_constraint(1000, e[9], e[11]);
    c[11] = make_constraint(3000, e[11], e[12]);
    c[12] = make_constraint(1000, e[12], e[13]);
    c[13] = make_constraint(3000, e[13], e[14]);


    c[1]->addTimeProcess(std::make_shared<node_process>(graph, f1));
    c[3]->addTimeProcess(std::make_shared<node_process>(graph, f2));
    c[5]->addTimeProcess(std::make_shared<node_process>(graph, f1_2));

    c[7]->addTimeProcess(std::make_shared<node_process>(graph, f3));

    c[9]->addTimeProcess(std::make_shared<node_process>(graph, f4));

    c[11]->addTimeProcess(std::make_shared<node_process>(graph, f5));
    c[13]->addTimeProcess(std::make_shared<node_process>(graph, f5_2));


    main_constraint->setDriveMode(ossia::clock::DriveMode::EXTERNAL);
    main_constraint->setGranularity(time_value(1000. * 64. / 44100.));
    main_constraint->setSpeed(1.0);

    main_constraint->offset(0.000000001_tv);
    main_constraint->start();

    for(int i = 0; i < 15  * 44100. / 64.; i++)
    {
      const ossia::time_value rate{1000000. * 64. / 44100.};
      main_constraint->tick(rate);

      st = g.state();
      copy_samples();

      std::this_thread::sleep_for(std::chrono::microseconds(int64_t(1000000. * 64. / 44100.)));

    }

    qDebug() << samples.size();
    QVERIFY(samples.size() > 0);
    SndfileHandle file("/tmp/out.wav", SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 2, 44100);
    file.write(samples.data(), samples.size());
    file.writeSync();

  }
};

QTEST_APPLESS_MAIN(PdDataflowTest)

#include "PdDataflowTest.moc"

#include <QtTest/QTest>
#include <ossia/dataflow/dataflow.hpp>
#include <z_libpd.h>

class pd_graph_node final :
    public ossia::graph_node
{
public:
  std::string add_dzero(const std::string& s)
  {
    return std::to_string(m_dollarzero) + "-" + s;
  }

  pd_graph_node(std::string file,
                int inputs,
                int outputs,
                std::vector<std::string> inmess,
                std::vector<std::string> outmess)
    : m_inputs{inputs}
    , m_outputs{outputs}
    , m_inmess{inmess}
    , m_outmess{outmess}
  {
    for(int i = 0; i < inputs ; i++)
      in_ports.push_back(ossia::make_inlet<ossia::audio_port>());
    for(int i = 0; i < outputs ; i++)
      out_ports.push_back(ossia::make_outlet<ossia::audio_port>());

    // TODO add $0 to everyone
    {
      m_instance = pdinstance_new();
      pd_setinstance(m_instance);

      // compute audio    [; pd dsp 1(
      libpd_start_message(1); // one entry in list
      libpd_add_float(1.0f);
      libpd_finish_message("pd", "dsp");

      auto handle = libpd_openfile(file.c_str(), "/home/jcelerier");
      m_dollarzero = libpd_getdollarzero(handle);


      for(auto& str : m_inmess)
      {
        in_ports.push_back(ossia::make_inlet<ossia::value_port>());
      }

      for(auto& str : m_outmess)
      {
        auto mess = add_dzero(str);
        libpd_bind(mess.c_str());

        out_ports.push_back(ossia::make_outlet<ossia::value_port>());
      }

      libpd_set_floathook([] (const char *recv, float f) {
        qDebug() << "received float: " << recv <<  f;
        //if(auto v = get_outlet(recv))
        {
        //  v->data.push_back(f);
        }
      });
      libpd_set_banghook([] (const char *recv) {
        qDebug() << "received bang: " << recv;
      });
      libpd_set_symbolhook([] (const char *recv, const char *sym) {
        qDebug() << "received sym: " << recv << sym;
      });
      libpd_set_listhook([] (const char *recv, int argc, t_atom *argv) {
        qDebug() << "received list: " << recv << argc;
      });
      libpd_set_messagehook([] (const char *recv, const char *msg, int argc, t_atom *argv) {
        qDebug() << "received message: " << recv << msg << argc;
      });

      libpd_set_noteonhook([] (int channel, int pitch, int velocity) {
        qDebug() << "received midi Note: " << channel << pitch << velocity;
      });
      libpd_set_controlchangehook([] (int channel, int controller, int value) {
        qDebug() << "received midi CC: " << channel << controller << value;

      });
    }
  }

  ossia::value_port* get_outlet(const char* str) const
  {
    for(int i = 0; i < m_outmess.size(); ++i)
    {
      if(m_outmess[i] == str)
      {
        return out_ports[i + m_outputs]->data.target<ossia::value_port>();
      }
    }

    return nullptr;
  }

  ~pd_graph_node()
  {
    pdinstance_free(m_instance);
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
  };

  void run(ossia::execution_state& e) override
  {
    pd_setinstance(m_instance);
    libpd_init_audio(m_inputs, m_outputs, 44100);


    const auto bs = libpd_blocksize();
    std::vector<float> inbuf;
    inbuf.resize(m_inputs * bs);

    std::vector<float> outbuf;
    outbuf.resize(m_outputs * bs);

    for(int i = m_inputs; i < in_ports.size(); ++i)
    {
      auto& dat = in_ports[i]->data.target<ossia::value_port>()->data;
      while(dat.size() > 0)
      {
        ossia::value val = dat.front();
        dat.pop_front();
        auto mess = add_dzero(m_inmess[i - m_inputs]);
        val.apply(ossia_to_pd_value{mess.c_str()});
      }
    }

    libpd_process_raw(inbuf.data(), outbuf.data());

    for(int i = 0; i < m_outputs; ++i)
    {
      auto ap = out_ports[i]->data.target<ossia::audio_port>();
      ap->samples.resize(ap->samples.size() + 1);

      std::copy_n(outbuf.begin() + i * bs, bs, ap->samples.back().begin());
    }
  }

  t_pdinstance * m_instance{};
  int m_dollarzero = 0;

  int m_inputs{}, m_outputs{};
  std::vector<std::string> m_inmess, m_outmess;
};


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
    pd_graph_node n{"audio.pd", 2, 2, {}, {}};
    ossia::execution_state e;
    n.run(e);
  }
};

QTEST_APPLESS_MAIN(PdDataflowTest)

#include "PdDataflowTest.moc"

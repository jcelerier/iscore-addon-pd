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
#include <Pd/Executor/PdExecutor.hpp>

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

    using namespace Pd;
    auto f1 = std::make_shared<PdGraphNode>(cwd, "gen1.pd", 0, 0, string_vec{}, string_vec{"out-1"});
    auto f1_2 = std::make_shared<PdGraphNode>(cwd, "gen1.pd", 0, 0, string_vec{}, string_vec{"out-1"});
    auto f2 = std::make_shared<PdGraphNode>(cwd, "gen2.pd", 0, 0, string_vec{"in-1"}, string_vec{"out-1"});
    auto f3 = std::make_shared<PdGraphNode>(cwd, "gen3.pd", 0, 2, string_vec{"in-1", "in-2"}, string_vec{});
    auto f4 = std::make_shared<PdGraphNode>(cwd, "gen4.pd", 2, 2, string_vec{"in-2"}, string_vec{});
    auto f5 = std::make_shared<PdGraphNode>(cwd, "gen5.pd", 2, 2, string_vec{"in-2"}, string_vec{});
    auto f5_2 = std::make_shared<PdGraphNode>(cwd, "gen5.pd", 2, 2, string_vec{"in-2"}, string_vec{});

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
      auto it_l = st.audioState.find(l_addr);
      auto it_r = st.audioState.find(r_addr);
      if(it_l == st.audioState.end() || it_r == st.audioState.end())
      {
        // Just copy silence
        samples.resize(samples.size() + 128);
        return;
      }

      auto& audio_l = it_l->second;
      auto& audio_r = it_r->second;
      QVERIFY(!audio_l.samples.empty());
      QVERIFY(!audio_r.samples.empty());

      auto pos = samples.size();
      samples.resize(samples.size() + 128);
      for(auto& arr : audio_l.samples)
      {
        for(int i = 0; i < 64; i++)
          samples[pos + i * 2] += arr[i];
      }
      for(auto& arr : audio_r.samples)
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

      g.state(st);
      copy_samples();
      st.clear();

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

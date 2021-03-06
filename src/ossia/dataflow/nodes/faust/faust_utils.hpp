#pragma once
#include <ossia/dataflow/graph_node.hpp>
#include <ossia/dataflow/port.hpp>

#if __has_include(<faust/dsp/poly-llvm-dsp.h>)
  #include <faust/dsp/poly-llvm-dsp.h>
#else
  #ifndef FAUSTFLOAT
  #define FAUSTFLOAT float
  #endif

  struct Soundfile { };
  struct Meta { void declare(const char*, const char*) { } };
  struct dsp { };
  struct UI
  {
      virtual ~UI() = default;

      virtual void openTabBox(const char* label) = 0;
      virtual void openHorizontalBox(const char* label) = 0;
      virtual void openVerticalBox(const char* label) = 0;
      virtual void closeBox() = 0;
      virtual void declare(FAUSTFLOAT* zone, const char* key, const char* val) = 0;
      virtual void addSoundfile(
          const char* label,
          const char* filename,
          Soundfile** sf_zone) = 0;
      virtual void addButton(const char* label, FAUSTFLOAT* zone) = 0;
      virtual void addCheckButton(const char* label, FAUSTFLOAT* zone) = 0;
      virtual void addVerticalSlider(
          const char* label,
          FAUSTFLOAT* zone,
          FAUSTFLOAT init,
          FAUSTFLOAT min,
          FAUSTFLOAT max,
          FAUSTFLOAT step) = 0;
      virtual void addHorizontalSlider(
          const char* label,
          FAUSTFLOAT* zone,
          FAUSTFLOAT init,
          FAUSTFLOAT min,
          FAUSTFLOAT max,
          FAUSTFLOAT step) = 0;
      virtual void addNumEntry(
          const char* label,
          FAUSTFLOAT* zone,
          FAUSTFLOAT init,
          FAUSTFLOAT min,
          FAUSTFLOAT max,
          FAUSTFLOAT step) = 0;
      virtual void addHorizontalBargraph(
          const char* label,
          FAUSTFLOAT* zone,
          FAUSTFLOAT min,
          FAUSTFLOAT max) = 0;
      virtual void addVerticalBargraph(
          const char* label,
          FAUSTFLOAT* zone,
          FAUSTFLOAT min,
          FAUSTFLOAT max) = 0;
  };

#endif

namespace ossia::nodes
{

template <typename T>
struct faust_setup_ui : UI
{
  faust_setup_ui(T& self)
  {
  }
};

template <typename Node>
struct faust_exec_ui final : UI
{
  Node& fx;
  faust_exec_ui(Node& n) : fx{n}
  {
  }

  void addButton(const char* label, FAUSTFLOAT* zone) override
  {
    fx.inputs().push_back(ossia::make_inlet<ossia::value_port>());
    fx.controls.push_back(
        {fx.inputs().back()->data.template target<ossia::value_port>(), zone});
  }

  void addCheckButton(const char* label, FAUSTFLOAT* zone) override
  {
    addButton(label, zone);
  }

  void addVerticalSlider(
      const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min,
      FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    fx.inputs().push_back(ossia::make_inlet<ossia::value_port>());
    fx.controls.push_back(
        {fx.inputs().back()->data.template target<ossia::value_port>(), zone});
  }

  void addHorizontalSlider(
      const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min,
      FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    addVerticalSlider(label, zone, init, min, max, step);
  }

  void addNumEntry(
      const char* label, FAUSTFLOAT* zone, FAUSTFLOAT init, FAUSTFLOAT min,
      FAUSTFLOAT max, FAUSTFLOAT step) override
  {
    addVerticalSlider(label, zone, init, min, max, step);
  }

  void addHorizontalBargraph(
      const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override
  {
    fx.outputs().push_back(ossia::make_outlet<ossia::value_port>());
  }

  void addVerticalBargraph(
      const char* label, FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max) override
  {
    addHorizontalBargraph(label, zone, min, max);
  }

  void openTabBox(const char* label) override
  {
  }
  void openHorizontalBox(const char* label) override
  {
  }
  void openVerticalBox(const char* label) override
  {
  }
  void closeBox() override
  {
  }
  void declare(FAUSTFLOAT* zone, const char* key, const char* val) override
  {
  }
  void
  addSoundfile(const char* label, const char* filename, Soundfile** sf_zone) override
  {
  }
};


/// Execution ///
template <typename Node, typename Dsp>
void faust_exec(Node& self, Dsp& dsp, const ossia::token_request& tk)
{
  if (tk.date > tk.prev_date)
  {
    std::size_t d = tk.date - tk.prev_date;
    for (auto ctrl : self.controls)
    {
      auto& dat = ctrl.first->get_data();
      if (!dat.empty())
      {
        *ctrl.second = ossia::convert<float>(dat.back().value);
      }
    }

    auto& audio_in
        = *self.inputs()[0]->data.template target<ossia::audio_port>();
    auto& audio_out
        = *self.outputs()[0]->data.template target<ossia::audio_port>();

    const std::size_t n_in = dsp.getNumInputs();
    const std::size_t n_out = dsp.getNumOutputs();

    float* inputs_ = (float*)alloca(n_in * d * sizeof(float));
    float* outputs_ = (float*)alloca(n_out * d * sizeof(float));

    float** input_n = (float**)alloca(sizeof(float*) * n_in);
    float** output_n = (float**)alloca(sizeof(float*) * n_out);

    // Copy inputs
    // TODO offset !!!
    for (std::size_t i = 0; i < n_in; i++)
    {
      input_n[i] = inputs_ + i * d;
      if (audio_in.samples.size() > i)
      {
        auto num_samples = std::min(
            (std::size_t)d, (std::size_t)audio_in.samples[i].size());
        for (std::size_t j = 0; j < num_samples; j++)
        {
          input_n[i][j] = (float)audio_in.samples[i][j];
        }

        if (d > audio_in.samples[i].size())
        {
          for (std::size_t j = audio_in.samples[i].size(); j < d; j++)
          {
            input_n[i][j] = 0.f;
          }
        }
      }
      else
      {
        for (std::size_t j = 0; j < d; j++)
        {
          input_n[i][j] = 0.f;
        }
      }
    }

    for (std::size_t i = 0; i < n_out; i++)
    {
      output_n[i] = outputs_ + i * d;
      for (std::size_t j = 0; j < d; j++)
      {
        output_n[i][j] = 0.f;
      }
    }

    dsp.compute(d, input_n, output_n);

    audio_out.samples.resize(n_out);
    for (std::size_t i = 0; i < n_out; i++)
    {
      audio_out.samples[i].resize(d);
      for (std::size_t j = 0; j < d; j++)
      {
        audio_out.samples[i][j] = (double)output_n[i][j];
      }
    }

    // TODO handle multichannel cleanly
    if (n_out == 1)
    {
      audio_out.samples.resize(2);
      audio_out.samples[1] = audio_out.samples[0];
    }
  }
}

/// Synth ///
template <typename Node, typename DspPoly>
void faust_exec_synth(Node& self, DspPoly& dsp, const ossia::token_request& tk)
{
  if (tk.date > tk.prev_date)
  {
    std::size_t d = tk.date - tk.prev_date;
    for (auto ctrl : self.controls)
    {
      auto& dat = ctrl.first->get_data();
      if (!dat.empty())
      {
        *ctrl.second = ossia::convert<float>(dat.back().value);
      }
    }

    auto& midi_in
        = *self.inputs()[0]->data.template target<ossia::midi_port>();
    auto& audio_out
        = *self.outputs()[0]->data.template target<ossia::audio_port>();

    const std::size_t n_in = dsp.getNumInputs();
    const std::size_t n_out = dsp.getNumOutputs();

    float* inputs_ = (float*)alloca(n_in * d * sizeof(float));
    float* outputs_ = (float*)alloca(n_out * d * sizeof(float));

    float** input_n = (float**)alloca(sizeof(float*) * n_in);
    float** output_n = (float**)alloca(sizeof(float*) * n_out);

    // Copy inputs
    // TODO offset !!!

    for(rtmidi::message& mess : midi_in.messages)
    {
      switch(mess.get_message_type())
      {
        case rtmidi::message_type::NOTE_ON:
        {
          dsp.keyOn(mess[0], mess[1], mess[2]);
          break;
        }
        case rtmidi::message_type::NOTE_OFF:
        {
          dsp.keyOff(mess[0], mess[1], mess[2]);
          break;
        }
        case rtmidi::message_type::CONTROL_CHANGE:
        {
          dsp.ctrlChange(mess[0], mess[1], mess[2]);
          break;
        }
        case rtmidi::message_type::PITCH_BEND:
        {
          dsp.pitchWheel(mess[0], mess.bytes[2] * 128 + mess.bytes[1]);
          break;
        }
        default:
          break;
          // TODO continue...
      }
    }

    for (std::size_t i = 0; i < n_in; i++)
    {
      input_n[i] = inputs_ + i * d;
      for (std::size_t j = 0; j < d; j++)
      {
        input_n[i][j] = 0.f;
      }
    }

    for (std::size_t i = 0; i < n_out; i++)
    {
      output_n[i] = outputs_ + i * d;
      for (std::size_t j = 0; j < d; j++)
      {
        output_n[i][j] = 0.f;
      }
    }

    dsp.compute(d, input_n, output_n);

    audio_out.samples.resize(n_out);
    for (std::size_t i = 0; i < n_out; i++)
    {
      audio_out.samples[i].resize(d);
      for (std::size_t j = 0; j < d; j++)
      {
        audio_out.samples[i][j] = (double)output_n[i][j];
      }
    }

    // TODO handle multichannel cleanly
    if (n_out == 1)
    {
      audio_out.samples.resize(2);
      audio_out.samples[1] = audio_out.samples[0];
    }
  }
}
}

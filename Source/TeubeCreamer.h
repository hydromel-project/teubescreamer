/*
  ==============================================================================

    TeubeCreamer.h
    Created: [Date of creation, if known, otherwise leave blank for now]
    Author:  Hydromel

    This file contains the basic framework code for a JUCE plugin processor.
    This plugin is made with the JUCE library - https://juce.com/
    Copyright (c) 2024 Hydromel

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

//==============================================================================
/**
 * TeubeCreamer - A TubeScreamer TS-1 style overdrive pedal emulation
 * Created by Hydromel
 */
class TeubeCreamerAudioProcessor : public juce::AudioProcessor {
public:
  //==============================================================================
  TeubeCreamerAudioProcessor();
  ~TeubeCreamerAudioProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(
      const juce::AudioProcessor::BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  using AudioProcessor::processBlock;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override;

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==============================================================================
  // Audio parameter getters for the UI
  float getDrive() const { return *parameters.getRawParameterValue("drive"); }
  float getTone() const { return *parameters.getRawParameterValue("tone"); }
  float getLevel() const { return *parameters.getRawParameterValue("level"); }

  // Audio processor value tree state for parameters
  juce::AudioProcessorValueTreeState parameters;

private:
  //==============================================================================
  // Class for the actual TubeScreamer circuit emulation
  class TubeScreamerCircuit {
  public:
    TubeScreamerCircuit() {}

    void prepare(double sampleRateIn, int samplesPerBlock) {
      sampleRate = sampleRateIn;
      // Initialize filter coefficients and states
      juce::dsp::ProcessSpec spec;
      spec.sampleRate = sampleRate;
      spec.maximumBlockSize = samplesPerBlock;
      spec.numChannels = 2;

      // Input buffer stage
      inputFilter.prepare(spec);
      inputFilter.reset();

      // Tone control filter
      toneFilter.prepare(spec);
      toneFilter.reset();

      // Output filter
      outputFilter.prepare(spec);
      outputFilter.reset();

      // Set default filter values
      updateFilters(0.5f, 0.5f);
    }

    void updateFilters(float driveParam, float toneParam) {
      // Input high pass filter (removes DC offset and very low frequencies)
      *inputFilter.coefficients =
          *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 20.0f);

      // Tone control circuit - simple low pass filter with variable cutoff
      // Map tone parameter [0,1] to frequency range [500Hz, 5kHz]
      float toneCutoff = 500.0f + toneParam * 4500.0f;
      *toneFilter.coefficients =
          *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,
                                                            toneCutoff);

      // Output high pass filter (mimics the capacitor at the output)
      *outputFilter.coefficients =
          *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 35.0f);
    }

    float processSample(float sample, float drive, float tone, float level) {
      // Input buffer stage
      sample = inputFilter.processSample(sample);

      // Add pre-gain (TS has an internal gain stage)
      sample *= 3.0f;

      // Apply drive gain (non-linear)
      // Map drive [0,1] to gain range
      float driveGain = 1.0f + drive * 50.0f;
      sample *= driveGain;

      // Soft clipping stage (diode clipping)
      // This is a simplified emulation of the asymmetric diode clipping
      sample = clipSample(sample);

      // Apply tone filter
      sample = toneFilter.processSample(sample);

      // Output filter
      sample = outputFilter.processSample(sample);

      // Apply output level
      return sample * level;
    }

  private:
    // Asymmetrical soft-clipping function emulating the diode characteristic
    float clipSample(float sample) {
      // Simplified asymmetrical soft clipping
      if (sample > 0.0f) {
        sample = std::tanh(sample);
      } else {
        // Slightly different clipping curve for negative values
        // (simulating asymmetrical diode clipping)
        sample = std::tanh(sample * 0.8f) * 1.2f;
      }
      return sample;
    }

    // Filters for the different stages of the circuit
    juce::dsp::IIR::Filter<float> inputFilter;
    juce::dsp::IIR::Filter<float> toneFilter;
    juce::dsp::IIR::Filter<float> outputFilter;
    double sampleRate = 44100.0;
  };

  // Tube Screamer circuit instances (one per channel)
  std::vector<TubeScreamerCircuit> tubeScreamerCircuits;

  // Smoothed parameter values for avoiding clicks and pops when parameters
  // change
  juce::SmoothedValue<float> driveSmoothed;
  juce::SmoothedValue<float> toneSmoothed;
  juce::SmoothedValue<float> levelSmoothed;

  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeubeCreamerAudioProcessor)
};

//==============================================================================
/**
 * Custom GUI for the TeubeCreamer plugin
 */
class TeubeCreamerAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  TeubeCreamerAudioProcessorEditor(TeubeCreamerAudioProcessor &);
  ~TeubeCreamerAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  // Reference to the processor
  TeubeCreamerAudioProcessor &audioProcessor;

  // Controls
  juce::Slider driveKnob;
  juce::Slider toneKnob;
  juce::Slider levelKnob;

  juce::Label driveLabel;
  juce::Label toneLabel;
  juce::Label levelLabel;

  // Parameter attachments
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      driveAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      toneAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      levelAttachment;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeubeCreamerAudioProcessorEditor)
};
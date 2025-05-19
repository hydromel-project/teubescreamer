#include "TeubeCreamer.h"

//==============================================================================
TeubeCreamerAudioProcessor::TeubeCreamerAudioProcessor()
    : AudioProcessor(
          juce::AudioProcessor::BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters",
                 {std::make_unique<juce::AudioParameterFloat>("drive", "Drive",
                                                              0.0f, 1.0f, 0.5f),
                  std::make_unique<juce::AudioParameterFloat>("tone", "Tone",
                                                              0.0f, 1.0f, 0.5f),
                  std::make_unique<juce::AudioParameterFloat>(
                      "level", "Level", 0.0f, 1.0f, 0.5f)}) {
  // Initialize parameter smoothing
  driveSmoothed.reset(getSampleRate(), 0.05);
  toneSmoothed.reset(getSampleRate(), 0.05);
  levelSmoothed.reset(getSampleRate(), 0.05);

  // Set initial parameter values
  driveSmoothed.setCurrentAndTargetValue(
      parameters.getRawParameterValue("drive")->load());
  toneSmoothed.setCurrentAndTargetValue(
      parameters.getRawParameterValue("tone")->load());
  levelSmoothed.setCurrentAndTargetValue(
      parameters.getRawParameterValue("level")->load());
}

TeubeCreamerAudioProcessor::~TeubeCreamerAudioProcessor() {}

//==============================================================================
const juce::String TeubeCreamerAudioProcessor::getName() const {
  return "TeubeCreamer";
}

bool TeubeCreamerAudioProcessor::acceptsMidi() const { return false; }

bool TeubeCreamerAudioProcessor::producesMidi() const { return false; }

bool TeubeCreamerAudioProcessor::isMidiEffect() const { return false; }

double TeubeCreamerAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int TeubeCreamerAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int TeubeCreamerAudioProcessor::getCurrentProgram() { return 0; }

void TeubeCreamerAudioProcessor::setCurrentProgram(int index) {}

const juce::String TeubeCreamerAudioProcessor::getProgramName(int index) {
  return {};
}

void TeubeCreamerAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

//==============================================================================
void TeubeCreamerAudioProcessor::prepareToPlay(double sampleRate,
                                               int samplesPerBlock) {
  // Initialize parameter smoothing
  driveSmoothed.reset(sampleRate, 0.05);
  toneSmoothed.reset(sampleRate, 0.05);
  levelSmoothed.reset(sampleRate, 0.05);

  // Create TubeScreamer circuits for each channel
  tubeScreamerCircuits.clear();
  auto totalNumInputChannels = getTotalNumInputChannels();
  tubeScreamerCircuits.resize(totalNumInputChannels);

  // Prepare each circuit
  for (auto &circuit : tubeScreamerCircuits) {
    circuit.prepare(sampleRate, samplesPerBlock);
  }
}

void TeubeCreamerAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool TeubeCreamerAudioProcessor::isBusesLayoutSupported(
    const juce::AudioProcessor::BusesLayout &layouts) const {
  // Support mono or stereo
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  // Input and output must have same channel count
  if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
    return false;

  return true;
}

void TeubeCreamerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                              juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  // Clear any output channels that didn't contain input data
  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Update smoothed parameter values
  driveSmoothed.setTargetValue(*parameters.getRawParameterValue("drive"));
  toneSmoothed.setTargetValue(*parameters.getRawParameterValue("tone"));
  levelSmoothed.setTargetValue(*parameters.getRawParameterValue("level"));

  // Update filter coefficients if tone has changed
  // This is less CPU intensive than updating every sample
  for (auto &circuit : tubeScreamerCircuits) {
    circuit.updateFilters(driveSmoothed.getCurrentValue(),
                          toneSmoothed.getCurrentValue());
  }

  // Process each channel
  for (int channel = 0; channel < totalNumInputChannels; ++channel) {
    auto *channelData = buffer.getWritePointer(channel);

    // Process each sample
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
      // Get current parameter values
      float drive = driveSmoothed.getNextValue();
      float tone = toneSmoothed.getNextValue();
      float level = levelSmoothed.getNextValue();

      // Process through the TubeScreamer circuit
      channelData[sample] = tubeScreamerCircuits[channel].processSample(
          channelData[sample], drive, tone, level);
    }
  }
}

//==============================================================================
bool TeubeCreamerAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *TeubeCreamerAudioProcessor::createEditor() {
  return new TeubeCreamerAudioProcessorEditor(*this);
}

//==============================================================================
void TeubeCreamerAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  // Store parameter values
  auto state = parameters.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void TeubeCreamerAudioProcessor::setStateInformation(const void *data,
                                                     int sizeInBytes) {
  // Restore parameter values
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(parameters.state.getType()))
      parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new TeubeCreamerAudioProcessor();
}

//==============================================================================
// TeubeCreamerAudioProcessorEditor implementation
//==============================================================================

TeubeCreamerAudioProcessorEditor::TeubeCreamerAudioProcessorEditor(
    TeubeCreamerAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  // Set up Drive knob
  driveKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  driveKnob.setRange(0.0f, 1.0f, 0.01f);
  driveKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  driveKnob.setPopupDisplayEnabled(true, false, this);
  driveKnob.setTextValueSuffix(" Drive");
  driveKnob.setDoubleClickReturnValue(true, 0.5f);
  addAndMakeVisible(driveKnob);

  // Set up Tone knob
  toneKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  toneKnob.setRange(0.0f, 1.0f, 0.01f);
  toneKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  toneKnob.setPopupDisplayEnabled(true, false, this);
  toneKnob.setTextValueSuffix(" Tone");
  toneKnob.setDoubleClickReturnValue(true, 0.5f);
  addAndMakeVisible(toneKnob);

  // Set up Level knob
  levelKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  levelKnob.setRange(0.0f, 1.0f, 0.01f);
  levelKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
  levelKnob.setPopupDisplayEnabled(true, false, this);
  levelKnob.setTextValueSuffix(" Level");
  levelKnob.setDoubleClickReturnValue(true, 0.5f);
  addAndMakeVisible(levelKnob);

  // Add labels
  driveLabel.setText("Drive", juce::dontSendNotification);
  driveLabel.attachToComponent(&driveKnob, false);
  driveLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(driveLabel);

  toneLabel.setText("Tone", juce::dontSendNotification);
  toneLabel.attachToComponent(&toneKnob, false);
  toneLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(toneLabel);

  levelLabel.setText("Level", juce::dontSendNotification);
  levelLabel.attachToComponent(&levelKnob, false);
  levelLabel.setJustificationType(juce::Justification::centred);
  addAndMakeVisible(levelLabel);

  // Create slider attachments to link with parameters
  driveAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.parameters, "drive", driveKnob);
  toneAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.parameters, "tone", toneKnob);
  levelAttachment =
      std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
          audioProcessor.parameters, "level", levelKnob);

  // Set window size
  setSize(400, 300);
}

TeubeCreamerAudioProcessorEditor::~TeubeCreamerAudioProcessorEditor() {}

void TeubeCreamerAudioProcessorEditor::paint(juce::Graphics &g) {
  // Fill background with a gradient
  g.setGradientFill(juce::ColourGradient(
      juce::Colour(0xff1a762b), getWidth() * 0.5f, getHeight() * 0.5f,
      juce::Colour(0xff072e0f), 0.0f, 0.0f, true));
  g.fillAll();

  // Draw logo/title
  g.setColour(juce::Colours::white);
  g.setFont(juce::Font("Impact", 36.0f, juce::Font::plain));
  g.drawFittedText("TeubeCreamer", getLocalBounds().reduced(10, 10),
                   juce::Justification::centredTop, 1);

  // Draw subtitle
  g.setFont(juce::Font(16.0f));
  g.drawFittedText("TS-1 Overdrive Emulation",
                   getLocalBounds().reduced(10, 50).withHeight(30),
                   juce::Justification::centredTop, 1);

  // Add a company logo/name at the bottom
  g.setFont(juce::Font(12.0f));
  g.drawFittedText("Hydromel Audio",
                   getLocalBounds().reduced(10, 10).withTop(getHeight() - 30),
                   juce::Justification::centredBottom, 1);
}

void TeubeCreamerAudioProcessorEditor::resized() {
  // Layout the controls - equally spaced across the width
  auto area = getLocalBounds().reduced(30, 100);
  int knobWidth = area.getWidth() / 3;

  driveKnob.setBounds(area.removeFromLeft(knobWidth).reduced(10));
  toneKnob.setBounds(area.removeFromLeft(knobWidth).reduced(10));
  levelKnob.setBounds(area.reduced(10));
}
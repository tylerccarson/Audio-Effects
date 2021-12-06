/*
  ==============================================================================

    Code by Juan Gil <https://juangil.com/>.
    Copyright (C) 2017-2020 Juan Gil.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

WahWahAudioProcessorEditor::WahWahAudioProcessorEditor (WahWahAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    const Array<AudioProcessorParameter*> parameters = processor.getParameters();
    int comboBoxCounter = 0;

    int editorHeight = 2 * editorMargin;
    for (int i = 0; i < parameters.size(); ++i) {
        if (const AudioProcessorParameterWithID* parameter =
                dynamic_cast<AudioProcessorParameterWithID*> (parameters[i])) {

            if (processor.parameters.parameterTypes[i] == "Slider") {
                Slider* aSlider;
                sliders.add (aSlider = new Slider());
                aSlider->setTextValueSuffix (parameter->label);
                aSlider->setTextBoxStyle (Slider::TextBoxLeft,
                                          false,
                                          sliderTextEntryBoxWidth,
                                          sliderTextEntryBoxHeight);

                SliderAttachment* aSliderAttachment;
                sliderAttachments.add (aSliderAttachment =
                    new SliderAttachment (processor.parameters.apvts, parameter->paramID, *aSlider));

                components.add (aSlider);
                editorHeight += sliderHeight;
                
                // OSC
                aSlider->onValueChange = [this, aSlider, parameter]
                {
                    float sliderVal = static_cast<float>(aSlider->getValue());
                    String sliderID = parameter->paramID;
                    DBG(sliderID + ": " + String(sliderVal));
                    
                    if (! sender.send("/param/" + sliderID, sliderVal)) {
                        DBG("Error: could not send OSC message.");
                    }
                };
            }

            //======================================

            else if (processor.parameters.parameterTypes[i] == "ToggleButton") {
                ToggleButton* aButton;
                toggles.add (aButton = new ToggleButton());
                aButton->setToggleState (parameter->getDefaultValue(), dontSendNotification);

                ButtonAttachment* aButtonAttachment;
                buttonAttachments.add (aButtonAttachment =
                    new ButtonAttachment (processor.parameters.apvts, parameter->paramID, *aButton));

                components.add (aButton);
                editorHeight += buttonHeight;
            }

            //======================================

            else if (processor.parameters.parameterTypes[i] == "ComboBox") {
                ComboBox* aComboBox;
                comboBoxes.add (aComboBox = new ComboBox());
                aComboBox->setEditableText (false);
                aComboBox->setJustificationType (Justification::left);
                aComboBox->addItemList (processor.parameters.comboBoxItemLists[comboBoxCounter++], 1);

                ComboBoxAttachment* aComboBoxAttachment;
                comboBoxAttachments.add (aComboBoxAttachment =
                    new ComboBoxAttachment (processor.parameters.apvts, parameter->paramID, *aComboBox));

                components.add (aComboBox);
                editorHeight += comboBoxHeight;
                
                // OSC
                aComboBox->onChange = [this, aComboBox, parameter]
                {
                    float comboVal = static_cast<float>(aComboBox->getSelectedId());
                    String comboID = parameter->paramID;
                    DBG(comboID + ": " + String(comboVal));

                    if (! sender.send("/param/" + comboID, comboVal)) {
                        DBG("Error: could not send OSC message.");
                    }
                };
            }

            //======================================

            Label* aLabel;
            labels.add (aLabel = new Label (parameter->name, parameter->name));
            aLabel->attachToComponent (components.getLast(), true);
            addAndMakeVisible (aLabel);

            components.getLast()->setName (parameter->name);
            components.getLast()->setComponentID (parameter->paramID);
            addAndMakeVisible (components.getLast());
        }
    }

    //======================================

    editorHeight += components.size() * editorPadding;
    setSize (editorWidth, editorHeight);
    startTimer (50);
    
    //======================================
    // TODO: change target host to our elk-pi endpoint.
    // Keep as localhost for development purposes.
    if (! sender.connect ("127.0.0.1", 9001)) {
        DBG("Error: could not connect to UDP port 9001.");
    } else {
        DBG("Connected to UDP port 9001.");
    }
}

WahWahAudioProcessorEditor::~WahWahAudioProcessorEditor()
{
}

//==============================================================================

void WahWahAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
}

void WahWahAudioProcessorEditor::resized()
{
    Rectangle<int> r = getLocalBounds().reduced (editorMargin);
    r = r.removeFromRight (r.getWidth() - labelWidth);

    for (int i = 0; i < components.size(); ++i) {
        if (Slider* aSlider = dynamic_cast<Slider*> (components[i]))
            components[i]->setBounds (r.removeFromTop (sliderHeight));

        if (ToggleButton* aButton = dynamic_cast<ToggleButton*> (components[i]))
            components[i]->setBounds (r.removeFromTop (buttonHeight));

        if (ComboBox* aComboBox = dynamic_cast<ComboBox*> (components[i]))
            components[i]->setBounds (r.removeFromTop (comboBoxHeight));

        r = r.removeFromBottom (r.getHeight() - editorPadding);
    }
}

//==============================================================================

void WahWahAudioProcessorEditor::timerCallback()
{
    updateUIcomponents();
}

void WahWahAudioProcessorEditor::updateUIcomponents()
{
    if (processor.paramMode.getTargetValue() == processor.modeAutomatic) {
        if (Slider* aSlider = dynamic_cast<Slider*> (findChildWithID (processor.paramFrequency.paramID)))
            aSlider->setValue (processor.centreFrequency);
        findChildWithID (processor.paramFrequency.paramID)->setEnabled (false);
        findChildWithID (processor.paramLFOfrequency.paramID)->setEnabled (true);
        findChildWithID (processor.paramMixLFOandEnvelope.paramID)->setEnabled (true);
        findChildWithID (processor.paramEnvelopeAttack.paramID)->setEnabled (true);
        findChildWithID (processor.paramEnvelopeRelease.paramID)->setEnabled (true);
    } else {
        findChildWithID (processor.paramFrequency.paramID)->setEnabled (true);
        findChildWithID (processor.paramLFOfrequency.paramID)->setEnabled (false);
        findChildWithID (processor.paramMixLFOandEnvelope.paramID)->setEnabled (false);
        findChildWithID (processor.paramEnvelopeAttack.paramID)->setEnabled (false);
        findChildWithID (processor.paramEnvelopeRelease.paramID)->setEnabled (false);
    }
}

//==============================================================================

/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <vector>

#include "global/types/string.h"

namespace mu::engraving {

class Fraction;
class Instrument;
class Note;
class Staff;
class XmlReader;
class XmlWriter;

struct NumericPulseDegree {
    int number = 0;
    int scorePitch = -1;
    int inputStep = 0;
    int inputIndex = -1;
    muse::String label;
    muse::String action;
};

struct NumericPulseProfile {
    muse::String id;
    muse::String fontFamily;
    std::vector<NumericPulseDegree> degrees;
    bool fixedBaseline = true;
    bool monophonic = true;
    bool showEmptyPulses = true;
    bool durationMarksAbove = true;
    // Zero means use the legacy monophonic setting (one note) or leave chord
    // size unlimited when monophonic is false. Values 1..9 set an explicit cap.
    int chordNoteLimit = 0;
    int minimumRegister = -1;
    int maximumRegister = 1;
    int defaultRegister = 0;

    bool isValid() const;
    void read(XmlReader&);
    void write(XmlWriter&) const;

    const NumericPulseDegree* degreeForNumber(int number) const;
    const NumericPulseDegree* degreeForPitch(int pitch) const;
    const NumericPulseDegree* degreeForInputIndex(int inputIndex) const;
};

const NumericPulseProfile* numericPulseProfile(const Instrument* instrument);
// Built-in Kepatihan tuning profiles used by the notation converter. These
// profiles are independent of the playback instrument/template.
const NumericPulseProfile* numericPulseProfileById(const muse::String& id);
const NumericPulseProfile* numericPulseProfile(const Staff* staff, const Fraction& tick);
int numericPulseDefaultChordNoteLimit(const Staff* staff, const Fraction& tick);
int numericPulseChordNoteLimit(const Staff* staff, const Fraction& tick);
double numericPulseRowDistance(const Staff* staff, const Fraction& tick);
int numericPulseNoteRow(const Note* note);
muse::String numericPulseDegreeText(const NumericPulseProfile* profile, int pitch);
int numericPulseRegister(const NumericPulseProfile* profile, int pitch);
int numericPulsePitch(const NumericPulseProfile* profile, const NumericPulseDegree* degree, int registerOffset);

}

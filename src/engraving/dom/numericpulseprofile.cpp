/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#include "numericpulseprofile.h"

#include <algorithm>
#include <array>
#include <set>
#include <string>
#include <vector>

#include "instrument.h"
#include "chord.h"
#include "instrtemplate.h"
#include "measure.h"
#include "note.h"
#include "part.h"
#include "score.h"
#include "staff.h"
#include "stafftype.h"
#include "rw/xmlreader.h"
#include "rw/xmlwriter.h"

namespace mu::engraving {

namespace {

NumericPulseProfile makeKepatihanProfile(const String& id, const std::array<int, 6>& pitches)
{
    NumericPulseProfile profile;
    profile.id = id;

    static const std::array<String, 6> actions {
        u"note-c", u"note-d", u"note-e", u"note-f", u"note-g", u"note-a"
    };

    for (int i = 0; i < 6; ++i) {
        NumericPulseDegree degree;
        degree.number = i + 1;
        degree.scorePitch = pitches[i];
        degree.inputStep = 28 + i;
        degree.inputIndex = i;
        degree.action = actions[i];
        degree.label = String::number(i + 1);
        profile.degrees.push_back(degree);
    }

    return profile;
}

const std::vector<NumericPulseProfile>& kepatihanProfiles()
{
    // Pitches are MIDI note numbers. Each profile uses the ascending register
    // of the supplied tuning; the renderer derives Kepatihan octave dots from
    // the actual note's register relative to these pitches.
    static const std::vector<NumericPulseProfile> profiles {
        makeKepatihanProfile(u"kepatihan-tuning-8-1-med-diversity", { 62, 63, 66, 69, 70, 73 }), // D Eb F# A Bb C#
        makeKepatihanProfile(u"kepatihan-tuning-8-2-med-diversity", { 64, 65, 66, 69, 72, 73 }), // E F F# A C C#
        makeKepatihanProfile(u"kepatihan-tuning-8-3-med-diversity", { 62, 65, 66, 67, 70, 73 }), // D F F# G Bb C#
        makeKepatihanProfile(u"kepatihan-tuning-8-4-med-diversity", { 64, 65, 68, 69, 70, 73 }), // E F G# A Bb C#
        makeKepatihanProfile(u"kepatihan-tuning-8-1-max-uniformity", { 62, 63, 65, 66, 68, 70 }), // D Eb F F# G# Bb
        makeKepatihanProfile(u"kepatihan-tuning-8-2-max-uniformity", { 65, 66, 68, 69, 71, 73 }), // F F# G# A B Db
        makeKepatihanProfile(u"kepatihan-tuning-8-3-max-uniformity", { 62, 66, 67, 69, 70, 72 }), // D F# G A Bb C
        makeKepatihanProfile(u"kepatihan-tuning-8-4-max-uniformity", { 63, 65, 69, 70, 72, 73 }), // Eb F A Bb C Db
        makeKepatihanProfile(u"kepatihan-tuning-7-1-max-diversity", { 62, 65, 66, 68, 69, 72 }), // D F F# G# A C
        makeKepatihanProfile(u"kepatihan-tuning-7-2-max-diversity", { 63, 65, 68, 69, 71, 73 }), // Eb F G# A B C#
        makeKepatihanProfile(u"kepatihan-tuning-7-3-max-diversity", { 64, 66, 69, 70, 72, 73 }), // E F# A A# C C#
        makeKepatihanProfile(u"kepatihan-tuning-7-4-max-diversity", { 62, 65, 66, 68, 69, 73 }), // D F F# G# A C#
    };
    return profiles;
}

} // namespace

bool NumericPulseProfile::isValid() const
{
    if (id.empty() || degrees.empty() || minimumRegister > defaultRegister || defaultRegister > maximumRegister
        || chordNoteLimit < 0 || chordNoteLimit > 9) {
        return false;
    }

    std::set<int> numbers;
    std::set<int> pitchClasses;
    std::set<int> inputIndexes;
    std::set<std::string> actions;
    for (const NumericPulseDegree& degree : degrees) {
        const int pitchClass = (degree.scorePitch % 12 + 12) % 12;
        const int minimumPitch = degree.scorePitch + minimumRegister * 12;
        const int maximumPitch = degree.scorePitch + maximumRegister * 12;
        if (degree.number <= 0 || degree.scorePitch < 0 || degree.scorePitch > 127
            || minimumPitch < 0 || maximumPitch > 127
            || degree.inputIndex < 0 || degree.inputIndex > 6 || degree.label.empty() || degree.action.empty()
            || !numbers.insert(degree.number).second
            || !pitchClasses.insert(pitchClass).second
            || !inputIndexes.insert(degree.inputIndex).second
            || !actions.insert(degree.action.toStdString()).second) {
            return false;
        }
    }
    return true;
}

void NumericPulseProfile::read(XmlReader& e)
{
    id = e.attribute("id");
    degrees.clear();

    while (e.readNextStartElement()) {
        const AsciiStringView tag(e.name());
        if (tag == "fixedBaseline") {
            fixedBaseline = e.readBool();
        } else if (tag == "fontFamily") {
            fontFamily = e.readText();
        } else if (tag == "monophonic") {
            monophonic = e.readBool();
        } else if (tag == "showEmptyPulses") {
            showEmptyPulses = e.readBool();
        } else if (tag == "durationMarksAbove") {
            durationMarksAbove = e.readBool();
        } else if (tag == "chordNoteLimit") {
            chordNoteLimit = e.readInt();
        } else if (tag == "minimumRegister") {
            minimumRegister = e.readInt();
        } else if (tag == "maximumRegister") {
            maximumRegister = e.readInt();
        } else if (tag == "defaultRegister") {
            defaultRegister = e.readInt();
        } else if (tag == "degree") {
            NumericPulseDegree degree;
            degree.number = e.intAttribute("number");
            degree.scorePitch = e.intAttribute("pitch", -1);
            degree.inputStep = e.intAttribute("inputStep");
            degree.inputIndex = e.intAttribute("inputIndex", -1);
            degree.action = e.attribute("action");
            degree.label = e.readText();
            degrees.push_back(degree);
        } else {
            e.unknown();
        }
    }
}

void NumericPulseProfile::write(XmlWriter& xml) const
{
    if (id.empty()) {
        return;
    }

    xml.startElement("numericPulse", { { "id", id } });
    xml.tag("fixedBaseline", fixedBaseline);
    if (!fontFamily.empty()) {
        xml.tag("fontFamily", fontFamily);
    }
    xml.tag("monophonic", monophonic);
    xml.tag("showEmptyPulses", showEmptyPulses);
    xml.tag("durationMarksAbove", durationMarksAbove);
    if (chordNoteLimit > 0) {
        xml.tag("chordNoteLimit", chordNoteLimit);
    }
    xml.tag("minimumRegister", minimumRegister);
    xml.tag("maximumRegister", maximumRegister);
    xml.tag("defaultRegister", defaultRegister);
    for (const NumericPulseDegree& degree : degrees) {
        xml.tag("degree", {
            { "number", degree.number },
            { "pitch", degree.scorePitch },
            { "inputStep", degree.inputStep },
            { "inputIndex", degree.inputIndex },
            { "action", degree.action }
        }, degree.label);
    }
    xml.endElement();
}

const NumericPulseDegree* NumericPulseProfile::degreeForNumber(int number) const
{
    for (const NumericPulseDegree& degree : degrees) {
        if (degree.number == number) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseDegree* NumericPulseProfile::degreeForPitch(int pitch) const
{
    const int pitchClass = (pitch % 12 + 12) % 12;
    for (const NumericPulseDegree& degree : degrees) {
        if ((degree.scorePitch % 12 + 12) % 12 == pitchClass) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseDegree* NumericPulseProfile::degreeForInputIndex(int inputIndex) const
{
    for (const NumericPulseDegree& degree : degrees) {
        if (degree.inputIndex == inputIndex) {
            return &degree;
        }
    }
    return nullptr;
}

const NumericPulseProfile* numericPulseProfile(const Instrument* instrument)
{
    if (!instrument) {
        return nullptr;
    }
    // A numeric profile can be assigned independently of playback. This is
    // important for scores whose original instrument id is tied to a custom
    // SoundFont or VST track.
    const String& profileId = instrument->numericPulseProfileId().empty()
        ? instrument->id() : instrument->numericPulseProfileId();
    if (!instrument->numericPulseProfileId().empty()) {
        if (const NumericPulseProfile* profile = numericPulseProfileById(profileId)) {
            return profile;
        }
    }
    const InstrumentTemplate* instrumentTemplate = searchTemplate(profileId);
    if (!instrumentTemplate || instrumentTemplate->numericPulseProfile.id.empty()) {
        return nullptr;
    }
    return &instrumentTemplate->numericPulseProfile;
}

const NumericPulseProfile* numericPulseProfileById(const String& id)
{
    for (const NumericPulseProfile& profile : kepatihanProfiles()) {
        if (profile.id == id) {
            return &profile;
        }
    }

    // Backward-compatible id used by the first Gendèr converter prototype.
    if (id == u"gender-barung-tuning-8-1-med-diversity") {
        return &kepatihanProfiles().front();
    }

    return nullptr;
}

const NumericPulseProfile* numericPulseProfile(const Staff* staff, const Fraction& tick)
{
    return staff && staff->part() ? numericPulseProfile(staff->part()->instrument(tick)) : nullptr;
}

int numericPulseDefaultChordNoteLimit(const Staff* staff, const Fraction& tick)
{
    if (!staff || !staff->part()) {
        return 1;
    }

    const Instrument* instrument = staff->part()->instrument(tick);
    const NumericPulseProfile* profile = numericPulseProfile(instrument);
    if (!profile) {
        return 1;
    }

    // A conversion can keep the Kepatihan pitch mapping in a profile id while
    // the instrument template supplies TSI-specific behavior such as its chord
    // size. Prefer that setting when the instrument itself has a numeric profile.
    const InstrumentTemplate* instrumentTemplate = instrument ? searchTemplate(instrument->id()) : nullptr;
    if (instrumentTemplate && !instrumentTemplate->numericPulseProfile.id.empty()) {
        if (instrumentTemplate->numericPulseProfile.chordNoteLimit > 0) {
            return instrumentTemplate->numericPulseProfile.chordNoteLimit;
        }
        if (!instrumentTemplate->numericPulseProfile.monophonic) {
            return 9;
        }
    }

    if (profile->chordNoteLimit > 0) {
        return profile->chordNoteLimit;
    }

    return profile->monophonic ? 1 : 9;
}

int numericPulseChordNoteLimit(const Staff* staff, const Fraction& tick)
{
    if (!numericPulseProfile(staff, tick)) {
        return 1;
    }

    const int defaultLimit = numericPulseDefaultChordNoteLimit(staff, tick);
    Score* score = staff->score();
    Measure* measure = score ? score->tick2measure(tick) : nullptr;
    if (!measure) {
        return defaultLimit;
    }

    const int overrideLimit = measure->numericPulseChordNoteLimit(staff->idx());
    return overrideLimit > 0 ? overrideLimit : defaultLimit;
}

double numericPulseRowDistance(const Staff* staff, const Fraction& tick)
{
    const StaffType* staffType = staff->staffType(tick);
    // Leave room for a number, register dots, and its rhythm marks, as on the
    // original Gendèr's separate numeric staves.
    return 4.0 * staffType->lineDistance().val() * staff->spatium(tick);
}

int numericPulseNoteRow(const Note* note)
{
    if (note->numericPulseRow() >= 0) {
        return note->numericPulseRow();
    }
    if (!note->chord()) {
        return 0;
    }

    std::vector<Note*> notes = note->chord()->notes();
    std::stable_sort(notes.begin(), notes.end(), [](const Note* a, const Note* b) { return a->pitch() > b->pitch(); });
    std::set<int> occupiedRows;
    for (const Note* n : notes) {
        if (n->numericPulseRow() >= 0) {
            occupiedRows.insert(n->numericPulseRow());
        }
    }
    int row = 0;
    for (const Note* n : notes) {
        if (n->numericPulseRow() >= 0) {
            continue;
        }
        while (occupiedRows.count(row)) {
            ++row;
        }
        if (n == note) {
            return row;
        }
        occupiedRows.insert(row++);
    }
    return 0;
}

muse::String numericPulseDegreeText(const NumericPulseProfile* profile, int pitch)
{
    const NumericPulseDegree* degree = profile ? profile->degreeForPitch(pitch) : nullptr;
    return degree ? degree->label : muse::String(u"×");
}

int numericPulseRegister(const NumericPulseProfile* profile, int pitch)
{
    const NumericPulseDegree* degree = profile ? profile->degreeForPitch(pitch) : nullptr;
    if (!degree) {
        return 0;
    }

    const int difference = pitch - degree->scorePitch;
    return difference % 12 == 0 ? difference / 12 : 0;
}

int numericPulsePitch(const NumericPulseProfile* profile, const NumericPulseDegree* degree, int registerOffset)
{
    if (!profile || !degree || registerOffset < profile->minimumRegister || registerOffset > profile->maximumRegister) {
        return -1;
    }

    const int pitch = degree->scorePitch + registerOffset * 12;
    return pitch >= 0 && pitch <= 127 ? pitch : -1;
}

}

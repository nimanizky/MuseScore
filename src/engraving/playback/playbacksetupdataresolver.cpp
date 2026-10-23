/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "playbacksetupdataresolver.h"

#include "mapping/keyboardssetupdataresolver.h"
#include "mapping/stringssetupdataresolver.h"
#include "mapping/windssetupdataresolver.h"
#include "mapping/percussionssetupdataresolver.h"
#include "mapping/voicessetupdataresolver.h"

#include "log.h"

using namespace mu::engraving;
using namespace muse::mpe;

static const PlaybackSetupData PIANO_SETUP_DATA = {
    SoundId::Piano, SoundCategory::Keyboards
};

static bool resolveGamelanSetupData(const Instrument* instrument, PlaybackSetupData& result)
{
    const std::string instrumentId = instrument->id().toStdString();

    // Bundled Gamelan SoundFonts use a dedicated setup id so they can be
    // selected automatically instead of sharing the generic FluidSynth
    // resource used by orchestral instruments.
    if (instrumentId == "gender-barung-tuning-8-1-med-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-1-med-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-2-med-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-2-med-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-3-med-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-3-med-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-4-med-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-4-med-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-1-max-uniformity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-1-max-uniformity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-1-med-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-1-med-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-2-med-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-2-med-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-3-med-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-3-med-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-4-med-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-4-med-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-1-max-uniformity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-1-max-uniformity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-2-max-uniformity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-2-max-uniformity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-3-max-uniformity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-3-max-uniformity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-8-4-max-uniformity") {
        result = PlaybackSetupData(String(u"rebab-tuning-8-4-max-uniformity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-7-1-max-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-7-1-max-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-7-2-max-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-7-2-max-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-7-3-max-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-7-3-max-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "rebab-tuning-7-4-max-diversity") {
        result = PlaybackSetupData(String(u"rebab-tuning-7-4-max-diversity"), SoundCategory::Strings);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-2-max-uniformity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-2-max-uniformity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-3-max-uniformity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-3-max-uniformity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-8-4-max-uniformity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-8-4-max-uniformity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-7-1-max-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-7-1-max-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-7-2-max-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-7-2-max-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-7-3-max-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-7-3-max-diversity"), SoundCategory::Percussions);
        return true;
    }
    if (instrumentId == "gender-barung-tuning-7-4-max-diversity") {
        result = PlaybackSetupData(String(u"gender-barung-tuning-7-4-max-diversity"), SoundCategory::Percussions);
        return true;
    }

    if (instrumentId == "gender-pelog") {
        result = { SoundId::Metallophone, SoundCategory::Percussions, { SoundSubCategory::Orff } };
        return true;
    }
    if (instrumentId == "gambang-pelog") {
        result = { SoundId::Xylophone, SoundCategory::Percussions, { SoundSubCategory::Wooden } };
        return true;
    }
    if (instrumentId == "siter-pelog") {
        result = { SoundId::Sitar, SoundCategory::Strings, { SoundSubCategory::Plucked } };
        return true;
    }
    if (instrumentId == "suling-pelog") {
        result = { SoundId::Flute, SoundCategory::Winds };
        return true;
    }
    if (instrumentId == "bonang-barung-pelog") {
        result = { SoundId::Metallophone, SoundCategory::Percussions, { SoundSubCategory::Metal } };
        return true;
    }
    if (instrumentId == "selompret-pelog") {
        result = { SoundId::Shawm, SoundCategory::Winds, { SoundSubCategory::Reed } };
        return true;
    }
    if (instrumentId == "saron-pelog") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orchestral } };
        return true;
    }
    if (instrumentId == "saron-slendro") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orchestral,
                     SoundSubCategory::Temple } };
        return true;
    }
    if (instrumentId == "bonang-penerus-pelog" || instrumentId == "bonang-penerus-slendro") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal } };
        return true;
    }
    if (instrumentId == "gender-penerus-pelog" || instrumentId == "gender-penerus-slendro") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orff } };
        return true;
    }
    if (instrumentId == "gender-slenthem-pelog" || instrumentId == "gender-slenthem-slendro") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal } };
        return true;
    }
    if (instrumentId == "saron-peking-pelog" || instrumentId == "saron-peking-slendro"
        || instrumentId == "saron-demung-pelog" || instrumentId == "saron-demung-slendro") {
        result = { SoundId::Metallophone, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orchestral } };
        return true;
    }
    if (instrumentId == "kenong-pelog" || instrumentId == "kenong-slendro"
        || instrumentId == "kethuk-pelog" || instrumentId == "kethuk-slendro"
        || instrumentId == "kempyang-pelog" || instrumentId == "kempyang-slendro"
        || instrumentId == "gong-suwukan-pelog") {
        result = { SoundId::Gong, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Temple } };
        return true;
    }
    if (instrumentId == "kendhang-kepatihan" || instrumentId == "kendhang-ketipung"
        || instrumentId == "kendhang-ciblon" || instrumentId == "kendhang-gendhing") {
        result = { SoundId::Drum, SoundCategory::Percussions };
        return true;
    }
    if (instrumentId == "gong-kemodong-pelog") {
        result = { SoundId::Gong, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orff } };
        return true;
    }
    if (instrumentId == "gong-ageng-pelog") {
        result = { SoundId::Gong, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Orchestral } };
        return true;
    }
    if (instrumentId == "kempul-pelog") {
        result = { SoundId::Gong, SoundCategory::Percussions,
                   { SoundSubCategory::Metal, SoundSubCategory::Temple } };
        return true;
    }
    if (instrumentId == "rebab-pelog") {
        result = { SoundId::Violin, SoundCategory::Strings,
                   { SoundSubCategory::Orchestral, SoundSubCategory::Japanese } };
        return true;
    }

    return false;
}

void PlaybackSetupDataResolver::resolveSetupData(const Instrument* instrument, PlaybackSetupData& result) const
{
    if (resolveGamelanSetupData(instrument, result)) {
        result.supportsSingleNoteDynamics = instrument->singleNoteDynamics();
        result.musicXmlSoundId = std::make_optional(instrument->musicXmlId().toStdString());
        return;
    }

    if (!instrument->soundId().empty()) {
        result = PlaybackSetupData::fromString(instrument->soundId());
        result.supportsSingleNoteDynamics = instrument->singleNoteDynamics();
        result.musicXmlSoundId = std::make_optional(instrument->musicXmlId().toStdString());
        return;
    }

    if (KeyboardsSetupDataResolver::resolve(instrument, result)) {
        return;
    }

    if (StringsSetupDataResolver::resolve(instrument, result)) {
        return;
    }

    if (WindsSetupDataResolver::resolve(instrument, result)) {
        return;
    }

    if (PercussionsSetupDataResolver::resolve(instrument, result)) {
        return;
    }

    if (VoicesSetupDataResolver::resolve(instrument, result)) {
        return;
    }

    LOGW() << "Unable to resolve setup data for instrument, id: " << instrument->id()
           << ", family: " << instrument->family() << "; fallback to piano";

    result = PIANO_SETUP_DATA;
}

void PlaybackSetupDataResolver::resolveChordSymbolsSetupData(const Instrument* instrument, PlaybackSetupData& result) const
{
    if (instrument->hasStrings()) {
        static const PlaybackSetupData GUITAR_SETUP_DATA = {
            SoundId::Guitar, SoundCategory::Strings, { SoundSubCategory::Acoustic,
                                                       SoundSubCategory::Nylon,
                                                       SoundSubCategory::Plucked }
        };

        result = GUITAR_SETUP_DATA;
    } else {
        result = PIANO_SETUP_DATA;
    }
}

void PlaybackSetupDataResolver::resolveMetronomeSetupData(PlaybackSetupData& result) const
{
    static const PlaybackSetupData METRONOME_SETUP_DATA = {
        SoundId::Block, SoundCategory::Percussions, { SoundSubCategory::Wooden }
    };

    result = METRONOME_SETUP_DATA;
}

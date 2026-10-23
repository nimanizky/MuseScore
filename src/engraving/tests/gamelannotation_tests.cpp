/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <set>
#include <vector>
#include <QImage>
#include "draw/painter.h"
#include "engraving/rendering/paintoptions.h"
#include "engraving/rendering/score/tdraw.h"

#include "engraving/dom/beam.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/tuplet.h"
#include "engraving/dom/text.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/input.h"
#include "engraving/dom/instrtemplate.h"
#include "engraving/dom/masterscore.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/note.h"
#include "engraving/dom/numericpulseprofile.h"
#include "engraving/dom/part.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/slur.h"
#include "engraving/dom/staff.h"
#include "engraving/dom/stafflines.h"
#include "engraving/dom/stafftype.h"
#include "engraving/dom/timesig.h"
#include "engraving/editing/edittie.h"
#include "engraving/editing/editnote.h"
#include "engraving/editing/noteinput.h"
#include "engraving/editing/transaction/transaction.h"
#include "engraving/rendering/score/numericpulserendering.h"

#include "utils/scorerw.h"

using namespace mu::engraving;

namespace {

MasterScore* makeNumericPulseScore(const String& instrumentId = u"saron-pelog")
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    if (!score) {
        return nullptr;
    }

    const StaffType* staffType = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    if (!staffType) {
        delete score;
        return nullptr;
    }

    score->staff(0)->setStaffType(Fraction(0, 1), *staffType);
    score->staff(0)->part()->instrument()->setId(instrumentId);
    InputState& inputState = score->inputState();
    inputState.setTrack(0);
    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    inputState.setDuration(DurationType::V_QUARTER);
    inputState.setNoteEntryMode(true);
    return score;
}

void enterDegree(MasterScore* score, int degree, bool addFlag = true)
{
    NoteInputParams params;
    params.numericDegree = degree;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter numeric pulse"), [&](Transaction& tx) {
        NoteInput::addPitch(tx, score, params, addFlag, false);
    });
}

}

TEST(Engraving_GamelanNotationTests, InstrumentLookupCacheInvalidatesAfterReplacement)
{
    InstrumentGroup group;
    InstrumentTemplate* original = new InstrumentTemplate;
    original->id = u"kepatihan-cache-replacement-test";
    original->trackName = u"Original";
    group.instrumentTemplates.push_back(original);
    instrumentGroups.push_back(&group);
    EXPECT_EQ(searchTemplate(original->id), original);
    group.clear();
    InstrumentTemplate* replacement = new InstrumentTemplate;
    replacement->id = u"kepatihan-cache-replacement-test";
    replacement->trackName = u"Replacement";
    group.instrumentTemplates.push_back(replacement);
    EXPECT_EQ(searchTemplate(replacement->id), replacement);
    EXPECT_EQ(searchTemplate(replacement->id)->trackName, u"Replacement");
    group.clear();
    EXPECT_EQ(searchTemplate(u"kepatihan-cache-replacement-test"), nullptr);
    instrumentGroups.pop_back();
}

TEST(Engraving_GamelanNotationTests, RepeatedTsiLookupBenchmark)
{
    const String id = u"gender-barung-tuning-8-1-med-diversity";
    const InstrumentTemplate* expected = searchTemplate(id);
    ASSERT_NE(expected, nullptr);
    using Clock = std::chrono::steady_clock;
    constexpr int iterations = 10000;
    const auto linearStart = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        const InstrumentTemplate* found = nullptr;
        for (const InstrumentGroup* group : instrumentGroups) {
            for (const InstrumentTemplate* instrument : group->instrumentTemplates) {
                if (instrument->id == id) {
                    found = instrument;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        ASSERT_EQ(found, expected);
    }
    const auto cachedStart = Clock::now();
    for (int i = 0; i < iterations; ++i) {
        ASSERT_EQ(searchTemplate(id), expected);
    }
    const auto end = Clock::now();
    std::cout << "TSI lookup, " << iterations << " calls: linear "
              << std::chrono::duration<double, std::milli>(cachedStart - linearStart).count()
              << " ms, cached " << std::chrono::duration<double, std::milli>(end - cachedStart).count()
              << " ms\n";
}

TEST(Engraving_GamelanNotationTests, PelogProfileDefinesSevenNumericDegrees)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-pelog");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    ASSERT_TRUE(profile.isValid());

    const std::pair<int, int> mapping[] = {
        { 1, 60 }, { 2, 62 }, { 3, 64 }, { 4, 65 }, { 5, 67 }, { 6, 69 }, { 7, 71 }
    };

    for (const auto& [degree, pitch] : mapping) {
        ASSERT_NE(profile.degreeForNumber(degree), nullptr);
        EXPECT_EQ(profile.degreeForNumber(degree)->scorePitch, pitch);
        ASSERT_NE(profile.degreeForPitch(pitch), nullptr);
        EXPECT_EQ(profile.degreeForPitch(pitch)->number, degree);
        EXPECT_EQ(numericPulseDegreeText(&profile, pitch), muse::String::number(degree));
    }
}

TEST(Engraving_GamelanNotationTests, EveryGenderBarungTsiDefaultsToTwoChordNotes)
{
    const std::array<const char16_t*, 12> instrumentIds { {
        u"gender-barung-tuning-8-1-med-diversity",
        u"gender-barung-tuning-8-2-med-diversity",
        u"gender-barung-tuning-8-3-med-diversity",
        u"gender-barung-tuning-8-4-med-diversity",
        u"gender-barung-tuning-7-1-max-diversity",
        u"gender-barung-tuning-7-2-max-diversity",
        u"gender-barung-tuning-7-3-max-diversity",
        u"gender-barung-tuning-7-4-max-diversity",
        u"gender-barung-tuning-8-1-max-uniformity",
        u"gender-barung-tuning-8-2-max-uniformity",
        u"gender-barung-tuning-8-3-max-uniformity",
        u"gender-barung-tuning-8-4-max-uniformity"
    } };

    for (const char16_t* instrumentId : instrumentIds) {
        const InstrumentTemplate* instrumentTemplate = searchTemplate(muse::String(instrumentId));
        ASSERT_NE(instrumentTemplate, nullptr) << muse::String(instrumentId).toStdString();
        ASSERT_TRUE(instrumentTemplate->numericPulseProfile.isValid()) << muse::String(instrumentId).toStdString();
        EXPECT_FALSE(instrumentTemplate->numericPulseProfile.monophonic) << muse::String(instrumentId).toStdString();
        EXPECT_EQ(instrumentTemplate->numericPulseProfile.chordNoteLimit, 2) << muse::String(instrumentId).toStdString();
    }
}

TEST(Engraving_GamelanNotationTests, GenderBarungChordLimitCanBeOverriddenPerMeasure)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    Measure* firstMeasure = score->firstMeasure();
    ASSERT_NE(firstMeasure, nullptr);
    Measure* secondMeasure = firstMeasure->nextMeasure();
    ASSERT_NE(secondMeasure, nullptr);

    firstMeasure->setNumericPulseChordNoteLimit(5, 0);
    EXPECT_EQ(numericPulseDefaultChordNoteLimit(score->staff(0), firstMeasure->tick()), 2);
    EXPECT_EQ(numericPulseChordNoteLimit(score->staff(0), firstMeasure->tick()), 5);
    EXPECT_EQ(numericPulseChordNoteLimit(score->staff(0), secondMeasure->tick()), 2);

    const String savePath = u"gender-barung-chord-limit-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    delete score;
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    Measure* reopenedFirst = reopened->firstMeasure();
    ASSERT_NE(reopenedFirst, nullptr);
    Measure* reopenedSecond = reopenedFirst->nextMeasure();
    ASSERT_NE(reopenedSecond, nullptr);
    EXPECT_EQ(reopenedFirst->numericPulseChordNoteLimit(0), 5);
    EXPECT_EQ(numericPulseChordNoteLimit(reopened->staff(0), reopenedFirst->tick()), 5);
    EXPECT_EQ(reopenedSecond->numericPulseChordNoteLimit(0), 0);
    EXPECT_EQ(numericPulseChordNoteLimit(reopened->staff(0), reopenedSecond->tick()), 2);

    delete reopened;
}

TEST(Engraving_GamelanNotationTests, GenderBarungAddsChordNotesUpToTheMeasureLimit)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    Measure* measure = score->firstMeasure();
    ASSERT_NE(measure, nullptr);
    measure->setNumericPulseChordNoteLimit(5, 0);

    enterDegree(score, 1, false); // establish the chord
    enterDegree(score, 2);
    enterDegree(score, 3);
    enterDegree(score, 4);
    enterDegree(score, 5);
    enterDegree(score, 6); // the fifth note is the local limit

    Chord* chord = measure->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->notes().size(), 5u);
    EXPECT_EQ(numericPulseChordNoteLimit(score->staff(0), measure->tick()), 5);
    EXPECT_EQ(numericPulseChordNoteLimit(score->staff(0), measure->nextMeasure()->tick()), 2);
    delete score;
}

TEST(Engraving_GamelanNotationTests, RequestedPelogInstrumentProfilesAreAvailable)
{
    struct ExpectedProfile {
        const char16_t* instrumentId;
        std::vector<int> degrees;
    };

    const std::array<ExpectedProfile, 9> expectedProfiles { {
        { u"saron-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gender-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gambang-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"rebab-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"suling-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"siter-pelog", { 1, 2, 3, 4, 5, 6, 7 } },
        { u"gong-kemodong-pelog", { 5, 6 } },
        { u"gong-ageng-pelog", { 6 } },
        { u"kempul-pelog", { 1, 3, 5, 6, 7 } }
    } };

    for (const ExpectedProfile& expected : expectedProfiles) {
        const InstrumentTemplate* instrumentTemplate = searchTemplate(muse::String(expected.instrumentId));
        ASSERT_NE(instrumentTemplate, nullptr) << muse::String(expected.instrumentId).toStdString();
        ASSERT_NE(instrumentTemplate->staffTypePreset, nullptr);
        EXPECT_TRUE(instrumentTemplate->staffTypePreset->isNumericPulseStaff());

        const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
        ASSERT_TRUE(profile.isValid()) << muse::String(expected.instrumentId).toStdString();
        EXPECT_TRUE(instrumentTemplate->containsGenre(u"common")) << muse::String(expected.instrumentId).toStdString();
        EXPECT_TRUE(instrumentTemplate->containsGenre(u"world")) << muse::String(expected.instrumentId).toStdString();
        ASSERT_EQ(profile.degrees.size(), expected.degrees.size());
        for (size_t i = 0; i < expected.degrees.size(); ++i) {
            EXPECT_EQ(profile.degrees[i].number, expected.degrees[i]);
        }
    }
}

TEST(Engraving_GamelanNotationTests, TwoHandedInstrumentsUseTwoNumericPulseStaves)
{
    // Gendèr, gambang and siter are played with two independent hands, so each
    // hand needs its own numeric-pulse line rather than sharing a single staff.
    const std::array<const char16_t*, 3> twoHandedInstruments { {
        u"gender-pelog",
        u"gambang-pelog",
        u"siter-pelog"
    } };

    for (const char16_t* instrumentId : twoHandedInstruments) {
        const InstrumentTemplate* instrumentTemplate = searchTemplate(muse::String(instrumentId));
        ASSERT_NE(instrumentTemplate, nullptr) << muse::String(instrumentId).toStdString();
        EXPECT_EQ(instrumentTemplate->staffCount, size_t(2)) << muse::String(instrumentId).toStdString();

        // Both hands must render as one-line numeric-pulse staves. staffLines is
        // only populated per staff index that the instrument file mentions, so a
        // missing <stafflines staff="2"> would silently leave the lower hand at
        // the five-line default.
        for (staff_idx_t staffIdx = 0; staffIdx < instrumentTemplate->staffCount; ++staffIdx) {
            EXPECT_EQ(instrumentTemplate->staffLines[staffIdx], 1)
                << muse::String(instrumentId).toStdString() << " staff " << staffIdx;
        }

        // The brace and spanning barlines are what read the two lines as one instrument.
        EXPECT_EQ(instrumentTemplate->bracket[0], BracketType::BRACE) << muse::String(instrumentId).toStdString();
        EXPECT_EQ(instrumentTemplate->bracketSpan[0], 2) << muse::String(instrumentId).toStdString();
        // barlineSpan is a per-staff bool: <barlineSpan>2</barlineSpan> marks staff 0
        // as joined to the staff below it.
        EXPECT_TRUE(instrumentTemplate->barlineSpan[0]) << muse::String(instrumentId).toStdString();

        // Widened so the lower hand can be written an octave below the baseline.
        EXPECT_LE(instrumentTemplate->minPitchA, 48) << muse::String(instrumentId).toStdString();
    }
}

TEST(Engraving_GamelanNotationTests, SlendroProfileRemainsAvailableForCompatibility)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-slendro");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    ASSERT_TRUE(profile.isValid());
    EXPECT_TRUE(instrumentTemplate->containsGenre(u"common"));
    EXPECT_TRUE(instrumentTemplate->containsGenre(u"world"));
    EXPECT_EQ(profile.degrees.size(), 5u);
    EXPECT_EQ(profile.degreeForNumber(1)->inputStep, 28);
    EXPECT_EQ(profile.degreeForNumber(2)->inputStep, 29);
    EXPECT_EQ(profile.degreeForNumber(3)->inputStep, 30);
    EXPECT_EQ(profile.degreeForNumber(5)->inputStep, 32);
    EXPECT_EQ(profile.degreeForNumber(6)->inputStep, 33);
    EXPECT_EQ(profile.degreeForNumber(4), nullptr);
    EXPECT_EQ(profile.degreeForNumber(7), nullptr);
}

TEST(Engraving_GamelanNotationTests, NumericPulseProfileRejectsDuplicateDegreeDefinitions)
{
    NumericPulseProfile profile;
    profile.id = u"invalid-duplicate";
    profile.degrees = {
        { 1, 60, 28, 0, u"1", u"note-c" },
        { 1, 62, 29, 1, u"1", u"note-d" }
    };
    EXPECT_FALSE(profile.isValid());
}

TEST(Engraving_GamelanNotationTests, NumericPulseRegisterMappingUsesProfileBounds)
{
    const InstrumentTemplate* instrumentTemplate = searchTemplate(u"saron-pelog");
    ASSERT_NE(instrumentTemplate, nullptr);
    const NumericPulseProfile& profile = instrumentTemplate->numericPulseProfile;
    const NumericPulseDegree* degree = profile.degreeForNumber(5);
    ASSERT_NE(degree, nullptr);

    EXPECT_EQ(numericPulsePitch(&profile, degree, -1), 55);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 0), 67);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 1), 79);
    EXPECT_EQ(numericPulsePitch(&profile, degree, 2), -1);
    EXPECT_EQ(numericPulseRegister(&profile, 55), -1);
    EXPECT_EQ(numericPulseRegister(&profile, 79), 1);
    EXPECT_EQ(numericPulseDegreeText(&profile, 79), muse::String(u"5"));
}

TEST(Engraving_GamelanNotationTests, RegisterDotsExpandGlyphAndMoveDurationMarksClear)
{
    const double spatium = 1.0;
    const muse::String text(u"5");
    const muse::RectF plain = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, 0, 1);
    const muse::RectF upper = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, 1, 1);
    const muse::RectF lower = rendering::score::numericPulseNoteBbox(text, 1.0, spatium, -1, 1);

    EXPECT_LT(upper.top(), plain.top());
    EXPECT_GT(lower.bottom(), plain.bottom());
    EXPECT_LT(rendering::score::numericPulseDurationMarkY(upper, spatium, 0), upper.top());
    EXPECT_DOUBLE_EQ(rendering::score::numericPulseRegisterDotDiameter(spatium), 0.60 * spatium);
}

TEST(Engraving_GamelanNotationTests, SelectedNumericRhythmChangesPreserveChordPitchesAndRows)
{
    for (const char16_t* instrumentId : { u"gender-barung-tuning-8-1-med-diversity", u"rebab-tuning-8-4-max-uniformity" }) {
        for (bool entering : { false, true }) {
            MasterScore* score = makeNumericPulseScore(instrumentId);
            ASSERT_NE(score, nullptr);
            enterDegree(score, 5, false);
            if (numericPulseDefaultChordNoteLimit(score->staff(0), Fraction(0, 1)) > 1) {
                enterDegree(score, 2);
            }
            Chord* original = score->firstMeasure()->findChord(Fraction(0, 1), 0);
            ASSERT_NE(original, nullptr);
            std::vector<std::pair<int, int>> pitchesAndRows;
            for (const Note* note : original->notes()) {
                pitchesAndRows.emplace_back(note->pitch(), note->numericPulseRow());
            }
            score->inputState().setNoteEntryMode(entering);
            for (DurationType duration : { DurationType::V_EIGHTH, DurationType::V_16TH, DurationType::V_HALF,
                                          DurationType::V_WHOLE, DurationType::V_QUARTER }) {
                Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
                ASSERT_NE(chord, nullptr);
                const DurationType oldDuration = chord->durationType().type();
                score->select(chord->upNote());
                score->transactionManager()->transaction(TranslatableString::untranslatable("Change selected rhythm"), [&](Transaction& tx) {
                    NoteInput::setDuration(tx, score, duration);
                });
                chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
                ASSERT_NE(chord, nullptr);
                EXPECT_EQ(chord->durationType().type(), duration);
                EXPECT_EQ(chord->notes().size(), pitchesAndRows.size());
                for (const auto& [pitch, row] : pitchesAndRows) {
                    ASSERT_NE(chord->findNote(pitch), nullptr);
                    EXPECT_EQ(chord->findNote(pitch)->numericPulseRow(), row);
                }
                score->undoRedo(true, nullptr);
                chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
                ASSERT_NE(chord, nullptr);
                EXPECT_EQ(chord->durationType().type(), oldDuration);
                score->undoRedo(false, nullptr);
            }
            delete score;
        }
    }
}

TEST(Engraving_GamelanNotationTests, NumericDigitMetricsCachePreservesGeometryAndIsBounded)
{
    using namespace rendering::score;
    clearNumericPulseTextMetricsCache();
    for (double scale : { 0.7, 1.0, 1.5 }) {
        for (int degree = 1; degree <= 9; ++degree) {
            const String text = String::number(degree);
            const muse::draw::FontMetrics metrics(numericPulseFont(scale));
            RectF expected = metrics.tightBoundingRect(text);
            expected.translate(-expected.center().x(), 0.0);
            for (int repeat = 0; repeat < 3; ++repeat) {
                const RectF actual = numericPulseTextBbox(text, scale);
                EXPECT_DOUBLE_EQ(actual.x(), expected.x());
                EXPECT_DOUBLE_EQ(actual.y(), expected.y());
                EXPECT_DOUBLE_EQ(actual.width(), expected.width());
                EXPECT_DOUBLE_EQ(actual.height(), expected.height());
                EXPECT_DOUBLE_EQ(numericPulseTextOriginX(text, scale), -metrics.horizontalAdvance(text) * 0.5);
            }
        }
    }
    for (int i = 1; i <= 100; ++i) {
        numericPulseTextBbox(u"1", 0.5 + i * 0.01);
    }
    EXPECT_LE(numericPulseTextMetricsCache().size(), 64u);
    clearNumericPulseTextMetricsCache();
    EXPECT_TRUE(numericPulseTextMetricsCache().empty());
}

TEST(Engraving_GamelanNotationTests, OptionalRealScoreRhythmBenchmark)
{
    const char* path = std::getenv("MUSESCORE_RHYTHM_BENCHMARK_SCORE");
    if (!path) {
        GTEST_SKIP() << "Set MUSESCORE_RHYTHM_BENCHMARK_SCORE for a read-only, in-memory edit benchmark";
    }
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(String::fromUtf8(path), true));
    ASSERT_NE(score, nullptr);
    Chord* target = nullptr;
    for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment && !target;
         segment = segment->next1(SegmentType::ChordRest)) {
        for (track_idx_t track = 0; track < score->ntracks(); ++track) {
            EngravingItem* element = segment->element(track);
            if (element && element->isChord() && !toChord(element)->tuplet()
                && numericPulseProfile(element->staff(), element->tick())) {
                target = toChord(element);
                break;
            }
        }
    }
    ASSERT_NE(target, nullptr);
    const Fraction tick = target->tick();
    const track_idx_t track = target->track();
    const int pitch = target->upNote()->pitch();
    score->inputState().setNoteEntryMode(false);
    for (DurationType duration : { DurationType::V_EIGHTH, DurationType::V_16TH, DurationType::V_QUARTER }) {
        Chord* chord = score->tick2measure(tick)->findChord(tick, track);
        ASSERT_NE(chord, nullptr);
        if (chord->durationType().type() == duration) {
            continue;
        }
        score->select(chord->upNote());
        const auto start = std::chrono::steady_clock::now();
        score->transactionManager()->transaction(TranslatableString::untranslatable("Benchmark rhythm edit"), [&](Transaction& tx) {
            NoteInput::setDuration(tx, score.get(), duration);
        });
        std::cout << "Real-score rhythm edit: "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
                  << " ms\n";
        chord = score->tick2measure(tick)->findChord(tick, track);
        ASSERT_NE(chord, nullptr);
        EXPECT_EQ(chord->upNote()->pitch(), pitch);
        EXPECT_EQ(chord->durationType().type(), duration);
        score->undoRedo(true, nullptr);
    }
    // Never save: the original score and its soundfont assignments are untouched.
}

TEST(Engraving_GamelanNotationTests, RegisterDotsStayCloseToVisibleDigits)
{
    for (int degree = 1; degree <= 6; ++degree) {
        const muse::String text = muse::String::number(degree);
        const double spatium = 1.0;
        const muse::RectF digit = rendering::score::numericPulseTextBbox(text, 1.0);
        const muse::RectF upper = rendering::score::numericPulseRegisterDotBbox(digit, spatium, 1, 0);
        const muse::RectF lower = rendering::score::numericPulseRegisterDotBbox(digit, spatium, -1, 0);
        EXPECT_NEAR(digit.top() - upper.bottom(), 0.10 * spatium, 1e-6);
        EXPECT_NEAR(lower.top() - digit.bottom(), 0.10 * spatium, 1e-6);
        EXPECT_GT(digit.top(), upper.bottom());
        EXPECT_GT(lower.top(), digit.bottom());
    }
}

TEST(Engraving_GamelanNotationTests, EveryTsiArrowCrossesOctavesInBothDirections)
{
    const std::array<const char16_t*, 12> tunings {
        u"8-1-med-diversity", u"8-2-med-diversity", u"8-3-med-diversity", u"8-4-med-diversity",
        u"8-1-max-uniformity", u"8-2-max-uniformity", u"8-3-max-uniformity", u"8-4-max-uniformity",
        u"7-1-max-diversity", u"7-2-max-diversity", u"7-3-max-diversity", u"7-4-max-diversity"
    };
    for (const char16_t* prefix : { u"gender-barung-tuning-", u"rebab-tuning-" }) {
        for (const char16_t* tuning : tunings) {
            const String instrumentId = String(prefix) + String(tuning);
            SCOPED_TRACE(instrumentId.toStdString());
            MasterScore* score = makeNumericPulseScore(instrumentId);
            ASSERT_NE(score, nullptr);
            enterDegree(score, 6, false);
            Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
            ASSERT_NE(chord, nullptr);
            Note* note = chord->upNote();
            ASSERT_NE(note, nullptr);
            const NumericPulseProfile* profile = numericPulseProfile(note->staff(), note->tick());
            ASSERT_NE(profile, nullptr);
            score->select(note);
            auto move = [&](bool up) {
                score->startCmd(TranslatableString::untranslatable("Step numeric degree"));
                EditNote::upDown(score, up, UpDownMode::NUMERIC_PULSE);
                score->endCmd();
            };
            move(true);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(1)->scorePitch + 12);
            EXPECT_EQ(numericPulseRegister(profile, note->pitch()), 1);
            move(true);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(2)->scorePitch + 12);
            move(false);
            move(false);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(6)->scorePitch);
            for (int i = 0; i < 5; ++i) {
                move(false);
            }
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(1)->scorePitch);
            move(false);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(6)->scorePitch - 12);
            EXPECT_EQ(numericPulseRegister(profile, note->pitch()), -1);
            move(false);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(5)->scorePitch - 12);
            score->undoRedo(true, nullptr);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(6)->scorePitch - 12);
            score->undoRedo(false, nullptr);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(5)->scorePitch - 12);
            score->startCmd(TranslatableString::untranslatable("Set upper register"));
            EditNote::setNumericPulseRegister(score, 1);
            score->endCmd();
            move(true);
            move(true);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(1)->scorePitch + 24);
            EXPECT_EQ(numericPulseRegister(profile, note->pitch()), 2);
            score->startCmd(TranslatableString::untranslatable("Set lower register"));
            EditNote::setNumericPulseRegister(score, -1);
            score->endCmd();
            move(false);
            EXPECT_EQ(note->pitch(), profile->degreeForNumber(6)->scorePitch - 24);
            EXPECT_EQ(numericPulseRegister(profile, note->pitch()), -2);
            delete score;
        }
    }
}

TEST(Engraving_GamelanNotationTests, GenderBarungRowsAcceptDirectInputAndKeepTheirNumbers)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    InputState& is = score->inputState();
    Measure* measure = score->firstMeasure();
    ASSERT_NE(measure, nullptr);
    score->transactionManager()->transaction(TranslatableString::untranslatable("Prepare numeric pulses"), [&](Transaction&) {
        score->normalizeNumericPulseMeasureRests();
    });
    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    const NumericPulseProfile* profile = numericPulseProfile(score->staff(0), segment->tick());
    ASSERT_NE(profile, nullptr);

    auto clickRow = [&](int row, int degree) {
        is.setNotes({ NoteInput::noteVal(score, profile->degreeForNumber(degree)->scorePitch, 0, false) });
        Position position;
        position.segment = segment;
        position.staffIdx = 0;
        position.numericPulseRow = row;
        score->transactionManager()->transaction(TranslatableString::untranslatable("Click numeric row"), [&](Transaction& tx) {
            EXPECT_TRUE(NoteInput::putNote(tx, score, position, true));
        });
    };
    clickRow(1, 6); // Enter the bottom rail first, without Add-to-chord.
    clickRow(0, 1);
    Chord* chord = measure->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 2u);
    Note* top = chord->findNote(profile->degreeForNumber(1)->scorePitch);
    Note* bottom = chord->findNote(profile->degreeForNumber(6)->scorePitch);
    ASSERT_NE(top, nullptr);
    ASSERT_NE(bottom, nullptr);
    EXPECT_EQ(top->numericPulseRow(), 0);
    EXPECT_EQ(bottom->numericPulseRow(), 1);
    EXPECT_EQ(chord->durationType().type(), DurationType::V_QUARTER);

    clickRow(0, 5); // Repitch only the top rail; do not replace the chord.
    EXPECT_EQ(chord->notes().size(), 2u);
    EXPECT_EQ(top->pitch(), profile->degreeForNumber(5)->scorePitch);
    EXPECT_EQ(bottom->pitch(), profile->degreeForNumber(6)->scorePitch);
    EXPECT_EQ(bottom->numericPulseRow(), 1);
    score->undoRedo(true, nullptr);
    EXPECT_EQ(top->pitch(), profile->degreeForNumber(1)->scorePitch);
    score->undoRedo(false, nullptr);

    measure->setNumericPulseChordNoteLimit(5, 0);
    clickRow(4, 3); // Leave rows 2 and 3 empty.
    EXPECT_EQ(chord->notes().size(), 3u);
    Note* lastRow = chord->findNote(profile->degreeForNumber(3)->scorePitch);
    ASSERT_NE(lastRow, nullptr);
    EXPECT_EQ(lastRow->numericPulseRow(), 4);
    score->doLayout();
    ASSERT_EQ(measure->staffLines(0)->lines().size(), 5u);
    EXPECT_GT(numericPulseRowDistance(score->staff(0), measure->tick()), 3.0 * score->staff(0)->spatium(measure->tick()));
    EXPECT_DOUBLE_EQ(lastRow->pos().y() - top->pos().y(), 4.0 * numericPulseRowDistance(score->staff(0), measure->tick()));

    is.setSegment(segment);
    is.setNumericPulseRow(0);
    score->transactionManager()->transaction(TranslatableString::untranslatable("Clear top numeric row"), [&](Transaction& tx) {
        NoteInput::addRest(tx, score);
    });
    EXPECT_EQ(chord->notes().size(), 2u);
    EXPECT_EQ(bottom->numericPulseRow(), 1);
    EXPECT_EQ(lastRow->numericPulseRow(), 4);
    score->undoRedo(true, nullptr);
    EXPECT_EQ(chord->notes().size(), 3u);
    const int bottomPitch = bottom->pitch();
    const int lastRowPitch = lastRow->pitch();
    const String savePath = u"gender-barung-rows-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    delete score;
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);
    Chord* reopenedChord = reopened->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(reopenedChord, nullptr);
    ASSERT_NE(reopenedChord->findNote(lastRowPitch), nullptr);
    ASSERT_NE(reopenedChord->findNote(bottomPitch), nullptr);
    EXPECT_EQ(reopenedChord->findNote(lastRowPitch)->numericPulseRow(), 4);
    EXPECT_EQ(reopenedChord->findNote(bottomPitch)->numericPulseRow(), 1);
    delete reopened;
}

TEST(Engraving_GamelanNotationTests, GenderBarungTimeSignatureStaysBetweenDefaultRails)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    Measure* measure = score->firstMeasure();
    Segment* segment = measure->findSegment(SegmentType::TimeSig, measure->tick());
    ASSERT_NE(segment, nullptr);
    TimeSig* timeSig = toTimeSig(segment->element(0));
    ASSERT_NE(timeSig, nullptr);
    for (int rows : { 0, 3, 6, 9 }) {
        measure->setNumericPulseChordNoteLimit(rows, 0);
        measure->nextMeasure()->setNumericPulseChordNoteLimit(9, 0);
        for (TimeSigPlacement placement : { TimeSigPlacement::NORMAL, TimeSigPlacement::ABOVE_STAVES,
                                           TimeSigPlacement::ACROSS_STAVES }) {
            score->style().set(Sid::timeSigPlacement, placement);
            score->style().set(Sid::timeSigNormalY, 3.0);
            score->doLayout();
            EXPECT_EQ(timeSig->timeSigPlacement(), TimeSigPlacement::NORMAL);
            const double center = (timeSig->ldata()->pz.y() + timeSig->ldata()->pn.y()) * .5 + timeSig->pos().y();
            EXPECT_NEAR(center, numericPulseRowDistance(score->staff(0), measure->tick()) * .5, 0.001);
        }
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, GenderBarungDegreeInputUsesTheActiveRowWithoutAdd)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    score->firstMeasure()->setNumericPulseChordNoteLimit(9, 0);
    score->transactionManager()->transaction(TranslatableString::untranslatable("Prepare numeric pulses"), [&](Transaction&) {
        score->normalizeNumericPulseMeasureRests();
    });
    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    InputState& is = score->inputState();
    for (int row : { 8, 0, 4 }) {
        is.setSegment(segment);
        is.setNumericPulseRow(row);
        enterDegree(score, 1, false); // Same pitch on three different rails is allowed.
    }
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->notes().size(), 3u);
    std::set<int> rows;
    for (const Note* note : chord->notes()) {
        rows.insert(note->numericPulseRow());
    }
    EXPECT_EQ(rows, (std::set<int> { 0, 4, 8 }));
    score->doLayout();
    ASSERT_EQ(score->firstMeasure()->staffLines(0)->lines().size(), 9u);
    for (const Note* note : chord->notes()) {
        Position position;
        EXPECT_TRUE(score->getPosition(&position, note->canvasPos(), 0));
        EXPECT_EQ(position.numericPulseRow, note->numericPulseRow());
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, EmptyPulseDotIsReadableAtNormalScale)
{
    const double spatium = 2.0;
    const muse::RectF bbox = rendering::score::numericPulseEmptyPulseBbox(spatium);

    EXPECT_DOUBLE_EQ(bbox.width(), 0.50 * spatium);
    EXPECT_DOUBLE_EQ(bbox.height(), 0.50 * spatium);
    EXPECT_LT(bbox.bottom(), 0.0);
}

TEST(Engraving_GamelanNotationTests, WholeMeasureEmptyPulseCountFollowsMeter)
{
    EXPECT_EQ(rendering::score::numericPulseMeasurePulseCount(Fraction(4, 4), Fraction(4, 4)), 4);
    EXPECT_EQ(rendering::score::numericPulseMeasurePulseCount(Fraction(3, 4), Fraction(3, 4)), 3);
    EXPECT_EQ(rendering::score::numericPulseMeasurePulseCount(Fraction(6, 8), Fraction(6, 8)), 6);
    EXPECT_EQ(rendering::score::numericPulseMeasurePulseCount(Fraction(1, 4), Fraction(4, 4)), 1);

    const double spatium = 2.0;
    const muse::RectF group = rendering::score::numericPulseEmptyPulseGroupBbox(spatium, 4, 20.0);
    EXPECT_GT(group.width(), rendering::score::numericPulseEmptyPulseBbox(spatium).width());
    EXPECT_LT(group.left(), 0.0);
    EXPECT_GT(group.right(), 0.0);
}

TEST(Engraving_GamelanNotationTests, WholeMeasureRestLaysOutOneDotPerPulse)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    Rest* rest = toRest(segment->element(0));
    ASSERT_NE(rest, nullptr);
    ASSERT_TRUE(rest->durationType().isMeasure());

    score->setLayoutAll();
    score->doLayout();

    const int pulseCount = rendering::score::numericPulseMeasurePulseCount(
        rest->measure()->ticks(), rest->measure()->timesig());
    const muse::RectF expected = rendering::score::numericPulseEmptyPulseGroupBbox(
        rest->spatium(), pulseCount, rest->measure()->width());
    EXPECT_EQ(pulseCount, 4);
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().left(), expected.left());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().right(), expected.right());

    delete score;
}

TEST(Engraving_GamelanNotationTests, FillingNumericMeasureCreatesIndependentlyEditablePulseRests)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    int normalizedMeasureCount = 0;
    {
        ScoreLoad scoreLoad;
        normalizedMeasureCount = score->normalizeNumericPulseMeasureRests();
    }
    EXPECT_GT(normalizedMeasureCount, 0);

    for (int pulseIndex = 0; pulseIndex < 4; ++pulseIndex) {
        Segment* segment = score->tick2segment(Fraction(pulseIndex, 4), false, SegmentType::ChordRest);
        ASSERT_NE(segment, nullptr) << pulseIndex;
        Rest* pulseRest = toRest(segment->element(0));
        ASSERT_NE(pulseRest, nullptr) << pulseIndex;
        EXPECT_EQ(pulseRest->durationType().type(), DurationType::V_QUARTER);
        EXPECT_FALSE(pulseRest->durationType().isMeasure());
    }

    delete score;
}

TEST(Engraving_GamelanNotationTests, NormalizedMeasureStartsInputAtFirstPulse)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    int normalizedMeasureCount = 0;
    {
        ScoreLoad scoreLoad;
        normalizedMeasureCount = score->normalizeNumericPulseMeasureRests();
    }
    ASSERT_GT(normalizedMeasureCount, 0);

    InputState& inputState = score->inputState();
    Segment* firstPulse = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(firstPulse, nullptr);
    inputState.setSegment(firstPulse);
    ASSERT_TRUE(inputState.cr()->isRest());
    EXPECT_EQ(toRest(inputState.cr())->durationType().type(), DurationType::V_QUARTER);

    enterDegree(score, 2);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->durationType().type(), DurationType::V_QUARTER);
    EXPECT_EQ(score->inputState().tick(), Fraction(1, 4));

    for (int pulseIndex = 1; pulseIndex < 4; ++pulseIndex) {
        Segment* segment = score->tick2segment(Fraction(pulseIndex, 4), false, SegmentType::ChordRest);
        ASSERT_NE(segment, nullptr) << pulseIndex;
        ASSERT_NE(segment->element(0), nullptr) << pulseIndex;
        EXPECT_TRUE(segment->element(0)->isRest()) << pulseIndex;
    }

    delete score;
}

TEST(Engraving_GamelanNotationTests, WesternAugmentationDotIsHiddenOnNumericPulseStaff)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDots(1);
    enterDegree(score, 4);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->upNote()->dots().size(), 1u);

    score->setLayoutAll();
    score->doLayout();
    EXPECT_TRUE(chord->upNote()->dot(0)->ldata()->bbox().isEmpty());

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericPulseStaffPreset)
{
    const StaffType* staffType = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    ASSERT_NE(staffType, nullptr);
    EXPECT_TRUE(staffType->isNumericPulseStaff());
    EXPECT_EQ(staffType->group(), StaffGroup::STANDARD);
    EXPECT_EQ(staffType->lines(), 1);
    EXPECT_TRUE(staffType->showBarlines());
    EXPECT_TRUE(staffType->genTimesig());
    EXPECT_FALSE(staffType->genClef());
    EXPECT_FALSE(staffType->genKeysig());
    EXPECT_FALSE(staffType->showLedgerLines());
}

TEST(Engraving_GamelanNotationTests, LegacySlendroStaffPresetNameResolvesToNumericPulse)
{
    const StaffType* numericPulse = StaffType::preset(StaffTypes::NUMERIC_PULSE);
    ASSERT_NE(numericPulse, nullptr);
    EXPECT_EQ(StaffType::presetFromXmlName(u"stdGamelanSlendro"), numericPulse);
}

TEST(Engraving_GamelanNotationTests, WesternInstrumentHasNoNumericPulseProfile)
{
    MasterScore* score = ScoreRW::readScore(u"durationtype_data/empty.mscx");
    ASSERT_NE(score, nullptr);
    EXPECT_EQ(numericPulseProfile(score->staff(0), Fraction(0, 1)), nullptr);
    EXPECT_FALSE(score->staff(0)->staffType(Fraction(0, 1))->isNumericPulseStaff());
    delete score;
}

TEST(Engraving_GamelanNotationTests, PelogAcceptsAllSevenDegreeInputs)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    for (int inputIndex = 0; inputIndex < 7; ++inputIndex) {
        NoteInputParams params;
        EXPECT_TRUE(NoteInput::resolveNoteInputParams(score, inputIndex, false, params));
        EXPECT_EQ(params.numericDegree, inputIndex + 1);
    }

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowPitchModeStepsThroughPelogDegrees)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 65);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(4));

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 64);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(3));

    delete score;
}

TEST(Engraving_GamelanNotationTests, OctavePitchModeMovesAndClampsNumericRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    EditNote::upDown(score, true, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 72);
    EXPECT_EQ(numericPulseRegister(numericPulseProfile(note->staff(), note->tick()), note->pitch()), 1);

    EditNote::upDown(score, true, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 72);

    EditNote::upDown(score, false, UpDownMode::OCTAVE);
    EXPECT_EQ(note->pitch(), 60);
    EditNote::setNumericPulseRegister(score, -1);
    EXPECT_EQ(note->pitch(), 48);

    delete score;
}

TEST(Engraving_GamelanNotationTests, GenderBatchOctavePreservesDegreesAndRhythm)
{
    MasterScore* score = makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1, false);
    enterDegree(score, 3);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 2u);
    const auto duration = chord->ticks();
    std::vector<int> original;
    std::vector<EngravingItem*> selected;
    for (Note* note : chord->notes()) {
        original.push_back(note->pitch());
        selected.push_back(note);
    }
    score->deselectAll();
    for (EngravingItem* item : selected) {
        score->select(item, SelectType::ADD);
    }
    ASSERT_EQ(score->selection().uniqueNotes().size(), 2u);
    for (bool up : { true, false, false, true }) {
        EditNote::upDown(score, up, UpDownMode::OCTAVE);
        for (size_t i = 0; i < chord->notes().size(); ++i) {
            original[i] += up ? 12 : -12;
            EXPECT_EQ(chord->notes()[i]->pitch(), original[i]);
        }
        EXPECT_EQ(chord->ticks(), duration);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericDegreeInputKeepsExplicitRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setNotes({ NoteInput::noteVal(score, 72, 0, false) });
    score->inputState().setNumericPulseRegisterOffset(1);
    score->inputState().setNumericPulseRegisterExplicit(true);

    enterDegree(score, 5);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_NE(chord->upNote(), nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 79);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(chord->staff(), chord->tick()), chord->upNote()->pitch()),
              muse::String(u"5"));

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericDegreeInputDefaultsToCenterRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setNotes({ NoteInput::noteVal(score, 48, 0, false) });

    enterDegree(score, 2);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_NE(chord->upNote(), nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 62);
    EXPECT_EQ(numericPulseRegister(numericPulseProfile(chord->staff(), chord->tick()), chord->upNote()->pitch()), 0);

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericDegreeInputUsesExplicitCenterRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    // A stale preview pitch must not override an explicitly selected center
    // register when the next numeric degree is entered.
    score->inputState().setNotes({ NoteInput::noteVal(score, 48, 0, false) });
    score->inputState().setNumericPulseRegisterOffset(0);
    score->inputState().setNumericPulseRegisterExplicit(true);

    enterDegree(score, 3);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_NE(chord->upNote(), nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 64);
    EXPECT_EQ(numericPulseRegister(numericPulseProfile(chord->staff(), chord->tick()), chord->upNote()->pitch()), 0);

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericMouseEntryUsesCenterRegister)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    // Materialize the pulse rests and leave the cursor on the second pulse,
    // matching a mouse click after the first note has been entered.
    enterDegree(score, 1);
    Segment* secondPulse = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(secondPulse, nullptr);

    InputState& inputState = score->inputState();
    inputState.setSegment(secondPulse);
    inputState.setNotes({ NoteInput::noteVal(score, 48, 0, false) });
    inputState.setNumericPulseRegisterExplicit(false);
    inputState.setNumericPulseRegisterOffset(0);

    Position position;
    position.segment = secondPulse;
    position.staffIdx = 0;
    position.line = 0;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Mouse numeric entry"),
                                              [&](Transaction& tx) {
                                                  NoteInput::putNote(tx, score, position, true);
                                              });

    Chord* chord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_NE(chord->upNote(), nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 60);
    EXPECT_EQ(numericPulseRegister(numericPulseProfile(chord->staff(), chord->tick()), chord->upNote()->pitch()), 0);

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericEntryDoesNotExtendPendingZeroLengthSlur)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    enterDegree(score, 1);
    Segment* secondPulse = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(secondPulse, nullptr);
    Rest* rest = toRest(secondPulse->element(0));
    ASSERT_NE(rest, nullptr);

    InputState& inputState = score->inputState();
    inputState.setSegment(secondPulse);
    Slur* pendingSlur = nullptr;
    Position position;
    position.segment = secondPulse;
    position.staffIdx = 0;
    position.line = 0;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Numeric entry with pending slur"),
                                              [&](Transaction& tx) {
                                                  pendingSlur = score->addSlur(rest, rest, nullptr);
                                                  inputState.setSlur(pendingSlur);
                                                  NoteInput::putNote(tx, score, position, true);
                                              });

    Chord* chord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 60);
    ASSERT_NE(pendingSlur, nullptr);
    EXPECT_TRUE(pendingSlur->ticks().isZero());

    inputState.setSlur(nullptr);
    delete score;
}

TEST(Engraving_GamelanNotationTests, SelectingNumericNoteResetsInputDefaults)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_HALF);
    score->inputState().setNotes({ NoteInput::noteVal(score, 48, 0, false) });
    score->inputState().setNumericPulseRegisterOffset(-1);
    score->inputState().setNumericPulseRegisterExplicit(true);

    enterDegree(score, 2);
    Chord* firstChord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(firstChord, nullptr);
    Note* firstNote = firstChord->upNote();
    ASSERT_NE(firstNote, nullptr);
    EXPECT_EQ(firstNote->pitch(), 50);
    EXPECT_EQ(firstChord->durationType(), DurationType::V_HALF);

    score->select(firstNote);
    score->inputState().update(score->selection());
    EXPECT_EQ(score->inputState().duration().type(), DurationType::V_QUARTER);
    EXPECT_TRUE(score->inputState().notes().empty());
    EXPECT_FALSE(score->inputState().numericPulseRegisterExplicit());

    score->inputState().setSegment(score->tick2segment(Fraction(1, 2), false, SegmentType::ChordRest));
    enterDegree(score, 3);
    Chord* secondChord = score->firstMeasure()->findChord(Fraction(1, 2), 0);
    ASSERT_NE(secondChord, nullptr);
    ASSERT_NE(secondChord->upNote(), nullptr);
    EXPECT_EQ(secondChord->upNote()->pitch(), 64);
    EXPECT_EQ(secondChord->durationType(), DurationType::V_QUARTER);

    delete score;
}

TEST(Engraving_GamelanNotationTests, NumericInputSplitsNewMeasureIntoQuarterPulses)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->appendMeasures(1);

    Measure* measure = score->lastMeasure();
    ASSERT_NE(measure, nullptr);
    InputState& inputState = score->inputState();
    inputState.setSegment(measure->first(SegmentType::ChordRest));
    inputState.setDuration(DurationType::V_QUARTER);
    enterDegree(score, 1);

    EXPECT_EQ(score->inputState().tick(), measure->tick() + Fraction(1, 4));
    Chord* chord = measure->findChord(measure->tick(), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->durationType(), DurationType::V_QUARTER);
    for (int pulseIndex = 1; pulseIndex < 4; ++pulseIndex) {
        ChordRest* pulse = measure->findChordRest(measure->tick() + Fraction(pulseIndex, 4), 0);
        ASSERT_NE(pulse, nullptr) << pulseIndex;
        ASSERT_TRUE(pulse->isRest()) << pulseIndex;
        EXPECT_EQ(pulse->durationType(), DurationType::V_QUARTER) << pulseIndex;
    }

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowPitchModeSkipsUnsupportedSlendroDegreesAndWraps)
{
    MasterScore* score = makeNumericPulseScore(u"saron-slendro");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 67);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

    score->startCmd(TranslatableString::untranslatable("Numeric degree up"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 69);

    score->startCmd(TranslatableString::untranslatable("Numeric degree upper boundary"));
    EditNote::upDown(score, true, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 72);

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 69);

    score->startCmd(TranslatableString::untranslatable("Numeric degree down"));
    EditNote::upDown(score, false, UpDownMode::NUMERIC_PULSE);
    score->endCmd();
    EXPECT_EQ(note->pitch(), 67);

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowUpReplacesEmptyPulseWithFirstProfileDegree)
{
    MasterScore* score = makeNumericPulseScore(u"gong-kemodong-pelog");
    ASSERT_NE(score, nullptr);

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse"), [&](Transaction& tx) {
        NoteInput::addRest(tx, score);
    });

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    const Fraction originalDuration = rest->durationTypeTicks();
    score->select(rest);

    score->startCmd(TranslatableString::untranslatable("Fill empty pulse"));
    Note* note = EditNote::replaceRestWithFirstNumericPulseDegree(score, rest);
    score->endCmd();

    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 43);
    EXPECT_EQ(note->chord()->durationTypeTicks(), originalDuration);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRest());

    delete score;
}

TEST(Engraving_GamelanNotationTests, ArrowUpReplacesFullMeasureRestWithoutCrashing)
{
    MasterScore* score = makeNumericPulseScore(u"saron-pelog");
    ASSERT_NE(score, nullptr);

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    ASSERT_TRUE(rest->durationType().isMeasure());
    const Fraction originalDuration = rest->ticks();
    ASSERT_FALSE(originalDuration.isZero());
    score->select(rest);

    score->startCmd(TranslatableString::untranslatable("Fill full-measure empty pulse"));
    Note* note = EditNote::replaceRestWithFirstNumericPulseDegree(score, rest);
    score->endCmd();

    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 60);
    EXPECT_EQ(note->chord()->durationTypeTicks(), originalDuration);

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    EXPECT_TRUE(item->isRest());

    delete score;
}

TEST(Engraving_GamelanNotationTests, ExplicitRestReplacementKeepsDurationAndUndo)
{
    MasterScore* score = makeNumericPulseScore(u"gong-kemodong-pelog");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 5);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    ASSERT_NE(note, nullptr);
    const Fraction originalDuration = chord->durationTypeTicks();
    score->select(note);

    score->startCmd(TranslatableString::untranslatable("Empty first numeric degree"));
    ASSERT_NE(EditNote::replaceFirstNumericPulseDegreeWithRest(score, note), nullptr);
    score->endCmd();

    Segment* segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    Rest* rest = toRest(item);
    EXPECT_EQ(rest->durationTypeTicks(), originalDuration);

    score->undoRedo(true, nullptr);
    segment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isChord());
    note = toChord(item)->upNote();
    ASSERT_NE(note, nullptr);
    EXPECT_EQ(note->pitch(), 43);
    EXPECT_EQ(numericPulseDegreeText(numericPulseProfile(note->staff(), note->tick()), note->pitch()),
              muse::String::number(5));

    delete score;
}

TEST(Engraving_GamelanNotationTests, PulseInputIsMonophonicAndDoesNotStack)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    InputState& inputState = score->inputState();
    enterDegree(score, 1);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 60);

    inputState.setSegment(score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest));
    enterDegree(score, 5);

    chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    ASSERT_EQ(chord->notes().size(), 1u);
    EXPECT_EQ(chord->upNote()->pitch(), 67);

    delete score;
}

TEST(Engraving_GamelanNotationTests, EmptyPulseCreatesExplicitRestAndAdvancesCursor)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    enterDegree(score, 4);
    ASSERT_EQ(score->inputState().tick(), Fraction(1, 4));

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse"), [&](Transaction& tx) {
        NoteInput::addRest(tx, score);
    });

    Segment* segment = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    const Rest* rest = toRest(item);
    EXPECT_FALSE(rest->generated());
    EXPECT_FALSE(rest->isGap());
    EXPECT_EQ(rest->durationType().type(), DurationType::V_QUARTER);
    EXPECT_EQ(score->inputState().tick(), Fraction(1, 2));

    score->setLayoutAll();
    score->doLayout();
    const muse::RectF expectedBbox = rendering::score::numericPulseEmptyPulseBbox(rest->spatium());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().x(), expectedBbox.x());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().y(), expectedBbox.y());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().width(), expectedBbox.width());
    EXPECT_DOUBLE_EQ(rest->ldata()->bbox().height(), expectedBbox.height());
    EXPECT_LT(rest->ldata()->bbox().bottom(), 0.0);

    delete score;
}

TEST(Engraving_GamelanNotationTests, EmptyPulseCanSplitLongerExistingRest)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);

    // Reproduce the toolbar cursor shown in the bug report: it is positioned
    // inside the initial full-measure rest, so no ChordRest exists at this tick.
    InputState& inputState = score->inputState();
    Segment* cursorSegment = score->firstMeasure()->getSegment(SegmentType::ChordRest, Fraction(1, 4));
    ASSERT_NE(cursorSegment, nullptr);
    inputState.setSegment(cursorSegment);
    ASSERT_EQ(inputState.cr(), nullptr);

    score->transactionManager()->transaction(TranslatableString::untranslatable("Enter empty pulse inside rest"),
                                              [&](Transaction& tx) { NoteInput::addRest(tx, score); });

    Segment* segment = score->tick2segment(Fraction(1, 4), false, SegmentType::ChordRest);
    ASSERT_NE(segment, nullptr);
    EngravingItem* item = segment->element(0);
    ASSERT_NE(item, nullptr);
    ASSERT_TRUE(item->isRest());
    EXPECT_FALSE(toRest(item)->generated());
    EXPECT_FALSE(toRest(item)->isGap());
    EXPECT_EQ(toRest(item)->durationType().type(), DurationType::V_QUARTER);
    EXPECT_EQ(score->inputState().tick(), Fraction(1, 2));

    delete score;
}

TEST(Engraving_GamelanNotationTests, RejectsUnsupportedInputDegrees)
{
    MasterScore* score = makeNumericPulseScore(u"saron-slendro");
    ASSERT_NE(score, nullptr);

    const std::array<int, 5> supportedSteps { 0, 1, 2, 4, 5 };
    for (int step : supportedSteps) {
        NoteInputParams params;
        EXPECT_TRUE(NoteInput::resolveNoteInputParams(score, step, false, params));
        EXPECT_NE(params.numericDegree, 0);
    }

    NoteInputParams params;
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 3, false, params));
    EXPECT_FALSE(NoteInput::resolveNoteInputParams(score, 6, false, params));
    delete score;
}

TEST(Engraving_GamelanNotationTests, EighthPulseGroupStaysFlatAndMonophonic)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_EIGHTH);

    for (int degree : { 1, 2, 3, 5 }) {
        enterDegree(score, degree);
    }

    score->setLayoutAll();
    score->doLayout();

    const std::array<Fraction, 4> ticks {
        Fraction(0, 1), Fraction(1, 8), Fraction(1, 4), Fraction(3, 8)
    };
    Beam* beam = nullptr;
    for (const Fraction& tick : ticks) {
        Chord* chord = score->firstMeasure()->findChord(tick, 0);
        ASSERT_NE(chord, nullptr);
        ASSERT_EQ(chord->notes().size(), 1u);
        EXPECT_EQ(chord->durationType().type(), DurationType::V_EIGHTH);
        if (!beam) {
            beam = chord->beam();
        } else {
            EXPECT_EQ(chord->beam(), beam);
        }
    }

    ASSERT_NE(beam, nullptr);
    EXPECT_TRUE(beam->up());
    EXPECT_DOUBLE_EQ(beam->slope(), 0.0);
    EXPECT_DOUBLE_EQ(beam->beamWidth(),
                     rendering::score::numericPulseRhythmMarkThickness(beam->spatium()) * beam->mag());
    EXPECT_DOUBLE_EQ(beam->beamDist(),
                     rendering::score::numericPulseRhythmMarkDistance(beam->spatium()) * beam->mag());

    const double baseline = score->firstMeasure()->findChord(ticks.front(), 0)->upNote()->ldata()->pos().y();
    for (const Fraction& tick : ticks) {
        EXPECT_DOUBLE_EQ(score->firstMeasure()->findChord(tick, 0)->upNote()->ldata()->pos().y(), baseline);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, SixteenthPulseGroupUsesTwoFlatRhythmMarks)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_16TH);

    for (int degree : { 1, 2, 3, 5 }) {
        enterDegree(score, degree);
    }
    score->setLayoutAll();
    score->doLayout();

    Beam* beam = nullptr;
    for (const Fraction& tick : { Fraction(0, 1), Fraction(1, 16), Fraction(1, 8), Fraction(3, 16) }) {
        Chord* chord = score->firstMeasure()->findChord(tick, 0);
        ASSERT_NE(chord, nullptr);
        EXPECT_EQ(chord->durationType().type(), DurationType::V_16TH);
        EXPECT_EQ(chord->durationType().hooks(), 2);
        beam = beam ? beam : chord->beam();
        EXPECT_EQ(chord->beam(), beam);
    }
    ASSERT_NE(beam, nullptr);
    EXPECT_TRUE(beam->up());
    EXPECT_DOUBLE_EQ(beam->slope(), 0.0);
    delete score;
}

TEST(Engraving_GamelanNotationTests, ConvertedWesternBeamKeepsFlatBarsAndDeleteUndo)
{
    for (DurationType duration : { DurationType::V_EIGHTH, DurationType::V_16TH }) {
        std::unique_ptr<MasterScore> score(makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity"));
        ASSERT_NE(score, nullptr);
        score->inputState().setDuration(duration);
        for (int degree : { 1, 3, 5, 6 }) {
            enterDegree(score.get(), degree, false);
        }
        // First lay out as Western notation, then assign the numeric staff and
        // tuning exactly as conversion does. Preserve existing note/beam data.
        score->staff(0)->part()->instrument()->setId(u"vibraphone");
        score->staff(0)->setStaffType(Fraction(0, 1), *StaffType::preset(StaffTypes::STANDARD));
        score->setLayoutAll();
        score->doLayout();
        score->staff(0)->part()->instrument()->setId(u"gender-barung-tuning-8-1-med-diversity");
        score->staff(0)->part()->instrument()->setNumericPulseProfileId(u"kepatihan-tuning-8-1-med-diversity");
        score->staff(0)->setStaffType(Fraction(0, 1), *StaffType::preset(StaffTypes::NUMERIC_PULSE));
        score->setLayoutAll();
        score->doLayout();
        const Fraction step = TDuration(duration).fraction();
        Chord* first = score->firstMeasure()->findChord(Fraction(0, 1), 0);
        Chord* second = score->firstMeasure()->findChord(step, 0);
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
        ASSERT_NE(first->beam(), nullptr);
        EXPECT_DOUBLE_EQ(first->beam()->slope(), 0.0);
        EXPECT_DOUBLE_EQ(first->beam()->beamDist(), rendering::score::numericPulseRhythmMarkDistance(first->spatium())
                         * first->beam()->mag());
        const int firstPitch = first->upNote()->pitch();
        const int secondPitch = second->upNote()->pitch();
        score->select(second->upNote());
        score->transactionManager()->transaction(TranslatableString::untranslatable("Delete converted number"), [&](Transaction&) {
            score->cmdDeleteSelection();
        });
        ASSERT_NE(score->firstMeasure()->findChordRest(step, 0), nullptr);
        EXPECT_TRUE(score->firstMeasure()->findChordRest(step, 0)->isRest());
        EXPECT_EQ(score->firstMeasure()->findChordRest(step, 0)->ticks(), step);
        EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0)->upNote()->pitch(), firstPitch);
        score->undoRedo(true, nullptr);
        ASSERT_NE(score->firstMeasure()->findChord(step, 0), nullptr);
        EXPECT_EQ(score->firstMeasure()->findChord(step, 0)->upNote()->pitch(), secondPitch);
        score->undoRedo(false, nullptr);
        EXPECT_TRUE(score->firstMeasure()->findChordRest(step, 0)->isRest());
    }
}

TEST(Engraving_GamelanNotationTests, KepatihanSubdivisionBarsCenterOnDigits)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    score->inputState().setDuration(DurationType::V_16TH);
    for (int degree : { 1, 2, 3, 5 }) {
        enterDegree(score, degree);
    }
    score->setLayoutAll();
    score->doLayout();
    const Chord* first = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    const Chord* last = score->firstMeasure()->findChord(Fraction(3, 16), 0);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(last, nullptr);
    const Beam* beam = first->beam();
    ASSERT_NE(beam, nullptr);
    ASSERT_FALSE(beam->beamSegments().empty());
    for (const BeamSegment* segment : beam->beamSegments()) {
        if (segment->isBeamlet) {
            continue;
        }
        const double firstOverhang = rendering::score::numericPulseGroupOverhang(
            rendering::score::numericPulseTextBbox(u"1", first->upNote()->magS()), first->spatium());
        const double lastOverhang = rendering::score::numericPulseGroupOverhang(
            rendering::score::numericPulseTextBbox(u"5", last->upNote()->magS()), last->spatium());
        EXPECT_NEAR(segment->line.x1() + beam->pagePos().x(), first->upNote()->pagePos().x() - firstOverhang, 0.01);
        EXPECT_NEAR(segment->line.x2() + beam->pagePos().x(), last->upNote()->pagePos().x() + lastOverhang, 0.01);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, KepatihanTripletNumberCentersOnDigits)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1);
    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Tuplet* tuplet = Factory::createTuplet(score->dummy()->measure());
    tuplet->setRatio(Fraction(3, 2));
    tuplet->setTicks(Fraction(1, 4));
    tuplet->setBaseLen(TDuration(DurationType::V_EIGHTH));
    tuplet->setTrack(0);
    tuplet->setTick(Fraction(0, 1));
    tuplet->setParent(chord->measure());
    score->startCmd(muse::TranslatableString::untranslatable("Triplet alignment test"));
    score->cmdCreateTuplet(chord, tuplet);
    score->endCmd();
    score->setLayoutAll();
    score->doLayout();
    ASSERT_NE(tuplet->number(), nullptr);
    EXPECT_NEAR(tuplet->number()->ldata()->pos().x() + tuplet->ldata()->pos().x(),
                (tuplet->p1().x() + tuplet->p2().x()) * 0.5, 0.01);
    delete score;
}

TEST(Engraving_GamelanNotationTests, DeleteSelectedKepatihanNumbersKeepsRestRowsAndUndo)
{
    for (bool entering : { false, true }) {
        for (bool wholeChord : { false, true }) {
            std::unique_ptr<MasterScore> score(makeNumericPulseScore(u"gender-barung-tuning-8-1-med-diversity"));
            ASSERT_NE(score, nullptr);
            enterDegree(score.get(), 1, false);
            enterDegree(score.get(), 3);
            enterDegree(score.get(), 5, false);
            Chord* first = score->firstMeasure()->findChord(Fraction(0, 1), 0);
            Chord* next = score->firstMeasure()->findChord(Fraction(1, 4), 0);
            ASSERT_NE(first, nullptr);
            ASSERT_NE(next, nullptr);
            ASSERT_EQ(first->notes().size(), 2u);
            const int neighborPitch = next->upNote()->pitch();
            const int remainingPitch = first->downNote()->pitch();
            const int remainingRow = first->downNote()->numericPulseRow();
            const auto notes = first->notes();
            score->deselectAll();
            if (wholeChord) {
                for (Note* note : notes) {
                    score->select(note, SelectType::ADD);
                }
            } else {
                score->select(first->upNote());
            }
            score->inputState().setNoteEntryMode(entering);
            score->transactionManager()->transaction(TranslatableString::untranslatable("Delete Kepatihan numbers"), [&](Transaction&) {
                score->cmdDeleteSelection();
            });
            ChordRest* result = score->firstMeasure()->findChordRest(Fraction(0, 1), 0);
            ASSERT_NE(result, nullptr);
            EXPECT_EQ(result->ticks(), Fraction(1, 4));
            EXPECT_EQ(result->isRest(), wholeChord);
            if (!wholeChord) {
                ASSERT_TRUE(result->isChord());
                EXPECT_EQ(toChord(result)->notes().size(), 1u);
                EXPECT_EQ(toChord(result)->upNote()->pitch(), remainingPitch);
                EXPECT_EQ(toChord(result)->upNote()->numericPulseRow(), remainingRow);
            } else {
                EXPECT_TRUE(score->selection().uniqueNotes().empty());
                EXPECT_EQ(score->selection().element(), result);
            }
            EXPECT_EQ(score->firstMeasure()->findChord(Fraction(1, 4), 0)->upNote()->pitch(), neighborPitch);
            score->undoRedo(true, nullptr);
            ASSERT_NE(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
            EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0)->notes().size(), 2u);
            score->undoRedo(false, nullptr);
            EXPECT_EQ(score->firstMeasure()->findChordRest(Fraction(0, 1), 0)->isRest(), wholeChord);
        }
    }
}

TEST(Engraving_GamelanNotationTests, OptionalRealScoreDeleteBenchmark)
{
    const char* path = std::getenv("MUSESCORE_RHYTHM_BENCHMARK_SCORE");
    if (!path) {
        GTEST_SKIP() << "Set MUSESCORE_RHYTHM_BENCHMARK_SCORE for an in-memory delete benchmark";
    }
    std::unique_ptr<MasterScore> score(ScoreRW::readScore(String::fromUtf8(path), true));
    ASSERT_NE(score, nullptr);
    Chord* target = nullptr;
    Chord* fallback = nullptr;
    for (Segment* segment = score->firstSegment(SegmentType::ChordRest); segment && !target;
         segment = segment->next1(SegmentType::ChordRest)) {
        for (track_idx_t track = 0; track < score->ntracks(); ++track) {
            EngravingItem* item = segment->element(track);
            if (item && item->isChord() && !toChord(item)->tuplet() && numericPulseProfile(item->staff(), item->tick())) {
                if (!fallback) {
                    fallback = toChord(item);
                }
                if (toChord(item)->beam()) {
                    target = toChord(item);
                    break;
                }
            }
        }
    }
    target = target ? target : fallback;
    ASSERT_NE(target, nullptr);
    const Fraction tick = target->tick();
    const track_idx_t track = target->track();
    const size_t originalCount = target->notes().size();
    for (bool wholeChord : { false, true }) {
        Chord* chord = score->tick2measure(tick)->findChord(tick, track);
        ASSERT_NE(chord, nullptr);
        score->deselectAll();
        if (wholeChord) {
            for (Note* note : chord->notes()) {
                score->select(note, SelectType::ADD);
            }
        } else {
            score->select(chord->upNote());
        }
        const auto start = std::chrono::steady_clock::now();
        score->transactionManager()->transaction(TranslatableString::untranslatable("Benchmark delete"), [&](Transaction&) {
            score->cmdDeleteSelection();
        });
        std::cout << "Real-score delete (whole chord=" << wholeChord << "): "
                  << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() << " ms\n";
        score->undoRedo(true, nullptr);
        ASSERT_NE(score->tick2measure(tick)->findChord(tick, track), nullptr);
        EXPECT_EQ(score->tick2measure(tick)->findChord(tick, track)->notes().size(), originalCount);
    }
    // No saving: benchmark changes exist only in memory.
}

TEST(Engraving_GamelanNotationTests, DeletePulseUndoRedoDoesNotShiftNeighbors)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 1);
    enterDegree(score, 2);

    Chord* firstChord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    Chord* secondChord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(firstChord, nullptr);
    ASSERT_NE(secondChord, nullptr);
    const int secondPitch = secondChord->upNote()->pitch();

    InputState& inputState = score->inputState();
    inputState.setSegment(firstChord->segment());
    inputState.setDuration(DurationType::V_QUARTER);
    inputState.setRest(true);
    Position position;
    position.segment = firstChord->segment();
    position.staffIdx = 0;
    position.line = 0;
    score->transactionManager()->transaction(TranslatableString::untranslatable("Clear Slendro pulse"), [&](Transaction& tx) {
        NoteInput::putNote(tx, score, position, true);
    });

    EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    Segment* emptyPulseSegment = score->tick2segment(Fraction(0, 1), false, SegmentType::ChordRest);
    ASSERT_NE(emptyPulseSegment, nullptr);
    EngravingItem* emptyPulse = emptyPulseSegment->element(0);
    ASSERT_NE(emptyPulse, nullptr);
    ASSERT_TRUE(emptyPulse->isRest());
    EXPECT_EQ(toRest(emptyPulse)->durationType().type(), DurationType::V_QUARTER);
    secondChord = score->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(secondChord, nullptr);
    EXPECT_EQ(secondChord->upNote()->pitch(), secondPitch);

    score->undoRedo(true, nullptr);
    EXPECT_NE(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    score->undoRedo(false, nullptr);
    EXPECT_EQ(score->firstMeasure()->findChord(Fraction(0, 1), 0), nullptr);
    delete score;
}

TEST(Engraving_GamelanNotationTests, NativeTieProvidesContinuation)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 3);

    Chord* chord = score->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    Note* note = chord->upNote();
    score->select(note);
    EditTie::cmdAddTie(score);

    ASSERT_NE(note->tieFor(), nullptr);
    ASSERT_NE(note->tieFor()->endNote(), nullptr);
    EXPECT_EQ(note->tieFor()->endNote()->pitch(), note->pitch());
    EXPECT_EQ(note->tieFor()->endNote()->tick(), Fraction(1, 4));
    score->setLayoutAll();
    score->doLayout();
    ASSERT_FALSE(note->tieFor()->segmentsEmpty());
    const auto* segment = note->tieFor()->frontSegment();
    // Check actual drawing on screen and in print, not just tie creation.
    for (bool printing : { false, true }) {
        QImage image(1600, 600, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        {
            muse::draw::Painter painter(&image, "Kepatihan tie regression");
            painter.translate(100 - segment->ldata()->bbox().left(), 100 - segment->ldata()->bbox().top());
            rendering::PaintOptions options;
            options.isPrinting = printing;
            rendering::score::TDraw::drawItem(segment, &painter, options);
        }
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                painted += qAlpha(image.pixel(x, y)) != 0;
            }
        }
        EXPECT_GT(painted, 0);
    }
    delete score;
}

TEST(Engraving_GamelanNotationTests, SaveReloadPreservesStaffAndPulseData)
{
    MasterScore* score = makeNumericPulseScore();
    ASSERT_NE(score, nullptr);
    enterDegree(score, 6);
    score->inputState().setDuration(DurationType::V_EIGHTH);
    enterDegree(score, 5);

    const String savePath = u"numeric-pulse-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    EXPECT_TRUE(reopened->staff(0)->staffType(Fraction(0, 1))->isNumericPulseStaff());
    ASSERT_NE(numericPulseProfile(reopened->staff(0), Fraction(0, 1)), nullptr);
    EXPECT_EQ(numericPulseProfile(reopened->staff(0), Fraction(0, 1))->id, muse::String(u"saron-pelog"));
    Chord* firstChord = reopened->firstMeasure()->findChord(Fraction(0, 1), 0);
    Chord* secondChord = reopened->firstMeasure()->findChord(Fraction(1, 4), 0);
    ASSERT_NE(firstChord, nullptr);
    ASSERT_NE(secondChord, nullptr);
    EXPECT_EQ(firstChord->upNote()->pitch(), 69);
    EXPECT_EQ(secondChord->upNote()->pitch(), 67);
    EXPECT_EQ(secondChord->durationType().type(), DurationType::V_EIGHTH);

    Measure* firstEmptyMeasure = reopened->firstMeasure()->nextMeasure();
    ASSERT_NE(firstEmptyMeasure, nullptr);
    for (int pulseIndex = 0; pulseIndex < 4; ++pulseIndex) {
        const Fraction pulseTick = firstEmptyMeasure->tick() + Fraction(pulseIndex, 4);
        ChordRest* pulse = firstEmptyMeasure->findChordRest(pulseTick, 0);
        ASSERT_NE(pulse, nullptr) << pulseIndex;
        ASSERT_TRUE(pulse->isRest()) << pulseIndex;
        EXPECT_EQ(toRest(pulse)->durationType().type(), DurationType::V_QUARTER);
    }

    delete reopened;
    delete score;
}

TEST(Engraving_GamelanNotationTests, AdditionalInstrumentProfileSurvivesSaveReload)
{
    MasterScore* score = makeNumericPulseScore(u"kempul-pelog");
    ASSERT_NE(score, nullptr);
    enterDegree(score, 7);

    const String savePath = u"kempul-pelog-roundtrip.mscx";
    ASSERT_TRUE(ScoreRW::saveScore(score, savePath));
    MasterScore* reopened = ScoreRW::readScore(savePath, true);
    ASSERT_NE(reopened, nullptr);

    const NumericPulseProfile* profile = numericPulseProfile(reopened->staff(0), Fraction(0, 1));
    ASSERT_NE(profile, nullptr);
    EXPECT_EQ(profile->id, muse::String(u"kempul-pelog"));
    Chord* chord = reopened->firstMeasure()->findChord(Fraction(0, 1), 0);
    ASSERT_NE(chord, nullptr);
    EXPECT_EQ(chord->upNote()->pitch(), 59);
    EXPECT_EQ(numericPulseDegreeText(profile, chord->upNote()->pitch()), muse::String(u"7"));

    delete reopened;
    delete score;
}

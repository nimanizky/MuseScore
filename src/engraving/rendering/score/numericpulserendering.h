/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "draw/fontmetrics.h"
#include "draw/types/font.h"
#include "draw/types/geometry.h"
#include "engraving/types/fraction.h"
#include "global/types/string.h"

namespace mu::engraving::rendering::score {

inline double numericPulseRhythmMarkThickness(double spatium)
{
    return 0.18 * spatium;
}

inline double numericPulseRhythmMarkDistance(double spatium)
{
    return 0.40 * spatium;
}

inline double numericPulseRhythmMarkGap(double spatium)
{
    return 0.45 * spatium;
}

inline double numericPulseStemLength(double spatium, int durationMarks)
{
    const int additionalMarks = std::max(0, durationMarks - 1);
    return (2.35 + 0.16 * additionalMarks) * spatium;
}

inline double numericPulseGroupOverhang(const muse::RectF& textBbox, double spatium)
{
    // Bars are anchored at digit centers; extend equally beyond both edges.
    return textBbox.width() * 0.5 + 0.40 * spatium;
}

inline muse::draw::Font numericPulseFont(double scale, const muse::String& family = {})
{
    muse::draw::Font font(u"Edwin", muse::draw::Font::Type::Text);
    if (!family.empty()) {
        font.setFamily(family, muse::draw::Font::Type::Text);
    }
    font.setPointSizeF(12.0 * scale);
    return font;
}

struct NumericPulseTextMetrics {
    muse::String text;
    muse::String fontFamily;
    double scale = 1.0;
    muse::RectF bbox;
    double originX = 0.0;
};

inline std::vector<NumericPulseTextMetrics>& numericPulseTextMetricsCache()
{
    thread_local std::vector<NumericPulseTextMetrics> cache;
    return cache;
}

inline void clearNumericPulseTextMetricsCache()
{
    numericPulseTextMetricsCache().clear();
}

inline NumericPulseTextMetrics numericPulseTextMetrics(const muse::String& text, double scale,
                                                       const muse::String& fontFamily = {})
{
    auto& cache = numericPulseTextMetricsCache();
    for (const auto& entry : cache) {
        if (entry.scale == scale && entry.text == text && entry.fontFamily == fontFamily) {
            return entry;
        }
    }
    const muse::draw::Font font = numericPulseFont(scale, fontFamily);
    // Anchor octave dots to the visible digit, without the font's leading or
    // descent padding, so dots remain attached to their number in a chord.
    muse::RectF bbox = muse::draw::FontMetrics(font).tightBoundingRect(text);
    bbox.translate(-bbox.center().x(), 0.0);
    NumericPulseTextMetrics result { text, fontFamily, scale, bbox,
        -muse::draw::FontMetrics(font).horizontalAdvance(text) * 0.5 };
    if (cache.size() >= 64) {
        cache.clear();
    }
    cache.push_back(result);
    return result;
}

inline muse::RectF numericPulseTextBbox(const muse::String& text, double scale,
                                        const muse::String& fontFamily = {})
{
    return numericPulseTextMetrics(text, scale, fontFamily).bbox;
}

inline double numericPulseTextOriginX(const muse::String& text, double scale,
                                      const muse::String& fontFamily = {})
{
    return numericPulseTextMetrics(text, scale, fontFamily).originX;
}

inline muse::RectF numericPulseEmptyPulseBbox(double spatium)
{
    // Do not use a text period here. Its glyph sits on the numeric-pulse staff
    // baseline and can disappear into the staff line. Keep the pulse dot fully
    // above that line and large enough to remain visible at normal zoom levels.
    const double diameter = 0.50 * spatium;
    const double centerY = -0.38 * spatium;
    return muse::RectF(-diameter * 0.5, centerY - diameter * 0.5, diameter, diameter);
}

inline int numericPulseMeasurePulseCount(const Fraction& measureDuration, const Fraction& timeSignature)
{
    if (!measureDuration.isValid() || !timeSignature.isValid() || timeSignature.denominator() <= 0) {
        return 1;
    }

    // The meter denominator defines the visible pulse unit: quarter-note pulses
    // in 4/4 and eighth-note pulses in 6/8. Irregular/pickup measures naturally
    // show only the pulses contained in their actual duration.
    const Fraction pulseCount = (measureDuration / Fraction(1, timeSignature.denominator())).reduced();
    return pulseCount.denominator() == 1 ? std::max(1, pulseCount.numerator()) : 1;
}

inline muse::RectF numericPulseEmptyPulseBbox(double spatium, int pulseIndex, int pulseCount, double availableWidth)
{
    muse::RectF bbox = numericPulseEmptyPulseBbox(spatium);
    if (pulseCount <= 1 || availableWidth <= 0.0) {
        return bbox;
    }

    const double paddedWidth = std::max(0.0, availableWidth - 2.0 * spatium);
    const double pulseSpacing = paddedWidth / pulseCount;
    const double centerX = (pulseIndex + 0.5) * pulseSpacing - paddedWidth * 0.5;
    bbox.translate(centerX, 0.0);
    return bbox;
}

inline muse::RectF numericPulseEmptyPulseGroupBbox(double spatium, int pulseCount, double availableWidth)
{
    muse::RectF bbox = numericPulseEmptyPulseBbox(spatium, 0, pulseCount, availableWidth);
    for (int pulseIndex = 1; pulseIndex < pulseCount; ++pulseIndex) {
        bbox = bbox.united(numericPulseEmptyPulseBbox(spatium, pulseIndex, pulseCount, availableWidth));
    }
    return bbox;
}

inline double numericPulseRegisterDotDiameter(double spatium)
{
    return 0.60 * spatium;
}

inline double numericPulseRegisterDotGap(double spatium)
{
    return 0.10 * spatium;
}

inline muse::RectF numericPulseRegisterDotBbox(const muse::RectF& textBbox, double spatium,
                                               int registerOffset, int index)
{
    const double diameter = numericPulseRegisterDotDiameter(spatium);
    const double distance = numericPulseRegisterDotGap(spatium) + diameter * 0.5
                            + index * (diameter + numericPulseRegisterDotGap(spatium));
    const double centerY = registerOffset > 0 ? textBbox.top() - distance : textBbox.bottom() + distance;
    return muse::RectF(-diameter * 0.5, centerY - diameter * 0.5, diameter, diameter);
}

inline muse::RectF numericPulseGlyphBbox(const muse::String& text, double scale, double spatium, int registerOffset,
                                         const muse::String& fontFamily = {})
{
    const muse::RectF textBbox = numericPulseTextBbox(text, scale, fontFamily);
    muse::RectF bbox = textBbox;
    for (int i = 0; i < std::abs(registerOffset); ++i) {
        bbox = bbox.united(numericPulseRegisterDotBbox(textBbox, spatium, registerOffset, i));
    }
    return bbox;
}

inline double numericPulseDurationMarkWidth(const muse::RectF& textBbox, double spatium)
{
    // Extend slightly past both sides of the numeric glyph so a standalone
    // duration mark reads clearly without looking like a full grouped beam.
    return std::max(textBbox.width() + 0.60 * spatium, 1.30 * spatium);
}

inline double numericPulseDurationMarkY(const muse::RectF& textBbox, double spatium, int index)
{
    return textBbox.top() - numericPulseRhythmMarkGap(spatium)
           - numericPulseRhythmMarkDistance(spatium) * index;
}

inline muse::RectF numericPulseAugmentationDotBbox(const muse::RectF& textBbox, double spatium, int index)
{
    const double diameter = 0.20 * spatium;
    const double gap = 0.16 * spatium;
    const double centerX = textBbox.right() + gap + diameter * (index + 0.5) + gap * index;
    const double centerY = textBbox.center().y();
    return muse::RectF(centerX - diameter * 0.5, centerY - diameter * 0.5, diameter, diameter);
}

inline int numericPulseHoldDotCount(const Fraction& duration)
{
    const double quarterNotes = duration.toDouble() * 4.0;
    return quarterNotes > 1.0 ? std::max(1, static_cast<int>(std::ceil(quarterNotes)) - 1) : 0;
}

struct NumericPulsePanjerMark {
    NumericPulseTextMetrics label;
    double baseline = 0.0;
    double lineStartX = 0.0;
    double lineY = 0.0;
    double endX = 0.0;
    double endDiameter = 0.0;
    double lineThickness = 0.0;
    muse::RectF bbox;
};

inline NumericPulsePanjerMark numericPulsePanjerMark(const muse::RectF& noteBbox, double spatium,
                                                     double scale, double endX)
{
    static const muse::String PANJER(u"panjer");
    const double labelScale = 0.68 * scale;
    NumericPulseTextMetrics label = numericPulseTextMetrics(PANJER, labelScale);
    const double labelShift = 0.80 * spatium;
    label.originX += labelShift;
    label.bbox.translate(labelShift, 0.0);
    const double top = noteBbox.bottom() + 0.42 * spatium;
    const double baseline = top - label.bbox.top();
    const double lineThickness = 0.08 * spatium;
    const double endDiameter = 0.20 * spatium;
    const double lineY = baseline + label.bbox.center().y();
    const muse::draw::FontMetrics labelMetrics(numericPulseFont(labelScale));
    const double lineStartX = label.originX + labelMetrics.tightBoundingRect(PANJER).right() + 0.24 * spatium;
    const muse::RectF labelBbox = label.bbox.translated(muse::PointF(0.0, baseline));
    endX = std::max(endX, lineStartX + spatium);
    const double lineLeft = std::min(lineStartX, endX);
    const double lineRight = std::max(lineStartX, endX);
    muse::RectF bbox = labelBbox.united(muse::RectF(lineLeft, lineY - lineThickness * 0.5,
                                                    lineRight - lineLeft, lineThickness));
    bbox = bbox.united(muse::RectF(endX - endDiameter * 0.5, lineY - endDiameter * 0.5,
                                   endDiameter, endDiameter));
    return { label, baseline, lineStartX, lineY, endX, endDiameter, lineThickness, bbox };
}

inline muse::RectF numericPulseNoteBbox(const muse::String& text, double scale, double spatium,
                                        int registerOffset, int durationMarks, const muse::String& fontFamily = {},
                                        int holdDots = 0, bool hasPanjer = false, double panjerEndX = 0.0)
{
    muse::RectF bbox = numericPulseGlyphBbox(text, scale, spatium, registerOffset, fontFamily);
    const muse::RectF textBbox = numericPulseTextBbox(text, scale, fontFamily);
    for (int i = 0; i < holdDots; ++i) {
        bbox = bbox.united(numericPulseAugmentationDotBbox(textBbox, spatium, i));
    }
    if (durationMarks > 0) {
        const double width = numericPulseDurationMarkWidth(textBbox, spatium);
        const double thickness = numericPulseRhythmMarkThickness(spatium);
        const double top = numericPulseDurationMarkY(bbox, spatium, durationMarks - 1) - thickness * 0.5;
        bbox = bbox.united(muse::RectF(-width * 0.5, top, width, thickness));
    }
    if (hasPanjer) {
        const auto mark = numericPulsePanjerMark(bbox, spatium, scale, panjerEndX);
        // Horizontal extent follows final measure spacing; reserve vertical room
        // without feeding the entire sustain span back into note spacing.
        bbox = bbox.united(muse::RectF(bbox.left(), mark.bbox.top(), bbox.width(), mark.bbox.height()));
    }
    return bbox;
}

}

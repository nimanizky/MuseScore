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
#include "noteinputbarmodel.h"

#include <string>
#include <QTimer>

#include "types/translatablestring.h"

#include "context/shortcutcontext.h"

#include "notation/inotationelements.h" // IWYU pragma: keep
#include "notation/inotationinteraction.h"
#include "notation/inotationnoteinput.h"
#include "notation/inotationselection.h"
#include "notation/inotationundostack.h"

#include "internal/notationuiactions.h"
#include "notationcommands.h"
#include "engraving/dom/engravingitem.h"
#include "engraving/dom/chord.h"
#include "engraving/dom/note.h"
#include "engraving/dom/numericpulseprofile.h"
#include "engraving/dom/score.h"
#include "engraving/dom/staff.h"

using namespace mu;
using namespace mu::notation;
using namespace muse;
using namespace muse::actions;
using namespace muse::ui;
using namespace muse::uicomponents;

static const QString TOOLBAR_NAME("noteInput");

static const ActionCode ADD_ACTION_CODE("add");
static const ActionCode CROSS_STAFF_BEAMING_CODE("cross-staff-beaming");
static const ActionCode TUPLET_ACTION_CODE("tuplet");

static bool isNumericDegreeAction(const QString& actionId, const engraving::NumericPulseProfile* profile)
{
    if (!profile) {
        return false;
    }
    for (const engraving::NumericPulseDegree& degree : profile->degrees) {
        if (degree.action.toQString() == actionId) {
            return true;
        }
    }
    return false;
}

static bool isDurationAction(const QString& actionId)
{
    return actionId == QStringLiteral("pad-note-1")
           || actionId == QStringLiteral("pad-note-2")
           || actionId == QStringLiteral("pad-note-4")
           || actionId == QStringLiteral("pad-note-8")
           || actionId == QStringLiteral("pad-note-16")
           || actionId == QStringLiteral("pad-note-32")
           || actionId == QStringLiteral("pad-note-64")
           || actionId == QStringLiteral("pad-note-128")
           || actionId == QStringLiteral("pad-note-256")
           || actionId == QStringLiteral("pad-note-512")
           || actionId == QStringLiteral("pad-note-1024");
}

static bool isNumericRegisterAction(const QString& actionId)
{
    return actionId.startsWith(QStringLiteral("numeric-register-"));
}

static int numericRegisterForAction(const QString& actionId)
{
    bool ok = false;
    const int value = actionId.mid(QStringLiteral("numeric-register-").size()).toInt(&ok);
    return ok ? value : 0;
}

static bool isAlternativeNoteInputModeAction(const ActionCode& actionCode)
{
    return actionCode.rfind("note-input-", 0) == 0;
}

NoteInputBarModel::NoteInputBarModel(QObject* parent)
    : AbstractMenuModel(parent)
{
}

QVariant NoteInputBarModel::data(const QModelIndex& index, int role) const
{
    int row = index.row();
    if (!isIndexValid(row)) {
        return QVariant();
    }

    const MenuItem* item = items().at(row);
    switch (role) {
    case OrderRole: return row;
    case SectionRole: return item->section();
    }

    return AbstractMenuModel::data(index, role);
}

QHash<int, QByteArray> NoteInputBarModel::roleNames() const
{
    QHash<int, QByteArray> roles = AbstractMenuModel::roleNames();
    roles[OrderRole] = "order";
    roles[SectionRole] = "section";

    return roles;
}

void NoteInputBarModel::classBegin()
{
    init();
}

void NoteInputBarModel::init()
{
    subscribeOnChanges();

    const auto listenToNoteInputState = [this]() {
        m_noteInputStateReceiver = std::make_unique<muse::async::Asyncable>();
        INotationPtr notation = context()->currentNotation();
        if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
            return;
        }
        notation->interaction()->noteInput()->stateChanged().onNotify(m_noteInputStateReceiver.get(), [this]() {
            if (m_noteInputRefreshScheduled) {
                return;
            }
            m_noteInputRefreshScheduled = true;
            QTimer::singleShot(0, this, [this]() {
                m_noteInputRefreshScheduled = false;
                const auto* profile = activeNumericPulseProfile();
                const QString profileId = profile ? profile->id.toQString() : QString();
                if (!profile || profileId != m_loadedNumericProfileId) {
                    load();
                    emit usesNumericPulseChanged();
                } else {
                    // Update once using the final state, never the temporary
                    // empty selection while a deletion/undo is in progress.
                    // Local controls are not registered global actions.
                    onActionsStateChanges({});
                }
            });
        });
    };

    uiState()->toolConfigChanged(TOOLBAR_NAME).onNotify(this, [this]() {
        load();
    });

    commandsController()->isNoteInputAllowedChanged().onReceive(this, [this](bool) {
        emit isInputAllowedChanged();
        load();
    });

    context()->currentNotationChanged().onNotify(this, [this, listenToNoteInputState]() {
        m_selectedNumericDegreeAction.clear();
        listenToNoteInputState();
        load();
        emit usesNumericPulseChanged();
    });

    listenToNoteInputState();

    load();
}

void NoteInputBarModel::load()
{
    MenuItemList items;
    const engraving::NumericPulseProfile* numericProfile = activeNumericPulseProfile();
    const bool usesNumericProfile = numericProfile != nullptr;
    m_loadedNumericProfileId = numericProfile ? numericProfile->id.toQString() : QString();
    const QString selectedDurationAction = usesNumericProfile ? currentNumericPulseDurationAction() : QString();
    const int selectedRegister = usesNumericProfile ? currentNumericPulseRegister() : 0;

    if (usesNumericProfile) {
        struct NumericPulseAction {
            ActionCode code;
            const char* title;
            const char* section;
            bool textButton;
        };
        static const NumericPulseAction numericPulseActions[] = {
            { "pad-note-4", "One pulse", "numeric-pulse", true },
            { "pad-note-8", "Half pulse", "numeric-pulse", true },
            { "pad-note-16", "Quarter pulse", "numeric-pulse", true },
            // Unlike pad-rest, the "rest" action enters one rest immediately
            // and advances the cursor. Numeric-pulse rendering displays it as a dot.
            { "rest", "Empty pulse", "numeric-rhythm", true },
            { "tie", "Continue / tie", "numeric-rhythm", true }
        };

        UiAction inputAction = uiActionsRegister()->action(ActionCode("note-input-by-note-name"));
        inputAction.title = TranslatableString::untranslatable("Numeric pulse input");
        items << makeActionItem(inputAction, QStringLiteral("numeric-input"));

        UiAction translationAction = uiActionsRegister()->action("toggle-kepatihan-translation");
        translationAction.title = TranslatableString::untranslatable("T");
        items << makeActionItem(translationAction, QStringLiteral("numeric-translation"));

        for (const engraving::NumericPulseDegree& degree : numericProfile->degrees) {
            UiAction action = uiActionsRegister()->action(ActionCode(degree.action.toStdString()));
            action.title = TranslatableString::untranslatable(degree.label);
            if (numericProfile->id == u"kendhang-kepatihan") {
                static const char* const strokeNames[] { "Ta stroke", "Tung stroke", "Dha stroke" };
                if (degree.number >= 1 && degree.number <= 3) {
                    action.description = TranslatableString::untranslatable(strokeNames[degree.number - 1]);
                }
            }
            action.iconCode = IconCode::Code::NONE;
            MenuItem* item = makeActionItem(action, QStringLiteral("numeric-degree"));
            UiActionState state = item->state();
            state.checked = degree.action.toQString() == m_selectedNumericDegreeAction;
            item->setState(state);
            items << item;
        }

        struct NumericRegisterAction {
            int registerOffset;
            const char16_t* label;
            const char* description;
        };
        static const NumericRegisterAction registerActions[] = {
            { -1, u"\u2193\u2022", "Lower register" },
            { 0, u"1", "Center register" },
            { 1, u"\u2191\u2022", "Upper register" }
        };
        for (const NumericRegisterAction& definition : registerActions) {
            const ActionCode code = "numeric-register-" + std::to_string(definition.registerOffset);
            UiAction action(code, UiCtxAny, mu::context::CTX_ANY, Checkable::Yes);
            action.title = TranslatableString::untranslatable(muse::String(definition.label));
            action.description = TranslatableString::untranslatable(definition.description);
            action.iconCode = IconCode::Code::NONE;
            MenuItem* item = makeActionItem(action, QStringLiteral("numeric-register"));
            item->setState(UiActionState::make_enabled(definition.registerOffset == selectedRegister));
            items << item;
        }

        for (const NumericPulseAction& definition : numericPulseActions) {
            UiAction action = uiActionsRegister()->action(definition.code);
            action.title = TranslatableString::untranslatable(definition.title);
            if (definition.textButton) {
                action.iconCode = IconCode::Code::NONE;
            }
            MenuItem* item = makeActionItem(action, QString::fromLatin1(definition.section));
            const QString actionId = QString::fromStdString(definition.code);
            if (isDurationAction(actionId)) {
                UiActionState state = item->state();
                state.checked = actionId == selectedDurationAction;
                item->setState(state);
            }
            items << item;
        }
    }

    ToolConfig noteInputConfig = uiState()->toolConfig(TOOLBAR_NAME, NotationUiActions::defaultNoteInputBarConfig());

    int section = 0;
    for (const ToolConfig::Item& citem : noteInputConfig.items) {
        if (!citem.show) {
            continue;
        }

        // Numeric-pulse notation has one predictable input mode. Keep the original
        // editing controls, but omit Western alternate entry modes which make
        // duration buttons insert the stored/default pitch.
        if (usesNumericProfile && isAlternativeNoteInputModeAction(citem.action)) {
            continue;
        }

        if (citem.action.empty()) {
            section++;
            continue;
        }

        MenuItemList subitems;
        if (citem.action == CROSS_STAFF_BEAMING_CODE) {
            subitems = makeCrossStaffBeamingItems();
        } else if (citem.action == TUPLET_ACTION_CODE) {
            subitems = makeTupletItems();
        }

        const QString sectionName = usesNumericProfile
                                    ? QStringLiteral("western-%1").arg(section)
                                    : QString::number(section);
        MenuItem* item = makeActionItem(uiActionsRegister()->action(citem.action), sectionName, subitems);
        const QString actionId = QString::fromStdString(citem.action);
        if (usesNumericProfile && isDurationAction(actionId)) {
            UiActionState state = item->state();
            state.checked = actionId == selectedDurationAction;
            item->setState(state);
        }
        items << item;
    }

    const QString addSection = usesNumericProfile
                               ? QStringLiteral("western-%1").arg(++section)
                               : QString::number(++section);
    items << makeAddItem(addSection);
    setItems(items);
}

bool NoteInputBarModel::isInputAllowed() const
{
    return commandsController()->isNoteInputAllowed();
}

const engraving::NumericPulseProfile* NoteInputBarModel::activeNumericPulseProfile() const
{
    INotationPtr notation = context()->currentNotation();
    if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
        return nullptr;
    }

    const NoteInputState& state = notation->interaction()->noteInput()->state();
    const engraving::Staff* staff = state.staff();
    if (const engraving::NumericPulseProfile* profile = engraving::numericPulseProfile(staff, state.tick())) {
        return profile;
    }

    INotationSelectionPtr selection = notation->interaction()->selection();
    const engraving::EngravingItem* selectedElement = selection ? selection->element() : nullptr;
    if (selectedElement) {
        if (const engraving::NumericPulseProfile* profile
                = engraving::numericPulseProfile(selectedElement->staff(), selectedElement->tick())) {
            return profile;
        }
    }

    // A newly-created score can have neither an input staff nor a selected
    // element yet. Use its first staff so Numeric Pulse controls are available
    // before the user chooses a Western duration or enters note-input mode.
    engraving::Score* score = notation->elements() ? notation->elements()->msScore() : nullptr;
    return score && score->nstaves() > 0
           ? engraving::numericPulseProfile(score->staff(0), engraving::Fraction(0, 1))
           : nullptr;
}

bool NoteInputBarModel::usesNumericPulse() const
{
    return activeNumericPulseProfile() != nullptr;
}

QString NoteInputBarModel::numericPulseFontFamily() const
{
    const engraving::NumericPulseProfile* profile = activeNumericPulseProfile();
    return profile ? profile->fontFamily.toQString() : QString();
}

void NoteInputBarModel::handleMenuItem(const QString& itemId)
{
    const engraving::NumericPulseProfile* numericProfile = activeNumericPulseProfile();
    const bool usesNumericProfile = numericProfile != nullptr;
    const bool selectNumericDegree = isNumericDegreeAction(itemId, numericProfile);
    const bool numericDuration = isDurationAction(itemId);
    const bool numericRegister = isNumericRegisterAction(itemId);
    const bool enterEmptyPulse = itemId == QStringLiteral("rest");
    INotationPtr notation = context()->currentNotation();
    INotationInteractionPtr interaction = notation ? notation->interaction() : nullptr;

    if (usesNumericProfile && numericDuration && interaction
        && !interaction->selection()->notes().empty()) {
        // Editing an existing rhythm must not first start note entry, reset
        // its duration to a quarter, or reset the pitch register. Dispatch the
        // requested duration once and keep the user's current input mode.
        AbstractMenuModel::handleMenuItem(itemId);
        return;
    }

    if (usesNumericProfile && numericRegister && interaction) {
        INotationNoteInputPtr noteInput = interaction->noteInput();
        INotationSelectionPtr selection = interaction->selection();
        if ((!selection || selection->isNone()) && noteInput && !noteInput->isNoteInputMode()) {
            noteInput->startNoteInput(NoteInputMethod::BY_NOTE_NAME);
        }
        interaction->setNumericPulseRegister(numericRegisterForAction(itemId));
        onActionsStateChanges({});
        return;
    }

    if (usesNumericProfile && (selectNumericDegree || numericDuration || enterEmptyPulse)) {
        INotationNoteInputPtr noteInput = interaction ? interaction->noteInput() : nullptr;
        if (noteInput) {
            const bool startingNumericInput = !noteInput->isNoteInputMode();
            const bool switchingToNumericInput = !noteInput->usingNoteInputMethod(NoteInputMethod::BY_NOTE_NAME);
            if (startingNumericInput) {
                // Numeric buttons are direct event-entry controls. Do not inherit
                // MuseScore's configurable Western BY_DURATION default, where the
                // first degree only primes a pitch and Rest only toggles rest mode.
                noteInput->startNoteInput(NoteInputMethod::BY_NOTE_NAME);
            } else if (switchingToNumericInput) {
                noteInput->setNoteInputMethod(NoteInputMethod::BY_NOTE_NAME);
            }

            const engraving::DurationType duration = noteInput->state().duration().type();
            const bool validNumericDuration = duration == engraving::DurationType::V_QUARTER
                                              || duration == engraving::DurationType::V_EIGHTH
                                              || duration == engraving::DurationType::V_16TH;
            if (!validNumericDuration) {
                // One numeric pulse in the current 4/4 workflow is a quarter note.
                // Never inherit a whole-note duration from a selected measure rest.
                noteInput->setDuration(engraving::DurationType::V_QUARTER);
            }
            if (startingNumericInput || switchingToNumericInput || !validNumericDuration) {
                // A selected Western note or an earlier score can leave an octave
                // pitch in InputState. Numeric entry must begin at the profile's
                // center register until the user explicitly chooses another one.
                noteInput->setNumericPulseRegister(numericProfile->defaultRegister);
            }
        }
    }

    if (usesNumericProfile && enterEmptyPulse && interaction) {
        // Dispatching the generic Rest action immediately after entering note
        // input can use its stale Western action state and do nothing. Numeric
        // Pulse's dot is a direct event-entry control, so insert it explicitly.
        interaction->putRestToSelection();
        onActionsStateChanges({});
        return;
    }

    AbstractMenuModel::handleMenuItem(itemId);

    if (selectNumericDegree) {
        m_selectedNumericDegreeAction = itemId;
        onActionsStateChanges({});
    }
}

void NoteInputBarModel::onActionsStateChanges(const ActionCodeList& codes)
{
    AbstractMenuModel::onActionsStateChanges(codes);

    if (!usesNumericPulse()) {
        return;
    }

    const QString selectedDurationAction = currentNumericPulseDurationAction();
    const int selectedRegister = currentNumericPulseRegister();
    for (MenuItem* item : items()) {
        if (!item) {
            continue;
        }

        UiActionState state = item->state();
        if (item->section() == QStringLiteral("numeric-degree")) {
            state.checked = item->id() == m_selectedNumericDegreeAction;
            item->setState(state);
        } else if (item->section() == QStringLiteral("numeric-register")) {
            state.checked = numericRegisterForAction(item->id()) == selectedRegister;
            item->setState(state);
        } else if (isDurationAction(item->id())) {
            state.checked = item->id() == selectedDurationAction;
            item->setState(state);
        }
    }
}

QString NoteInputBarModel::currentNumericPulseDurationAction() const
{
    INotationPtr notation = context()->currentNotation();
    if (!notation || !notation->interaction() || !notation->interaction()->noteInput()) {
        return {};
    }

    switch (notation->interaction()->noteInput()->state().duration().type()) {
    case engraving::DurationType::V_WHOLE: return QStringLiteral("pad-note-1");
    case engraving::DurationType::V_HALF: return QStringLiteral("pad-note-2");
    case engraving::DurationType::V_QUARTER: return QStringLiteral("pad-note-4");
    case engraving::DurationType::V_EIGHTH: return QStringLiteral("pad-note-8");
    case engraving::DurationType::V_16TH: return QStringLiteral("pad-note-16");
    case engraving::DurationType::V_32ND: return QStringLiteral("pad-note-32");
    case engraving::DurationType::V_64TH: return QStringLiteral("pad-note-64");
    case engraving::DurationType::V_128TH: return QStringLiteral("pad-note-128");
    case engraving::DurationType::V_256TH: return QStringLiteral("pad-note-256");
    case engraving::DurationType::V_512TH: return QStringLiteral("pad-note-512");
    case engraving::DurationType::V_1024TH: return QStringLiteral("pad-note-1024");
    default: return {};
    }
}

int NoteInputBarModel::currentNumericPulseRegister() const
{
    const engraving::NumericPulseProfile* profile = activeNumericPulseProfile();
    if (!profile) {
        return 0;
    }

    INotationPtr notation = context()->currentNotation();
    INotationInteractionPtr interaction = notation ? notation->interaction() : nullptr;
    INotationNoteInputPtr noteInput = interaction ? interaction->noteInput() : nullptr;
    if (noteInput && noteInput->state().numericPulseRegisterExplicit()) {
        return noteInput->state().numericPulseRegisterOffset();
    }
    return profile->defaultRegister;
}

MenuItem* NoteInputBarModel::makeActionItem(const UiAction& action, const QString& section,
                                            const muse::uicomponents::MenuItemList& subitems)
{
    MenuItem* item = new MenuItem(action, this);
    // "add" is a local toolbar menu action, not an action registered in
    // UiActionsRegister. Querying its state produces a noisy error on every
    // toolbar reload.
    if (action.code != ADD_ACTION_CODE && action.code.rfind("numeric-register-", 0) != 0) {
        item->setState(uiActionsRegister()->actionState(action.code));
    }
    item->setSection(section);
    item->setSubitems(subitems);
    return item;
}

MenuItem* NoteInputBarModel::makeAddItem(const QString& section)
{
    static const UiAction addAction(ADD_ACTION_CODE, UiCtxAny, mu::context::CTX_ANY,
                                    TranslatableString("global", "Add"),
                                    IconCode::Code::PLUS);

    return makeActionItem(addAction, section, makeAddItems());
}

MenuItemList NoteInputBarModel::makeCrossStaffBeamingItems()
{
    MenuItemList items {
        makeMenuItem("move-up"),
        makeMenuItem("move-down")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeTupletItems()
{
    MenuItemList items {
        makeMenuItem(ADD_DUPLET_COMMAND),
        makeMenuItem(ADD_TRIPLET_COMMAND),
        makeMenuItem(ADD_QUADRUPLET_COMMAND),
        makeMenuItem(ADD_QUINTUPLET_COMMAND),
        makeMenuItem(ADD_SEXTUPLET_COMMAND),
        makeMenuItem(ADD_SEPTUPLET_COMMAND),
        makeMenuItem(ADD_OCTUPLET_COMMAND),
        makeMenuItem(ADD_NONUPLET_COMMAND),
        makeMenuItem(SHOW_TUPLET_CONFIGURE_COMMAND)
    };

    return items;
}

MenuItemList NoteInputBarModel::makeAddItems()
{
    MenuItemList items {
        makeMenu(TranslatableString("notation", "Notes"), makeNotesItems()),
        makeMenu(TranslatableString("notation", "Intervals"), makeIntervalsItems()),
        makeMenu(TranslatableString("notation", "Measures"), makeMeasuresItems()),
        makeMenu(TranslatableString("notation", "Frames"), makeFramesItems()),
        makeMenu(TranslatableString("notation", "Text"), makeTextItems()),
        makeMenu(TranslatableString("notation", "Lines"), makeLinesItems()),
        makeMenu(TranslatableString("notation", "Chords and fretboard diagrams"), makeChordAndFretboardDiagramsItems()),
    };

    return items;
}

MenuItemList NoteInputBarModel::makeNotesItems()
{
    MenuItemList items {
        makeMenuItem("command://notation/enter-note-c"),
        makeMenuItem("command://notation/enter-note-d"),
        makeMenuItem("command://notation/enter-note-e"),
        makeMenuItem("command://notation/enter-note-f"),
        makeMenuItem("command://notation/enter-note-g"),
        makeMenuItem("command://notation/enter-note-a"),
        makeMenuItem("command://notation/enter-note-b"),
        makeSeparator(),
        makeMenuItem("command://notation/add-note-c"),
        makeMenuItem("command://notation/add-note-d"),
        makeMenuItem("command://notation/add-note-e"),
        makeMenuItem("command://notation/add-note-f"),
        makeMenuItem("command://notation/add-note-g"),
        makeMenuItem("command://notation/add-note-a"),
        makeMenuItem("command://notation/add-note-b")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeIntervalsItems()
{
    MenuItemList items {
        makeMenuItem("interval1"),
        makeMenuItem("interval2"),
        makeMenuItem("interval3"),
        makeMenuItem("interval4"),
        makeMenuItem("interval5"),
        makeMenuItem("interval6"),
        makeMenuItem("interval7"),
        makeMenuItem("interval8"),
        makeMenuItem("interval9"),
        makeMenuItem("interval10"),
        makeSeparator(),
        makeMenuItem("interval-2"),
        makeMenuItem("interval-3"),
        makeMenuItem("interval-4"),
        makeMenuItem("interval-5"),
        makeMenuItem("interval-6"),
        makeMenuItem("interval-7"),
        makeMenuItem("interval-8"),
        makeMenuItem("interval-9"),
        makeMenuItem("interval-10")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeMeasuresItems()
{
    MenuItemList items {
        makeMenuItem("insert-measure"),
        makeMenuItem("append-measure"),
        makeSeparator(),
        makeMenuItem("insert-measures"),
        makeMenuItem("insert-measures-after-selection"),
        makeSeparator(),
        makeMenuItem("insert-measures-at-start-of-score"),
        makeMenuItem("append-measures")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeFramesItems()
{
    MenuItemList items {
        makeMenuItem("insert-hbox"),
        makeMenuItem("insert-vbox"),
        makeMenuItem("insert-textframe"),
        makeMenuItem("insert-fretframe"),
        makeSeparator(),
        makeMenu(TranslatableString("notation", "Insert at end of score"), makeFramesAppendItems())
    };

    return items;
}

MenuItemList NoteInputBarModel::makeFramesAppendItems()
{
    MenuItemList items {
        makeMenuItem("append-hbox"),
        makeMenuItem("append-vbox"),
        makeMenuItem("append-textframe"),
        makeMenuItem("append-fretframe")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeTextItems()
{
    MenuItemList items {
        makeMenuItem("title-text"),
        makeMenuItem("subtitle-text"),
        makeMenuItem("composer-text"),
        makeMenuItem("poet-text"),
        makeMenuItem("part-text"),
        makeSeparator(),
        makeMenuItem("system-text"),
        makeMenuItem("staff-text"),
        makeMenuItem("add-dynamic"),
        makeMenuItem("expression-text"),
        makeMenuItem("rehearsalmark-text"),
        makeMenuItem("instrument-change-text"),
        makeMenuItem("fingering-text"),
        makeSeparator(),
        makeMenuItem("sticking-text"),
        makeMenuItem("chord-text"),
        makeMenuItem("roman-numeral-text"),
        makeMenuItem("nashville-number-text"),
        makeMenuItem("lyrics"),
        makeMenuItem("figured-bass"),
        makeMenuItem("tempo")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeLinesItems()
{
    MenuItemList items {
        makeMenuItem("command://notation/add-slur"),
        makeMenuItem("add-hairpin"),
        makeMenuItem("add-hairpin-reverse"),
        makeMenuItem("add-8va"),
        makeMenuItem("add-8vb"),
        makeMenuItem("add-noteline")
    };

    return items;
}

MenuItemList NoteInputBarModel::makeChordAndFretboardDiagramsItems()
{
    MenuItemList items {
        makeMenuItem("chord-text"),
        makeMenuItem("add-fretboard-diagram"),
        makeSeparator(),
        makeMenuItem("insert-fretframe", TranslatableString("notation", "Fretboard diagram legend"))
    };

    return items;
}

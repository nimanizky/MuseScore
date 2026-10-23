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

pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

Item {
    id: root

    property alias orientation: gridView.orientation

    property bool floating: false

    property int maximumWidth: 0
    property int maximumHeight: 0

    width: gridView.isHorizontal ? childrenRect.width : 76
    height: !gridView.isHorizontal ? childrenRect.height : 40

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "NoteInputBar"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("notation", "Note input toolbar")
    }

    NoteInputBarModel {
        id: noteInputModel
    }

    QtObject {
        id: prv

        function horizontalContentWidth() {
            var sections = ({})
            var sectionCount = 0

            for (var i = 0; i < noteInputModel.length; ++i) {
                var section = noteInputModel.get(i).section
                if (sections[section] === undefined) {
                    sections[section] = true
                    ++sectionCount
                }
            }

            if (noteInputModel.length === 0) {
                return 0
            }

            // Each item occupies one cell plus column spacing. Each section
            // contributes its separator and the spacing before the next one.
            return noteInputModel.length * (gridView.cellWidth + gridView.columnSpacing)
                   + sectionCount
                   + Math.max(0, sectionCount - 1) * gridView.columnSpacing
        }

        function resolveHorizontalGridViewWidth() {
            var contentWidth = horizontalContentWidth()

            if (root.floating) {
                return contentWidth
            }

            var requiredFreeSpace = gridView.cellWidth * 3 + gridView.rowSpacing * 4

            if (root.maximumWidth - contentWidth < requiredFreeSpace) {
                return contentWidth - requiredFreeSpace
            }

            return contentWidth
        }

        function resolveVerticalGridViewHeight() {
            if (root.floating) {
                return gridView.contentHeight
            }

            var requiredFreeSpace = gridView.cellHeight * 3 + gridView.rowSpacing * 4

            if (root.maximumHeight - gridView.contentHeight < requiredFreeSpace) {
                return gridView.contentHeight - requiredFreeSpace
            }

            return gridView.contentHeight
        }
    }

    GridViewSectional {
        id: gridView

        sectionRole: "section"

        rowSpacing: 4
        columnSpacing: 4

        cellWidth: 32
        cellHeight: cellWidth

        sectionWidth: isHorizontal ? 1 : width
        sectionHeight: isHorizontal ? height : 1

        clip: true

        model: noteInputModel

        sectionDelegate: SeparatorLine {
            required property int itemIndex

            orientation: gridView.isHorizontal ? Qt.Vertical : Qt.Horizontal
            visible: itemIndex !== 0
        }

        itemDelegate: FlatButton {
            id: btn

            required property var itemModel

            readonly property MenuItem item: Boolean(itemModel) ? itemModel.item : null
            readonly property bool hasMenu: Boolean(item) && item.subitems.length !== 0
            readonly property var numericPulseLabels: ({
                "pad-note-4": "1", "pad-note-8": "½", "pad-note-16": "¼",
                "rest": ".", "tie": "—"
            })
            readonly property bool useNumericPulseLabel: noteInputModel.usesNumericPulse
                                                         && Boolean(item)
                                                         && item.section.startsWith("numeric-")
                                                         && (item.section === "numeric-degree"
                                                             || item.section === "numeric-register"
                                                             || item.section === "numeric-translation"
                                                             || numericPulseLabels[item.id] !== undefined)
            readonly property bool isNumericPulseDegree: useNumericPulseLabel
                                                         && item.section === "numeric-degree"
            readonly property string numericPulseLabel: item
                                                         && (item.section === "numeric-degree"
                                                             || item.section === "numeric-register"
                                                             || item.section === "numeric-translation")
                                                         ? item.title
                                                         : numericPulseLabels[item.id] || ""

            width: gridView.cellWidth
            height: gridView.cellWidth

            textFont: isNumericPulseDegree && noteInputModel.numericPulseFontFamily.length > 0
                      ? Qt.font({ family: noteInputModel.numericPulseFontFamily, pixelSize: 22 })
                      : ui.theme.bodyFont

            enabled: noteInputModel.isInputAllowed

            accentButton: (Boolean(item) && item.checked) || menuLoader.isMenuOpened
            transparent: !accentButton

            icon: Boolean(item) && !useNumericPulseLabel ? item.icon : IconCode.NONE
            text: useNumericPulseLabel ? numericPulseLabel : ""
            iconFont: ui.theme.toolbarIconsFont

            toolTipTitle: Boolean(item) && isNumericPulseDegree && Boolean(item.description)
                          ? item.description : Boolean(item) ? item.title : ""
            toolTipDescription: Boolean(item) && isNumericPulseDegree && Boolean(item.description)
                                ? "" : Boolean(item) ? item.description : ""
            toolTipShortcut: Boolean(item) ? item.shortcuts : ""

            navigation.panel: root.navigationPanel
            navigation.name: Boolean(item) ? item.id : ""
            navigation.accessible.name: Boolean(item) && isNumericPulseDegree && Boolean(item.description)
                                        ? item.description : Boolean(item) ? item.title : ""
            navigation.order: Boolean(itemModel) ? itemModel.order : 0
            isClickOnKeyNavTriggered: false
            navigation.onTriggered: {
                if (btn.hasMenu) {
                    toggleMenuOpened()
                } else {
                    handleMenuItem()
                }
            }

            function toggleMenuOpened() {
                menuLoader.toggleOpened(item.subitems)
            }

            function handleMenuItem() {
                Qt.callLater(noteInputModel.handleMenuItem, item.id)
            }

            onClicked: {
                if (btn.hasMenu) {
                    toggleMenuOpened()
                } else {
                    handleMenuItem()
                }
            }

            mouseArea.onPressAndHold: function(event) {
                if (menuLoader.isMenuOpened || !btn.hasMenu) {
                    event.accepted = false // do not suppress the click event
                    return
                }

                btn.toggleMenuOpened()
            }

            StyledMenuLoader {
                id: menuLoader

                onHandleMenuItem: function(itemId) {
                    noteInputModel.handleMenuItem(itemId)
                }
            }
        }
    }

    PopupButton {
        id: customizeButton

        anchors.margins: 4

        width: gridView.cellWidth
        height: gridView.cellHeight

        icon: IconCode.SETTINGS_COG
        iconFont: ui.theme.toolbarIconsFont
        toolTipTitle: qsTrc("notation", "Customize toolbar")
        toolTipDescription: qsTrc("notation", "Show/hide toolbar buttons")
        transparent: true

        enabled: noteInputModel.isInputAllowed

        navigation.panel: root.navigationPanel
        navigation.order: 100
        navigation.accessible.name: qsTrc("notation", "Customize toolbar")

        popupAnchorItem: root.floating ? null : ui.rootItem
        popupComponent: NoteInputBarCustomisePopup {}
    }

    states: [
        State {
            when: gridView.isHorizontal

            PropertyChanges {
                target: gridView
                width: prv.resolveHorizontalGridViewWidth()
                height: root.height
                sectionWidth: 1
                sectionHeight: root.height
                rows: 1
                columns: gridView.noLimit
            }

            AnchorChanges {
                target: customizeButton
                anchors.left: gridView.right
                anchors.verticalCenter: root.verticalCenter
            }
        },
        State {
            when: !gridView.isHorizontal

            PropertyChanges {
                target: gridView
                width: root.width
                height: prv.resolveVerticalGridViewHeight()
                sectionWidth: root.width
                sectionHeight: 1
                rows: gridView.noLimit
                columns: 2
            }

            AnchorChanges {
                target: customizeButton
                anchors.top: gridView.bottom
                anchors.right: parent.right
            }
        }
    ]
}

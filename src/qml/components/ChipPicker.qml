// Copyright (c) 2026 madalone. Chip picker: a value out of a short list, as a grid of text chips or colour swatches.
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

import Haptic 1.0

import "qrc:/components" as Components

// A choice out of a short list, drawn as a grid of chips under the texts of a settings row (Components.SettingRow with
// controlBelow, docs/design-system.md sections 5 and 6).
//
// - A text chip shows its label: the chosen one is drawn pressed (textPrimary fill, bg label), the others on surface
//   with a divider edge.
// - A colour chip is filled with its colour or gradient. The chosen one carries a check mark (the current value is a
//   check mark), in the text token that reads best on that swatch.
//
// The picker takes the keypad focus as a whole and draws the ring around the grid: LEFT/RIGHT step the value, a tap
// picks one, OK does nothing. With `multiple` every chip is a toggle of its own, so the ring marks one chip, the one
// OK toggles: LEFT/RIGHT move it. The host stores what `picked` reports.
//
// The chips are placed with explicit x/y math: a GridLayout costs about 30 ms per child on the device.
Item {
    id: chipPicker

    // { value, label } for a text chip, { value, color } or { value, gradient } for a colour chip
    property var model: []
    // the chosen value; a value that is not in the model (it belongs to another picker of the same setting, such as
    // the gradients under the solid colours) marks no chip
    property var currentValue
    // with `multiple`: the values that are on
    property bool multiple: false
    property var currentValues: []
    property int columns: 3
    // LEFT/RIGHT carry on from the last chip to the first one and back, instead of stopping at the ends
    property bool wrap: false
    // a click when LEFT/RIGHT change the value; off when choosing gives feedback of its own, such as a sound preview
    property bool stepHaptic: true

    readonly property real cellSpacing: 10
    readonly property real cellHeight: 60
    readonly property real cellWidth: (width - (columns - 1) * cellSpacing) / columns
    readonly property int currentIndex: indexOf(currentValue)
    // the chip in the ring of a multiple picker
    property int cursor: 0

    // a tap, LEFT/RIGHT or OK chose this value: the host stores it, or toggles it in a multiple picker
    signal picked(var value)

    width: parent ? parent.width : 0
    height: Math.max(0, Math.ceil(model.length / columns) * (cellHeight + cellSpacing) - cellSpacing)

    function indexOf(value) {
        for (let i = 0; i < model.length; i++) {
            if (model[i].value === value) {
                return i;
            }
        }

        return -1;
    }

    function step(delta) {
        const count = model.length;
        if (count === 0) {
            return;
        }

        if (multiple) {
            cursor = Math.max(0, Math.min(count - 1, cursor + delta));
            return;
        }

        let next;
        if (currentIndex < 0) {
            // the value is in another picker of the same setting: come in from the side the key points from
            next = delta > 0 ? 0 : count - 1;
        } else if (wrap) {
            next = (currentIndex + delta + count) % count;
        } else {
            next = currentIndex + delta;
            if (next < 0 || next >= count) {
                return;
            }
        }

        if (stepHaptic) {
            Haptic.play(Haptic.Click);
        }
        picked(model[next].value);
    }

    // WCAG relative luminance
    function luminance(c) {
        function channel(v) {
            return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4);
        }

        return 0.2126 * channel(c.r) + 0.7152 * channel(c.g) + 0.0722 * channel(c.b);
    }

    // the text token with the higher contrast on a swatch, for its check mark
    function markColor(fill) {
        const l = luminance(fill);
        const onDark = (l + 0.05) / (luminance(colors.bg) + 0.05);
        const onLight = (luminance(colors.textPrimary) + 0.05) / (l + 0.05);
        return onDark >= onLight ? colors.bg : colors.textPrimary;
    }

    // the colour of a gradient at its middle, where the check mark sits
    function middleOf(gradient) {
        let middle = gradient.stops[0];
        for (let i = 1; i < gradient.stops.length; i++) {
            if (Math.abs(gradient.stops[i].position - 0.5) < Math.abs(middle.position - 0.5)) {
                middle = gradient.stops[i];
            }
        }

        return middle.color;
    }

    function toggleCursor(event) {
        if (multiple && cursor < model.length) {
            picked(model[cursor].value);
        } else {
            event.accepted = false;
        }
    }

    Keys.onLeftPressed: step(-1)
    Keys.onRightPressed: step(1)
    Keys.onReturnPressed: toggleCursor(event)
    Keys.onEnterPressed: toggleCursor(event)

    Components.Selectable {
        anchors.margins: -6
        selected: chipPicker.activeFocus && !chipPicker.multiple
    }

    Repeater {
        model: chipPicker.model

        Rectangle {
            id: chip

            readonly property bool swatch: modelData.color !== undefined || modelData.gradient !== undefined
            readonly property bool chosen: chipPicker.multiple ? chipPicker.currentValues.indexOf(modelData.value) >= 0
                                                               : chipPicker.currentIndex === index
            // the colour under the check mark; a gradient's stops have no change signal, so its middle is read once
            property color sample: modelData.color !== undefined ? modelData.color : colors.transparent

            Component.onCompleted: {
                if (modelData.gradient !== undefined) {
                    sample = chipPicker.middleOf(modelData.gradient);
                }
            }

            x: (index % chipPicker.columns) * (chipPicker.cellWidth + chipPicker.cellSpacing)
            y: Math.floor(index / chipPicker.columns) * (chipPicker.cellHeight + chipPicker.cellSpacing)
            width: chipPicker.cellWidth
            height: chipPicker.cellHeight
            radius: ui.cornerRadiusSmall
            color: swatch ? sample : (chosen ? colors.textPrimary : colors.surface)
            gradient: modelData.gradient !== undefined ? modelData.gradient : null
            border { color: colors.divider; width: 2 }

            // the ring of a multiple picker, on the chip OK toggles
            Components.Selectable {
                anchors.margins: -5
                radius: ui.cornerRadiusSmall + 5
                selected: chipPicker.multiple && chipPicker.activeFocus && chipPicker.cursor === index
            }

            Text {
                visible: !chip.swatch
                anchors.centerIn: parent
                width: parent.width - 12
                text: modelData.label !== undefined ? modelData.label : ""
                color: chip.chosen ? colors.bg : colors.textPrimary
                font: fonts.caption()
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
            }

            Components.Icon {
                visible: chip.swatch && chip.chosen
                anchors.centerIn: parent
                icon: "uc:check"
                size: 60
                color: chipPicker.markColor(chip.sample)
            }

            Components.HapticMouseArea {
                anchors.fill: parent
                onClicked: {
                    if (chipPicker.multiple) {
                        chipPicker.cursor = index;
                    }
                    chipPicker.picked(modelData.value);
                }
            }
        }
    }
}

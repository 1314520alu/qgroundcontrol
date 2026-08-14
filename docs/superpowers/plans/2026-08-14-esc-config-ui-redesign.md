# ESC Config UI Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Rebuild `APMESCComponent.qml` as Layout A (left config / right calibration) with numbered step cards, **pixel-faithful to** [`docs/superpowers/specs/assets/esc-config-ui-preview-unirc10-pro.png`](../specs/assets/esc-config-ui-preview-unirc10-pro.png).

**Architecture:** Pure QML redesign inside existing `SetupPage` + `FactPanelController`. No new C++. Facts stay `MOT_*`/`Q_M_*` + `ESC_CALIBRATION`/`Q_ESC_CAL` + optional DShot. Visual chrome is local `Rectangle` panels matching the lock PNG (same pattern as `APMServoComponent.qml` Layout C).

**Tech Stack:** Qt 6 QML, `SetupPage`, `FactComboBox` / `FactTextField`, `QGCButton`, `QGCLabel`, `QGCFlickable` (steps only), `ScreenTools`, `QGCPalette`.

## Global Constraints

- **Visual lock:** Match [`docs/superpowers/specs/2026-08-14-esc-config-ui-redesign-design.md`](../specs/2026-08-14-esc-config-ui-redesign-design.md) + PNG. Structure/chrome deviations are bugs.
- **Layout A only** — left/right split; no stacked-first on short landscape when width allows split.
- **Temporary device:** SIYI UniRC 10 Pro 10.1″ 1920×1200 for screenshots; still keep G20-class short height workable.
- Fact System only; calibration still sets Fact rawValue `= 3`.
- Do not restyle Vehicle Setup sidebar / toolbar.
- Conventional Commits; no unrelated refactors.
- Prefer `qgcPal` / `ScreenTools` over hard-coded hex (map mockup blue → `buttonHighlight` for CTA + badges).

---

## File map

| File | Role |
|------|------|
| `src/AutoPilotPlugins/APM/APMESCComponent.qml` | Entire page UI rewrite |
| `docs/superpowers/specs/2026-08-14-esc-config-ui-redesign-design.md` | Spec (already locked) |
| `docs/superpowers/specs/assets/esc-config-ui-preview-unirc10-pro.png` | Visual lock — open while implementing |

No new QML files unless a step component grows past ~80 lines *and* is reused; default = keep everything in `APMESCComponent.qml`.

---

### Task 1: Split shell + visual panels (no behavior change yet)

**Files:**
- Modify: `src/AutoPilotPlugins/APM/APMESCComponent.qml`
- Reference: lock PNG + `src/AutoPilotPlugins/APM/APMServoComponent.qml` (panel `Rectangle` idiom)

**Interfaces:**
- Consumes: existing `SetupPage` `availableWidth` / `availableHeight`, existing Fact properties already in file
- Produces: `pageRoot` Item sized to viewport; `configPanel` + `calPanel` side by side

- [ ] **Step 1: Open visual lock and note chrome**

Open `docs/superpowers/specs/assets/esc-config-ui-preview-unirc10-pro.png`. Confirm: two content cards, PWM pair, red banner, blue CTA, 8 numbered rows.

- [ ] **Step 2: Replace stacked `ColumnLayout` of `QGCGroupBox` with split shell**

Replace the current `escPageComponent` body with a viewport-filling split. Keep all existing Fact `property` declarations at the top of the component.

```qml
Component {
    id: escPageComponent

    Item {
        id: pageRoot
        width: availableWidth
        height: availableHeight

        // … keep existing Fact properties / _escPrefix / _isDshot / etc. …

        QGCPalette { id: qgcPal; colorGroupEnabled: true }

        readonly property real _pad: ScreenTools.defaultFontPixelHeight * 0.55
        readonly property real _gap: ScreenTools.defaultFontPixelWidth * 1.2
        readonly property real _panelRadius: ScreenTools.defaultBorderRadius
        readonly property bool _split: width >= ScreenTools.defaultFontPixelWidth * 55

        // Temporary: always prefer split when wide enough (UniRC 10 Pro / G20).
        // Tall narrow: stack config then cal (fallback only).

        RowLayout {
            anchors.fill: parent
            spacing: pageRoot._gap
            visible: pageRoot._split

            // Config panel (~55%)
            Rectangle {
                id: configPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 55
                color: qgcPal.window
                border.width: 1
                border.color: qgcPal.groupBorder
                radius: pageRoot._panelRadius
                // placeholder ColumnLayout for Task 2
            }

            // Calibration panel (~45%)
            Rectangle {
                id: calPanel
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 45
                color: qgcPal.window
                border.width: 1
                border.color: qgcPal.groupBorder
                radius: pageRoot._panelRadius
                // placeholder for Task 3
            }
        }

        // Fallback stack when too narrow — same panels, ColumnLayout
        ColumnLayout {
            anchors.fill: parent
            spacing: pageRoot._gap
            visible: !pageRoot._split
            // reparent or duplicate panel content via shared Components in later tasks
        }
    }
}
```

Also set on `SetupPage` root if present:

```qml
showPageDescription: false   // title/hint live in left panel (match lock)
```

- [ ] **Step 3: Smoke-build**

Run: `just build` (or incremental target that rebuilds QML).  
Expected: build succeeds; ESC page shows two empty bordered panels side-by-side on a wide window.

- [ ] **Step 4: Commit**

```bash
git add src/AutoPilotPlugins/APM/APMESCComponent.qml
git commit -m "$(cat <<'EOF'
refactor(APM): split ESC setup page into Layout A panels

EOF
)"
```

---

### Task 2: Left panel — Configuration (match lock)

**Files:**
- Modify: `src/AutoPilotPlugins/APM/APMESCComponent.qml` (`configPanel` contents)

**Interfaces:**
- Consumes: `_motPwmType`, `_motPwmMin/Max`, `_motSpinArm/Min/Max`, DShot facts, visibility flags
- Produces: UI that mirrors left half of lock PNG

- [ ] **Step 1: Implement left panel column matching lock order**

Inside `configPanel`:

```qml
ColumnLayout {
    anchors.fill: parent
    anchors.margins: pageRoot._pad
    spacing: ScreenTools.defaultFontPixelHeight * 0.45

    QGCLabel {
        text: qsTr("Configuration")
        font.bold: true
    }
    QGCLabel {
        text: qsTr("Configure and calibrate electronic speed controllers.")
        font.pointSize: ScreenTools.smallFontPointSize
        opacity: 0.55
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }

    // Output type + reboot chip (chip to the RIGHT of combo, same row)
    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth
        visible: _motPwmTypeAvailable

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            QGCLabel { text: qsTr("Output type") }
            FactComboBox {
                fact: _motPwmType
                indexModel: false
                Layout.fillWidth: true
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignBottom
            radius: height / 2
            color: qgcPal.button
            border.color: qgcPal.buttonBorder
            border.width: 1
            implicitHeight: ScreenTools.implicitButtonHeight * 0.85
            implicitWidth: rebootLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 2
            QGCLabel {
                id: rebootLabel
                anchors.centerIn: parent
                text: qsTr("Requires vehicle reboot")
                font.pointSize: ScreenTools.smallFontPointSize
                color: qgcPal.text
                opacity: 0.7
            }
        }
    }

    // PWM Min | Max — TWO COLUMNS (lock: side-by-side, label above field)
    RowLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelWidth
        visible: _motPwmMinAvailable || _motPwmMaxAvailable

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            visible: _motPwmMinAvailable
            QGCLabel { text: qsTr("Output PWM min") }
            FactTextField {
                fact: _motPwmMin
                Layout.fillWidth: true
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            visible: _motPwmMaxAvailable
            QGCLabel { text: qsTr("Output PWM max") }
            FactTextField {
                fact: _motPwmMax
                Layout.fillWidth: true
            }
        }
    }

    // Spin rows — full width, label left / field right OR label-above to match lock density
    LabelledFactTextField {
        label: qsTr("Spin when armed")
        fact: _motSpinArm
        visible: _motSpinArmAvailable
        textFieldShowHelp: true
        Layout.fillWidth: true
    }
    LabelledFactTextField {
        label: qsTr("Spin minimum")
        fact: _motSpinMin
        visible: _motSpinMinAvailable
        textFieldShowHelp: true
        Layout.fillWidth: true
    }
    LabelledFactTextField {
        label: qsTr("Spin maximum")
        fact: _motSpinMax
        visible: _motSpinMaxAvailable
        textFieldShowHelp: true
        Layout.fillWidth: true
    }

    LabelledFactComboBox {
        label: qsTr("DShot ESC type")
        fact: _servoDshotEsc
        indexModel: false
        visible: _isDshot && _servoDshotEscAvailable
        Layout.fillWidth: true
    }
    LabelledFactComboBox {
        label: qsTr("DShot output rate")
        fact: _servoDshotRate
        indexModel: false
        visible: _isDshot && _servoDshotRateAvailable
        Layout.fillWidth: true
    }

    Item { Layout.fillHeight: true } // push content to top like lock
}
```

**Visual checks vs PNG:** reboot control is muted pill (not red/primary); PWM are **paired**; no `QGCGroupBox` title chrome.

- [ ] **Step 2: Build and compare**

Run: `just build`. On desktop or UniRC 10 Pro, open ESC page and compare left panel to lock PNG side-by-side. Adjust padding/spacing only if mismatch is obvious.

- [ ] **Step 3: Commit**

```bash
git add src/AutoPilotPlugins/APM/APMESCComponent.qml
git commit -m "$(cat <<'EOF'
feat(APM): ESC config panel matches Layout A lock

EOF
)"
```

---

### Task 3: Right panel — warning, CTA, numbered steps

**Files:**
- Modify: `src/AutoPilotPlugins/APM/APMESCComponent.qml` (`calPanel` contents)

**Interfaces:**
- Consumes: `_escCalibration`, `_escCalibrationAvailable`
- Produces: lock-faithful calibration column; writes Fact `= 3` on CTA

- [ ] **Step 1: Define step model (8 short strings)**

Map existing English steps to a compact model (keep `qsTr`):

```qml
readonly property var _calSteps: [
    qsTr("Disconnect USB and battery so the flight controller powers down"),
    qsTr("Connect the battery"),
    qsTr("The arming tone will play (if a buzzer is attached)"),
    qsTr("If there is a safety button, press until solid red"),
    qsTr("You will hear a musical tone then two beeps"),
    qsTr("A few seconds later, beeps for each battery cell"),
    qsTr("A single long beep means end points are set"),
    qsTr("Disconnect the battery and power up normally")
]
```

- [ ] **Step 2: Implement right panel chrome**

```qml
ColumnLayout {
    anchors.fill: parent
    anchors.margins: pageRoot._pad
    spacing: ScreenTools.defaultFontPixelHeight * 0.4
    visible: _escCalibrationAvailable

    QGCLabel {
        text: qsTr("Calibration")
        font.bold: true
    }

    // Warning banner — pale red fill, warningText, optional icon
    Rectangle {
        Layout.fillWidth: true
        radius: ScreenTools.defaultBorderRadius
        color: Qt.rgba(qgcPal.warningText.r, qgcPal.warningText.g, qgcPal.warningText.b, 0.12)
        border.color: qgcPal.warningText
        border.width: 1
        implicitHeight: warnRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.5

        RowLayout {
            id: warnRow
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth * 0.8
            spacing: ScreenTools.defaultFontPixelWidth * 0.6

            QGCColoredImage {
                source: "/res/warning.svg"   // use existing QGC warning asset if present; else omit icon
                width: ScreenTools.defaultFontPixelHeight
                height: width
                color: qgcPal.warningText
                visible: status === Image.Ready
            }
            QGCLabel {
                Layout.fillWidth: true
                text: qsTr("WARNING: Remove props prior to calibration!")
                color: qgcPal.warningText
                wrapMode: Text.WordWrap
                font.bold: true
            }
        }
    }

    QGCButton {
        Layout.fillWidth: true
        text: qsTr("Calibrate")
        // Visual lock: vivid blue CTA (mockup), not muted gray
        backgroundColor: qgcPal.buttonHighlight
        textColor: qgcPal.buttonHighlightText
        enabled: _escCalibration && _escCalibration.rawValue === 0
        onClicked: if (_escCalibration) _escCalibration.rawValue = 3
    }

    QGCFlickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        contentWidth: width
        contentHeight: stepsCol.implicitHeight
        clip: true

        ColumnLayout {
            id: stepsCol
            width: parent.width
            spacing: ScreenTools.defaultFontPixelHeight * 0.28
            enabled: _escCalibration && _escCalibration.rawValue === 3
            opacity: enabled ? 1.0 : 0.55

            Repeater {
                model: pageRoot._calSteps
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    radius: ScreenTools.defaultBorderRadius
                    color: qgcPal.window
                    border.width: 1
                    border.color: qgcPal.groupBorder
                    implicitHeight: stepRow.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.35

                    RowLayout {
                        id: stepRow
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.6
                        spacing: ScreenTools.defaultFontPixelWidth * 0.7

                        Rectangle {
                            width: ScreenTools.defaultFontPixelHeight * 1.35
                            height: width
                            radius: width / 2
                            color: qgcPal.buttonHighlight
                            QGCLabel {
                                anchors.centerIn: parent
                                text: String(index + 1)
                                color: qgcPal.buttonHighlightText
                                font.bold: true
                            }
                        }
                        QGCLabel {
                            Layout.fillWidth: true
                            text: modelData
                            wrapMode: Text.WordWrap
                        }
                    }
                }
            }
        }
    }
}
```

If `/res/warning.svg` is missing, grep for an existing warning icon under `resources/` / `src/UI/` and use that; if none, keep text-only banner (still pale red).

- [ ] **Step 3: Visual acceptance vs lock PNG**

Checklist (all required):

1. Warning is a **banner block**, not bare red label.  
2. CTA is **full-width blue** under the banner.  
3. Steps are **8 separate rounded rows** with **blue circular numbers**, not a bullet paragraph.  
4. Only the steps area scrolls if needed; config + banner + CTA stay visible on UniRC 10 Pro.  
5. `adb -s <serial> exec-out screencap -p > ~/Desktop/qgc-esc-after.png` and visually diff against lock PNG.

- [ ] **Step 4: Build**

Run: `just build`. Expected: success.

- [ ] **Step 5: Commit**

```bash
git add src/AutoPilotPlugins/APM/APMESCComponent.qml
git commit -m "$(cat <<'EOF'
feat(APM): ESC calibration panel with numbered step cards

EOF
)"
```

---

### Task 4: Narrow fallback + lint

**Files:**
- Modify: `src/AutoPilotPlugins/APM/APMESCComponent.qml`

**Interfaces:**
- Consumes: `_split` from Task 1
- Produces: stacked panels when `!_split` without duplicating Fact logic

- [ ] **Step 1: Extract panel bodies into `Component`s**

```qml
Component { id: configPanelBody; /* ColumnLayout from Task 2 */ }
Component { id: calPanelBody; /* ColumnLayout from Task 3 */ }
```

Load with `Loader { sourceComponent: configPanelBody; anchors.fill: parent }` inside both split `Rectangle`s and stacked fallback `Rectangle`s so there is a single visual implementation.

- [ ] **Step 2: Lint**

Run: `just lint` (or `pre-commit run qmllint --files src/AutoPilotPlugins/APM/APMESCComponent.qml`).  
Expected: pass / no new qmllint errors on this file.

- [ ] **Step 3: Final visual check on UniRC 10 Pro**

```bash
source ./env-qgc.sh
adb devices -l
adb -s <device> exec-out screencap -p > ~/Desktop/qgc-esc-unirc10.png
```

Compare to `docs/superpowers/specs/assets/esc-config-ui-preview-unirc10-pro.png`. Fix spacing/colors if CTA/steps/PWM pair diverge.

- [ ] **Step 4: Commit**

```bash
git add src/AutoPilotPlugins/APM/APMESCComponent.qml
git commit -m "$(cat <<'EOF'
fix(APM): ESC Layout A narrow fallback and visual polish

EOF
)"
```

---

## Spec coverage (self-review)

| Spec requirement | Task |
|------------------|------|
| Layout A ~55/45 split | 1 |
| One viewport; steps inner-scroll OK | 3 |
| Numbered ①–⑧ step cards | 3 |
| PWM Min\|Max side-by-side | 2 |
| Reboot muted chip | 2 |
| Warning banner + blue CTA | 3 |
| DShot only when DShot | 2 |
| Fact `= 3` calibration | 3 |
| UniRC 10 Pro temp verify | 3–4 |
| No sidebar restyle | all |
| Visual lock PNG fidelity | 2–4 |

## Placeholder scan

None intentionally left; warning icon path may need one-line grep adjustment in Task 3 Step 2.

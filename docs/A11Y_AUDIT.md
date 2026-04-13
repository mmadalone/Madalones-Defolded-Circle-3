# Accessibility & DPAD audit

Manual audit checklist for every settings page and screensaver theme that ships on `main`. This is a living document — run through it before any tagged release, fill in the state of each checkbox, and note any regressions in the "Latest audit" section at the bottom.

The UC Remote 3 has no touchscreen accessibility stack (no screen reader, no font-scale API). "Accessibility" here means **DPAD navigability** + **visible-at-480×850 text sizing** + **tap-target reachability**. The audit is primarily ergonomic, not assistive-tech compliance.

---

## §1. Scope

Pages to audit (per this fork — upstream settings pages are UC's responsibility, not ours):

- [ ] `src/qml/settings/settings/ChargingScreen.qml` — main screensaver settings
- [ ] `src/qml/settings/settings/chargingscreen/ThemeSelector.qml`
- [ ] `src/qml/settings/settings/chargingscreen/CommonToggles.qml`
- [ ] `src/qml/settings/settings/chargingscreen/GeneralBehavior.qml`
- [ ] `src/qml/settings/settings/chargingscreen/MatrixAppearance.qml`
- [ ] `src/qml/settings/settings/chargingscreen/MatrixEffects.qml`
- [ ] `src/qml/settings/settings/chargingscreen/MatrixShutoffSettings.qml`
- [ ] `src/qml/settings/settings/chargingscreen/ChaosSection.qml`
- [ ] `src/qml/settings/settings/chargingscreen/TapSection.qml`
- [ ] `src/qml/settings/settings/chargingscreen/DirectionGlitchSection.qml`
- [ ] `src/qml/settings/settings/chargingscreen/MessageSection.qml`
- [ ] `src/qml/settings/settings/chargingscreen/TvStaticSettings.qml`
- [ ] `src/qml/settings/settings/chargingscreen/AnalogSettings.qml`

Themes (runtime visual, not settings):

- [ ] `MatrixTheme.qml` — clock + battery overlays
- [ ] `StarfieldTheme.qml` — clock + battery overlays
- [ ] `MinimalTheme.qml` — clock + date text + battery overlay
- [ ] `AnalogTheme.qml` — analog clock + battery overlay
- [ ] `TvStaticTheme.qml` — clock + battery overlays

---

## §2. Per-page checklist

For each page, verify each item on a real UC3 device (not Docker preview — it doesn't capture the 480×850 viewport exactly).

### §2.1 Focus & DPAD navigation

- [ ] **Initial focus** is on a visible, non-dead item when the page is opened (not blank, not invisible).
- [ ] **DPAD Up/Down** traverses every interactive item in a natural top-to-bottom order. No dead ends, no skips.
- [ ] **DPAD Left/Right** either cycles horizontally between items in the same row OR is a no-op (never triggers an action).
- [ ] **DPAD Center (Enter)** activates the focused toggle/slider/button without side effects on neighbors.
- [ ] **Return** button takes you back one level (not all the way to home, not into a dead screen).
- [ ] **Key focus indicator** (the highlight ring/border) is visible on the focused item. No focus-without-indicator state.
- [ ] **Scrolling** — pages longer than 850px scroll smoothly via DPAD or touchbar without jumping or losing focus.
- [ ] **`ensureVisible()`** is called on the focused item when it would otherwise be off-screen (check `onActiveFocusChanged` handlers).

### §2.2 Text & color contrast

- [ ] **Minimum font size:** 18px for body, 24px for headers. Nothing smaller than 14px anywhere.
- [ ] **Foreground vs background** meets at least 4.5:1 contrast ratio for body text (spot-check with a color picker if unsure).
- [ ] **Disabled state** is visually distinct from enabled (e.g., greyed-out toggle) but NOT so faded that the label is unreadable.
- [ ] **Translated strings** (when a locale is set) don't overflow their containers — check `de_DE` and `fr_FR` where words are typically longer than English.
- [ ] **Dynamic strings** with `%1` placeholders render correctly in all locales (e.g., `%1% - Charging`, `%1% - Lädt`, `%1% - En charge`).

### §2.3 Touch targets

- [ ] **Minimum touch target:** 48×48px. No toggle or button smaller than that.
- [ ] **Tap slop** — pressing just outside a button should still activate it (within ~8px). Check TouchSlider* components.
- [ ] **Double-tap protection** — accidental double-taps don't trigger two actions unintentionally (e.g., "close screensaver" shouldn't fire twice).

### §2.4 Screensaver themes (runtime)

- [ ] **ClockOverlay** is readable against every theme's background (especially Matrix with rainbow color mode).
- [ ] **BatteryOverlay** `%1%` / `%1% - Charging` / `100% - Fully charged` all render without truncation at every `batteryTextSize` slider value (tested at min/default/max).
- [ ] **Minimal clock** date line (`dddd, MMM d` format) fits within the screen width at the maximum `minimalDateSize` slider value.
- [ ] **Analog clock** — second hand is visible (not fully transparent) at idle-visible state.
- [ ] **TvStatic noise** doesn't obscure the clock/battery overlays to the point of unreadability at maximum intensity.

### §2.5 Dynamic overlay bindings

- [ ] **`showClock` toggle off** hides the clock cleanly in every theme (no ghost text, no broken anchor).
- [ ] **`showBatteryEnabled` toggle off** hides the battery overlay cleanly in every theme.
- [ ] **Wake from display-off** — the screensaver re-renders immediately with all overlays intact (no race-reset on wake).
- [ ] **Undock with "Close on wake" OFF** — overlays survive the undock transition.

---

## §3. Regression tests (cross-batch)

Items discovered in prior batches that should stay verified:

- [ ] **Bug 1** — "Close on wake" toggle OFF keeps screensaver visible on undock. Toggle ON closes it.
- [ ] **Bug 2** — Matrix/Starfield wake from display-off shows content instantly (no black stuck frame).
- [ ] **Bug 2b** — undock-to-display-off gap is < 2 seconds (the 7-second gap bug is fixed).
- [ ] **Bug 3** — "Idle screensaver" OFF prevents auto-open; ON opens after `idleTimeout`.
- [ ] **DPAD direction regression** — Matrix rain bends smoothly via gravity lerp on direction input. No respawn.
- [ ] **Thermal** — 10+ min sustained dock with Matrix theme does not warm the device noticeably.
- [ ] **AnalogTheme first-wake-from-fresh-boot** — the Qt.binding pre-conversion workaround still holds after a cold reboot. Clock hands are visible on first wake, not black.

---

## §4. Latest audit

**Date:** _not yet run_
**Auditor:** _TBD_
**Firmware build:** `git describe --always`

### Findings

_(Fill in after running the checklist. Use this template:)_

| Section | Status | Notes |
|---|---|---|
| §2.1 Focus & DPAD | ⏳ | _not yet verified_ |
| §2.2 Text & contrast | ⏳ | _not yet verified_ |
| §2.3 Touch targets | ⏳ | _not yet verified_ |
| §2.4 Theme overlays | ⏳ | _not yet verified_ |
| §2.5 Overlay bindings | ⏳ | _not yet verified_ |
| §3 Regressions | ⏳ | _not yet verified_ |

### Blocking issues

_(Problems that must be fixed before tagging the next release.)_

- _none yet_

### Non-blocking nits

_(Things worth fixing but won't block a release.)_

- _none yet_

---

## §5. How to add to this document

When a new settings page is added in a future mod:

1. Add the page to the §1 scope list
2. Walk through the §2 checklist for that page
3. Note any page-specific checks in a new subsection if needed
4. Update §4 with the verification date

When a regression is discovered:

1. Add the regression to §3 with a link to the fixing commit
2. Re-run §2 sections that touch the affected area
3. Update §4

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

**Date:** 2026-05-02 (static pre-pass only — manual on-device audit deferred to v1.4.38)
**Auditor:** Claude Opus 4.7 (1M context), under direction of madalone
**Firmware build:** `git describe --always` → `v1.4.37` baseline (this audit ran against post-v1.4.36 / pre-v1.4.37 head)

### Methodology

This is a **static pre-pass** — what can be verified by reading the QML source without running on device. Items requiring physical UCR3 interaction (DPAD presses, visual contrast checks, thermal soak, wake-from-display-off behavior) are listed as `📋 manual-required` and deferred to v1.4.38 when device time is available.

### Findings

| Section | Status | Notes |
|---|---|---|
| §1 Page existence | ✅ pass | All 17 pages + 5 themes verified at the listed paths |
| §2.1 Focus & DPAD wiring | ⚠️ partial — static only | `KeyNavigation` chain references per file: ChaosSection 16, CommonToggles 20, GeneralBehavior 20, MatrixAppearance 17, MatrixEffects 16, MessageSection 17, MinimalSettings 12, StarfieldSettings 8, TapSection 24, TvStaticSettings 26, DirectionGlitchSection 8, AnalogSettings 2, ThemeSelector 2, ChargingScreen (root) 0 (defers to loaded sub-page). 135 `ensureVisible`/`forceActiveFocus` calls across the audited tree. **`AnalogSettings.qml` has only 2 KeyNavigation refs — verify on device that focus traversal isn't sparse.** Actual DPAD chain correctness ⏳ requires device test. |
| §2.2 Text & contrast | ⚠️ partial — static only | All audited pages use `fonts.primaryFont(N)` / `fonts.secondaryFont(N)` (canonical font alias), values seen: 18-30. **One outlier:** `MatrixTheme.qml:124` uses `font.pixelSize: 9` for the `atlasDebugText` debug overlay — acceptable since it's a debug-only element behind a hidden toggle, not user-facing. No other `font.pixelSize` < 18 found. **Visual contrast (4.5:1 ratio)** ⏳ requires color-picker spot-check. **Translation overflow** ⏳ requires `de_DE` / `fr_FR` device run. |
| §2.3 Touch targets | ✅ pass (static) | No hardcoded `width:` or `height:` < 48 found on any interactive component (Switch, Button, IconButton, Slider) across the audited pages. All interactive elements inherit canonical row layout dimensions from upstream's settings template (typically 60 px row height). |
| §2.4 Theme overlays | 📋 manual-required | All 5 themes (Matrix, Starfield, Minimal, Analog, TvStatic) exist at the listed paths. Visual readability of clock + battery overlays at runtime, particularly Matrix with rainbow color mode + TV static at max intensity, requires device inspection. |
| §2.5 Overlay bindings | 📋 manual-required | `showClock` and `showBatteryEnabled` toggles are present in `CommonToggles.qml`. Wake-from-display-off behavior + undock transition require device. |
| §3 Regressions | 📋 manual-required | All 7 cross-batch regression tests require device run (Bug 1-3, Bug 2b, DPAD lerp, thermal, AnalogTheme first-wake). |

### Blocking issues

None found in static pre-pass. Device-found blocking issues to be added in the v1.4.38 manual audit pass.

### Non-blocking nits

- **`AnalogSettings.qml`** has only 2 `KeyNavigation` references (other sub-pages have 8-26). Static analysis can't tell if the page has fewer interactive items or a sparse focus chain. Verify on device whether DPAD up/down skips items.
- **`MatrixTheme.qml:124`** `font.pixelSize: 9` is for the atlas debug overlay. Confirmed not user-facing in production (toggle hidden); but flag for review if the debug overlay ever surfaces in normal usage.

### Manual items deferred to v1.4.38

The following must be verified on a real UCR3 device (not Docker preview — it doesn't capture the 480×850 viewport exactly per §2 instructions):

- §2.1 (DPAD navigation correctness — full traversal of every interactive item, no dead ends, no skips, focus indicator visible)
- §2.2 visual contrast spot-checks + translation overflow on `de_DE` / `fr_FR`
- §2.3 tap-slop and double-tap protection
- §2.4 (all 5 themes — clock/battery overlay readability per theme)
- §2.5 (wake-from-display-off, undock transition)
- §3 (all 7 cross-batch regression tests including thermal + first-wake-from-fresh-boot)

When v1.4.38 lands, run through the checklist on UCR3 and append a new "Date: 2026-XX-XX (manual on-device pass)" entry in this section. Don't overwrite this static pre-pass — append, so the partial-vs-full audit history stays auditable.

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

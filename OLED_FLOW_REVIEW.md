# OLED UI — State Machine

## State Machine (XState style)

```
machine {
  initial: MENU

  // BACK always pops the state history stack.
  // Each forward transition pushes current state onto the stack.
  // history: []  (grows on forward nav, shrinks on BACK)

  MENU {
    on {
      UP              → MENU          [move cursor up, wrap]
      DOWN            → MENU          [move cursor down, wrap]
      BACK            → MENU          [pop submenu; noop at root]
      SEL / submenu   → MENU          [push submenu]
      SEL / sniffer   → SCANNING      [push MENU; call StartScan]
      SEL / attack, needs_aps, aps_available
                      → SELECT_APS    [push MENU; store pending_action]
      SEL / attack, needs_aps, no_aps
                      → MENU          [show warning, stay; nothing pushed]
      SEL / attack, no_aps_needed
                      → ATTACKING     [push MENU; call StartScan]
      SEL / special   → MENU          [inline handler, redraw; nothing pushed]
    }
  }

  SCANNING {
    on {
      BACK            → SCAN_STOPPED  [call StopScan; replace top of stack]
      // BACK does not pop to MENU — it transitions to SCAN_STOPPED
      // SCAN_STOPPED's BACK will pop to MENU
    }
  }

  ATTACKING {
    on {
      BACK            → history.pop() [call StopScan]
      // pops back to wherever ATTACKING was pushed from:
      //   MENU → ATTACKING → BACK → MENU
      //   ATTACK_PICKER → ATTACKING → BACK → ATTACK_PICKER
    }
  }

  SCAN_STOPPED {
    on {
      SEL / aps > 0   → SELECT_APS   [push SCAN_STOPPED]
      BACK            → history.pop() [→ MENU]
    }
  }

  SELECT_APS {
    on {
      UP              → SELECT_APS    [move cursor up, wrap]
      DOWN            → SELECT_APS    [move cursor down, wrap]
      SEL             → SELECT_APS    [toggle AP checkbox; "Select" button]
      BACK / none_selected
                      → SELECT_APS    [show warning 1.5s, stay]
      BACK / has_selection
                      → history.pop() [→ SCAN_STOPPED or MENU depending on who pushed]
                                      [if popped to SCAN_STOPPED → also push ATTACK_PICKER]
                                      [if popped to MENU → call StartScan with pending_action]
    }
  }

  ATTACK_PICKER {
    on {
      UP              → ATTACK_PICKER [move cursor up, wrap]
      DOWN            → ATTACK_PICKER [move cursor down, wrap]
      SEL             → ATTACKING     [push ATTACK_PICKER; call StartScan]
      BACK            → history.pop() [→ SELECT_APS]
    }
  }
}
```

---

## Transition Graph

```
                        ┌──────┐
                        │ MENU │◄─────────────────────────────────────┐
                        └──┬───┘                                       │
                           │                                           │ BACK
          ┌────────────────┼──────────────────┐                       │
          │ SEL: sniffer   │ SEL: attack       │ SEL: attack           │
          │                │ (no AP needed)    │ (needs APs)           │
          ▼                ▼                   ▼                       │
      SCANNING         ATTACKING ─────────►SELECT_APS                 │
          │            ▲    │               │      │                   │
          │ BACK        │    │ BACK          │BACK  │ BACK             │
          ▼             │    ▼  (history)    │ 0sel │ ≥1sel            │
     SCAN_STOPPED       │   MENU            │(warn)▼                  │
          │             │                   │  ATTACK_PICKER          │
          │ SEL(APs>0)  │                   │       │                 │
          ▼             │                   │  BACK │ SEL             │
      SELECT_APS        │                   │       ▼                 │
          │             │                   └──►ATTACKING ────────────┘
          │ BACK ≥1sel  │
          ▼             │
     ATTACK_PICKER      │
          │             │
          │ SEL          │
          └─────────────┘
            (→ ATTACKING)
```

---

## Screen Renders (128×64 OLED, 21 chars wide)

### MENU
```
┌─────────────────────┐
│        WiFi         │  ← banner (inverted)
├─────────────────────┤
│ > Sniffers          │  ← selected row (cursor >)
│   Attacks           │
│   Add SSID          │
│   Join WiFi         │
├─────────────────────┤
│                Back │  ← bottom bar (left empty at root)
└─────────────────────┘
```

### SCANNING
```
┌─────────────────────┐
│    Beacon Sniff     │  ← banner
├─────────────────────┤
│ -78 ESSID1          │  ← live results scroll in
│ -82 ESSID2          │
│ -90 ESSID3          │
│ -74 ESSID4          │
├─────────────────────┤
│                Stop │
└─────────────────────┘
```

### ATTACKING
```
┌─────────────────────┐
│      Rick Roll      │  ← banner
├─────────────────────┤
│                     │
│  Pkts/sec: 42       │  ← overwritten in-place each second
│                     │
│                     │
├─────────────────────┤
│                Stop │
└─────────────────────┘
```

### SCAN_STOPPED
```
┌─────────────────────┐
│    Scan Stopped     │  ← banner
├─────────────────────┤
│                     │
│  13 APs found.      │
│  SEL: Select APs    │  ← omitted if 0 APs
│                     │
├─────────────────────┤
│ Select         Back │  ← left omitted if 0 APs
└─────────────────────┘
```

### SELECT_APS
```
┌─────────────────────┐
│     Select APs      │  ← banner
├─────────────────────┤
│ >[*] Excitel_Neha   │  ← cursor + selected
│                     │  ← padding line
│  [ ] Airtel_Manas   │
│                     │  ← padding line
├─────────────────────┤
│ Select           Go │  ← SEL=toggle checkbox, BACK=confirm/go
└─────────────────────┘
```

### ATTACK_PICKER
```
┌─────────────────────┐
│    Attack With?     │  ← banner
├─────────────────────┤
│ > Deauth            │
│   Deauth Targeted   │
│   Bad Msg           │
│   Sleep             │
├─────────────────────┤
│                Back │  ← BACK pops to SELECT_APS
└─────────────────────┘
```

---

## Context (data carried across states)

```
state            UiState    current state
history[]        UiState[]  back stack (push on forward nav, pop on BACK)
pending_action   uint16_t   attack action stored when entering SELECT_APS from MENU
ap_sel_cursor    int8_t     cursor position in SELECT_APS
ap_sel_offset    int8_t     scroll offset in SELECT_APS
picker_cursor    int8_t     cursor in ATTACK_PICKER
picker_offset    int8_t     scroll offset in ATTACK_PICKER
menu_stack       []         depth, items, cursor, offset per level (internal to MENU)
```

---

## Entry Actions (run once on state enter)

| Entering state  | Entry action                                                        |
|-----------------|---------------------------------------------------------------------|
| MENU            | full redraw from menu_stack                                         |
| SCANNING        | drain display_buffer; draw banner + "Scanning..."; draw bottom bar  |
| ATTACKING       | drain display_buffer; draw banner + "Attacking..."; draw bottom bar |
| SCAN_STOPPED    | draw banner + AP count; draw bottom bar                             |
| SELECT_APS      | reset ap_sel_cursor/offset; full redraw                             |
| ATTACK_PICKER   | reset picker_cursor/offset; full redraw                             |

---

## Decisions

1. **Packet rate in ATTACKING** — overwrite in-place. A fixed y-coordinate for line 1 of the scroll area. Each second `displayTransmitRate()` clears that line and rewrites it. No scrolling.

2. **`access_points` on SCANNING enter** — apply a TTL of 1 minute. If the list is older than 60s, clear it before starting the new scan. If younger, keep it (user may be doing a quick re-scan).

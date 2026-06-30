# WDG Upload UI Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a device UI flow that lets users pick a wardrive CSV from SD and upload it to WDG Wars with visible success/failure feedback.

**Architecture:** Reuse the existing SD file menu pattern in `MenuFunctions` and add a separate upload mode so firmware update/delete behavior stays unchanged. Implement `WiFiScan::wdgwarsUpload()` with a local `WiFiClientSecure` instance, loading the WDG API key from settings and streaming the selected CSV as multipart form data.

**Tech Stack:** Arduino C++, ESP32 `WiFiClientSecure`, SD filesystem, existing `MenuFunctions` and `WiFiScan` classes, GitHub Actions firmware matrix.

---

### Task 1: Add WDG Upload Source-Structure Check

**Files:**
- Create local verification command only: no committed test file.

- [ ] **Step 1: Verify the flow is currently absent**

Run:

```powershell
rg -n "WDG Upload|wdgwarsUpload|WDG_KEY_NAME|upload-csv|WiFiClientSecure" esp32_marauder
```

Expected: no matches on the PR branch before implementation.

### Task 2: Implement WDG Upload Support

**Files:**
- Modify: `esp32_marauder/settings.h`
- Modify: `esp32_marauder/WiFiScan.h`
- Modify: `esp32_marauder/WiFiScan.cpp`

- [ ] **Step 1: Add settings and secure client include**

Add `WDG_KEY_NAME` and include `WiFiClientSecure.h` where `WiFiScan` can compile the upload method.

- [ ] **Step 2: Add `WiFiScan::wdgwarsUpload(String filePath)`**

Implement SD existence/open checks, API key validation, multipart upload to `wdgwars.pl/api/v2/upload-csv`, progress display, response parsing, and success/failure screen feedback.

- [ ] **Step 3: Compile-check by source scan**

Run:

```powershell
rg -n "bool WiFiScan::wdgwarsUpload|WiFiClientSecure client|POST /api/v2/upload-csv|WDG Upload OK|WDG Upload Failed" esp32_marauder/WiFiScan.*
```

Expected: all key implementation points are present.

### Task 3: Add SD Picker UI

**Files:**
- Modify: `esp32_marauder/MenuFunctions.h`
- Modify: `esp32_marauder/MenuFunctions.cpp`

- [ ] **Step 1: Add upload mode to SD file menu builder**

Extend `setupSDFileList()` and `buildSDFileMenu()` with an upload mode that lists `.csv` files.

- [ ] **Step 2: Add a `WDG Upload` device menu entry**

When SD is supported, add a menu item that opens the CSV picker. Selecting a CSV calls `wifi_scan_obj.wdgwarsUpload("/" + filename)` and then returns to the file list.

- [ ] **Step 3: Verify source wiring**

Run:

```powershell
rg -n "WDG Upload|buildSDFileMenu\\(false, true\\)|wdgwarsUpload" esp32_marauder/MenuFunctions.*
```

Expected: the device menu entry, upload menu build call, and selected-file upload call are present.

### Task 4: Build On Fork

**Files:**
- No additional source changes unless build failures require fixes.

- [ ] **Step 1: Push branch to fork**

Run:

```powershell
git push origin optimize-build-workflow
```

- [ ] **Step 2: Trigger or observe GitHub Actions**

Run:

```powershell
gh workflow run build_parallel.yml --repo KynexXero/ESP32Marauder --ref optimize-build-workflow
gh run list --repo KynexXero/ESP32Marauder --branch optimize-build-workflow --limit 3
```

- [ ] **Step 3: Confirm matrix status**

Run:

```powershell
gh run watch <run-id> --repo KynexXero/ESP32Marauder --exit-status
```

Expected: build completes or reports actionable compiler errors for follow-up fixes.

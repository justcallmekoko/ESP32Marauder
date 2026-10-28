[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$installer = Join-Path $PSScriptRoot "install-board-profile.ps1"
$temporaryRoot = Join-Path ([IO.Path]::GetTempPath()) ("waveshare-profile-test-" + [guid]::NewGuid().ToString("N"))

function Assert-True {
    param(
        [Parameter(Mandatory)][bool]$Condition,
        [Parameter(Mandatory)][string]$Message
    )

    if (-not $Condition) {
        throw "Assertion failed: $Message"
    }
}

function New-TestCore {
    param([Parameter(Mandatory)][string]$Root)

    $core = Join-Path $Root "packages\esp32\hardware\esp32\3.3.5"
    New-Item -ItemType Directory -Path $core -Force | Out-Null
}

try {
    $validDataRoot = Join-Path $temporaryRoot "valid\Arduino15"
    $validSettings = Join-Path $temporaryRoot "valid\.arduinoIDE\pluggable-monitor-settings.json"
    New-TestCore -Root $validDataRoot
    New-Item -ItemType Directory -Path (Split-Path -Parent $validSettings) -Force | Out-Null
    @'
{
  "esp32:esp32:waveshare_c5_marauder-COM41-serial": {
    "dtr": { "selectedValue": "on" },
    "rts": { "selectedValue": "on" },
    "baudrate": { "selectedValue": "115200" }
  },
  "esp32:esp32:waveshare_c5_marauder-COM57-serial": {
    "dtr": { "selectedValue": "on" },
    "rts": { "selectedValue": "off" }
  },
  "esp32:esp32:unrelated-COM88-serial": {
    "dtr": { "selectedValue": "on" },
    "rts": { "selectedValue": "on" }
  },
  "esp32:esp32:waveshare_c5_marauder-COM99-serial-extra": {
    "dtr": { "selectedValue": "on" },
    "rts": { "selectedValue": "on" }
  }
}
'@ | Set-Content -LiteralPath $validSettings -Encoding UTF8

    & $installer -ArduinoDataRoot $validDataRoot -ArduinoIdeMonitorSettingsPath $validSettings
    $migrated = [IO.File]::ReadAllText($validSettings) | ConvertFrom-Json
    $waveshareCom41 = $migrated.PSObject.Properties["esp32:esp32:waveshare_c5_marauder-COM41-serial"].Value
    $waveshareCom57 = $migrated.PSObject.Properties["esp32:esp32:waveshare_c5_marauder-COM57-serial"].Value
    $unrelated = $migrated.PSObject.Properties["esp32:esp32:unrelated-COM88-serial"].Value
    $nearMatch = $migrated.PSObject.Properties["esp32:esp32:waveshare_c5_marauder-COM99-serial-extra"].Value
    Assert-True ($waveshareCom41.dtr.selectedValue -eq "off") "Waveshare COM41 DTR was not disabled"
    Assert-True ($waveshareCom41.rts.selectedValue -eq "off") "Waveshare COM41 RTS was not disabled"
    Assert-True ($waveshareCom41.baudrate.selectedValue -eq "115200") "Waveshare baud rate changed"
    Assert-True ($waveshareCom57.dtr.selectedValue -eq "off") "Waveshare COM57 DTR was not disabled"
    Assert-True ($waveshareCom57.rts.selectedValue -eq "off") "An existing Waveshare RTS off value changed"
    Assert-True ($unrelated.dtr.selectedValue -eq "on") "Unrelated DTR setting changed"
    Assert-True ($unrelated.rts.selectedValue -eq "on") "Unrelated RTS setting changed"
    Assert-True ($nearMatch.dtr.selectedValue -eq "on") "A monitor key without the exact serial suffix changed"
    Assert-True ($nearMatch.rts.selectedValue -eq "on") "A monitor key without the exact serial suffix changed"
    $validBackups = @(Get-ChildItem -LiteralPath (Split-Path -Parent $validSettings) -Filter "pluggable-monitor-settings.json.backup-*")
    Assert-True ($validBackups.Count -eq 1) "Expected one backup before monitor migration"
    $backupSettings = [IO.File]::ReadAllText($validBackups[0].FullName) | ConvertFrom-Json
    $backupCom41 = $backupSettings.PSObject.Properties["esp32:esp32:waveshare_c5_marauder-COM41-serial"].Value
    Assert-True ($backupCom41.dtr.selectedValue -eq "on") "Monitor backup does not contain the pre-migration value"

    $migratedHash = (Get-FileHash -LiteralPath $validSettings -Algorithm SHA256).Hash
    & $installer -ArduinoDataRoot $validDataRoot -ArduinoIdeMonitorSettingsPath $validSettings
    Assert-True ((Get-FileHash -LiteralPath $validSettings -Algorithm SHA256).Hash -eq $migratedHash) "No-op migration rewrote the settings file"
    $validBackups = @(Get-ChildItem -LiteralPath (Split-Path -Parent $validSettings) -Filter "pluggable-monitor-settings.json.backup-*")
    Assert-True ($validBackups.Count -eq 1) "No-op migration created another monitor backup"

    $malformedDataRoot = Join-Path $temporaryRoot "malformed\Arduino15"
    $malformedSettings = Join-Path $temporaryRoot "malformed\.arduinoIDE\pluggable-monitor-settings.json"
    New-TestCore -Root $malformedDataRoot
    New-Item -ItemType Directory -Path (Split-Path -Parent $malformedSettings) -Force | Out-Null
    [IO.File]::WriteAllText($malformedSettings, "{ malformed", [Text.UTF8Encoding]::new($false))
    $malformedBefore = (Get-FileHash -LiteralPath $malformedSettings -Algorithm SHA256).Hash
    $migrationWarnings = @()
    & $installer -ArduinoDataRoot $malformedDataRoot -ArduinoIdeMonitorSettingsPath $malformedSettings -WarningVariable migrationWarnings
    $malformedBoards = Join-Path $malformedDataRoot "packages\esp32\hardware\esp32\3.3.5\boards.local.txt"
    Assert-True (Test-Path -LiteralPath $malformedBoards -PathType Leaf) "Malformed monitor JSON prevented board installation"
    Assert-True ((Get-FileHash -LiteralPath $malformedSettings -Algorithm SHA256).Hash -eq $malformedBefore) "Malformed monitor JSON was rewritten"
    Assert-True (@(Get-ChildItem -LiteralPath (Split-Path -Parent $malformedSettings) -Filter "pluggable-monitor-settings.json.backup-*").Count -eq 0) "Malformed monitor JSON was backed up despite no migration"
    Assert-True ($migrationWarnings.Count -gt 0) "Malformed monitor JSON did not emit a warning"

    $missingDataRoot = Join-Path $temporaryRoot "missing\Arduino15"
    $missingSettings = Join-Path $temporaryRoot "missing\.arduinoIDE\pluggable-monitor-settings.json"
    New-TestCore -Root $missingDataRoot
    & $installer -ArduinoDataRoot $missingDataRoot -ArduinoIdeMonitorSettingsPath $missingSettings
    $missingBoards = Join-Path $missingDataRoot "packages\esp32\hardware\esp32\3.3.5\boards.local.txt"
    Assert-True (Test-Path -LiteralPath $missingBoards -PathType Leaf) "Missing monitor JSON prevented board installation"
    Assert-True (-not (Test-Path -LiteralPath $missingSettings)) "Missing monitor JSON was unexpectedly created"

    Write-Host "All isolated board-profile installer tests passed." -ForegroundColor Green
} finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

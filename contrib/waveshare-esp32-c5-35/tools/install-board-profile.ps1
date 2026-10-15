[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$CoreVersion = "3.3.5",
    [string]$ArduinoDataRoot = (Join-Path $env:LOCALAPPDATA "Arduino15"),
    [string]$ArduinoIdeMonitorSettingsPath = (Join-Path $env:USERPROFILE ".arduinoIDE\pluggable-monitor-settings.json")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$packageRoot = Split-Path -Parent $PSScriptRoot
$fragmentPath = Join-Path $packageRoot "profile\boards.local.fragment.txt"
$coreRoot = Join-Path $ArduinoDataRoot "packages\esp32\hardware\esp32\$CoreVersion"
$boardsFile = Join-Path $coreRoot "boards.local.txt"
$beginMarker = "# BEGIN WAVESHARE ESP32-C5 MARAUDER"
$endMarker = "# END WAVESHARE ESP32-C5 MARAUDER"
$monitorKeyPrefix = "esp32:esp32:waveshare_c5_marauder-"
$monitorKeySuffix = "-serial"

function Update-WaveshareMonitorSettings {
    param([Parameter(Mandatory)][string]$Path)

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return
    }

    try {
        $originalJson = [IO.File]::ReadAllText($Path)
        $settings = $originalJson | ConvertFrom-Json -ErrorAction Stop
        if ($null -eq $settings) {
            throw "The settings file contains no JSON object."
        }

        $changed = $false
        foreach ($entry in $settings.PSObject.Properties) {
            $isWaveshareSerialEntry =
                $entry.Name.StartsWith($monitorKeyPrefix, [StringComparison]::Ordinal) -and
                $entry.Name.EndsWith($monitorKeySuffix, [StringComparison]::Ordinal) -and
                $entry.Name.Length -gt ($monitorKeyPrefix.Length + $monitorKeySuffix.Length)
            if (-not $isWaveshareSerialEntry -or $null -eq $entry.Value) {
                continue
            }

            foreach ($lineName in @("dtr", "rts")) {
                $line = $entry.Value.PSObject.Properties[$lineName]
                if ($null -eq $line -or $null -eq $line.Value) {
                    continue
                }

                $selectedValue = $line.Value.PSObject.Properties["selectedValue"]
                if ($null -ne $selectedValue -and $selectedValue.Value -cne "off") {
                    $selectedValue.Value = "off"
                    $changed = $true
                }
            }
        }

        if (-not $changed) {
            return
        }

        $updatedJson = $settings | ConvertTo-Json -Depth 100
        $timestamp = Get-Date -Format "yyyyMMdd-HHmmss-fff"
        Copy-Item -LiteralPath $Path -Destination "$Path.backup-$timestamp"
        [IO.File]::WriteAllText($Path, "$updatedJson`r`n", [Text.UTF8Encoding]::new($false))
        Write-Host "Disabled cached DTR/RTS for the Waveshare C5 Marauder monitor." -ForegroundColor Green
    } catch {
        Write-Warning "Board profile installed, but Arduino IDE monitor settings were not migrated: $($_.Exception.Message)"
    }
}

if (-not (Test-Path -LiteralPath $coreRoot -PathType Container)) {
    throw "ESP32 Arduino core $CoreVersion was not found at $coreRoot"
}
if (-not (Test-Path -LiteralPath $fragmentPath -PathType Leaf)) {
    throw "Profile fragment not found: $fragmentPath"
}

$fragment = [IO.File]::ReadAllText($fragmentPath).Trim()
if (-not $fragment.StartsWith($beginMarker) -or -not $fragment.EndsWith($endMarker)) {
    throw "The profile fragment does not contain the expected managed markers."
}

$existing = if (Test-Path -LiteralPath $boardsFile) {
    [IO.File]::ReadAllText($boardsFile)
} else {
    ""
}

$beginCount = ([regex]::Matches($existing, [regex]::Escape($beginMarker))).Count
$endCount = ([regex]::Matches($existing, [regex]::Escape($endMarker))).Count
if ($beginCount -ne $endCount -or $beginCount -gt 1) {
    throw "Malformed managed section in $boardsFile (begin=$beginCount, end=$endCount)"
}

$withoutManagedBlock = if ($beginCount -eq 1) {
    [regex]::Replace(
        $existing,
        "(?s)$([regex]::Escape($beginMarker)).*?$([regex]::Escape($endMarker))\s*",
        ""
    ).TrimEnd()
} else {
    $existing.TrimEnd()
}

$updated = if ([string]::IsNullOrWhiteSpace($withoutManagedBlock)) {
    "$fragment`r`n"
} else {
    "$withoutManagedBlock`r`n`r`n$fragment`r`n"
}

if ($PSCmdlet.ShouldProcess($boardsFile, "Install Waveshare C5 Marauder board profile")) {
    if (Test-Path -LiteralPath $boardsFile) {
        $timestamp = Get-Date -Format "yyyyMMdd-HHmmss-fff"
        Copy-Item -LiteralPath $boardsFile -Destination "$boardsFile.backup-$timestamp"
    }
    [IO.File]::WriteAllText($boardsFile, $updated, [Text.UTF8Encoding]::new($false))
    Write-Host "Installed board profile: esp32:esp32:waveshare_c5_marauder" -ForegroundColor Green
    Write-Host "Restart Arduino IDE before selecting the new board."
    Write-Host "The source snapshot and pinned libraries are intentionally not installed by this script."
    Update-WaveshareMonitorSettings -Path $ArduinoIdeMonitorSettingsPath
}

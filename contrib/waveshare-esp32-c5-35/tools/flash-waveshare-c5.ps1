[CmdletBinding()]
param(
    [Parameter(Mandatory)]
    [ValidatePattern('^(?i:COM[0-9]+)$')]
    [string]$Port,

    [string]$Esptool,

    [switch]$FullImage
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$packageRoot = Split-Path -Parent $PSScriptRoot
$firmwareRoot = Join-Path $packageRoot "firmware"

function Resolve-Esptool {
    if ($Esptool) {
        if (Test-Path -LiteralPath $Esptool -PathType Leaf) {
            return (Resolve-Path -LiteralPath $Esptool).Path
        }
        $explicitCommand = Get-Command $Esptool -ErrorAction SilentlyContinue
        if ($explicitCommand) {
            return $explicitCommand.Source
        }
        throw "The requested esptool executable was not found: $Esptool"
    }

    foreach ($name in @("esptool", "esptool.exe")) {
        $command = Get-Command $name -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }
    }

    $arduinoToolRoot = Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\esptool_py"
    if (Test-Path -LiteralPath $arduinoToolRoot -PathType Container) {
        $candidate = Get-ChildItem -LiteralPath $arduinoToolRoot -Directory |
            Sort-Object Name -Descending |
            ForEach-Object { Join-Path $_.FullName "esptool.exe" } |
            Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
            Select-Object -First 1
        if ($candidate) {
            return $candidate
        }
    }

    throw "esptool was not found. Install it or pass -Esptool with its executable path."
}

function Assert-Artifact {
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$ExpectedSha256
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Firmware artifact is missing: $Path"
    }
    $actual = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $ExpectedSha256) {
        throw "Firmware hash mismatch for ${Path}: expected $ExpectedSha256, got $actual"
    }
    Write-Host "  [ok] $(Split-Path -Leaf $Path) $actual"
}

function Invoke-Native {
    param(
        [Parameter(Mandatory)] [string]$Executable,
        [Parameter(Mandatory)] [string[]]$Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code ${LASTEXITCODE}: $Executable"
    }
}

$availablePorts = [IO.Ports.SerialPort]::GetPortNames()
if ($availablePorts -notcontains $Port) {
    throw "Serial port $Port is not present. Available ports: $($availablePorts -join ', ')"
}

$esptoolPath = Resolve-Esptool
$appImage = Join-Path $firmwareRoot "esp32_marauder_waveshare_c5_35_app.bin"
$otaSelector = Join-Path $firmwareRoot "boot_app0.bin"
$mergedImage = Join-Path $firmwareRoot "esp32_marauder_waveshare_c5_35_merged.bin"

Write-Host "Verifying firmware artifacts" -ForegroundColor Cyan
if ($FullImage) {
    Assert-Artifact $mergedImage "b925ad7e7c6bda9072b3b006b58db3d80af1d5929011148a9a9ac2ac29e66cbd"
    Write-Warning "Full-image mode overwrites the complete 8 MiB internal flash. The separate microSD card is not erased."
    $arguments = @(
        "--chip", "esp32c5",
        "--port", $Port,
        "--baud", "115200",
        "--before", "default-reset",
        "--after", "hard-reset",
        "--no-stub",
        "write-flash",
        "0x0", $mergedImage
    )
}
else {
    Assert-Artifact $otaSelector "f94c5d786a7a8fab06ac5d10e33bf37711a6697636dc037559ea19cc410a17f0"
    Assert-Artifact $appImage "64307554007412dd8e4d480e27310024909879ed1697107765d493dd907eaac2"
    $arguments = @(
        "--chip", "esp32c5",
        "--port", $Port,
        "--baud", "115200",
        "--before", "default-reset",
        "--after", "hard-reset",
        "--no-stub",
        "write-flash",
        "--flash-mode", "keep",
        "--flash-freq", "keep",
        "--flash-size", "keep",
        "0xe000", $otaSelector,
        "0x10000", $appImage
    )
}

Write-Host "Flashing $Port with $esptoolPath" -ForegroundColor Cyan
Invoke-Native $esptoolPath $arguments
Write-Host "[PASS] Flash completed and esptool verified the transferred hashes." -ForegroundColor Green

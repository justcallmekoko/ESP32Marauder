[CmdletBinding()]
param(
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path,
    [switch]$PrepareOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$packageRoot = Split-Path -Parent $PSScriptRoot
$localRoot = Join-Path $packageRoot ".build"
$libraryRoot = Join-Path $localRoot "libraries"
$buildRoot = Join-Path $localRoot "output"
$sketchRoot = Join-Path $RepositoryRoot "esp32_marauder"

# GCC can retain absolute source paths in __FILE__ diagnostics inside the
# application image. Map every host-specific build root to a stable public
# prefix so published binaries do not disclose usernames or workstation paths.
$prefixMapSources = @(
    @{ Path = $packageRoot; Target = "package" },
    @{ Path = $RepositoryRoot; Target = "source" },
    @{ Path = (Join-Path $env:LOCALAPPDATA "Arduino15"); Target = "arduino" },
    @{ Path = [Environment]::GetFolderPath("UserProfile"); Target = "workspace" }
)
$prefixMapFlags = [Collections.Generic.List[string]]::new()
# GCC applies the last matching prefix map. Add broad roots first and the most
# specific roots last so diagnostics retain only useful public prefixes.
foreach ($mapping in ($prefixMapSources | Sort-Object { $_.Path.Length })) {
    if ([string]::IsNullOrWhiteSpace($mapping.Path)) {
        continue
    }
    $resolvedSource = [IO.Path]::GetFullPath($mapping.Path).TrimEnd('\', '/')
    $forwardSource = $resolvedSource.Replace('\', '/')
    $sourceForms = @(
        $resolvedSource,
        "/$resolvedSource",
        $forwardSource,
        "/$forwardSource"
    ) | Sort-Object -Unique
    foreach ($sourceForm in $sourceForms) {
        $prefixMapFlags.Add(('"-ffile-prefix-map={0}={1}"' -f $sourceForm, $mapping.Target))
        $prefixMapFlags.Add(('"-fmacro-prefix-map={0}={1}"' -f $sourceForm, $mapping.Target))
    }
}
$compilerPrivacyFlags = $prefixMapFlags -join " "

function Invoke-Native {
    param(
        [Parameter(Mandatory)] [string]$Executable,
        [Parameter(Mandatory)] [string[]]$Arguments
    )

    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $Executable $($Arguments -join ' ')"
    }
}

function Resolve-ArduinoCli {
    $command = Get-Command arduino-cli -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $candidate = Join-Path $env:ProgramFiles "Arduino CLI\arduino-cli.exe"
    if (Test-Path -LiteralPath $candidate -PathType Leaf) {
        return $candidate
    }

    throw "arduino-cli was not found. Install Arduino CLI and esp32:esp32 3.3.5 first."
}

$requiredSourceFiles = @(
    (Join-Path $RepositoryRoot "User_Setup_marauder_waveshare_c5.h"),
    (Join-Path $RepositoryRoot "esp32_marauder\configs.h"),
    (Join-Path $RepositoryRoot "esp32_marauder\esp32_marauder.ino")
)
foreach ($requiredFile in $requiredSourceFiles) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "The Waveshare source snapshot is not applied to this source tree; missing $requiredFile"
    }
}
if ([IO.File]::ReadAllText((Join-Path $RepositoryRoot "esp32_marauder\configs.h")) -notmatch "MARAUDER_WAVESHARE_C5") {
    throw "The selected source tree does not contain the Waveshare C5 target. Apply source/waveshare-c5-tested-snapshot.patch first."
}

$arduinoCli = Resolve-ArduinoCli
$git = (Get-Command git -ErrorAction Stop).Source
New-Item -ItemType Directory -Force -Path $libraryRoot, $buildRoot | Out-Null

$dependencies = @(
    @{ Name = "CustomESP32Ping";           Url = "https://github.com/marian-craciunescu/ESP32Ping.git";       Ref = "1.6";      Commit = "aadac3c08f5e6f5062faf26ae1b17e566e22728c" },
    @{ Name = "CustomAsyncTCP";            Url = "https://github.com/ESP32Async/AsyncTCP.git";                Ref = "v3.4.8";   Commit = "b2e5f4f368b137442f66a9ba7242b8759e6aef59" },
    @{ Name = "CustomMicroNMEA";           Url = "https://github.com/stevemarple/MicroNMEA.git";              Ref = "v2.0.6";   Commit = "ca30fc393cc6dd52d2afafe0733634fe07c5b954" },
    @{ Name = "CustomESPAsyncWebServer";   Url = "https://github.com/ESP32Async/ESPAsyncWebServer.git";       Ref = "v3.8.1";   Commit = "4fc46e0c1b6ed559f7cd0c2548ed163a3e2b3412" },
    @{ Name = "CustomTFT_eSPI";            Url = "https://github.com/H4W9/TFT_eSPI.git";                      Ref = "ESP32-C5"; Commit = "08eb2f73df08ad03cdbc0518cc2ad79118eb9ed0" },
    @{ Name = "CustomXPT2046_Touchscreen"; Url = "https://github.com/PaulStoffregen/XPT2046_Touchscreen.git"; Ref = "v1.4";      Commit = "d57f64c8b5f2bc5b8d10d121550806eeff7b06d9" },
    @{ Name = "Customlv_arduino";          Url = "https://github.com/lvgl/lv_arduino.git";                    Ref = "3.0.0";     Commit = "8651c35e977bddfd25bf9ef59a041f6ab243c7f2" },
    @{ Name = "CustomJPEGDecoder";         Url = "https://github.com/Bodmer/JPEGDecoder.git";                 Ref = "1.8.0";     Commit = "6a0912ba05532eab707bd092bd4fa43477e38f5c" },
    @{ Name = "CustomNimBLE-Arduino";      Url = "https://github.com/h2zero/NimBLE-Arduino.git";              Ref = "2.3.8";     Commit = "494d44196d78660656c0a6ee38fa8776f94d3ba2" },
    @{ Name = "CustomAdafruit_NeoPixel";   Url = "https://github.com/adafruit/Adafruit_NeoPixel.git";         Ref = "1.12.0";    Commit = "15bfa178f2c8e21f732cce7850bc03f8b056291b" },
    @{ Name = "CustomArduinoJson";         Url = "https://github.com/bblanchon/ArduinoJson.git";              Ref = "v6.18.2";   Commit = "ebf58320ca5bd5698a4a3b10f37a0865a11d53a3" },
    @{ Name = "CustomLinkedList";          Url = "https://github.com/ivanseidel/LinkedList.git";              Ref = "v1.3.3";   Commit = "0439a72707924d90859ad2968a22412161783978" },
    @{ Name = "CustomEspSoftwareSerial";   Url = "https://github.com/plerup/espsoftwareserial.git";           Ref = "8.1.0";     Commit = "9e61fa07c3a81b90fa1c2b333f963c0b70b74fe3" },
    @{ Name = "CustomAdafruit_BusIO";      Url = "https://github.com/adafruit/Adafruit_BusIO.git";            Ref = "1.15.0";    Commit = "fc25cd496702d45cf63b86b3b87b870e761a126b" },
    @{ Name = "CustomAdafruit_MAX1704X";   Url = "https://github.com/adafruit/Adafruit_MAX1704X.git";         Ref = "1.0.2";     Commit = "afc9f58db9ab1b1a2d565f4ad031c866e2b72a25" }
)

Write-Host "[1/4] Preparing pinned libraries" -ForegroundColor Cyan
foreach ($dependency in $dependencies) {
    $destination = Join-Path $libraryRoot $dependency.Name
    if (-not (Test-Path -LiteralPath (Join-Path $destination ".git"))) {
        if (Test-Path -LiteralPath $destination) {
            throw "Incomplete dependency directory exists: $destination"
        }
        Invoke-Native $git @("clone", "--quiet", "--depth", "1", "--branch", $dependency.Ref, $dependency.Url, $destination)
    }

    $actualCommit = (& $git -C $destination rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0 -or $actualCommit -ne $dependency.Commit) {
        throw "Dependency mismatch for $($dependency.Name): expected $($dependency.Commit), got $actualCommit"
    }
    Write-Host "  [ok] $($dependency.Name) $actualCommit"
}

$bundledSource = Join-Path $RepositoryRoot "libraries\Adafruit_TCA8418"
$bundledDestination = Join-Path $libraryRoot "CustomAdafruit_TCA8418"
if (-not (Test-Path -LiteralPath $bundledDestination)) {
    Copy-Item -LiteralPath $bundledSource -Destination $bundledDestination -Recurse
} else {
    Copy-Item -Path (Join-Path $bundledSource "*") -Destination $bundledDestination -Recurse -Force
}

Write-Host "[2/4] Selecting the private TFT profile" -ForegroundColor Cyan
$tftRoot = Join-Path $libraryRoot "CustomTFT_eSPI"
Copy-Item -Path (Join-Path $RepositoryRoot "User*.h") -Destination $tftRoot -Force
$selectorPath = Join-Path $tftRoot "User_Setup_Select.h"
$selector = [IO.File]::ReadAllText($selectorPath)
$selectorPattern = '(?m)^\s*(?://)?#include <User_Setup_marauder_waveshare_c5\.h>\s*$'
if ($selector -notmatch $selectorPattern) {
    throw "Waveshare C5 setup entry is missing from $selectorPath"
}
$selector = [regex]::Replace($selector, $selectorPattern, '#include <User_Setup_marauder_waveshare_c5.h>')
[IO.File]::WriteAllText($selectorPath, $selector, [Text.UTF8Encoding]::new($false))

Write-Host "[3/4] Verifying the toolchain" -ForegroundColor Cyan
Invoke-Native $arduinoCli @("version")
$coreList = (& $arduinoCli core list | Out-String)
if ($LASTEXITCODE -ne 0 -or $coreList -notmatch '(?m)^esp32:esp32\s+3\.3\.5(?:\s|$)') {
    throw "Required core esp32:esp32 3.3.5 is not installed."
}
if ($PrepareOnly) {
    Write-Host "[PASS] Dependencies and TFT profile prepared; compilation skipped." -ForegroundColor Green
    return
}

Write-Host "[4/4] Compiling the Waveshare ESP32-C5 target" -ForegroundColor Cyan
$fqbn = "esp32:esp32:esp32c5:FlashSize=8M,PartitionScheme=default_8MB,PSRAM=enabled"
$arguments = @(
    "compile", "--fqbn", $fqbn,
    "--build-path", $buildRoot,
    "--libraries", $libraryRoot,
    "--warnings", "none",
    "--clean",
    "--build-property", "compiler.c.extra_flags=$compilerPrivacyFlags",
    "--build-property", "compiler.cpp.extra_flags=-DMARAUDER_WAVESHARE_C5 -fno-exceptions $compilerPrivacyFlags",
    "--build-property", "compiler.c.elf.extra_flags=-Wl,-zmuldefs",
    $sketchRoot
)
Invoke-Native $arduinoCli $arguments

$firmware = Join-Path $buildRoot "esp32_marauder.ino.bin"
if (-not (Test-Path -LiteralPath $firmware -PathType Leaf)) {
    throw "Compilation completed but the expected application image was not found: $firmware"
}
$hash = Get-FileHash -LiteralPath $firmware -Algorithm SHA256
Write-Host "[PASS] Firmware compiled" -ForegroundColor Green
Write-Host "Binary: $firmware"
Write-Host "Size:   $((Get-Item -LiteralPath $firmware).Length) bytes"
Write-Host "SHA256: $($hash.Hash.ToLowerInvariant())"

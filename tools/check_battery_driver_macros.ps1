$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$configsPath = Join-Path $repoRoot "esp32_marauder/configs.h"
$configs = Get-Content -LiteralPath $configsPath -Raw

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Get-MatchOrFail {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    $match = [regex]::Match($Text, $Pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)

    if (-not $match.Success) {
        throw $Message
    }

    return $match
}

Assert-Match `
    -Text $configs `
    -Pattern "(?s)#if defined\(MARAUDER_V6\) \|\| defined\(MARAUDER_V6_1\).*?#define HAS_BATTERY.*?#define HAS_IP5306" `
    -Message "MARAUDER_V6/MARAUDER_V6_1 must keep IP5306 battery support enabled."

$cleanupMatch = Get-MatchOrFail `
    -Text $configs `
    -Pattern "(?s)//  If we know what we have, we can delete what we're not using(?<cleanup>.*?)#endif\s+// HAS_BATTERY" `
    -Message "Could not find battery driver cleanup section."

$cleanup = $cleanupMatch.Groups["cleanup"].Value

$ip5306Match = Get-MatchOrFail `
    -Text $cleanup `
    -Pattern "(?s)#elif defined\(HAS_IP5306\)(?<ip5306Branch>.*?)(?=\r?\n\s*#elif|\r?\n\s*#else|\r?\n\s*#endif)" `
    -Message "HAS_IP5306 must have an explicit cleanup branch before the generic fallback."

$ip5306Branch = $ip5306Match.Groups["ip5306Branch"].Value

Assert-Match `
    -Text $ip5306Branch `
    -Pattern "#undef HAS_AXP2101" `
    -Message "HAS_IP5306 cleanup must disable HAS_AXP2101."

Assert-Match `
    -Text $ip5306Branch `
    -Pattern "#undef HAS_MAX1704X" `
    -Message "HAS_IP5306 cleanup must disable HAS_MAX1704X."

Assert-Match `
    -Text $ip5306Branch `
    -Pattern "#undef HAS_AXP192" `
    -Message "HAS_IP5306 cleanup must disable HAS_AXP192."

if ($ip5306Branch -match "#undef HAS_IP5306") {
    throw "HAS_IP5306 cleanup must preserve HAS_IP5306 for v6/v6.1 battery support."
}

Write-Host "Battery driver macro checks passed."

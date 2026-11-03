param(
    [string]$DistDir = (Join-Path $PSScriptRoot '..\dist')
)

$ErrorActionPreference = 'Stop'

$dist = (Resolve-Path -LiteralPath $DistDir).Path
$targets = @()

$app = Join-Path $dist 'WorkEncourager.exe'
if (Test-Path -LiteralPath $app) {
    $targets += Get-Item -LiteralPath $app
}

$targets += Get-ChildItem -LiteralPath $dist -Filter 'WorkEncourager-Setup-*.exe' -File -ErrorAction SilentlyContinue
$targets = $targets | Sort-Object Name -Unique

if ($targets.Count -eq 0) {
    throw "No release files found in $dist"
}

$lines = foreach ($file in $targets) {
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    "$hash  $($file.Name)"
}

$output = Join-Path $dist 'SHA256SUMS.txt'
Set-Content -LiteralPath $output -Value $lines -Encoding ASCII
Get-Content -LiteralPath $output

$root = $PSScriptRoot
$gen = Join-Path $env:LOCALAPPDATA 'RobRunInternal\generated_offsets.h'
if (-not (Test-Path -LiteralPath $gen)) { 'generated_offsets.h absent - injecte d abord'; exit 1 }
$headers = @((Join-Path $root 'game\offsets\robrun_offsets.h'), (Join-Path $root 'game\offsets\robrun_sdk_offsets.h'))
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$map = @{}
foreach ($l in (Get-Content -LiteralPath $gen)) {
  if ($l -match '^#define\s+(RR_\w+)\s+(0x[0-9A-Fa-f]+)') {
    $m1 = $Matches[1]; $m2 = $Matches[2]
    $map[$m1] = @{ v = $m2; unres = ($l -match 'UNRESOLVED') }
  }
}
$applied = 0; $unres = 0
foreach ($h in $headers) {
  $lines = New-Object System.Collections.Generic.List[string]
  (Get-Content -LiteralPath $h) | ForEach-Object { $lines.Add($_) }
  Copy-Item -LiteralPath $h -Destination "$h.modifs-backup-$stamp" -Force
  for ($k = 0; $k -lt $lines.Count; $k++) {
    if ($lines[$k] -match '^(\s*#define\s+)(RR_\w+)(\s+)(0x[0-9A-Fa-f]+)(u?)(.*)$') {
      $p1 = $Matches[1]; $mac = $Matches[2]; $p3 = $Matches[3]; $p6 = $Matches[6]
      if ($map.ContainsKey($mac)) {
        $lines[$k] = $p1 + $mac + $p3 + $map[$mac].v + 'u' + $p6
        $applied++
        if ($map[$mac].unres) { $unres++ }
        $map.Remove($mac)
      }
    }
  }
  Set-Content -LiteralPath $h -Value $lines -Encoding ASCII
}
"applied=$applied unresolved=$unres"
if ($map.Count -gt 0) { 'absentes des headers: ' + (@($map.Keys) -join ', ') }
& (Join-Path $root 'gen_autoupdate_table.ps1')

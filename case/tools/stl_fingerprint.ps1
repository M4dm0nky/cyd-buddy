param([Parameter(Mandatory)][string[]]$Files)
$inv = [Globalization.CultureInfo]::InvariantCulture
foreach ($f in $Files) {
  $set = New-Object 'System.Collections.Generic.SortedSet[string]'
  foreach ($m in [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)')) {
    $v = 1..3 | ForEach-Object { [double]::Parse($m.Groups[$_].Value, $inv).ToString('F4', $inv) }
    [void]$set.Add(($v -join ' '))
  }
  $txt = ($set -join "`n")
  $h = [BitConverter]::ToString((New-Object Security.Cryptography.SHA256Managed).ComputeHash([Text.Encoding]::UTF8.GetBytes($txt))).Replace('-', '')
  "{0} {1} {2}" -f (Split-Path $f -Leaf), $h, $set.Count
}

# Ueberhang-Pruefung fuer Druckteile (STL in Druckausrichtung: Z = oben, Bett bei Z 0). Wird von check_roboter.ps1 per Dot-Source geladen.
# Vergleicht ein Teil MIT einer Geometrie (A) mit demselben Teil OHNE sie (B) und bewertet nur die NEUEN, nach unten zeigenden Facetten (nz < -0.7072 = flacher als 45 Grad):
#   - "steil":  -0.95 < nz < -0.7072  = geneigte Flanke mit mehr als 45 Grad Ueberhang  -> immer Fehler
#   - "flach":  nz <= -0.95           = Decke (Bruecke). Zusammenhaengende Decken werden zu Komponenten verbunden; die kleinere der beiden waagerechten
#                                       Ausdehnungen (X, Y) ist die Bruecke und muss <= $MaxBridge sein.
# AUSNAHME (eng und ausdruecklich): -ExPoint/-ExDir/-ExR/-ExZmax nehmen einen Zylinder um eine Achse aus der Bewertung (Roboter: Linsenloch-Fase, Entscheidung 2026-10-28,
# Stuetzen am Linsenloch akzeptiert). Ausgenommen werden NUR Facetten, deren Schwerpunkt hoechstens ExR von der Achse entfernt liegt UND hoechstens ExZmax ueber dem Bett.
# Facetten mit Flaeche < 0.05 mm2 (Rechen-Splitter) und Facetten auf dem Bett (Z < 0.05) werden ignoriert.
function Read-StlFacets([string]$file) {
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $t = Get-Content $file -Raw
  $fm = [regex]::Matches($t, 'facet normal\s+(\S+)\s+(\S+)\s+(\S+)\s+outer loop\s+vertex\s+(\S+)\s+(\S+)\s+(\S+)\s+vertex\s+(\S+)\s+(\S+)\s+(\S+)\s+vertex\s+(\S+)\s+(\S+)\s+(\S+)')
  $out = New-Object System.Collections.ArrayList
  foreach ($m in $fm) {
    $g = $m.Groups; $v = New-Object double[] 12
    for ($i = 1; $i -le 12; $i++) { $v[$i - 1] = [double]::Parse($g[$i].Value, $inv) }
    $null = $out.Add($v)
  }
  return , $out
}
function Eval-Overhang([string]$A, [string]$B, [double]$MaxBridge = 15.0, [double[]]$ExPoint = $null, [double[]]$ExDir = $null, [double]$ExR = 0, [double]$ExZmax = 0) {
  $fa = Read-StlFacets $A; $fb = Read-StlFacets $B
  $h = @{}
  foreach ($v in $fb) {
    $cx = ($v[3] + $v[6] + $v[9]) / 3; $cy = ($v[4] + $v[7] + $v[10]) / 3; $cz = ($v[5] + $v[8] + $v[11]) / 3
    $k = '{0}|{1}|{2}' -f [math]::Floor($cx), [math]::Floor($cy), [math]::Floor($cz)
    if (-not $h.ContainsKey($k)) { $h[$k] = New-Object System.Collections.ArrayList }
    $null = $h[$k].Add(@($cx, $cy, $cz, $v[2]))
  }
  $flat = New-Object System.Collections.ArrayList; $steep = 0; $steepBox = ''; $excluded = 0
  if ($ExPoint) { $dn = [math]::Sqrt($ExDir[0] * $ExDir[0] + $ExDir[1] * $ExDir[1] + $ExDir[2] * $ExDir[2]) }
  foreach ($v in $fa) {
    $nz = $v[2]; if ($nz -ge -0.7072) { continue }
    if ([math]::Min($v[5], [math]::Min($v[8], $v[11])) -lt 0.05) { continue }
    $ax = $v[6] - $v[3]; $ay = $v[7] - $v[4]; $az = $v[8] - $v[5]; $bx = $v[9] - $v[3]; $by = $v[10] - $v[4]; $bz = $v[11] - $v[5]
    $ar = 0.5 * [math]::Sqrt([math]::Pow($ay * $bz - $az * $by, 2) + [math]::Pow($az * $bx - $ax * $bz, 2) + [math]::Pow($ax * $by - $ay * $bx, 2))
    if ($ar -lt 0.05) { continue }
    $cx = ($v[3] + $v[6] + $v[9]) / 3; $cy = ($v[4] + $v[7] + $v[10]) / 3; $cz = ($v[5] + $v[8] + $v[11]) / 3
    $inB = $false
    foreach ($dx in -1, 0, 1) { foreach ($dy in -1, 0, 1) { foreach ($dz in -1, 0, 1) {
      $k = '{0}|{1}|{2}' -f ([math]::Floor($cx) + $dx), ([math]::Floor($cy) + $dy), ([math]::Floor($cz) + $dz)
      if ($h.ContainsKey($k)) { foreach ($c in $h[$k]) { if ([math]::Abs($c[0] - $cx) -lt 0.4 -and [math]::Abs($c[1] - $cy) -lt 0.4 -and [math]::Abs($c[2] - $cz) -lt 0.4 -and [math]::Abs($c[3] - $nz) -lt 0.1) { $inB = $true } } }
    } } }
    if ($inB) { continue }
    if ($ExPoint -and $cz -le $ExZmax) {
      $px = $cx - $ExPoint[0]; $py = $cy - $ExPoint[1]; $pz = $cz - $ExPoint[2]
      $qx = $py * $ExDir[2] - $pz * $ExDir[1]; $qy = $pz * $ExDir[0] - $px * $ExDir[2]; $qz = $px * $ExDir[1] - $py * $ExDir[0]
      if ([math]::Sqrt($qx * $qx + $qy * $qy + $qz * $qz) / $dn -le $ExR) { $excluded++; continue }
    }
    if ($nz -gt -0.95) { $steep++; $steepBox += (' [{0:N1},{1:N1},{2:N1}]' -f $cx, $cy, $cz) } else { $null = $flat.Add($v) }
  }
  # flache Decken zu Komponenten verbinden (gemeinsame Ecken), dann Bruecke = kleinere waagerechte Ausdehnung
  $parent = @{}
  function Find($k) { while ($parent[$k] -ne $k) { $parent[$k] = $parent[$parent[$k]]; $k = $parent[$k] }; return $k }
  $vkeys = @{}
  for ($i = 0; $i -lt $flat.Count; $i++) {
    $parent["f$i"] = "f$i"
    $v = $flat[$i]
    foreach ($o in 3, 6, 9) {
      $k = '{0:N2}|{1:N2}|{2:N2}' -f $v[$o], $v[$o + 1], $v[$o + 2]
      if ($vkeys.ContainsKey($k)) { $r1 = Find $vkeys[$k]; $r2 = Find "f$i"; if ($r1 -ne $r2) { $parent[$r2] = $r1 } } else { $vkeys[$k] = "f$i" }
    }
  }
  $comp = @{}
  for ($i = 0; $i -lt $flat.Count; $i++) { $r = Find "f$i"; if (-not $comp.ContainsKey($r)) { $comp[$r] = New-Object System.Collections.ArrayList }; $null = $comp[$r].Add($flat[$i]) }
  $worst = 0.0; $worstBox = ''; $list = @()
  foreach ($r in $comp.Keys) {
    $xs = @(); $ys = @()
    foreach ($v in $comp[$r]) { foreach ($o in 3, 6, 9) { $xs += $v[$o]; $ys += $v[$o + 1] } }
    $bx = $xs | Measure-Object -Minimum -Maximum; $by = $ys | Measure-Object -Minimum -Maximum
    $dx = $bx.Maximum - $bx.Minimum; $dy = $by.Maximum - $by.Minimum; $span = [math]::Min($dx, $dy)
    $list += ('Decke X[{0:N1}..{1:N1}] Y[{2:N1}..{3:N1}] Bruecke {4:N1} mm' -f $bx.Minimum, $bx.Maximum, $by.Minimum, $by.Maximum, $span)
    if ($span -gt $worst) { $worst = $span; $worstBox = $list[-1] }
  }
  return @{ Excluded = $excluded; Steep = $steep; SteepAt = $steepBox; Ceilings = $comp.Count; Worst = $worst; WorstBox = $worstBox; List = $list }
}

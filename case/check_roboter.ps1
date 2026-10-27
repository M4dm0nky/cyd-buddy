# MadBuddy Roboter: alle Pruefungen. Exitcode 0 = alles gruen, 1 = mindestens eine Pruefung fehlgeschlagen.
# Aufruf (im PowerShell-Tool):  .\case\check_roboter.ps1 [-Define window_shrink=2] [-NoPng]
param(
  [string[]]$Define = @(),   # zusaetzliche -D name=wert (nur Zahlen/Boolsche), z. B. window_shrink=2 fuer die Gegenprobe
  [switch]$NoPng
)
$ErrorActionPreference = 'Stop'
$here   = Split-Path -Parent $MyInvocation.MyCommand.Path
$scad   = Join-Path $here 'madbuddy_roboter.scad'
$oscad  = 'C:\Program Files\OpenSCAD\openscad.com'
$custom = $Define.Count -gt 0
$stlDir = if ($custom) { Join-Path $env:TEMP 'madbuddy_roboter_check_stl' } else { Join-Path $here 'stl' }
$pngDir = Join-Path $here 'render'
New-Item -ItemType Directory -Force $stlDir, $pngDir | Out-Null
$script:fails = 0

# Druckteile. Spaetere Tasks haengen ihre Teile hier an (Task 3: neck, torso_front, torso_back, arm, hand; Task 4: seat, bottom, foot).
$parts = @("head_front", "bezel", "head_back", "ear", "deco", "torso_front", "torso_back", "neck", "arm", "hand")

function Ok($m)   { Write-Host "[ OK ] $m" -ForegroundColor Green }
function Bad($m)  { Write-Host "[FAIL] $m" -ForegroundColor Red; $script:fails++ }
function Note($m) { Write-Host "[ -- ] $m" -ForegroundColor Yellow }

# OpenSCAD aufrufen (mit Zeitlimit), liefert @{Code; Text}
function Run-Scad([string[]]$ScadArgs, [int]$TimeoutSec = 600) {
  $argList = @()
  foreach ($d in $Define) { $argList += '-D'; $argList += $d }
  $argList += $ScadArgs
  $argList += $scad
  $errFile = [IO.Path]::GetTempFileName(); $outFile = [IO.Path]::GetTempFileName()
  $p = Start-Process -FilePath $oscad -ArgumentList $argList -NoNewWindow -PassThru `
        -RedirectStandardError $errFile -RedirectStandardOutput $outFile
  $null = $p.Handle   # Handle halten, sonst ist ExitCode leer
  if (-not $p.WaitForExit($TimeoutSec * 1000)) {
    try { Stop-Process -Id $p.Id -Force } catch {}
    return @{ Code = -999; Text = "TIMEOUT nach $TimeoutSec s" }
  }
  $p.WaitForExit()
  $txt = (Get-Content $errFile -Raw) + (Get-Content $outFile -Raw)
  Remove-Item $errFile, $outFile -ErrorAction SilentlyContinue
  return @{ Code = $p.ExitCode; Text = $txt }
}
function PartArg([string]$name) { return @('-D', ('part=\"{0}\"' -f $name)) }
function FacetCount([string]$file) {
  if (-not (Test-Path $file)) { return 0 }
  return ([regex]::Matches((Get-Content $file -Raw), 'facet normal')).Count
}
$emptyMsg = 'Current top level object is empty'

Write-Host "== madbuddy_roboter check ==  Define: $($Define -join ' ')"

# 1) STL-Export jedes Teils ohne Fehler/Warnungen
foreach ($p in $parts) {
  $out = Join-Path $stlDir "roboter_$p.stl"
  if (Test-Path $out) { Remove-Item $out }
  $r = Run-Scad ((PartArg $p) + @('-o', $out))
  $n = FacetCount $out
  if ($r.Code -ne 0)                { Bad "STL $p : Exitcode $($r.Code) $($r.Text.Trim())" }
  elseif ($r.Text -match 'WARNING') { Bad "STL $p : WARNING in Ausgabe: $(($r.Text -split "`n" | Select-String WARNING | Select-Object -First 3) -join ' | ')" }
  elseif ($r.Text -match 'ERROR')   { Bad "STL $p : ERROR in Ausgabe" }
  elseif ($n -lt 100)               { Bad "STL $p : zu wenige Facetten ($n)" }
  else                              { Ok "STL $p ($n Facetten)" }
}

# 2) Standsicherheit: nur, wenn das SCAD eine Sitzflaeche (seat_poly) kennt
$r = Run-Scad ((PartArg 'info') + @('-o', (Join-Path $env:TEMP 'madbuddy_roboter_info.stl')))
if ($r.Text -match 'WARNING|ERROR') { Bad "Info: WARNING/ERROR: $($r.Text.Trim())" }
if ($r.Text -match 'STAND_MIN_MARGIN=([-0-9.e]+) STAND_MIN=([-0-9.e]+)') {
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $m = [double]::Parse($Matches[1], $inv); $mn = [double]::Parse($Matches[2], $inv)
  if ($m -ge $mn) { Ok ("Stand: Lot des Schwerpunkts {0:N1} mm innerhalb der Sitzflaeche (verlangt >= {1})" -f $m, $mn) }
  else { Bad ("Stand: Lot des Schwerpunkts nur {0:N1} mm innerhalb der Sitzflaeche, verlangt >= {1}" -f $m, $mn) }
} elseif ($r.Text -match 'STAND_SKIPPED') {
  Note "Stand: uebersprungen, es gibt noch keine Sitzflaeche (seat_poly leer, kommt mit Task 4)"
} else { Bad "Stand: weder STAND_MIN_MARGIN noch STAND_SKIPPED aus info gelesen" }

# 3) Kollisionspruefung: Gehaeuse gegen alle Pruefkoerper (Platine, Glas, Rueckseite, Buchsen, Stecker, aktive Flaeche + Sichtkegel)
$col = Join-Path $env:TEMP 'madbuddy_roboter_collision.stl'
if (Test-Path $col) { Remove-Item $col }
$r = Run-Scad ((PartArg 'collision') + @('-o', $col))
$n = FacetCount $col
# Die bekannte Leer-Meldung ("WARNING: Current top level object is empty") ist erwartet und wird vorher entfernt. Alles andere mit WARNING/ERROR zaehlt immer.
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Kollision: WARNING/ERROR: $($rest.Trim())" }
elseif ($r.Text -match $emptyMsg) { Ok "Kollision: Schnittmenge leer (kollisionsfrei, Fenster gibt aktive Flaeche + Sichtkegel frei)" }
elseif ($r.Code -eq 0 -and $n -eq 0) { Ok "Kollision: STL ohne Facetten (kollisionsfrei)" }
elseif ($r.Code -eq 0) { Bad "Kollision: Gehaeuse schneidet Pruefkoerper ($n Facetten Schnittvolumen) -> $col" }
else { Bad "Kollision: Exitcode $($r.Code) $($r.Text.Trim())" }

# 3b) Positive-Kontrolle: die Pruefung muss einen absichtlichen Treffer erkennen (nur ohne -Define)
if (-not $custom) {
  $st = Join-Path $env:TEMP 'madbuddy_roboter_selftest.stl'
  if (Test-Path $st) { Remove-Item $st }
  $r = Run-Scad ((PartArg 'selftest') + @('-o', $st))
  if ((FacetCount $st) -gt 0) { Ok "Selbsttest: absichtliche Kollision wird erkannt" }
  else { Bad "Selbsttest: absichtliche Kollision NICHT erkannt (Pruefung taugt nichts)" }
}

# 3c) Kernloecher des Halssockels muessen im Material stecken: Bohrvolumen gegen head_back ohne Kernloecher, Y-Ausdehnung >= 7,5 mm
$pt = Join-Path $env:TEMP 'madbuddy_roboter_pilot_test.stl'
if (Test-Path $pt) { Remove-Item $pt }
$r = Run-Scad ((PartArg 'pilot_test') + @('-o', $pt))
if ((FacetCount $pt) -eq 0) { Bad "Halssockel-Kernloecher: Bohrvolumen trifft kein Material (Loch liegt in der Luft)" }
else {
  $inv = [Globalization.CultureInfo]::InvariantCulture
  $ys = [regex]::Matches((Get-Content $pt -Raw), 'vertex\s+\S+\s+(\S+)\s+\S+') | ForEach-Object { [double]::Parse($_.Groups[1].Value, $inv) }
  $ext = ($ys | Measure-Object -Maximum).Maximum - ($ys | Measure-Object -Minimum).Minimum
  if ($ext -ge 7.5) { Ok ("Halssockel-Kernloecher: {0:N2} mm im Material (>= 7,5)" -f $ext) }
  else { Bad ("Halssockel-Kernloecher: nur {0:N2} mm im Material, verlangt >= 7,5" -f $ext) }
}

# 3d) Kernloecher im Oberkoerper (Hals-Mulde: Z, Sitz: Z, Dome: Y): Ausdehnung des Bohrvolumens im Material >= 7,5 mm
$inv = [Globalization.CultureInfo]::InvariantCulture
foreach ($t in @(@{ n = 'pilot_neck'; ax = 2; l = 'Oberkoerper Hals-Kernloecher (Z)' }, @{ n = 'pilot_seat'; ax = 2; l = 'Oberkoerper Sitz-Kernloecher (Z)' }, @{ n = 'pilot_dome'; ax = 1; l = 'Oberkoerper Dom-Kernloecher (Y)' })) {
  $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $t.n)
  if (Test-Path $f) { Remove-Item $f }
  $null = Run-Scad ((PartArg $t.n) + @('-o', $f))
  if ((FacetCount $f) -eq 0) { Bad "$($t.l): Bohrvolumen trifft kein Material (Loch liegt in der Luft)"; continue }
  $vs = [regex]::Matches((Get-Content $f -Raw), 'vertex\s+(\S+)\s+(\S+)\s+(\S+)') | ForEach-Object { [double]::Parse($_.Groups[1 + $t.ax].Value, $inv) }
  $ext = ($vs | Measure-Object -Maximum).Maximum - ($vs | Measure-Object -Minimum).Minimum
  if ($ext -ge 7.5) { Ok ("{0}: {1:N2} mm im Material (>= 7,5)" -f $t.l, $ext) } else { Bad ("{0}: nur {1:N2} mm im Material, verlangt >= 7,5" -f $t.l, $ext) }
}

# 3e) Koerper-Kollisionen: Arm/Oberkoerper, Arm/Hand, Hals/Oberkoerper+Kopf, Oberkoerper/Kopf (Schnittmenge muss leer sein)
$cb = Join-Path $env:TEMP 'madbuddy_roboter_collision_body.stl'
if (Test-Path $cb) { Remove-Item $cb }
$r = Run-Scad ((PartArg 'collision_body') + @('-o', $cb)) 900
$rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
if ($rest -match 'WARNING|ERROR') { Bad "Koerper-Kollision: WARNING/ERROR: $($rest.Trim())" }
elseif ($r.Text -match $emptyMsg -or ($r.Code -eq 0 -and (FacetCount $cb) -eq 0)) { Ok "Koerper-Kollision: Arm/Hand/Hals/Oberkoerper/Kopf kollisionsfrei" }
else { Bad "Koerper-Kollision: Teile schneiden sich ($(FacetCount $cb) Facetten) -> $cb" }

# 3e2) Positive-Kontrolle Koerper-Kollision: Arm 3 mm in den Oberkoerper geschoben MUSS Treffer liefern (nur ohne -Define)
if (-not $custom) {
  $cs = Join-Path $env:TEMP 'madbuddy_roboter_cbody_selftest.stl'
  if (Test-Path $cs) { Remove-Item $cs }
  $null = Run-Scad ((PartArg 'collision_body_selftest') + @('-o', $cs))
  if ((FacetCount $cs) -gt 0) { Ok "Selbsttest Koerper-Kollision: Arm 3 mm im Oberkoerper wird erkannt" }
  else { Bad "Selbsttest Koerper-Kollision: Arm im Oberkoerper NICHT erkannt" }
}

# 3g) Montage: untere Halsschrauben nur in torso_front, jede Schraube mit Schraubendreher Ø6 x 60 entlang der Achse erreichbar (Phase 1 und 2), plus Positiv-Kontrollen
$pb = Join-Path $env:TEMP 'madbuddy_roboter_pilot_neck_back.stl'
if (Test-Path $pb) { Remove-Item $pb }
$r = Run-Scad ((PartArg 'pilot_neck_back') + @('-o', $pb))
if ($r.Text -match 'WARNING: Current top level object is empty' -or (FacetCount $pb) -eq 0) { Ok "Montage: Kernloecher der unteren Halsschrauben liegen nicht in torso_back (nur in torso_front)" }
else { Bad "Montage: untere Halsschraube greift in torso_back" }
foreach ($t in @(@{ n = 'drv_p1'; l = 'Montage Phase 1 (Hals von oben, 2 Schrauben)' }, @{ n = 'drv_p2'; l = 'Montage Phase 2 (Kopfschrauben von unten, 2 Schrauben)' })) {
  $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $t.n)
  if (Test-Path $f) { Remove-Item $f }
  $r = Run-Scad ((PartArg $t.n) + @('-o', $f)) 900
  $rest = ($r.Text -split "`n" | Where-Object { $_ -notmatch [regex]::Escape($emptyMsg) }) -join "`n"
  if ($rest -match 'WARNING|ERROR') { Bad "$($t.l): WARNING/ERROR: $($rest.Trim())" }
  elseif ($r.Text -match $emptyMsg -or (FacetCount $f) -eq 0) { Ok "$($t.l): Schraubendreher Ø6 x 60 kollisionsfrei erreichbar" }
  else { Bad "$($t.l): Schraubendreher kollidiert ($(FacetCount $f) Facetten) -> $f" }
}
if (-not $custom) {
  foreach ($n in @('drv_selftest_p1', 'drv_selftest_p2')) {
    $f = Join-Path $env:TEMP ("madbuddy_roboter_{0}.stl" -f $n)
    if (Test-Path $f) { Remove-Item $f }
    $null = Run-Scad ((PartArg $n) + @('-o', $f)) 900
    if ((FacetCount $f) -gt 0) { Ok "Selbsttest $n : versetzter Schraubendreher wird als Kollision erkannt" }
    else { Bad "Selbsttest $n : versetzter Schraubendreher NICHT erkannt" }
  }
}

# 3f) Positive-Kontrolle Oberkoerper: zu grosse XIAO-Platte muss die Schale treffen (nur ohne -Define)
if (-not $custom) {
  $ts = Join-Path $env:TEMP 'madbuddy_roboter_torso_selftest.stl'
  if (Test-Path $ts) { Remove-Item $ts }
  $null = Run-Scad ((PartArg 'torso_selftest') + @('-o', $ts))
  if ((FacetCount $ts) -gt 0) { Ok "Selbsttest Oberkoerper: absichtliche Kollision wird erkannt" }
  else { Bad "Selbsttest Oberkoerper: absichtliche Kollision NICHT erkannt" }
}

# 4) PNG-Renders
if (-not $NoPng) {
  $views = @(
    @{ n = 'assembly'; cam = '0,0,0,72,0,330,0'; part = 'assembly'; nodum = $true },
    @{ n = 'exploded'; cam = '0,0,0,70,0,320,0'; part = 'exploded'; nodum = $true },
    @{ n = 'section';  cam = '0,0,0,90,0,90,0';  part = 'section'; render = $true }
  )
  foreach ($v in $views) {
    $png = Join-Path $pngDir ("roboter_{0}.png" -f $v.n)
    if (Test-Path $png) { Remove-Item $png }
    $a = (PartArg $v.part) + @('-o', $png, '--camera', $v.cam, '--viewall', '--autocenter', '--imgsize', '1400,1400', '--colorscheme', 'Tomorrow')
    if ($v.nodum) { $a = @('-D', 'show_dummies=false') + $a }   # Ansicht ohne Pruefkoerper, damit die Figur das Bild fuellt
    if ($v.render) { $a += '--render' }   # CGAL statt Vorschau: Schnitt wird in der Vorschau nicht abgezogen
    $r = Run-Scad $a 400
    if ($r.Code -ne 0)                           { Bad "PNG $($v.n): Exitcode $($r.Code) $($r.Text.Trim())" }
    elseif ($r.Text -match 'WARNING')            { Bad "PNG $($v.n): WARNING: $($r.Text.Trim())" }
    elseif (-not (Test-Path $png) -or (Get-Item $png).Length -lt 5000) { Bad "PNG $($v.n): Datei fehlt/leer" }
    else { Ok "PNG $($v.n) -> $png" }
  }
}

# Aufraeumen: keine OpenSCAD-Reste
Get-CimInstance Win32_Process -Filter "Name like 'openscad%'" | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

if ($script:fails -gt 0) { Write-Host "== FEHLGESCHLAGEN: $($script:fails) Pruefung(en) ==" -ForegroundColor Red; exit 1 }
Write-Host "== ALLES GRUEN ==" -ForegroundColor Green
exit 0

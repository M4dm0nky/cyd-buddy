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
$parts = @('head_front', 'bezel', 'head_back', 'ear', 'deco')

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
if ($r.Text -match 'WARNING|ERROR' -and $r.Text -notmatch $emptyMsg) { Bad "Kollision: WARNING/ERROR: $($r.Text.Trim())" }
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

# 4) PNG-Renders
if (-not $NoPng) {
  $views = @(
    @{ n = 'assembly'; cam = '0,0,0,72,0,330,0'; part = 'assembly' },
    @{ n = 'exploded'; cam = '0,0,0,70,0,320,0'; part = 'exploded' },
    @{ n = 'section';  cam = '0,0,0,90,0,90,0';  part = 'section'; render = $true }
  )
  foreach ($v in $views) {
    $png = Join-Path $pngDir ("roboter_{0}.png" -f $v.n)
    if (Test-Path $png) { Remove-Item $png }
    $a = (PartArg $v.part) + @('-o', $png, '--camera', $v.cam, '--viewall', '--autocenter', '--imgsize', '1200,900', '--colorscheme', 'Tomorrow')
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

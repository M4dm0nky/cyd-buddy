// MadBuddy: gemeinsame Kamera-/XIAO-Geometrie -- Kameratasche mit Riegel, XIAO-Wiege, Pruefkoerper
// Wird von den Gehaeuse-Dateien mit include <xiao_cam_common.scad> eingebunden (nach cyd_common.scad). OpenSCAD 2021.01.
// Alle Module arbeiten in EINEM lokalen System: Innenseite der Frontwand = Z 0, Blickrichtung (nach aussen) = +Z,
// Mitte der Linse = Ursprung (X/Y), Kamera-Horizontale (lange Bildseite) = X. Die einbindende Datei platziert/kippt das System.
// Die Aussenflaeche der Frontwand liegt bei Z = wall_t (jedes Modul nimmt wall_t, Vorgabe cam_wall_t).
// Erwartet aus der einbindenden Datei: nichts. Definiert hier (NICHT in der einbindenden Datei wiederholen, sonst WARNING):
//   cam_*, xiao_* (siehe unten).
// Quellen: .superpowers/sdd/2026-10-27-roboter-druckteile/kamera-recherche.md (Seeed-Zeichnungen), Spec docs/superpowers/specs/2026-10-27-kamera-halter-design.md

// ---------------------------------------------------------------- Auswahl
cam_type   = "OV3660";   // "OV3660" (laut Seeed-Bild im Lieferumfang) oder "OV5640"; Aufdruck am eigenen Teil pruefen
cam_wall_t = 2.0;        // Vorgabe fuer die Wandstaerke der Frontwand (die einbindende Datei uebergibt ihren Wert)

// ---------------------------------------------------------------- Kameramodul
cam_body     = cam_type == "OV3660" ? 8.0 : 8.5;   // QUELLE: Seeed-Zeichnung (OV3660 8,0 x 8,0; OV5640 8,5 x 8,5)
cam_depth    = cam_type == "OV3660" ? 5.3 : 5.5;   // QUELLE: Seeed-Zeichnung, Gesamttiefe inkl. Tubus (OV5640 ca., schlecht lesbar)
cam_tube_d   = 6.2;    // QUELLE: Seeed-Zeichnung OV3660 (Zeichnung, mittel); OV5640 VORLÄUFIG gleich
cam_tube_len = 2.7;    // VORLÄUFIG (Zeichnung 2,70, Zuordnung unsicher) Tubusueberstand ueber den Koerper -- mit Schieblehre nachmessen
cam_fpc_w    = 6.5;    // QUELLE: Seeed-Zeichnung OV3660 (OV5640 VORLÄUFIG gleich)
cam_fpc_len  = cam_type == "OV3660" ? 21.2 : 70.5; // QUELLE: Seeed-Zeichnung, ab Koerperkante bis Ende
cam_fpc_t    = 0.3;    // QUELLE: Seeed-Zeichnung (Zeichnung, mittel) FPC-Dicke
cam_fov_d    = cam_type == "OV3660" ? 102 : 69;    // QUELLE: Hersteller (diagonal, Grad)
cam_fov_h    = cam_type == "OV3660" ? 85 : 58;     // QUELLE: Hersteller OV3660 (horizontal); OV5640 VORLÄUFIG (aus 69 Grad diagonal, 4:3)
cam_bend_r   = 2.0;    // QUELLE: Recherche (FPC-Biegeradius mindestens 2 mm)

// ---------------------------------------------------------------- Tasche und Riegel
cam_play       = 0.25;  // Spiel je Seite um den Kamerakoerper (Vorgabe Spec: Koerper + 0,25 mm je Seite)
cam_hole_play  = 0.5;   // Linsenloch = Tubus + 0,5 (OV3660: 6,7) -- QUELLE: Recherche
cam_lens_rec   = 0.3;   // Linse endet 0,3 mm hinter der Aussenflaeche -- QUELLE: Spec
cam_chamfer    = 1.0;   // Fase aussen am Linsenloch, 45 Grad (Tiefe = Aufweitung je Seite)
cam_frame_wall = 2.0;   // Wandstaerke des Rahmens um die Tasche (>= 2 mm)
cam_groove     = 1.0;   // Tiefe der seitlichen Riegelnuten ueber die Tasche hinaus
cam_clip_t     = 1.2;   // Dicke des Riegels (Ausnahme von der 2-mm-Regel) -- QUELLE: Spec
cam_clip_over  = 0.1;   // Uebermass (Klemmung): Nut ist um so viel niedriger als der Riegel -- QUELLE: Spec
cam_clip_fit   = 0.15;  // seitliches Spiel Riegel/Nut (= fit_peg der Roboter-Datei, als Presspassung)
cam_clip_press = 2.0;   // der Riegel liegt so weit (mm) auf dem Kamerakoerper auf (Quersteg)
cam_slot_extra = 0.7;   // FPC-Schlitz = FPC-Breite + 0,7 -- QUELLE: Spec

// ---------------------------------------------------------------- XIAO ESP32S3 Sense (Stapel) und Wiege
xiao_w       = 21.0;  // QUELLE: Seeed-Wiki (Platine, lange Seite) -- wird in X gelegt
xiao_l       = 17.8;  // QUELLE: Seeed-Wiki (Händler teils 17,5)
xiao_stack   = 15.0;  // VORLÄUFIG (Seeed-Wiki "mit Erweiterung", Kamera-Anteil unklar) Stapelhoehe -- nachmessen
xiao_pcb_t   = 1.2;   // VORLÄUFIG (typisch 1,0 bis 1,2) Platinendicke -- nachmessen
xiao_conn_in = 2.0;   // VORLÄUFIG (Schaetzung) Mitte des Kamerasteckers ab dem Platinenende, Ende gegenueber der USB-Buchse
xiao_conn_h  = 1.4;   // VORLÄUFIG (Schaetzung) Steckerhoehe ueber der Sense-Platine; die FPC-Ebene liegt in halber Hoehe
xiao_conn_w  = 12.5;  // QUELLE: Seeed-Zeichnung (Versteifung/Steckerbreite am FPC-Ende)
xiao_play    = 0.3;   // Spiel um den Stapel (= fit)
xiao_gap     = 0.2;   // Luft zwischen Rahmenunterseite und Stapelvorderseite
xiao_post_t  = 2.4;   // Dicke der Anschlag-Pfosten in X (>= 2 mm)
// Lage des Stapelmittelpunkts relativ zur Linse (X, Y): das Ende mit dem Kamerastecker zeigt nach +X zur Kamera.
// Der Stapel liegt parallel zur Frontwand hinter dem Kamera-Rahmen (Z-Abstand wegen der Rahmentiefe), nach +X versetzt.
xiao_offset  = [4.5, 0.0]; // VORLÄUFIG (Entwurf): X so gewaehlt, dass Stecker <= 16 mm Luftlinie von der Koerperkante entfernt liegt
cam_fpc_air_max = 16.0;    // QUELLE: Spec (21,2 mm FPC, lockere Schleife) max. Luftlinie Koerperkante -> Stecker

// ================================================================= abgeleitete Werte
cam_half    = cam_body / 2 + cam_play;           // halbe Taschenweite
cam_outer_h = cam_half + cam_groove + cam_frame_wall; // halbe Rahmenweite
cam_hole_d  = cam_tube_d + cam_hole_play;        // Linsenloch
cam_tan_h   = tan(cam_fov_h / 2);
cam_tan_v   = sqrt(tan(cam_fov_d / 2) * tan(cam_fov_d / 2) - cam_tan_h * cam_tan_h);   // aus diagonal + horizontal
// Z-Lagen (lokal, Frontwand-Innenseite = 0, Aussenflaeche = w)
function cam_zbf(w = cam_wall_t) = min(0, w - cam_lens_rec - cam_tube_len);           // Vorderseite Kamerakoerper (Anschlag)
function cam_ztip(w = cam_wall_t) = cam_zbf(w) + cam_tube_len;                        // Linsenspitze
function cam_zbb(w = cam_wall_t) = cam_zbf(w) - (cam_depth - cam_tube_len);           // Rueckseite Kamerakoerper = FPC-Ebene
function cam_zg1(w = cam_wall_t) = cam_zbb(w) - (cam_clip_t - cam_clip_over);        // Nutboden (Oberseite der hinteren Lippe)
function cam_zrear(w = cam_wall_t) = cam_zg1(w) - cam_frame_wall;                     // Rahmen-Rueckseite
function xiao_z0(w = cam_wall_t) = cam_zrear(w) - xiao_gap;                           // Stapelvorderseite (Oberkante Stecker)
function xiao_zbot(w = cam_wall_t) = xiao_z0(w) - xiao_stack;                         // Stapelrueckseite
// Kamerastecker auf der Sense-Platine (FPC-Ebene) und Luftlinie ab der Koerperkante
function xiao_conn_pt(w = cam_wall_t) = [xiao_offset[0] + xiao_w / 2 - xiao_conn_in, xiao_offset[1], xiao_z0(w) - xiao_conn_h / 2];
function cam_fpc_air(w = cam_wall_t) = norm(xiao_conn_pt(w) - [cam_body / 2, 0, cam_zbb(w)]);

// ================================================================= Tasche (positiv + negativ)
module cam_pocket_add(wall_t = cam_wall_t) {
  // Rahmen um die Tasche: von der Wand (0,5 mm in die Wand hinein) bis zur Rahmen-Rueckseite
  zr = cam_zrear(wall_t);
  translate([-cam_outer_h, -cam_outer_h, zr]) cube([2 * cam_outer_h, 2 * cam_outer_h, 0.5 - zr]);
}

module cam_pocket_cut(wall_t = cam_wall_t) {
  zbf = cam_zbf(wall_t); zbb = cam_zbb(wall_t); zg1 = cam_zg1(wall_t); zr = cam_zrear(wall_t);
  gh = cam_half + cam_groove;
  sw = cam_fpc_w + cam_slot_extra;
  // Linsenloch durch Anschlagplatte und Wand, aussen 45 Grad gefast
  translate([0, 0, zbf - 0.01]) cylinder(d = cam_hole_d, h = wall_t - cam_chamfer - zbf + 0.02);
  translate([0, 0, wall_t - cam_chamfer]) cylinder(d1 = cam_hole_d, d2 = cam_hole_d + 2 * cam_chamfer, h = cam_chamfer + 0.01);
  translate([0, 0, wall_t]) cylinder(d = cam_hole_d + 2 * cam_chamfer, h = 1);
  // Koerpertasche: von der Anschlagflaeche nach hinten offen (Kamera wird von hinten eingesetzt)
  translate([-cam_half, -cam_half, zr - 1]) cube([2 * cam_half, 2 * cam_half, zbf - zr + 1]);
  // Riegelnuten: hinter dem Koerper, U-foermig, nach +X offen (Riegel wird von der FPC-Seite eingeschoben)
  translate([-gh, -gh, zg1]) cube([2 * gh, 2 * gh, zbb - zg1]);
  translate([0, -gh, zg1]) cube([cam_outer_h + 1, 2 * gh, zbb - zg1]);
  // FPC-Schlitz in der +X-Wand
  translate([0, -sw / 2, zg1]) cube([cam_outer_h + 1, sw, zbb + 0.5 - zg1]);
}

// ================================================================= Riegel (eigenes Druckteil, liegt flach: Z 0 ... cam_clip_t)
module cam_clip() {
  gh = cam_half + cam_groove - cam_clip_fit;      // halbe Riegelbreite (in der Nut)
  sw = cam_fpc_w + cam_slot_extra;
  x0 = -gh;                                       // Quersteg hinten (-X)
  x1 = -(cam_body / 2 - cam_clip_press);          // Steg liegt cam_clip_press mm auf dem Koerper auf
  xe = cam_outer_h - 0.75;                        // Armenden (greifbar, innerhalb des Rahmens)
  linear_extrude(cam_clip_t) union() {
    translate([x0, -gh]) square([x1 - x0, 2 * gh]);
    for (s = [-1, 1]) translate([x0, s > 0 ? sw / 2 : -gh]) square([xe - x0, gh - sw / 2]);
  }
}
module cam_clip_place(wall_t = cam_wall_t) {    // Riegel in Einbaulage (direkt hinter der Koerperrueckseite)
  translate([0, 0, cam_zbb(wall_t) - cam_clip_t]) cam_clip();
}

// ================================================================= XIAO-Wiege (positiv + negativ)
// Je zwei schmale Pfosten an beiden Stirnenden (USB-Ende und Kameraende) von der Frontwand bis zur Stapelrueckseite.
// Die Laengsseiten und die Mitte am Kamerastecker (xiao_conn_w) bleiben frei: dort sitzen die angeloeteten Draehte und der FPC.
// Druck ohne Stuetzen: reine senkrechte Pfosten. Eingelegt wird der Stapel von hinten (Frontteil liegt mit der Aussenseite unten, Einlegen von oben);
// gegen Herausfallen nach hinten halten ihn die Anschlagpfosten der Rueckwand.
module xiao_cradle_add(wall_t = cam_wall_t) {
  zb = xiao_zbot(wall_t);
  bx = xiao_offset[0]; by = xiao_offset[1];
  gap_h = xiao_conn_w / 2 + 0.15;                           // freie Mitte
  yo = xiao_l / 2;                                          // Pfosten enden buendig mit der Stapelkante (nicht weiter, sonst Kopfwand)
  for (ex = [-1, 1], sy = [-1, 1])
    translate([ex > 0 ? bx + xiao_w / 2 + xiao_play : bx - xiao_w / 2 - xiao_play - xiao_post_t,
               sy > 0 ? by + gap_h : by - yo, zb])
      cube([xiao_post_t, yo - gap_h, 0.5 - zb]);
}
module xiao_cradle_cut(wall_t = cam_wall_t) {               // Stapelraum + Spiel (haelt Rahmen/Wand frei)
  translate([xiao_offset[0] - xiao_w / 2 - xiao_play, xiao_offset[1] - xiao_l / 2 - xiao_play, xiao_zbot(wall_t)])
    cube([xiao_w + 2 * xiao_play, xiao_l + 2 * xiao_play, xiao_stack]);
}

// ================================================================= Pruefkoerper
module cam_seg(p, q) {                                       // dünnes Band zwischen zwei Punkten (FPC)
  hull() for (r = [p, q]) translate([r[0], r[1] - cam_fpc_w / 2, r[2] - cam_fpc_t]) cube([0.01, cam_fpc_w, cam_fpc_t]);
}
module cam_dummy(wall_t = cam_wall_t) {                     // Koerper, Tubus, FPC-Band bis zum Stecker
  zbf = cam_zbf(wall_t); zbb = cam_zbb(wall_t);
  c = xiao_conn_pt(wall_t);
  translate([-cam_body / 2, -cam_body / 2, zbb]) cube([cam_body, cam_body, zbf - zbb]);
  translate([0, 0, zbf - 0.01]) cylinder(d = cam_tube_d, h = cam_tube_len + 0.01);
  p1 = [cam_outer_h + 0.5, 0, zbb];
  p2 = [c[0] - 3, c[1], c[2]];
  cam_seg([0, 0, zbb], p1);
  cam_seg(p1, p2);
  cam_seg(p2, c);
}
module xiao_stack_dummy(wall_t = cam_wall_t) {
  translate([xiao_offset[0] - xiao_w / 2, xiao_offset[1] - xiao_l / 2, xiao_zbot(wall_t)])
    cube([xiao_w, xiao_l, xiao_stack]);
}
module cam_fov_cone(L = 30, wall_t = cam_wall_t) {          // Sichtpyramide ab der Linsenspitze (102 Grad diagonal / 85 Grad horizontal)
  zt = cam_ztip(wall_t);
  hull() {
    translate([0, 0, zt - 0.01]) cylinder(d = cam_tube_d, h = 0.01);
    translate([-(cam_tube_d / 2 + L * cam_tan_h), -(cam_tube_d / 2 + L * cam_tan_v), zt + L])
      cube([2 * (cam_tube_d / 2 + L * cam_tan_h), 2 * (cam_tube_d / 2 + L * cam_tan_v), 0.01]);
  }
}

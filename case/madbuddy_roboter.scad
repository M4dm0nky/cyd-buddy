// MadBuddy Roboter (Stufe 3): druckbare Teile fuer die sitzende Roboterfigur. Dieses Paket: Kopf (Front, Blende, Rueckwand, Ohren, Zierschrauben).
// OpenSCAD 2021.01. Teil waehlen mit:  -D 'part="head_front"'
// Konstruktionslage des Kopfes wie Stufe 2: X nach rechts (Blick auf das Gesicht), Y nach oben, Z zum Betrachter.
// Gesichtsebene = Z 0, das Innere liegt bei negativem Z. Ursprung X/Y = Mitte der aktiven Display-Flaeche (disp_x = 0).
// Weltlage (place_head): Z oben, das Gesicht blickt nach -Y, der Kopf ist um head_tilt nach hinten geneigt.
// part = "head_front" | "bezel" | "head_back" | "ear" | "deco" | "assembly" | "exploded" | "section" | "collision" | "selftest" | "info"
// Druckausrichtung: die Teile-Zweige am Dateiende liefern jedes Teil fertig auf dem Bett, ohne Stuetzen.
//
// ERWEITERN (Task 3/4): neue Teile als Module in Konstruktionslage + Weltlage anlegen, dann
//   1. in world_shells() / world_dummies() einhaengen,
//   2. ihre Massen an die Liste 'masses' haengen (concat),
//   3. seat_poly (Standflaeche, Welt-XY) setzen -> die Standpruefung wird dann aktiv,
//   4. in check_roboter.ps1 die Liste $parts erweitern und im part-Zweig unten den Druckzweig ergaenzen.

part = "assembly";

// ---------------------------------------------------------------- Steuerung / Pruefhilfen
show_dummies = true;   // nur Ansicht: Pruefkoerper (Platine, Display ...) mitzeigen

// ---------------------------------------------------------------- Allgemein
fit        = 0.3;    // Passungsspiel in mm (Variable); cyd_common.scad braucht es nicht, der Roboter schon
fit_peg    = 0.15;   // Spiel fuer Steckzapfen/Zierschrauben, als Presspassung gedacht
wall       = 2.4;    // Wandstaerke (>= 2 mm)
front_t    = 2.0;    // Dicke der Frontplatte (Gesicht)
back_t     = 2.6;    // Dicke der Rueckwand (Platz fuer Schraubenkopf-Senkung)
ch         = 4.0;    // 45-Grad-Fase rundum am Gesicht und an der Rueckseite (statt Kugelrundung, ohne Stuetzen druckbar)
lip_h      = 3.0;    // Hoehe der Falz (Passlippe) zwischen Front und Rueckwand
head_tilt  = 8;      // QUELLE: Konzept (roboter_konzept.scad head_tilt): Neigung des Kopfes nach hinten in Grad
$fn        = 64;

// ---------------------------------------------------------------- Werte, die cyd_common.scad vom Gehaeuse erwartet
glass_gap  = 0.2;    // Luft zwischen Display-Oberkante und Frontplatte (wie Stufe 2)
disp_x     = 0;      // Roboter: Display sitzt mittig im Kopf

// CYD-Masse, Fenster und Pruefkoerper. Muss NACH front_t, glass_gap, fit, disp_x stehen: OpenSCAD wertet Zuweisungen der Reihe nach aus.
include <cyd_common.scad>

// ---------------------------------------------------------------- Kopfform
head_w      = 114;   // QUELLE: Konzept (h_w): Kopfbreite
head_h      = 84;    // QUELLE: Konzept (h_h): Kopfhoehe
head_r      = 14;    // QUELLE: Konzept (h_r): Eckradius in der Frontansicht
rear_air    = 16;    // Zusatztiefe hinten (Kabelweg, Halssockel): der Halssockel (Bohrung 24,3) braucht in der Rueckwand ca. 28 mm Tiefe
dome_d      = 6.4;   // Schraubdom aussen (wie Stufe 2)
dome_x      = 36.0;  // Domposition X (links/rechts), wie Stufe 2
pilot_d     = 2.6;   // Kernloch fuer M3 in PETG
pilot_len   = 7.5;
screw_d     = 3.4;   // Durchgang M3 in der Rueckwand
cbore_d     = 6.2;   // Senkung Schraubenkopf
cbore_t     = 1.8;

// ---------------------------------------------------------------- Blende (bezel) und Nut
// Die Blende liegt AUF dem Gesicht (1,2 dick) und rastet mit einem umlaufenden Steg in eine Nut. Eine flaechige Mulde waere bei
// "Gesicht nach unten" eine freitragende Flaeche von ca. 12 mm: nicht ohne Stuetzen druckbar. Die Nut ist nur 2 mm breit.
bz_hx       = 43;    // Blende Halbbreite (Blende 86 x 60, hell zwischen Fenster und Zierschrauben)
bz_hy       = 30;    // Blende Halbhoehe
bz_r        = 8;     // Blende Eckradius
bezel_t     = 1.2;   // QUELLE: Aufgabe: Rahmen 1,2 mm dick
nut_t       = 0.8;   // QUELLE: Aufgabe: Tiefe der Mulde/Nut 0,8
ridge_w     = 2.0;   // Stegbreite der Blende
ridge_in    = 1.5;   // Abstand des Stegs vom Blendenrand

// ---------------------------------------------------------------- Zierschrauben (als Einlage, Teil "deco")
deco_d      = 4;     // QUELLE: Aufgabe: Zierschraube Kopf Ø 4
deco_h      = 0.6;   // QUELLE: Aufgabe: 0,6 erhaben
deco_in     = 0.8;   // Einsatztiefe in der Frontplatte (Frontplatte behaelt 1,2)
deco_pts    = [for (sx = [-1, 1], sy = [-1, 1]) [sx * 46, sy * 32]];  // ausserhalb der Blende, in der Flaeche des Gesichts

// ---------------------------------------------------------------- Ohren
ear_d       = 30;    // QUELLE: Aufgabe: Scheibe Ø 30
ear_t       = 10;    // QUELLE: Aufgabe: 10 dick
peg_d       = 6;     // QUELLE: Aufgabe: Zapfen Ø 6
peg_l       = 6;     // QUELLE: Aufgabe: Zapfen 6 lang (Loch 6,3 tief)
peg_flat    = 0.5;   // Abflachung gegen Verdrehen (Tiefe)
peg_ch      = 0.5;   // QUELLE: Aufgabe: 0,5er Fase
ear_screw_d = 8;     // QUELLE: Aufgabe: Zierschraube Ø 8
ear_deco_in = 1.0;   // Einsatztiefe in der Ohrscheibe
ear_deco_h  = 0.8;   // erhaben (VORLÄUFIG, Geschmack)
ear_y       = 22;    // Ohr-Mitte Y: so weit oben, dass die Scheibe (Ø30) die USB-C-Oeffnung (Y bis +5) freilaesst
boss_w      = 1.6;   // Wand des Stegs um das Ohrloch

// ---------------------------------------------------------------- Hals (Schnittstelle zu Task 3)
neck_d       = 22;   // QUELLE: Spec: Rohr Ø 22
neck_flange_d = 24;  // VORLÄUFIG (mit Task 3 abstimmen): Ø des Halsflansches
neck_bolt_x  = 9;    // VORLÄUFIG (mit Task 3 abstimmen): 2 M3-Kernloecher bei X = +-9 (liegen im Rohrring, Rohr Ø22 innen >= 10)
cable_d      = 12;   // Kabelloch (>= 10, JST-Stecker passt hindurch)
pad_h        = 8;    // Halssockel steht so weit unten aus dem Kopf
bore_t       = 3;    // Tiefe der Flanschbohrung im Sockel
neck_pt      = [0, -8, 82];  // QUELLE: Konzept: Anschlusspunkt des Kopfes in der Weltlage (Mitte Sockelflaeche)

// ================================================================= abgeleitete Werte
a_lip     = (wall - fit) / 2;                 // Aussenhaelfte der Wand im Falzbereich
hx        = head_w / 2;
hy        = head_h / 2;
hx_in     = hx - wall;
z_split   = z_pcb_bot;
z_floor   = z_rear - rear_clear - rear_air;
z_back    = z_floor - back_t;
dome_y    = pcb_h / 2 + fit + dome_d / 2 + 0.1;
dome_pts  = [for (sx = [-1, 1], sy = [-1, 1]) [sx * dome_x, sy * dome_y]];
z_neck    = (z_back + z_split) / 2;           // Hals-Mitte in Z (Konstruktionslage)
pad_w     = neck_flange_d + 4;
ear_z     = z_back / 2;                       // Ohr-Mitte in Z: Mitte der Kopftiefe
// Innenflaeche folgt der 45-Grad-Fase parallel (Wanddicke senkrecht = wall): Einzug d(z) = ch + z + wall*sqrt(2)
d_cf      = ch - front_t + wall * sqrt(2);    // Einzug des Hohlraums unter der Frontplatte
z_cf      = wall - ch - wall * sqrt(2);       // ab hier ist der Einzug wieder = wall
d_bf      = ch - back_t + wall * sqrt(2);     // Einzug des Hohlraums ueber der Rueckplatte
z_bf      = z_back + ch + wall * (sqrt(2) - 1); // ab hier ist der Einzug wieder = wall
hole_ear  = peg_d + fit_peg;
assert(hx_in >= pcb_x1 + sock_over + usb_clear, "Kopf zu schmal fuer die Platine");
assert(z_split - z_back >= neck_flange_d + fit + 3, "Rueckwand zu flach fuer den Halssockel");

// ================================================================= 2D-Formen
module outer2d() { rrect(hx, hy, head_r); }
module inset(d) { offset(r = -d) outer2d(); }
module ring2d() { difference() { rrect(bz_hx - ridge_in, bz_hy - ridge_in, bz_r - ridge_in); rrect(bz_hx - ridge_in - ridge_w, bz_hy - ridge_in - ridge_w, bz_r - ridge_in - ridge_w); } }
// Tropfenform: Loch mit 45-Grad-Dach (Spitze nach +v), druckbar ohne Stuetzen. clipw > 0: Spitze auf diese Dachbreite kappen.
module tear2d(d, clipw = 0) {
  r = d / 2;
  intersection() {
    union() { circle(d = d); rotate(45) square(r); }
    if (clipw > 0) translate([-d, -d]) square([2 * d, d + r * sqrt(2) - clipw / 2]);
  }
}

// Extrusion mit exakter 45-Grad-Fase (konvexe Form: Huelle zweier Platten)
module chamf_solid(z0, z1, c, at_top = true) {
  if (at_top) hull() {
    translate([0, 0, z0]) linear_extrude(z1 - z0 - c) outer2d();
    translate([0, 0, z1 - 0.001]) linear_extrude(0.001) inset(c);
  } else hull() {
    translate([0, 0, z0 + c]) linear_extrude(z1 - z0 - c) outer2d();
    translate([0, 0, z0]) linear_extrude(0.001) inset(c);
  }
}

// ================================================================= Teile (Konstruktionslage)
module usb_cuts() {
  for (u = usb_list) if (u[4])
    translate([pcb_x1 - 3, u[0] - u[2] / 2, usb_zc - usb_open_h / 2])
      cube([hx + 3 - pcb_x1 + 1, u[2], usb_open_h]);
}

module front_cavity() {
  hull() {
    translate([0, 0, z_split - 0.01]) linear_extrude(z_cf - z_split + 0.01) inset(wall);
    translate([0, 0, -front_t - 0.01]) linear_extrude(0.01) inset(d_cf);
  }
}

module front_shell() {
  difference() {
    union() {
      difference() {
        chamf_solid(z_split, 0, ch, true);
        // Falz-Aussparung (Aussenhaelfte der Wand bleibt)
        translate([0, 0, z_split - 0.01]) linear_extrude(lip_h + 0.01) offset(r = -a_lip) outer2d();
        front_cavity();
      }
      // Schraubdome: von der Frontplatte bis zur Innenseite der Rueckwand
      for (p = dome_pts) translate([p[0], p[1], z_floor]) cylinder(d = dome_d, h = -front_t / 2 - z_floor);
    }
    window_cut();
    // Nut fuer den Steg der Blende (2 mm breit, Bruecke 2 mm)
    translate([0, 0, -nut_t]) linear_extrude(nut_t + 0.01) offset(delta = fit_peg) ring2d();
    // Einsaetze fuer die Zierschrauben
    for (p = deco_pts) translate([p[0], p[1], -deco_in]) cylinder(d = deco_d + fit_peg, h = deco_in + 0.01);
    usb_cuts();
    for (p = dome_pts) translate([p[0], p[1], z_floor - 0.01]) cylinder(d = pilot_d, h = pilot_len);
  }
}

// Fenster der Blende: setzt die Fase von window_cut() (win_bevel) durch die Blendendicke fort
module bezel_window() {
  s = tan(win_bevel);
  translate([disp_x + win_off_x, win_off_y, 0]) hull() {
    translate([0, 0, -nut_t - 0.01]) linear_extrude(0.01) rrect(win_w / 2 + (-nut_t + front_t) * s, win_h / 2 + (-nut_t + front_t) * s, win_r);
    translate([0, 0, bezel_t]) linear_extrude(0.01) rrect(win_w / 2 + (bezel_t + front_t + 0.01) * s, win_h / 2 + (bezel_t + front_t + 0.01) * s, win_r);
  }
}
module bezel() {
  difference() {
    union() {
      linear_extrude(bezel_t) rrect(bz_hx, bz_hy, bz_r);
      translate([0, 0, -nut_t + 0.1]) linear_extrude(nut_t - 0.1 + 0.01) ring2d();   // Steg, 0,1 kuerzer als die Nut tief ist
    }
    bezel_window();
  }
}

module ear_boss_solid() {
  // Steg um das Ohrloch: Huelle aus Rohr und Fusspunkt auf der Rueckplatte (keine Ueberhaenge)
  bd = hole_ear + 2 * boss_w;
  x0 = hx - peg_l - 0.3 - 1.5;
  hull() {
    translate([x0, ear_y, ear_z]) rotate([0, 90, 0]) cylinder(d = bd, h = hx_in - x0 + 0.5);
    translate([x0, ear_y - bd / 2, z_floor - 0.01]) cube([hx_in - x0 + 0.5, bd, 0.01]);
  }
}
module ear_hole() {
  // Loch mit Abflachung unten (-Z) und 45-Grad-Dach oben (+Z)
  translate([hx - peg_l - 0.3, ear_y, ear_z]) rotate([90, 0, 90]) linear_extrude(peg_l + 0.3 + 1)
    intersection() {
      tear2d(hole_ear);
      translate([-hole_ear, -(hole_ear / 2 - peg_flat)]) square([2 * hole_ear, 2 * hole_ear]);
    }
}

module back_cavity() {
  hull() {
    translate([0, 0, z_floor]) linear_extrude(0.01) inset(d_bf);
    translate([0, 0, z_bf]) linear_extrude(z_split + 0.01 - z_bf) inset(wall);
  }
}

module head_back() {
  difference() {
    union() {
      difference() {
        chamf_solid(z_back, z_split, ch, false);
        back_cavity();
      }
      // Falz
      translate([0, 0, z_split - 0.01]) linear_extrude(lip_h - 0.2 + 0.01)
        difference() {
          offset(r = -(a_lip + fit)) outer2d();
          offset(r = -wall) outer2d();
          for (p = dome_pts) translate(p) circle(d = dome_d + 0.5);
        }
      // Platinen-Andruckfuesse mit Zentrierstift (wie Stufe 2)
      for (p = hole_pts) translate([p[0], p[1], z_floor - 0.01]) {
        cylinder(d = 6.4, h = z_pcb_bot - 0.1 - z_floor + 0.01);
        cylinder(d = hole_d - 0.4, h = z_pcb_bot - 0.1 - z_floor + 0.01 + (pcb_t - 0.2));
      }
      // Ohr-Stege innen
      for (m = [0, 1]) mirror([m, 0, 0]) ear_boss_solid();
      // Halssockel unten: Klotz von der Rueckplatte bis zur Trennebene (keine Ueberhaenge)
      translate([-pad_w / 2, -hy - pad_h, z_back]) cube([pad_w, pad_h + wall - 0.5, z_split - z_back]);
    }
    // Schrauben von hinten
    for (p = dome_pts) {
      translate([p[0], p[1], z_back - 1]) cylinder(d = screw_d, h = back_t + 2);
      translate([p[0], p[1], z_back - 0.01]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    usb_cuts();
    // BOOT/RESET
    translate([btn_x - btn_open_w / 2, -pcb_h / 2 + btn_y_from_bottom - btn_open_h / 2, z_back - 1])
      cube([btn_open_w, btn_open_h, back_t + 2]);
    // Ohrloecher
    for (m = [0, 1]) mirror([m, 0, 0]) ear_hole();
    // Halssockel: Flanschbohrung (Dach auf 12 mm gekappt), Kabelloch, 2 M3-Kernloecher
    translate([0, -hy - pad_h + bore_t, z_neck]) rotate([90, 0, 0]) linear_extrude(bore_t + 0.01)
      tear2d(neck_flange_d + fit, 12);
    translate([0, -hy + wall + 1, z_neck]) rotate([90, 0, 0]) linear_extrude(wall + pad_h + 2) tear2d(cable_d);
    for (sx = [-1, 1]) translate([sx * neck_bolt_x, -hy - pad_h + bore_t + 0.01, z_neck]) rotate([90, 0, 0])
      cylinder(d = pilot_d, h = 6.5);
  }
}

module ear() {
  // Konstruktionslage des Ohrs: aussen z = 0 (Zierschraube erhaben nach -z), Scheibe z 0..ear_t, Zapfen z ear_t..ear_t+peg_l,
  // Abflachung am Zapfen bei lokal -X
  difference() {
    union() {
      hull() {
        cylinder(d = ear_d - 2, h = 0.01);
        translate([0, 0, 1]) cylinder(d = ear_d, h = ear_t - 1);
      }
      intersection() {
        translate([0, 0, ear_t - 0.01]) union() {
          cylinder(d = peg_d, h = peg_l - peg_ch + 0.01);
          translate([0, 0, peg_l - peg_ch]) cylinder(d1 = peg_d, d2 = peg_d - 2 * peg_ch, h = peg_ch);
        }
        translate([-(peg_d / 2 - peg_flat), -peg_d, ear_t - 1]) cube([2 * peg_d, 2 * peg_d, peg_l + 2]);
      }
    }
    translate([0, 0, -0.01]) cylinder(d = ear_screw_d + fit_peg, h = ear_deco_in + 0.01);
  }
}
module ear_deco()  { cylinder(d = ear_screw_d, h = ear_deco_in + ear_deco_h - 0.2); translate([0, 0, ear_deco_in + ear_deco_h - 0.2]) cylinder(d1 = ear_screw_d, d2 = ear_screw_d - 0.4, h = 0.2); }
module head_deco() { cylinder(d = deco_d, h = deco_in + deco_h - 0.2); translate([0, 0, deco_in + deco_h - 0.2]) cylinder(d1 = deco_d, d2 = deco_d - 0.4, h = 0.2); }

// Einbaulage der beiden Ohren (Konstruktionslage des Kopfes). Rechts: Zapfen zeigt nach -X, links nach +X, Abflachung beide nach -Z.
module ear_xf_right() { translate([hx + ear_t, ear_y, ear_z]) rotate([0, -90, 0]) children(); }
module ear_xf_left()  { translate([-(hx + ear_t), ear_y, ear_z]) rotate([0, 90, 0]) rotate([0, 0, 180]) children(); }
module ear_right() { ear_xf_right() ear(); }
module ear_left()  { translate([-(hx + ear_t), ear_y, ear_z]) rotate([0, 90, 0]) rotate([0, 0, 180]) ear(); }
module ears() { ear_right(); ear_left(); }
// Zierschrauben in Einbaulage: Kopf (Gesichtsebene) und Ohren
module decos() {
  for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco();
  ear_xf_right() ear_deco_in_ear();
  ear_xf_left() ear_deco_in_ear();
}
module ear_deco_in_ear() { translate([0, 0, ear_deco_in]) mirror([0, 0, 1]) ear_deco(); }

// ================================================================= Pruefkoerper (Dummies) in Konstruktionslage
module plugs_dummy() {
  // nur dort, wo eine Gehaeuseoeffnung ist
  for (u = usb_list) if (u[4])
    translate([pcb_x1 + sock_over, u[0] - u[3] / 2, usb_zc - plug_h / 2]) cube([hx + 25, u[3], plug_h]);
}
module head_dummies() {
  pcb_dummy(); glass_dummy(); rear_dummy(); sockets_dummy(); plugs_dummy(); buttons_dummy();
  active_cone(); sd_dummy();
}
module head_shells() { front_shell(); bezel(); head_back(); ears(); decos(); }

// ================================================================= Weltlage
// Konstruktionslage -> Welt: Sockelmitte (0, -hy-pad_h, z_neck) auf neck_pt, Gesicht nach -Y, um head_tilt nach hinten geneigt
module place_head() {
  translate(neck_pt) rotate([-head_tilt, 0, 0]) rotate([90, 0, 0]) translate([0, hy + pad_h, -z_neck]) children();
}
function head_w(p) = let(q = [p[0], -(p[2] - z_neck), p[1] + hy + pad_h], c = cos(head_tilt), s = sin(head_tilt))
  neck_pt + [q[0], q[1] * c + q[2] * s, -q[1] * s + q[2] * c];

module shells()   { place_head() head_shells(); }     // Task 3/4: weitere Gehaeuseteile hier ergaenzen
module dummies()  { place_head() head_dummies(); }    // Task 3/4: weitere Pruefkoerper hier ergaenzen

// ================================================================= Schwerpunkt / Standsicherheit (Schaetzung)
// Massen in Konstruktionslage des Kopfes [g, x, y, z], VORLÄUFIG (Schaetzung)
head_masses_c = [
  [45, 0, 0, z_split / 2],                    // Frontschale + Dome
  [6, 0, 0, 0.3],                             // Blende
  [60, 0, -2, (z_back + z_split) / 2],        // Rueckwand + Sockel
  [12, hx + 4, ear_y, ear_z],                 // Ohr rechts
  [12, -hx - 4, ear_y, ear_z],                // Ohr links
  [25, 0, 0, (z_pcb_bot + z_rear) / 2],       // Platine mit Bauteilen
  [10, 0, 0, z_glass_bot + disp_h / 2]        // Display
];
function to_world(m) = let(w = head_w([m[1], m[2], m[3]])) [m[0], w[0], w[1], w[2]];
// [g, x, y, z] in Weltkoordinaten. Task 3/4 haengen ihre Listen mit concat(...) an.
masses = concat([for (m = head_masses_c) to_world(m)]);
function msum(i, k = 0) = k >= len(masses) ? 0 : (i == 0 ? masses[k][0] : masses[k][0] * masses[k][i]) + msum(i, k + 1);
mtot  = msum(0);
com_x = msum(1) / mtot;
com_y = msum(2) / mtot;
com_z = msum(3) / mtot;

stand_min = 5;      // QUELLE: Spec: Lot des Schwerpunkts >= 5 mm innerhalb der Sitzflaeche
seat_poly = [];     // Task 4: konvexe Sitzflaeche in Welt-XY, gegen den Uhrzeigersinn [[x,y],...]. Leer = Standpruefung uebersprungen.
has_seat  = len(seat_poly) > 2;
function edge_d(a, b, p) = ((b[0] - a[0]) * (p[1] - a[1]) - (b[1] - a[1]) * (p[0] - a[0])) / norm(b - a);
function stand_margin(p) = min([for (i = [0 : len(seat_poly) - 1]) edge_d(seat_poly[i], seat_poly[(i + 1) % len(seat_poly)], p)]);

module info() {
  echo(str("HEAD_W=", head_w, " HEAD_H=", head_h, " DEPTH=", -z_back, " SPLIT=", z_split, " NECK_Z=", z_neck));
  echo(str("MASS=", mtot, " COM=[", com_x, ",", com_y, ",", com_z, "]"));
  echo(str("WINDOW=", win_w, "x", win_h));
  if (has_seat) echo(str("STAND_MIN_MARGIN=", stand_margin([com_x, com_y]), " STAND_MIN=", stand_min));
  else echo("STAND_SKIPPED=kein seat vorhanden");
  cube(1);
}

// ================================================================= Ansichten
module colored_head() {
  color("#3a7bd5") { front_shell(); head_back(); }
  color("#161616") bezel();
  color("#ffb000") ears();
  color("#c0c0c0") decos();
}
module colored_dummies() {
  color("#2e8b4a", 0.6) pcb_dummy();
  color("#0b1a2a", 0.9) glass_dummy();
  color("#888888", 0.5) rear_dummy();
  color("#c0c0c0") { sockets_dummy(); buttons_dummy(); sd_dummy(); }
}
module assembly_parts() {
  place_head() { colored_head(); if (show_dummies) colored_dummies(); }
}
module exploded_parts() {
  place_head() {
    translate([0, 0, 28]) color("#161616") bezel();
    translate([0, 0, 14]) color("#c0c0c0") decos_head_only();
    color("#3a7bd5") front_shell();
    translate([0, 0, -34]) color("#3a7bd5") head_back();
    translate([30, 0, 0]) color("#ffb000") ear_right();
    translate([-30, 0, 0]) color("#ffb000") ear_left();
    if (show_dummies) translate([0, 0, -16]) colored_dummies();
  }
}
module decos_head_only() { for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco(); }

// ================================================================= Ausgabe
// Druckausrichtung: Gesicht bzw. Aussenseite nach unten, kein Teil braucht Stuetzen.
if (part == "head_front") rotate([180, 0, 0]) front_shell();                // Gesicht (Z 0) auf dem Bett
else if (part == "bezel") translate([0, 0, bezel_t]) rotate([180, 0, 0]) bezel(); // Blendenoberseite auf dem Bett, Steg nach oben
else if (part == "head_back") translate([0, 0, -z_back]) head_back();        // Rueckseite (Z back) auf dem Bett
else if (part == "ear") ear();                                                 // Aussenseite auf dem Bett, Zapfen nach oben
else if (part == "deco") {                                                     // Einsaetze flach: 4 Kopf-, 2 Ohr-Zierschrauben
  for (i = [0 : 3]) translate([i * 8, 0, 0]) head_deco();
  for (i = [0 : 1]) translate([i * 12, 12, 0]) ear_deco();
}
else if (part == "assembly") assembly_parts();
else if (part == "exploded") exploded_parts();
else if (part == "section")
  difference() {
    assembly_parts();
    translate([0, -500, -500]) cube(1000);
  }
else if (part == "collision")
  intersection() { shells(); dummies(); }
else if (part == "selftest")
  // Positive-Kontrolle fuer die Kollisionspruefung: absichtlich zu grosse Platine, MUSS Treffer liefern
  intersection() { shells(); place_head() scale([1.1, 1.1, 1]) pcb_dummy(); }
else if (part == "info") info();

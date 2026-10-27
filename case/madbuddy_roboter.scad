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
rear_air    = 17;    // Zusatztiefe hinten (Kabelweg, Halssockel): Rueckwand braucht Bohrung 24,3 + 2 x 2 mm Rand = 28,3 mm Tiefe
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
boss_r_extra = 2.0; // Steg ums Ohrloch: Radius = Tropfenspitze + 2 mm (Wand >= 2)

// ---------------------------------------------------------------- Hals (Schnittstelle zu Task 3)
neck_d       = 22;   // QUELLE: Spec: Rohr Ø 22
neck_flange_d = 24;  // VORLÄUFIG (mit Task 3 abstimmen): Ø des Halsflansches
neck_bolt_x  = 9;    // VORLÄUFIG (mit Task 3 abstimmen): 2 M3-Kernloecher bei X = +-9 (liegen im Rohrring, Rohr Ø22 innen >= 10)
cable_d      = 12;   // Kabelloch (>= 10, JST-Stecker passt hindurch)
pad_h        = 11;   // Halssockel steht so weit unten aus dem Kopf (Kernloch 7,6 + Restboden im Material)
neck_pilot_len = 7.6; // Kernloch M3 im Material (>= 7,5)
bore_t       = 3;  // Tiefe der Flanschbohrung im Sockel
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
pad_w     = neck_flange_d + fit + 4;       // 2 mm Rand links/rechts der Flanschbohrung
ear_z     = z_back / 2;                       // Ohr-Mitte in Z: Mitte der Kopftiefe
// Innenflaeche folgt der 45-Grad-Fase parallel (Wanddicke senkrecht = wall): Einzug d(z) = ch + z + wall*sqrt(2)
d_cf      = ch - front_t + wall * sqrt(2);    // Einzug des Hohlraums unter der Frontplatte
z_cf      = wall - ch - wall * sqrt(2);       // ab hier ist der Einzug wieder = wall
d_bf      = ch - back_t + wall * sqrt(2);     // Einzug des Hohlraums ueber der Rueckplatte
z_bf      = z_back + ch + wall * (sqrt(2) - 1); // ab hier ist der Einzug wieder = wall
hole_ear  = peg_d + fit_peg;
assert(hx_in >= pcb_x1 + sock_over + usb_clear, "Kopf zu schmal fuer die Platine");
bore_d    = neck_flange_d + fit;
bore_roof = bore_d / 2 * sqrt(2) - 6;         // Hoehe der gekappten Tropfenspitze ueber der Mitte (Dachbreite 12)
assert((z_split - z_back) / 2 - bore_d / 2 >= 2, "Halssockel: unter/neben der Bohrung in Z weniger als 2 mm Wand");
assert((z_split - z_back) / 2 - bore_roof >= 2, "Halssockel: Dach ueber der Bohrung unter 2 mm");
assert(pad_w / 2 - bore_d / 2 >= 2, "Halssockel: neben der Bohrung in X weniger als 2 mm Wand");
assert(pad_h - bore_t + wall - 0.5 >= neck_pilot_len + 1.5, "Halssockel: Kernloch zu lang fuer das Material (Restboden < 1,5 mm)");
assert(boss_r_extra >= 2, "Ohrsteg: Wand unter 2 mm");

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

module head_front() {
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
  bd = 2 * (hole_ear / 2 * sqrt(2) + boss_r_extra);
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

module head_back(pilots = true) {
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
    if (pilots) neck_pilots();
  }
}
// 2 M3-Kernloecher: ab Boden der Flanschbohrung nach +Y ins Material (Richtung Kopfinneres)
module neck_pilots() {
  for (sx = [-1, 1]) translate([sx * neck_bolt_x, -hy - pad_h + bore_t - 0.01, z_neck]) rotate([-90, 0, 0])
    cylinder(d = pilot_d, h = neck_pilot_len + 0.01);
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
module head_shells() { head_front(); bezel(); head_back(); ears(); decos(); }

// ================================================================= Weltlage
// Konstruktionslage -> Welt: Sockelmitte (0, -hy-pad_h, z_neck) auf neck_pt, Gesicht nach -Y, um head_tilt nach hinten geneigt
module place_head() {
  translate(neck_pt) rotate([-head_tilt, 0, 0]) rotate([90, 0, 0]) translate([0, hy + pad_h, -z_neck]) children();
}
function head_to_world(p) = let(q = [p[0], -(p[2] - z_neck), p[1] + hy + pad_h], c = cos(head_tilt), s = sin(head_tilt))
  neck_pt + [q[0], q[1] * c + q[2] * s, -q[1] * s + q[2] * c];

// HALSACHSE FUER TASK 3: Achse und Flanschebene des Halses sind um head_tilt gegen die Senkrechte geneigt (Kopf lehnt nach hinten),
// weil der Sockel fest am Kopf sitzt. neck_axis_w zeigt vom Sockel in den Kopf (nach oben und um head_tilt nach hinten, +Y).
// Die Flanschebene steht senkrecht auf dieser Achse und geht durch neck_pt. Der Hals unten (Oberkoerper) ist dagegen waagerecht:
// der Hals braucht entweder eine Schraegung/Knick oder der Kopf-Flansch wird gegen den Oberkoerper-Flansch um head_tilt gekippt.
neck_axis_w = [0, sin(head_tilt), cos(head_tilt)];
// Lokales Koordinatensystem am Halsflansch: Ursprung neck_pt, lokales +Z = neck_axis_w, lokales Y = Weltrichtung "hinten" in der Flanschebene.
module neck_frame() { translate(neck_pt) rotate([-head_tilt, 0, 0]) children(); }

// ================================================================= Oberkoerper, Hals, Arme, Haende (Task 3)
// Oberkoerper-Konstruktionslage = Weltlage um torso_origin verschoben (place_torso): X rechts, Y nach hinten, Z oben.
// Lokaler Ursprung = Mitte der Unterkante. Teilungsebene y = 0 (Mitte): vorn = Brust mit Kamera, hinten = Rueckwand.
t_w        = 64;     // QUELLE: Konzept (t_w): Breite
t_d        = 44;     // QUELLE: Konzept (t_d): Tiefe
t_h        = 48;     // QUELLE: Konzept (t_h): Hoehe
t_r        = 10;     // QUELLE: Konzept (t_r): Eckradius oben (Frontansicht)
t_ch       = 3.5;    // 45-Grad-Fase an Brust und Rueckseite (ohne Stuetzen druckbar)
torso_origin = [0, -8, 28];   // QUELLE: Konzept: [0, t_y - t_d/2, leg_h - 4] = Mitte der Unterkante des Oberkoerpers in Weltlage (Task 4 baut darauf auf)
t_plate_z  = 4;      // Bodenplatte liegt 4 mm ueber der Unterkante: darunter ist die Steckkante (Rand), in die der Sitz (Task 4) greift
t_cable_d  = 11;     // Kabeldurchfuehrung unten (>= 10)
t_dome_pts = [[-16, 10], [16, 10], [-16, 41], [16, 41]];   // M3-Dome (Teilung vorn/hinten), Lage X/Z
t_dome_d   = 6.4;
t_dome_pilot = 10;   // Kernloch im vorderen Dom (>= 7,5)
t_seat_pts = [for (sx = [-1, 1], sy = [-1, 1]) [sx * 24, sy * 12]];   // 4 M3-Kernloecher unten fuer den Sitz (X/Y), Schrauben von unten
t_seat_pilot = 7.6;
t_seat_blk_z = 13.5; // Oberkante der Pfosten ueber der Unterkante
t_deco_pts = [for (sx = [-1, 1], z = [12, 40.8]) [sx * 23, z]];   // QUELLE: Konzept: Brust-Schrauben (Einlagen)
// --- Kamera / XIAO: einfach gehalten und parametrisch, die Halterung wird nach Task 4 neu bestimmt
cam_tilt   = 15;     // QUELLE: Aufgabe: Kamera nach oben gekippt
cam_z      = 24;     // Linsenhoehe ueber der Unterkante (Konzept 0,6 * t_h; tiefer, damit der Halsblock Platz hat)
xiao_w     = 21.0;   // VORLÄUFIG (Datenblatt)
xiao_h     = 17.8;   // VORLÄUFIG (Datenblatt)
xiao_stack = 15.0;   // VORLÄUFIG (Datenblatt) Stapelhoehe inkl. Linsentubus
xiao_body_d = xiao_stack - 2.2;  // Platinen-/Modultiefe hinter der Frontflaeche (wie Stufe 2: Stapel - Platte - 0,2)
lens_d     = 8.0;    // VORLÄUFIG (Datenblatt) Linse
lens_hole  = 9.0;    // VORLÄUFIG (Datenblatt) Linsenloch
lens_fov   = 30;     // VORLÄUFIG (Datenblatt) halber Bildwinkel Freiraumkegel
rib_t      = 1.6;    // Klemmrippen links/rechts (wie Stufe 2)
rib_h      = 14;     // Rippenhoehe (Z)
rib_len    = 14;     // Rippenlaenge (Y)
stop_w     = 3.5;    // Anschlagpfosten unter dem XIAO (wie Stufe 2)
stop_x     = 7.75;
xiao_face  = 5.1;    // Abstand Frontflaeche XIAO hinter der Aussenebene entlang der Linsenachse: gerechnet, damit die untere Vorderkante der gekippten Platte hinter der Brustinnenwand liegt
// --- Hals
neck_lo_d  = 36;     // Ø unterer Flansch (Schrauben bei y = +-14,3, ausserhalb des Kopfsockels)
neck_fl_t  = 2.8;    // Flanschdicke (Bohrung im Kopfsockel ist 3 tief, 0,2 Luft)
neck_bore  = 12;     // QUELLE: Aufgabe: Innen-Ø >= 12
neck_lobe_d = 6.6;   // Schraubenkopf-Tasche am oberen Flansch (Zugang von unten durch den Oberkoerper)
neck_lo_bolt_y = 14.3;
neck_rec_floor = 45; // Boden der Flanschmulde im Oberkoerper (Mulde 3 tief)
neck_lo_z  = neck_rec_floor + 0.05;
neck_blk_z = 35;     // Unterkante des Halsblocks im Oberkoerper
neck_pilot_t = 7.6;  // Kernloch M3 im Halsblock (>= 7,5)
// --- Arm / Hand (Weltlage)
arm_x      = 40.5;   // Armebene |x|: Schulterkugel 1 mm neben der Seitenwand (Konzept 37, dort ragte die Kugel in den Koerper)
ball_r     = 8.5;    // QUELLE: Konzept: Schulterkugel R 8,5
ball_in    = 7.5;    // Abflachung innen (zum Koerper)
arm_flat   = 4.95;   // Abflachung aussen = Druckflaeche (45-Grad-Flanke, der Arm druckt auf der Aussenseite)
arm_r1     = 7;      // QUELLE: Konzept: Arm-Hull r 7 an der Schulter
arm_r2     = 6;      // QUELLE: Konzept: r 6 an der Hand
hand_r     = 8;      // QUELLE: Konzept: Hand scale([1,1.3,0.8]) sphere(r=8)
hand_sc    = [1, 1.3, 0.8];
hand_cut   = 3.3;    // Abflachung unten (Hand liegt auf dem Oberschenkel, Flanke >= 45 Grad)
hand_contact_z = 32; // QUELLE: Konzept: leg_h = Oberflaeche der Oberschenkel unter den Haenden (Task 4)
hand_c_y   = -31;    // Hand-Mitte Y (Weltlage)
arm_S      = [arm_x, 0, hand_contact_z + hand_cut + 31];       // Schulter, Welt (rechter Arm); Arm 45 Grad nach vorn unten
arm_H      = [arm_x, hand_c_y, hand_contact_z + hand_cut];     // Hand-Mitte, Welt (rechter Arm)
arm_d      = [0, -sqrt(0.5), -sqrt(0.5)];                      // Achsrichtung Schulter -> Hand
arm_peg_d  = 5;      // QUELLE: Aufgabe: Zapfen Ø 5 fuer die Hand
arm_s_end  = -9;     // Armende: Abstand zur Hand-Mitte entlang der Achse (Ellipsoid-Stuetzwert 8,63 + 0,37)
arm_peg_tip = -1.3;
hand_hole_end = -1;
arm_sh_peg_l = 7;    // Schulterzapfen Ø 6 (peg_d), Laenge ab Innenflaeche
sh_bore_depth = 6.5;
sh_y_l     = arm_S[1] - torso_origin[1];   // Schulter lokal Y
sh_z_l     = arm_S[2] - torso_origin[2];   // Schulter lokal Z
side       = 1;      // Druckzweig arm/hand: 1 = rechts, -1 = links (gespiegelt)

d_cf_t = t_ch - wall + wall * sqrt(2);
y_cf_t = t_ch + wall * (sqrt(2) - 1);
assert(t_cable_d >= 10, "Kabeldurchfuehrung unten < 10");
assert(neck_bore >= 12, "Hals Innen-Ø < 12");
assert(neck_rec_floor - neck_pilot_t - neck_blk_z >= 1.5, "Halsblock: Restboden unter Kernloch < 1,5");
assert(t_seat_blk_z - t_plate_z - t_seat_pilot >= 1.5, "Sitzpfosten: Kernloch zu lang");
assert(t_d / 2 - wall >= t_dome_pilot + 1.5, "Dom: Kernloch zu lang");
assert(t_dome_pilot >= 7.5, "Dom-Kernloch < 7,5");
assert(sh_y_l - hole_ear / 2 * sqrt(2) - 1.0 >= 2, "Schulterbohrung: Wand zur Trennebene unter 2 mm");
assert(neck_d / 2 - neck_bore / 2 >= 2, "Halsrohr: Wand unter 2 mm");
assert(arm_x - ball_in > t_w / 2, "Schulterkugel ragt in den Oberkoerper");
assert(arm_sh_peg_l >= 6 && arm_peg_tip - arm_s_end >= 6, "Zapfen kuerzer als 6 mm");

module place_torso() { translate(torso_origin) children(); }
module t_out_low() { hull() { translate([-t_w / 2, -10]) square([t_w, 11]); for (sx = [-1, 1]) translate([sx * (t_w / 2 - t_r), t_h - t_r]) circle(r = t_r); } }
module t_outline() { hull() { translate([-t_w / 2, 0]) square([t_w, 1]); for (sx = [-1, 1]) translate([sx * (t_w / 2 - t_r), t_h - t_r]) circle(r = t_r); } }
module t_inset(d) { offset(r = -d) t_outline(); }
module t_cav2d(d) { offset(r = -d) t_out_low(); }
// 2D-Form (X/Z) entlang Y extrudieren: belegt y0..y1
module t_ext(y0, y1) { translate([0, y1, 0]) rotate([90, 0, 0]) linear_extrude(y1 - y0) children(); }
module t_solid() {
  hull() {
    t_ext(-t_d / 2 + t_ch, t_d / 2 - t_ch) t_outline();
    t_ext(-t_d / 2, -t_d / 2 + 0.001) t_inset(t_ch);
    t_ext(t_d / 2 - 0.001, t_d / 2) t_inset(t_ch);
  }
}
module t_cavity() {
  hull() for (m = [0, 1]) mirror([0, m, 0]) {
    t_ext(-t_d / 2 + wall - 0.01, -t_d / 2 + wall) t_cav2d(d_cf_t);
    t_ext(-t_d / 2 + y_cf_t - 0.01, -t_d / 2 + y_cf_t) t_cav2d(wall);
  }
}
module t_half(front) { intersection() { t_solid(); translate([-50, front ? -t_d / 2 - 1 : 0, -10]) cube([100, t_d / 2 + 1, 100]); } }
// Innenkontur des Steckrandes (XY-Schnitt bei z < t_plate_z), fuer Task 4: der Sitz greift mit fit Spiel hinein
module torso_pocket2d() {
  hxp = t_w / 2 - wall; hyp = t_d / 2 - wall;
  polygon([[t_w / 2 - d_cf_t, -hyp], [hxp, -(t_d / 2 - y_cf_t)], [hxp, t_d / 2 - y_cf_t], [t_w / 2 - d_cf_t, hyp],
           [-(t_w / 2 - d_cf_t), hyp], [-hxp, t_d / 2 - y_cf_t], [-hxp, -(t_d / 2 - y_cf_t)], [-(t_w / 2 - d_cf_t), -hyp]]);
}

// ---- Kamera
module cam_frame() { translate([0, -t_d / 2, cam_z]) rotate([90 - cam_tilt, 0, 0]) children(); }
module xiao_dummy_l() cam_frame() {
  translate([-xiao_w / 2, -xiao_h / 2, -xiao_face - xiao_body_d]) cube([xiao_w, xiao_h, xiao_body_d]);
  translate([0, 0, -xiao_face - 0.01]) cylinder(d = lens_d, h = xiao_face + 0.01);
}
module lens_cone_l() cam_frame() hull() {
  translate([0, 0, -0.5]) cylinder(d = lens_d, h = 0.5);
  translate([0, 0, 30]) cylinder(r = lens_d / 2 + 30 * tan(lens_fov), h = 0.01);
}
module xiao_dummy_r() { place_torso() xiao_dummy_l(); }
module lens_cone_r()  { place_torso() lens_cone_l(); }
module cam_mount() {   // Klemmrippen + Anschlag, nur Vorderhaelfte
  for (sx = [-1, 1]) translate([sx * (xiao_w / 2 + 0.15 + rib_t / 2) - rib_t / 2, -t_d / 2 + wall - 0.1, cam_z - rib_h / 2]) cube([rib_t, rib_len, rib_h]);
  for (sx = [-1, 1]) cam_frame() translate([sx * stop_x - stop_w / 2, -xiao_h / 2 - 3.4, -xiao_face - xiao_body_d]) cube([stop_w, 3.3, xiao_body_d + 0.5]);
}
module lens_cut() cam_frame() {
  translate([0, 0, -6]) cylinder(d = lens_hole, h = 10);
  hull() { translate([0, 0, -1]) cylinder(d = lens_hole, h = 0.01); translate([0, 0, 5]) cylinder(d = lens_hole + 12, h = 0.01); }   // 45-Grad-Senkung aussen
}

// ---- Einbauten
module t_adds() {
  for (p = t_dome_pts) translate([p[0], t_d / 2 - 0.5, p[1]]) rotate([90, 0, 0]) cylinder(d = t_dome_d, h = t_d - 1);
  translate([-18, -t_d / 2, neck_blk_z]) cube([36, t_d, t_h - neck_blk_z]);                  // Halsblock
  for (sx = [-1, 1]) translate([sx > 0 ? t_w / 2 - 8.5 : -t_w / 2, 1.0, sh_z_l - 5.1]) cube([8.5, t_d / 2 - 1.0 - 0.5, 10.2]); // Schulterbosse bis zur Rueckwand
  for (p = t_seat_pts) translate([p[0] - 3.5, p[1] < 0 ? -t_d / 2 + 0.5 : p[1] - 3.5, t_plate_z])
    cube([7, p[1] < 0 ? t_d / 2 - 0.5 + p[1] + 3.5 : t_d / 2 - 0.5 - p[1] + 3.5, t_seat_blk_z - t_plate_z]);
  cam_mount();
}
module neck_pilots_t()  { for (sy = [-1, 1]) translate([0, sy * neck_lo_bolt_y, neck_rec_floor - neck_pilot_t]) cylinder(d = pilot_d, h = neck_pilot_t + 0.01); }
module seat_pilots_t()  { for (p = t_seat_pts) translate([p[0], p[1], t_plate_z - 0.01]) cylinder(d = pilot_d, h = t_seat_pilot + 0.01); }
module dome_pilots_t()  { for (p = t_dome_pts) translate([p[0], 0.01, p[1]]) rotate([90, 0, 0]) cylinder(d = pilot_d, h = t_dome_pilot + 0.01); }
module sh_bore2d() { intersection() { rotate(90) tear2d(hole_ear); translate([-hole_ear, -hole_ear]) square([hole_ear + hole_ear / 2 - peg_flat, 2 * hole_ear]); } }
module sh_bores() for (m = [0, 1]) mirror([m, 0, 0]) translate([t_w / 2 - sh_bore_depth, sh_y_l, sh_z_l]) rotate([90, 0, 90]) linear_extrude(sh_bore_depth + 1) sh_bore2d();
module dumb2d() { circle(d = neck_bore); for (sx = [-1, 1]) translate([sx * neck_bolt_x, 0]) circle(d = neck_lobe_d); }
module neck_cuts() {
  translate([0, 0, neck_rec_floor]) cylinder(d = neck_lo_d + fit, h = 5);
  translate([0, 0, neck_blk_z - 1]) linear_extrude(neck_rec_floor - neck_blk_z + 1.01) dumb2d();
}
module t_cuts_common(pilots) {
  neck_cuts();
  translate([0, 0, t_plate_z - 1]) cylinder(d = t_cable_d, h = wall + 2);
  if (pilots) { neck_pilots_t(); seat_pilots_t(); }
}
module t_lip_ring() { intersection() { t_ext(-(lip_h - 0.2), 0.01) difference() { t_cav2d(a_lip + fit); t_cav2d(wall); } t_solid(); } }

module torso_front(pilots = true) {
  difference() {
    union() {
      difference() { t_half(true); t_cavity(); }
      intersection() { t_half(true); translate([-50, -50, t_plate_z]) cube([100, 100, wall]); }
      intersection() { t_half(true); t_adds(); }
    }
    t_ext(-lip_h, 0.01) difference() { t_cav2d(a_lip); t_cav2d(wall); }     // Falz: Innenhaelfte der Wand
    lens_cut();
    t_cuts_common(pilots);
    if (pilots) dome_pilots_t();
    for (p = t_deco_pts) translate([p[0], -t_d / 2 - 0.01, p[1]]) rotate([-90, 0, 0]) cylinder(d = deco_d + fit_peg, h = deco_in + 0.01);
  }
}
module torso_back(pilots = true) {
  difference() {
    union() {
      difference() { t_half(false); t_cavity(); }
      intersection() { t_half(false); translate([-50, -50, t_plate_z]) cube([100, 100, wall]); }
      intersection() { t_half(false); t_adds(); }
      t_lip_ring();
    }
    t_cuts_common(pilots);
    sh_bores();
    for (p = t_dome_pts) {
      translate([p[0], t_d / 2 + 1, p[1]]) rotate([90, 0, 0]) cylinder(d = screw_d, h = t_d / 2 + 2);
      translate([p[0], t_d / 2 + 0.01, p[1]]) rotate([90, 0, 0]) cylinder(d = cbore_d, h = cbore_t + 0.01);
    }
    for (x = [-8, -4, 0, 4, 8]) translate([x - 0.8, t_d / 2 - wall - 1, 17]) cube([1.6, wall + 2, 14]);   // Lueftungsschlitze
  }
}
module torso_decos() for (p = t_deco_pts) translate([p[0], -t_d / 2 + deco_in, p[1]]) rotate([90, 0, 0]) head_deco();

// ---- Hals (Lokalkoordinaten des Oberkoerpers; oberer Flansch um head_tilt gekippt, passt zum Kopfsockel)
module neck_up() { translate(neck_pt - torso_origin) rotate([-head_tilt, 0, 0]) children(); }
module neck() {
  difference() {
    union() {
      translate([0, 0, neck_lo_z]) cylinder(d = neck_lo_d, h = neck_fl_t);
      hull() { translate([0, 0, neck_lo_z + neck_fl_t - 0.01]) cylinder(d = neck_d, h = 0.01); neck_up() translate([0, 0, -0.01]) cylinder(d = neck_d, h = 0.01); }
      // oberer Flansch mit Abflachung zur Gesichtsseite (lokal -Y): die Bohrung im Kopfsockel hat dort ein auf 12 mm gekapptes Dach (bore_roof)
      neck_up() intersection() { cylinder(d = neck_flange_d, h = neck_fl_t); translate([-neck_flange_d, -(bore_roof - 0.3), -1]) cube([2 * neck_flange_d, 2 * neck_flange_d, neck_fl_t + 2]); }
    }
    hull() { translate([0, 0, neck_lo_z - 1]) cylinder(d = neck_bore, h = 0.01); neck_up() translate([0, 0, neck_fl_t + 1]) cylinder(d = neck_bore, h = 0.01); }
    for (sx = [-1, 1]) hull() {
      translate([sx * neck_bolt_x, 0, neck_lo_z - 1]) cylinder(d = neck_lobe_d, h = 0.01);
      neck_up() translate([sx * neck_bolt_x, 0, 0]) cylinder(d = neck_lobe_d, h = 0.01);
    }
    for (sx = [-1, 1]) neck_up() translate([sx * neck_bolt_x, 0, -1]) cylinder(d = screw_d, h = neck_fl_t + 2);
    for (sy = [-1, 1]) translate([0, sy * neck_lo_bolt_y, neck_lo_z - 1]) cylinder(d = screw_d, h = neck_fl_t + 2);
  }
}

// ---- Arm und Hand (Weltlage)
module arm_dir() rotate([135, 0, 0]) children();     // lokale z-Achse -> arm_d
module peg2d() { rotate(-90) tear2d(arm_peg_d); rotate(90) tear2d(arm_peg_d); }   // Raute: Spitzen +-X, druckt liegend ohne Stuetzen
module sh_peg() {
  intersection() {
    union() { cylinder(d = peg_d, h = arm_sh_peg_l - peg_ch + 0.01); translate([0, 0, arm_sh_peg_l - peg_ch]) cylinder(d1 = peg_d, d2 = peg_d - 2 * peg_ch, h = peg_ch); }
    translate([-peg_d, -peg_d, -1]) cube([2 * peg_d, peg_d + peg_d / 2 - peg_flat, arm_sh_peg_l + 2]);
  }
}
module arm_r() {
  e0 = arm_H + arm_s_end * arm_d;
  intersection() {
    union() {
      translate(arm_S) sphere(r = ball_r);
      hull() { translate(arm_S) sphere(r = arm_r1); translate(e0) arm_dir() cylinder(r = arm_r2, h = 0.01); }
    }
    translate([arm_x - ball_in, -200, -200]) cube([ball_in + arm_flat, 400, 400]);
  }
  translate(e0 - 0.01 * arm_d) arm_dir() linear_extrude(arm_peg_tip - arm_s_end + 0.01) peg2d();
  translate([arm_x - ball_in + 0.01, arm_S[1], arm_S[2]]) rotate([0, -90, 0]) sh_peg();
}
module arm(s = 1) { if (s > 0) arm_r(); else mirror([1, 0, 0]) arm_r(); }
module hand_l() {   // Mitte im Ursprung, Flaeche unten bei z = -hand_cut
  difference() {
    intersection() { scale(hand_sc) sphere(r = hand_r, $fn = 48); translate([-20, -20, -hand_cut]) cube([40, 40, 40]); }
    translate(-10 * arm_d) arm_dir() linear_extrude(10 + hand_hole_end) offset(delta = fit_peg) peg2d();
  }
}
module hand(s = 1) { translate([s * arm_x, hand_c_y, arm_H[2]]) hand_l(); }
module arm_print() translate([-(arm_S[2] - arm_S[1]) * cos(45), 0, arm_x + arm_flat]) rotate([0, 90, 0]) rotate([-45, 0, 0]) arm_r();

module body_shells() { place_torso() { torso_front(); torso_back(); neck(); torso_decos(); } for (s = [1, -1]) { arm(s); hand(s); } }
module shells()   { place_head() head_shells(); body_shells(); }
module dummies()  { place_head() head_dummies(); xiao_dummy_r(); lens_cone_r(); }

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
function to_world(m) = let(w = head_to_world([m[1], m[2], m[3]])) [m[0], w[0], w[1], w[2]];
// [g, x, y, z] in Weltkoordinaten. Task 3/4 haengen ihre Listen mit concat(...) an.
arm_masses = [for (s = [-1, 1]) each [[8, s * arm_x, (arm_S[1] + arm_H[1]) / 2, (arm_S[2] + arm_H[2]) / 2], [6, s * arm_x, hand_c_y, arm_H[2]]]];
masses = concat([for (m = head_masses_c) to_world(m)],
  [[38, 0, -18, 52], [45, 0, 2, 52], [8, 0, -8, 78], [8, 0, -18, 52]],   // Oberkoerper vorn/hinten, Hals, XIAO
  arm_masses);
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
  echo(str("NECK_AXIS_W=", neck_axis_w, " NECK_PT=", neck_pt, " TILT=", head_tilt));
  if (has_seat) echo(str("STAND_MIN_MARGIN=", stand_margin([com_x, com_y]), " STAND_MIN=", stand_min));
  else echo("STAND_SKIPPED=kein seat vorhanden");
  cube(1);
}

// ================================================================= Ansichten
module colored_head() {
  color("#3a7bd5") { head_front(); head_back(); }
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
  colored_body();
}
module colored_body() {
  place_torso() {
    color("#ff7a29") { torso_front(); torso_back(); }
    color("#263238") neck();
    color("#c0c0c0") torso_decos();
    if (show_dummies) color("#8a4fc9", 0.8) xiao_dummy_l();
  }
  for (s = [1, -1]) { color("#ff7a29") arm(s); color("#263238") hand(s); }
}
module exploded_parts() {
  place_head() {
    translate([0, 0, 28]) color("#161616") bezel();
    translate([0, 0, 14]) color("#c0c0c0") decos_head_only();
    color("#3a7bd5") head_front();
    translate([0, 0, -34]) color("#3a7bd5") head_back();
    translate([30, 0, 0]) color("#ffb000") ear_right();
    translate([-30, 0, 0]) color("#ffb000") ear_left();
    if (show_dummies) translate([0, 0, -16]) colored_dummies();
  }
  place_torso() {
    translate([0, -30, 0]) color("#ff7a29") torso_front();
    translate([0, 30, 0]) color("#ff7a29") torso_back();
    translate([0, 0, 22]) color("#263238") neck();
  }
  for (s = [1, -1]) translate([s * 25, 0, 0]) { color("#ff7a29") arm(s); color("#263238") hand(s); }
}
module decos_head_only() { for (p = deco_pts) translate([p[0], p[1], -deco_in]) head_deco(); }

// ================================================================= Ausgabe
// Druckausrichtung: Gesicht bzw. Aussenseite nach unten, kein Teil braucht Stuetzen.
if (part == "head_front") rotate([180, 0, 0]) head_front();                // Gesicht (Z 0) auf dem Bett
else if (part == "bezel") translate([0, 0, bezel_t]) rotate([180, 0, 0]) bezel(); // Blendenoberseite auf dem Bett, Steg nach oben
else if (part == "head_back") translate([0, 0, -z_back]) head_back();        // Rueckseite (Z back) auf dem Bett
else if (part == "ear") ear();                                                 // Aussenseite auf dem Bett, Zapfen nach oben
else if (part == "deco") {                                                     // Einsaetze flach: 4 Kopf-, 2 Ohr-Zierschrauben
  for (i = [0 : 3]) translate([i * 8, 0, 0]) head_deco();
  for (i = [0 : 1]) translate([i * 12, 12, 0]) ear_deco();
  for (i = [0 : 3]) translate([i * 8, -12, 0]) head_deco();   // Task 3: 4 weitere Einlagen fuer die Brust
}
else if (part == "torso_front") translate([0, 0, t_d / 2]) rotate([90, 0, 0]) torso_front();     // Brust (y = -22) auf dem Bett
else if (part == "torso_back")  translate([0, 0, t_d / 2]) rotate([-90, 0, 0]) torso_back();     // Rueckseite (y = +22) auf dem Bett
else if (part == "neck")        translate([0, 0, -neck_lo_z]) neck();                              // stehend auf dem unteren Flansch
else if (part == "arm") { if (side > 0) arm_print(); else mirror([0, 1, 0]) arm_print(); }       // liegend auf der Aussenseite
else if (part == "hand")        translate([0, 0, hand_cut]) hand_l();                              // abgeflachte Unterseite auf dem Bett
else if (part == "pilot_neck")  intersection() { union() { torso_front(false); torso_back(false); } neck_pilots_t(); }
else if (part == "pilot_seat")  intersection() { union() { torso_front(false); torso_back(false); } seat_pilots_t(); }
else if (part == "pilot_dome")  intersection() { torso_front(false); dome_pilots_t(); }
else if (part == "collision_body") union() {
  intersection() { place_torso() { torso_front(); torso_back(); } union() { arm(1); arm(-1); } }
  intersection() { union() { arm(1); arm(-1); } union() { hand(1); hand(-1); } }
  intersection() { place_torso() neck(); union() { place_torso() { torso_front(); torso_back(); } place_head() head_shells(); } }
  intersection() { place_torso() { torso_front(); torso_back(); neck(); } place_head() head_shells(); }
}
else if (part == "torso_selftest")
  intersection() { shells(); place_torso() cam_frame() scale([1.6, 1.6, 1]) translate([-xiao_w / 2, -xiao_h / 2, -xiao_face - xiao_body_d]) cube([xiao_w, xiao_h, xiao_body_d]); }
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
else if (part == "pilot_test")
  // Nachweis Kernloecher: Bohrvolumen gegen head_back OHNE Kernloecher, muss Material treffen (Y-Ausdehnung >= neck_pilot_len)
  intersection() { head_back(false); neck_pilots(); }
else if (part == "info") info();

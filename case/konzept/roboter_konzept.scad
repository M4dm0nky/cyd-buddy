// Konzept: MadBuddy als sitzender Roboter. Z oben, Gesicht blickt nach -Y.
// Beine = Akkuhalter DFR0969 (103 x 48), Oberkoerper = XIAO-Kamera, Kopf = CYD.
$fn = 24;
cut = false;           // true = Seitenschnitt (zeigt Akku, XIAO, CYD)

// Beine
leg_len = 62; leg_w = 27; leg_h = 32; leg_gap = 2; leg_r = 9;
foot_l = 18; foot_h = 44; foot_w = 44;
// Oberkoerper
t_w = 64; t_d = 44; t_h = 48; t_r = 10; t_y = 14;   // t_y = Rueckseite hinter der Huefte
// Kopf (CYD)
h_w = 114; h_h = 84; h_d = 32; h_r = 14; head_tilt = 8; neck_h = 6;
win_w = 58.2; win_h = 43.8;
cam_tilt = 15;  // Kamera schaut leicht nach oben

module rbox(w, d, h, r) {  // abgerundeter Quader, Ursprung Mitte unten
  hull() for (sx = [-1, 1], sy = [-1, 1]) translate([sx * (w / 2 - r), sy * (d / 2 - r), 0]) cylinder(r = r, h = h);
}
module rbox3(w, d, h, r) {  // rundum verrundet
  hull() for (sx = [-1, 1], sy = [-1, 1], sz = [0, 1])
    translate([sx * (w / 2 - r), sy * (d / 2 - r), r + sz * (h - 2 * r)]) sphere(r = r);
}

// Becken = Akku quer (103 x 48), kurze Stummelbeine nach vorn
p_w = 112; p_d = 58; p_h = 32;
p_y = t_y - t_d / 2;            // Mitte unter dem Oberkoerper
leg_x = 28;                     // Beinmitte links/rechts
module legs() {
  // Zwei dicke Oberschenkel, hinten zusammen = Po (Akku quer darin), vorn leicht schlanker
  for (sx = [-1, 1]) translate([sx * leg_x, 0, 0]) {
    color("#3a7bd5") hull() {
      translate([0, p_y + p_d / 2 - 14, 0]) rbox3(p_w / 2, 28, p_h, 13);
      translate([0, -leg_len + 14, 0]) rbox3(46, 28, 28, 12);
    }
    color("#263238") translate([0, -leg_len - foot_l / 2 + 8, 0]) rbox3(foot_w, foot_l, foot_h, 7);
  }
}

module torso() {
  translate([0, t_y - t_d / 2, leg_h - 4]) {
    color("#ff7a29") rbox3(t_w, t_d, t_h, t_r);
    // Kamera-Linse in der Brust, leicht nach oben gekippt
    translate([0, -t_d / 2, t_h * 0.6]) rotate([90 + cam_tilt, 0, 0]) {
      color("#263238") cylinder(d = 16, h = 3);
      color("#111") cylinder(d = 9, h = 4);
      color("#5ad1ff") translate([0, 0, 4]) cylinder(d = 5, h = 0.4);
    }
    // Brust-Schrauben
    for (sx = [-1, 1], sz = [0.25, 0.85]) translate([sx * (t_w / 2 - 9), -t_d / 2 + 0.6, t_h * sz])
      rotate([90, 0, 0]) color("#c0c0c0") cylinder(d = 3.5, h = 1.2);
  }
}

module arms() {
  for (sx = [-1, 1]) color("#ff7a29") {
    sh = [sx * (t_w / 2 + 5), t_y - t_d / 2, leg_h + t_h - 14];
    hand = [sx * (t_w / 2 + 4), p_y - p_d / 2 + 2, p_h + 4];
    hull() { translate(sh) sphere(r = 7); translate(hand) sphere(r = 6); }
    translate(sh) color("#ffb000") sphere(r = 8.5);
    translate(hand) color("#263238") scale([1, 1.3, 0.8]) sphere(r = 8);
  }
}

module head() {
  translate([0, t_y - t_d / 2, leg_h - 4 + t_h + neck_h]) rotate([-head_tilt, 0, 0]) {
    // Hals
    color("#263238") translate([0, 0, -neck_h - 2]) cylinder(d = 22, h = neck_h + 4);
    translate([0, -h_d / 2 + 6, 0]) {
      difference() {
        color("#3a7bd5") rbox3(h_w, h_d, h_h, h_r - 4);
        translate([0, -h_d / 2 - 1, h_h / 2 - 2]) cube([win_w, 6, win_h], center = true);
      }
      // Display
      color("#0b1a2a") translate([0, -h_d / 2 + 2.2, h_h / 2 - 2]) cube([win_w, 1, win_h], center = true);
      // Augen (wie auf dem Display)
      for (sx = [-1, 1]) color("#5ad1ff") translate([sx * 13, -h_d / 2 + 1.6, h_h / 2]) rotate([90, 0, 0]) cylinder(d = 14, h = 0.3);
      // Ohren mit Schraube
      for (sx = [-1, 1]) translate([sx * (h_w / 2), 0, h_h / 2]) rotate([0, 90, 0]) {
        color("#ffb000") cylinder(d = 30, h = 10, center = true);
        color("#c0c0c0") translate([0, 0, sx * 5.2]) cylinder(d = 8, h = 1.2, center = true);
      }
      // Eck-Schrauben auf der Front
      for (sx = [-1, 1], sz = [-1, 1]) color("#c0c0c0")
        translate([sx * (h_w / 2 - 9), -h_d / 2 + 0.4, h_h / 2 + sz * (h_h / 2 - 9)]) rotate([90, 0, 0]) cylinder(d = 4, h = 1);
    }
  }
}

// Inneres (nur im Schnitt)
module internals() {
  // Akkuhalter: Platine + 2 Zellen in den Oberschenkeln
  color("#111") translate([-24, -103 + t_y - 6, 2.5]) cube([48, 103, 1.6]);
  for (sx = [-1, 1]) color("#21c35b") translate([sx * 11, -95, 4.5 + 9.3]) rotate([-90, 0, 0]) cylinder(d = 18.6, h = 65);
  // XIAO in der Brust
  color("#8a4fc9") translate([-10.5, t_y - t_d + 2, leg_h + t_h * 0.6 - 18]) cube([21, 15, 18]);
}

module ground() color([0.85, 0.85, 0.88]) translate([-120, -170, -2]) cube([240, 240, 2]);

module all() { legs(); torso(); arms(); head(); }

if (cut) {
  internals();
  // CYD-Platine im Kopf
  translate([0, t_y - t_d / 2, leg_h - 4 + t_h + neck_h]) rotate([-head_tilt, 0, 0])
    color("#2e8b4a") translate([-43, -h_d / 2 + 6 - 10, h_h / 2 - 27]) cube([86, 8, 50]);
  %all();
} else all();
// Ladebuchse im Boden: Aussparung unter dem Hintern (USB-C)
if (!cut) color("#111") translate([-6, t_y - 12, -0.01]) cube([12, 5, 0.3]);
ground();

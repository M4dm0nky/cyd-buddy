"""Packt ASCII-STL-Dateien als Objekte in eine 3MF (Core-Spezifikation, von Bambu Studio lesbar).
Aufruf: python -I stl2_3mf.py <ziel.3mf> [--arrange BREITE,ABSTAND] [--group NAME] [--end] <name>=<datei.stl>[@rx,ry,rz][:x,y] ...
Rotation in Grad (nur 0/180 je Achse noetig). Jedes Einzelobjekt wird auf z=0 gestellt und mittig auf x,y gesetzt.
--group NAME : alle folgenden Specs (bis --end oder Ende der Liste) werden EIN Objekt mit Bauteilen (<components>).
               Die Gruppe wird gemeinsam verschoben (gemeinsame Bounding-Box), die Teile behalten ihre Lage zueinander.
--end        : beendet eine Gruppe; danach folgen wieder Einzelobjekte (fuer AMS: Gruppe Kopf + uebrige Teile einzeln).
--arrange B,A: Einzelobjekte und Gruppen werden zeilenweise in einer Breite B mm mit Abstand A mm angeordnet
               (ueberschreibt :x,y der Einzelobjekte)."""
import re, sys, zipfile, math

def read_stl(path):
    t = open(path, encoding="utf-8", errors="ignore").read()
    pts = [tuple(map(float, m)) for m in re.findall(r"vertex\s+([-\d.eE+]+)\s+([-\d.eE+]+)\s+([-\d.eE+]+)", t)]
    if not pts:
        raise SystemExit(f"{path}: keine ASCII-STL (mit --export-format asciistl exportieren)")
    idx, verts, tris = {}, [], []
    for i in range(0, len(pts), 3):
        tri = []
        for p in pts[i:i + 3]:
            k = (round(p[0], 5), round(p[1], 5), round(p[2], 5))
            if k not in idx:
                idx[k] = len(verts); verts.append(k)
            tri.append(idx[k])
        if len(set(tri)) == 3:
            tris.append(tri)
    return verts, tris

def rot(v, rx, ry, rz):
    x, y, z = v
    for ang, ax in ((rx, "x"), (ry, "y"), (rz, "z")):
        c, s = round(math.cos(math.radians(ang))), round(math.sin(math.radians(ang)))
        if ax == "x": y, z = y * c - z * s, y * s + z * c
        if ax == "y": x, z = x * c + z * s, -x * s + z * c
        if ax == "z": x, y = x * c - y * s, x * s + y * c
    return (x, y, z)

def load_part(spec):
    """Liest einen Spec; liefert (name, verts, tris, pos) mit gedrehten, noch nicht zentrierten Punkten."""
    name, rest = spec.split("=", 1)
    pos = (0.0, 0.0); r = (0, 0, 0)
    m = re.search(r":(-?[\d.]+),(-?[\d.]+)$", rest)   # nur "x,y" am Ende; der Doppelpunkt von "C:\..." zaehlt nicht
    if m: rest, pos = rest[:m.start()], (float(m.group(1)), float(m.group(2)))
    m = re.search(r"@(-?[\d.]+),(-?[\d.]+),(-?[\d.]+)$", rest)
    if m: rest, r = rest[:m.start()], tuple(float(g) for g in m.groups())
    verts, tris = read_stl(rest)
    return name, [rot(v, *r) for v in verts], tris, pos

def bbox(parts):
    vs = [v for p in parts for v in p[1]]
    return (min(v[0] for v in vs), min(v[1] for v in vs), min(v[2] for v in vs),
            max(v[0] for v in vs), max(v[1] for v in vs), max(v[2] for v in vs))

def parse(args):
    """Zerlegt die Argumente in Einzelobjekte und Gruppen: Liste von (gruppenname oder None, [parts])."""
    items, group, gname, arrange = [], None, None, None
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--group":
            gname, group = args[i + 1], []; items.append((gname, group)); i += 2
        elif a == "--end":
            group = None; i += 1
        elif a == "--arrange":
            w, g = args[i + 1].split(","); arrange = (float(w), float(g)); i += 2
        else:
            part = load_part(a)
            if group is not None: group.append(part)
            else: items.append((None, [part]))
            i += 1
    for g, parts in items:
        if g is not None and not parts:
            raise SystemExit(f"Gruppe {g}: keine Teile")
    return items, arrange

def place(items, arrange):
    """Verschiebt jedes Objekt (bzw. jede Gruppe gemeinsam) auf z=0 und x,y. Liefert Liste (gname, [(name, verts, tris)])."""
    out, cursor_x, cursor_y, row_h = [], 0.0, 0.0, 0.0
    for g, parts in items:
        x0, y0, z0, x1, y1, _ = bbox(parts)
        if arrange:
            w, gap = arrange
            if cursor_x > 0 and cursor_x + (x1 - x0) > w:
                cursor_x, cursor_y, row_h = 0.0, cursor_y + row_h + gap, 0.0
            dx, dy = cursor_x - x0, cursor_y - y0
            cursor_x += (x1 - x0) + gap; row_h = max(row_h, y1 - y0)
        else:
            pos = parts[0][3]   # bei Gruppen zaehlt die Position des ersten Teils
            dx, dy = pos[0] - (x0 + x1) / 2, pos[1] - (y0 + y1) / 2
        out.append((g, [(n, [(v[0] + dx, v[1] + dy, v[2] - z0) for v in vs], ts) for n, vs, ts, _ in parts]))
    return out

def main(out, args):
    items, arrange = parse(args)
    placed = place(items, arrange)
    model = ['<?xml version="1.0" encoding="UTF-8"?>',
             '<model unit="millimeter" xml:lang="de-DE" xmlns="http://schemas.microsoft.com/3dmanufacturing/core/2015/02">',
             '<resources>']
    build, nid, stats = [], 1, []
    for g, parts in placed:
        ids = []
        for name, verts, tris in parts:
            model.append(f'<object id="{nid}" name="{name}" type="model"><mesh><vertices>')
            model += [f'<vertex x="{x:.5f}" y="{y:.5f}" z="{z:.5f}"/>' for x, y, z in verts]
            model.append('</vertices><triangles>')
            model += [f'<triangle v1="{a}" v2="{b}" v3="{c}"/>' for a, b, c in tris]
            model.append('</triangles></mesh></object>')
            ids.append(nid); nid += 1
            stats.append(f"{name}: {len(verts)} Punkte, {len(tris)} Dreiecke")
        if g is None:
            build.append(ids[0])
        else:
            model.append(f'<object id="{nid}" name="{g}" type="model"><components>')
            model += [f'<component objectid="{i}"/>' for i in ids]
            model.append('</components></object>')
            build.append(nid); nid += 1
    model.append('</resources><build>')
    model += [f'<item objectid="{i}"/>' for i in build]
    model.append('</build></model>')
    ct = ('<?xml version="1.0" encoding="UTF-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
          '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
          '<Default Extension="model" ContentType="application/vnd.ms-package.3dmanufacturing-3dmodel+xml"/></Types>')
    rels = ('<?xml version="1.0" encoding="UTF-8"?><Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
            '<Relationship Target="/3D/3dmodel.model" Id="rel0" Type="http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel"/></Relationships>')
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED) as z:
        z.writestr("[Content_Types].xml", ct)
        z.writestr("_rels/.rels", rels)
        z.writestr("3D/3dmodel.model", "\n".join(model))
    print("\n".join(stats))
    print(f"geschrieben: {out} ({len(build)} Bauobjekte)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2:])

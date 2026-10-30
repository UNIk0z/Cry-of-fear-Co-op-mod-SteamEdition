"""v0.7: the pictures of the "Player Model" box, one per outfit with a rendered preview.

  python -I scripts/menu/make-modelbox.py <game>/cryoffear <simon.mdl> <out folder>

The game's computer panel shows one picture (client-menu.hpp model_box_picture),
and the box loads the one of the model shown. Each picture is the game's own
computer_logon.tga with the Player Model texts and a small preview of Simon in
that outfit, drawn here from the built simon.mdl and the game's own costume
textures by a small software renderer. The panel's text (model name, number)
sits at (194,162), 211 x 152; Next (157,320) and Done (238,320), 75 x 23; the
red power button (478,447) closes it. The package ships only an XOR delta of each
picture and Install rebuilds it from the player's own file.
Needs Pillow (on the packaging PC only).
"""
import importlib.util
import math
import os
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

spec = importlib.util.spec_from_file_location('joinbox', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'make-joinbox.py'))
joinbox = importlib.util.module_from_spec(spec)
spec.loader.exec_module(joinbox)
FONTS = joinbox.FONTS

# Picture name, costume folder and file stem (client-menu.hpp ModelOutfits has the same order).
OUTFITS = [('sc_mb0', None, None), ('sc_mb1', 'Leatherhoff', 'Leatherhoff'), ('sc_mb2', 'ModDB', 'ModDB'), ('sc_mb3', 'Hello_Kitty', 'Hello_Kitty'),
           ('sc_mb4', 'AoM', 'AoM'), ('sc_mb5', 'Camo', 'Camo'), ('sc_mb6', 'HLC', 'HLC'), ('sc_mb7', 'Black_Metal', 'Black_Metal'),
           ('sc_mb8', 'Team_Psykskallar', 'Team_Psykskallar'), ('sc_mb9', 'Awesome', 'Awesome'), ('sc_mb10', 'Sick_Simon', 'Sick_Simon'), ('sc_mb11', 'Twitcher', 'Twitcher')]
BOX = (80, 98, 188, 312)  # the preview frame on the picture


def i32(b, o):
    return struct.unpack_from('<i', b, o)[0]


def floats(b, o, n):
    return struct.unpack_from('<%df' % n, b, o)


def bone_matrices(b):
    """Every bone's world matrix in the first frame of the "idle" sequence."""
    num, at = i32(b, 140), i32(b, 144)
    seq = next(o for o in (i32(b, 168) + 176 * n for n in range(i32(b, 164))) if b[o:o + 32].split(b'\0')[0] == b'idle')
    anim = i32(b, seq + 124)
    out = []
    for n in range(num):
        o = at + n * 112
        parent = i32(b, o + 32)
        value, scale = floats(b, o + 64, 6), floats(b, o + 88, 6)
        offsets = struct.unpack_from('<6H', b, anim + n * 12)
        v = list(value)
        for j in range(6):
            if offsets[j]:
                p = anim + n * 12 + offsets[j]
                valid, total = b[p], b[p + 1]
                raw = struct.unpack_from('<h', b, p + 2 * (1 if valid > 0 else 0))[0]
                v[j] = value[j] + raw * scale[j]
        sr, cr = math.sin(v[3] / 2), math.cos(v[3] / 2)
        sp, cp = math.sin(v[4] / 2), math.cos(v[4] / 2)
        sy, cy = math.sin(v[5] / 2), math.cos(v[5] / 2)
        x, y, z, w = sr * cp * cy - cr * sp * sy, cr * sp * cy + sr * cp * sy, cr * cp * sy - sr * sp * cy, cr * cp * cy + sr * sp * sy
        m = [[1 - 2 * y * y - 2 * z * z, 2 * x * y - 2 * w * z, 2 * x * z + 2 * w * y, v[0]],
             [2 * x * y + 2 * w * z, 1 - 2 * x * x - 2 * z * z, 2 * y * z - 2 * w * x, v[1]],
             [2 * x * z - 2 * w * y, 2 * y * z + 2 * w * x, 1 - 2 * x * x - 2 * y * y, v[2]]]
        if parent >= 0:
            q = out[parent]
            m = [[sum(q[r][k] * m[k][c] for k in range(3)) + (q[r][3] if c == 3 else 0) for c in range(4)] for r in range(3)]
        out.append(m)
    return out


def triangles(b):
    """(texture index, [(x, y, z, s, t) * 3]) of every triangle of the first body group."""
    bones = bone_matrices(b)
    skin = i32(b, 200)
    part = i32(b, 208)
    model = i32(b, part + 72)
    nmesh, meshes, nverts, vinfo, verts = i32(b, model + 72), i32(b, model + 76), i32(b, model + 80), i32(b, model + 84), i32(b, model + 88)
    world = []
    for k in range(nverts):
        m = bones[b[vinfo + k]]
        p = floats(b, verts + 12 * k, 3)
        world.append(tuple(m[r][0] * p[0] + m[r][1] * p[1] + m[r][2] * p[2] + m[r][3] for r in range(3)))
    out = []
    for n in range(nmesh):
        mesh = meshes + n * 20
        tex = struct.unpack_from('<h', b, skin + 2 * i32(b, mesh + 8))[0]
        p = i32(b, mesh + 4)
        while True:
            count = struct.unpack_from('<h', b, p)[0]
            p += 2
            if count == 0:
                break
            fan = count < 0
            cmds = []
            for _ in range(abs(count)):
                vert, _norm, s, t = struct.unpack_from('<4h', b, p)
                p += 8
                cmds.append(world[vert] + (s, t))
            for k in range(2, len(cmds)):
                if fan:
                    out.append((tex, (cmds[0], cmds[k - 1], cmds[k])))
                else:
                    out.append((tex, (cmds[k - 2], cmds[k - 1], cmds[k]) if k % 2 == 0 else (cmds[k - 1], cmds[k - 2], cmds[k])))
    return out


def model_textures(b):
    out = {}
    for i in range(i32(b, 180)):
        o = i32(b, 184) + i * 80
        name = b[o:o + 64].split(b'\0')[0].decode('latin1').lower()
        w, h, at = i32(b, o + 68), i32(b, o + 72), i32(b, o + 76)
        pal = b[at + w * h:at + w * h + 768]
        img = Image.frombytes('P', (w, h), b[at:at + w * h])
        img.putpalette(pal)
        out[i] = (name, img.convert('RGB'))
    return out


def read_costume(path):
    data = open(path, 'rb').read()
    if data[2] != 2 or data[16] != 24:
        raise SystemExit('expected an uncompressed 24-bit TGA: ' + path)
    w, h = struct.unpack_from('<HH', data, 12)
    o = 18 + data[0]
    img = Image.frombytes('RGB', (w, h), data[o:o + w * h * 3], 'raw', 'BGR')
    return img if data[17] & 0x20 else img.transpose(Image.FLIP_TOP_BOTTOM)


def costume_file(game, folder, stem, part):
    root = os.path.join(game, 'models', 'costumes', folder)
    for name in os.listdir(root):
        if name.lower() == (stem + '_' + part + '.tga').lower():
            return os.path.join(root, name)
    return None


def render(tris, textures, costume, size):
    """A z-buffered, flat-lit drawing, three quarters from the front, 3x supersampled."""
    yaw = math.radians(25)
    c, s = math.cos(yaw), math.sin(yaw)
    pts = [(-(x * s + y * c), -z, x * c - y * s) for _tex, tri in tris for x, y, z, _s, _t in tri]
    minx, maxx = min(p[0] for p in pts), max(p[0] for p in pts)
    miny, maxy = min(p[1] for p in pts), max(p[1] for p in pts)
    w, h = size[0] * 3, size[1] * 3
    k = min((w - 12) / (maxx - minx), (h - 12) / (maxy - miny))
    ox, oy = w / 2 - k * (minx + maxx) / 2, h / 2 - k * (miny + maxy) / 2
    color = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    px = color.load()
    depth = [[-1e9] * w for _ in range(h)]
    pixels = {i: tex[1].load() for i, tex in textures.items()}
    for i, (name, _img) in textures.items():
        if costume.get(name):
            pixels[i] = costume[name].load()
    sizes = {i: tex[1].size for i, tex in textures.items()}
    for tex, tri in tris:
        v = [(-(x * s_ + y * c_), -z, x * c_ - y * s_, u, t) for x, y, z, u, t in tri for s_, c_ in [(s, c)]]
        ax, ay, az = v[0][0] * k + ox, v[0][1] * k + oy, v[0][2]
        bx, by, bz = v[1][0] * k + ox, v[1][1] * k + oy, v[1][2]
        cx, cy, cz = v[2][0] * k + ox, v[2][1] * k + oy, v[2][2]
        area = (bx - ax) * (cy - ay) - (by - ay) * (cx - ax)
        if abs(area) < 1e-6:
            continue
        ux, uy, uz = v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2]
        wx, wy, wz = v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2]
        nx, ny, nz = uy * wz - uz * wy, uz * wx - ux * wz, ux * wy - uy * wx
        length = math.sqrt(nx * nx + ny * ny + nz * nz) or 1
        light = 0.62 + 0.38 * abs((nx * -0.3 + ny * -0.5 + nz * 0.8) / length)
        tw, th = sizes[tex]
        tp = pixels[tex]
        for y in range(max(0, int(min(ay, by, cy))), min(h - 1, int(max(ay, by, cy)) + 1) + 1):
            row = depth[y]
            for x in range(max(0, int(min(ax, bx, cx))), min(w - 1, int(max(ax, bx, cx)) + 1) + 1):
                l1 = ((bx - x) * (cy - y) - (by - y) * (cx - x)) / area
                l2 = ((cx - x) * (ay - y) - (cy - y) * (ax - x)) / area
                l3 = 1 - l1 - l2
                if l1 < 0 or l2 < 0 or l3 < 0:
                    continue
                z = l1 * az + l2 * bz + l3 * cz
                if z <= row[x]:
                    continue
                row[x] = z
                r, g, b = tp[int(l1 * v[0][3] + l2 * v[1][3] + l3 * v[2][3]) % tw, int(l1 * v[0][4] + l2 * v[1][4] + l3 * v[2][4]) % th]
                px[x, y] = (int(r * light), int(g * light), int(b * light), 255)
    return color.resize(size, Image.LANCZOS)


def base_picture(source):
    header, img, tail = joinbox.read_tga(source)
    if img.size != (554, 480):
        raise SystemExit('unexpected computer_logon.tga size')
    px = img.load()
    joinbox.fill_rows(px, 75, 72, 200, 87)
    joinbox.fill_columns(px, 132, 98, 418, 160)
    joinbox.fill_columns(px, 74, 161, 232, 174)
    joinbox.fill_columns(px, 432, 161, 482, 175)
    joinbox.fill_columns(px, 77, 203, 150, 248)
    joinbox.fill_columns(px, 77, 255, 403, 280)
    joinbox.fill_rows(px, 183, 324, 208, 339)
    joinbox.fill_rows(px, 256, 324, 296, 339)
    joinbox.fill_columns(px, 318, 317, 476, 345)
    joinbox.fill_rows(px, 12, 430, 60, 478)
    draw = ImageDraw.Draw(img)
    form = px[430, 300]
    draw.rectangle((77, 161, 476, 312), fill=form)   # the form, empty: the preview and the text card go here
    draw.rectangle((192, 164, 476, 312), fill=tuple(min(255, c + 22) for c in form), outline=(150, 118, 100))   # under the panel's text
    bold = lambda size: ImageFont.truetype(FONTS + 'tahomabd.ttf', size)
    plain = lambda size: ImageFont.truetype(FONTS + 'tahoma.ttf', size)
    joinbox.text(draw, (79, 73), 'Player Model', bold(13), (126, 120, 132))
    joinbox.text(draw, (335, 118), 'CRY OF FEAR', ImageFont.truetype(FONTS + 'arialbd.ttf', 26), (250, 246, 248), (92, 66, 62), 'mm')
    joinbox.text(draw, (335, 142), 'Co-op', plain(14), (238, 226, 226), (92, 66, 62), 'mm')
    dark = (40, 26, 22)
    joinbox.text(draw, (194, 331), 'Next', plain(12), dark, anchor='mm')
    joinbox.text(draw, (275, 331), 'Done', plain(12), dark, anchor='mm')
    return header, img, tail


def framed(picture, preview):
    img = picture.copy()
    draw = ImageDraw.Draw(img)
    draw.rectangle(BOX, fill=(46, 40, 44), outline=(96, 70, 60))
    if preview is None:
        font = ImageFont.truetype(FONTS + 'tahoma.ttf', 12)
        joinbox.text(draw, ((BOX[0] + BOX[2]) // 2, (BOX[1] + BOX[3]) // 2 - 8), 'No', font, (220, 210, 205), anchor='mm')
        joinbox.text(draw, ((BOX[0] + BOX[2]) // 2, (BOX[1] + BOX[3]) // 2 + 8), 'preview', font, (220, 210, 205), anchor='mm')
    else:
        img.paste(preview, (BOX[0] + 2, BOX[1] + 2), preview)
    return img


def main(game, simon, out):
    source = os.path.join(game, 'gfx', 'vgui', 'puzzle', 'computer', 'computer_logon.tga')
    header, picture, tail = base_picture(source)
    data = open(simon, 'rb').read()
    tris, textures = triangles(data), model_textures(data)
    size = (BOX[2] - BOX[0] - 3, BOX[3] - BOX[1] - 3)
    names = {'hoodie': [n for n, _ in textures.values() if n.startswith('hoodie')], 'jeans': [n for n, _ in textures.values() if n.startswith('jeans')], 'face': [n for n, _ in textures.values() if n.startswith('head_')]}
    os.makedirs(out, exist_ok=True)
    for name, folder, stem in OUTFITS:
        costume = {}
        if folder:
            for part, targets in names.items():
                path = costume_file(game, folder, stem, part)
                if path:
                    for target in targets:
                        costume[target] = read_costume(path)
        joinbox.write_tga(os.path.join(out, name + '.tga'), header, framed(picture, render(tris, textures, costume, size)), tail)
        print('made', name)
    joinbox.write_tga(os.path.join(out, 'sc_mbx.tga'), header, framed(picture, None), tail)


if __name__ == '__main__':
    if len(sys.argv) != 4:
        raise SystemExit(__doc__)
    main(sys.argv[1], sys.argv[2], sys.argv[3])

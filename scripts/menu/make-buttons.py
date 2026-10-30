# coop-fixes-013: the two main-menu buttons "Host Simon's Story" and
# "Join Simon's Story". Drawn here, not taken from the game: the same format as
# the game's own buttons (280 x 38, 32-bit TGA, bottom-left origin, one colour
# with the shape in the alpha channel: white, or red 217,0,0 when the mouse is
# over it), Arial with the text's cap height 17 px from y 8, and a soft
# horizontal smear behind the text like the originals.
# Usage: python make-buttons.py OUTPUT_DIR   (needs Pillow and Windows Arial)
import math, random, struct, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

W, H = 280, 38
LABELS = {'sc_host': "Host Simon's Story", 'sc_join': "Join Simon's Story", 'sc_mdl': "Player Model"}   # v0.7: Player Model
FONT = 'C:/Windows/Fonts/arial.ttf'

def alpha_mask(text):
    size = 24
    font = ImageFont.truetype(FONT, size)
    while font.getbbox(text)[2] - font.getbbox(text)[0] > W - 60:
        size -= 1
        font = ImageFont.truetype(FONT, size)
    left, top, right, bottom = font.getbbox('H')
    cap = bottom - top
    text_box = font.getbbox(text)
    width = text_box[2] - text_box[0]
    x = (W - width) // 2 - text_box[0]
    y = 8 - top + (17 - cap) // 2
    mask = Image.new('L', (W, H), 0)
    ImageDraw.Draw(mask).text((x, y), text, font=font, fill=255)
    # The smear: brighter in the middle rows, fading out past the text ends.
    rng = random.Random(13)
    smear = Image.new('L', (W, H), 0)
    px = smear.load()
    x0, x1 = x + text_box[0] - 28, x + text_box[2] + 28
    rows = [rng.uniform(0.35, 1.0) for _ in range(H)]
    for yy in range(7, 26):
        for xx in range(max(0, x0), min(W, x1)):
            edge = min(xx - x0, x1 - xx) / 28.0
            fade = max(0.0, min(1.0, edge))
            centre = math.exp(-((yy - 16.5) / 7.0) ** 2)
            px[xx, yy] = int(34 * fade * centre * rows[yy])
    smear = smear.filter(ImageFilter.BoxBlur(1))
    out = Image.new('L', (W, H), 0)
    o, s, m = out.load(), smear.load(), mask.load()
    for yy in range(H):
        for xx in range(W):
            o[xx, yy] = max(m[xx, yy], s[xx, yy])
    return out

def write_tga(path, alpha, rgb):
    header = struct.pack('<BBBHHBHHHHBB', 0, 0, 2, 0, 0, 0, 0, 0, W, H, 32, 8)
    a = alpha.load()
    body = bytearray()
    for yy in range(H - 1, -1, -1):          # bottom-left origin
        for xx in range(W):
            body += bytes((rgb[2], rgb[1], rgb[0], a[xx, yy]))
    footer = struct.pack('<II', 0, 0) + b'TRUEVISION-XFILE.\0'
    with open(path, 'wb') as f:
        f.write(header + bytes(body) + footer)

if __name__ == '__main__':
    out = sys.argv[1]
    for name, text in LABELS.items():
        mask = alpha_mask(text)
        write_tga(f'{out}/{name}.tga', mask, (255, 255, 255))
        write_tga(f'{out}/{name}_mo.tga', mask, (217, 0, 0))

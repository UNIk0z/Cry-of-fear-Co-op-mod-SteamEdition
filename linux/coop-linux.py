#!/usr/bin/env python3
# Cry of Fear Co-op for Linux (Steam + Proton). Python 3 standard library only.
#
# The same steps as the Windows package (files/setup-coop.ps1 and
# files/install-story-package.ps1): every co-op map and extra is built from
# this PC's own game files and checked against the SHA256 in release.json, so
# the result is byte-identical to a Windows install. Cry of Fear itself runs
# through Proton; the co-op adapter is the same Windows DLL.
#
#   coop-linux.py install    install or update (game closed)
#   coop-linux.py run        start Cry of Fear with the co-op menu
#   coop-linux.py uninstall  set the co-op files aside
#   coop-linux.py check      say what install would do, change nothing
#   coop-linux.py spray <picture.png|.bmp>   make it your spray logo (tempdecal.wad)
# Options: --game "<...>/steamapps/common/Cry of Fear/cryoffear"
import base64, datetime, hashlib, json, os, re, shutil, struct, subprocess, sys, urllib.parse, uuid, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
APP_ID = '223710'
NATIVE_HL_SHA = '0036b91c01e92ed205513f52563053a55a66a32d257efb5f476b8f1f3dde0e63'


class Stop(Exception):
    pass


def say(text):
    print(text, flush=True)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            h.update(block)
    return h.hexdigest()


def read(path):
    with open(path, 'rb') as f:
        return f.read()


# Windows ignores letter case in file names and Proton does too, but Linux does
# not: the catalog's paths (written on Windows) are found here whatever their case.
def find(root, rel):
    path = root
    for part in [p for p in re.split(r'[\\/]', rel) if p]:
        exact = os.path.join(path, part)
        if os.path.exists(exact):
            path = exact
            continue
        try:
            names = os.listdir(path)
        except OSError:
            names = []
        match = [n for n in names if n.lower() == part.lower()]
        path = os.path.join(path, match[0] if match else part)
    return path


def exists(root, rel):
    return os.path.isfile(find(root, rel))


# ---------------------------------------------------------------- Steam
def steam_roots():
    home = os.path.expanduser('~')
    roots = []
    for p in ('.steam/steam', '.steam/root', '.local/share/Steam', '.var/app/com.valvesoftware.Steam/.local/share/Steam',
              'snap/steam/common/.local/share/Steam'):
        full = os.path.realpath(os.path.join(home, p))
        if os.path.isdir(full) and full not in roots:
            roots.append(full)
    return roots


def find_game():
    libraries = []
    for root in steam_roots():
        libraries.append(root)
        vdf = find(root, 'steamapps/libraryfolders.vdf')
        if os.path.isfile(vdf):
            text = open(vdf, encoding='utf-8', errors='replace').read()
            libraries += [m.replace('\\\\', '\\') for m in re.findall(r'"path"\s+"([^"]+)"', text)]
    first = None
    for library in libraries:
        candidate = find(library, 'steamapps/common/Cry of Fear/cryoffear')
        if not exists(candidate, 'cl_dlls/hl.dll'):
            continue
        if exists(library, 'steamapps/appmanifest_223710.acf'):
            return os.path.realpath(candidate)
        first = first or os.path.realpath(candidate)
    return first


def persona_name():
    for root in steam_roots():
        vdf = find(root, 'config/loginusers.vdf')
        if not os.path.isfile(vdf):
            continue
        text = open(vdf, encoding='utf-8', errors='replace').read()
        chosen = None
        for body in re.findall(r'"\d{17}"\s*\{([^{}]*)\}', text):
            m = re.search(r'"PersonaName"\s+"([^"]*)"', body)
            if not m:
                continue
            if re.search(r'"MostRecent"\s+"1"', body) or not chosen:
                chosen = m.group(1)
        if chosen:
            clean = ''.join(c for c in chosen if 32 <= ord(c) < 0x2000 and c not in '";\\%').strip()[:31].strip()
            return clean or None
    return None


def game_running():
    try:
        out = subprocess.run(['ps', '-eo', 'args'], capture_output=True, text=True).stdout.lower()
    except OSError:
        return False
    return any(re.search(r'(^|[\\/ ])(cof|cryoffear|coflaunchapp)\.exe', line) for line in out.splitlines())


# ---------------------------------------------------------------- builders
def patched_bsp(data, plan):
    if sha(data) != plan['sourceSha256']:
        raise Stop('Original BSP SHA256 mismatch. Nothing installed.')
    if len(data) < 124 or struct.unpack_from('<i', data, 0)[0] != 30:
        raise Stop('Expected complete BSP30.')
    old_offset, old_length = struct.unpack_from('<ii', data, 4)
    if old_offset < 124 or old_length < 1 or old_offset + old_length > len(data):
        raise Stop('Invalid entity lump.')
    end = data.find(b'\0', old_offset, old_offset + old_length)
    if end < 0:
        raise Stop('Missing entity terminator.')
    text = data[old_offset:end].decode('latin-1')
    previous = len(text)
    for edit in sorted(plan['edits'], key=lambda e: e['start'], reverse=True):
        s, e = edit['start'], edit['end']
        if s < 0 or e <= s or e > previous or text[s] != '{' or text[e - 1] != '}':
            raise Stop('Invalid/overlapping entity edit. Nothing installed.')
        text = text[:s] + edit['to'] + text[e:]
        previous = s
    entity = (text + plan['appendText'] + '\0').encode('latin-1')
    new_offset = -(-len(data) // 4) * 4
    total = new_offset + len(entity)
    models = [m for m in (plan.get('modelClones') or []) if m is not None]
    if models:
        old_model_offset, old_model_length = struct.unpack_from('<ii', data, 116)
        if old_model_offset < 124 or old_model_length < 64 or old_model_length % 64 or old_model_offset + old_model_length > len(data):
            raise Stop('Invalid original model lump.')
        new_model_offset = -(-total // 4) * 4
        new_model_length = old_model_length + len(models) * 64
        total = new_model_offset + new_model_length
    out = bytearray(total)
    out[:len(data)] = data
    out[new_offset:new_offset + len(entity)] = entity
    struct.pack_into('<ii', out, 4, new_offset, len(entity))
    if models:
        out[new_model_offset:new_model_offset + old_model_length] = data[old_model_offset:old_model_offset + old_model_length]
        for i, model in enumerate(models):
            at = new_model_offset + old_model_length + i * 64
            if model['sourceModel'] < 1 or (model['sourceModel'] + 1) * 64 > old_model_length or model['index'] != old_model_length // 64 + i \
                    or len(model['mins']) != 3 or len(model['maxs']) != 3:
                raise Stop('Invalid readiness model clone.')
            src = old_model_offset + model['sourceModel'] * 64
            out[at:at + 64] = data[src:src + 64]
            for k in range(3):
                lo = struct.unpack('<f', struct.pack('<f', float(model['mins'][k])))[0]
                hi = struct.unpack('<f', struct.pack('<f', float(model['maxs'][k])))[0]
                if lo != lo or hi != hi or abs(lo) == float('inf') or abs(hi) == float('inf') or lo >= hi:
                    raise Stop('Invalid readiness bounds.')
                struct.pack_into('<f', out, at + k * 4, lo)
                struct.pack_into('<f', out, at + 12 + k * 4, hi)
                struct.pack_into('<f', out, at + 24 + k * 4, 0.0)
        struct.pack_into('<ii', out, 116, new_model_offset, new_model_length)
    out = bytes(out)
    if sha(out) != plan['outputSha256'] or len(out) != plan['fileBytes']:
        raise Stop('Generated BSP SHA256/size mismatch. Nothing installed.')
    return out


def xor_delta(original, delta):
    data = bytearray(zlib.decompress(delta, -15))
    if len(data) != len(original):
        raise Stop('Delta length mismatch.')
    n = len(data)
    return (int.from_bytes(data, 'little') ^ int.from_bytes(original, 'little')).to_bytes(n, 'little')


def _r(b, o):
    return struct.unpack_from('<i', b, o)[0]


def _w(b, o, v):
    struct.pack_into('<i', b, o, v)


def _f32(x):
    return struct.unpack('<f', struct.pack('<f', x))[0]


def simon_model(police, simon, bone_map, xf):
    # StoryCoopSimonModel.Build in install-story-package.ps1, step for step.
    if len(xf) != len(bone_map) * 12:
        raise ValueError('transforms')
    X = (len(police) + 15) & ~15
    o = bytearray(X + len(simon))
    o[:len(police)] = police
    o[X:] = simon
    nt, ti, td, nsr, nsf, si, nbp, bpi = struct.unpack_from('<8i', simon, 180)
    for i in range(nt):
        t = X + ti + 80 * i
        _w(o, t + 76, _r(o, t + 76) + X)

    def point(at, m, translate):
        v = struct.unpack_from('<3f', o, at)
        r = []
        for a in range(3):
            t = xf[m + a * 3] * v[0]
            t = t + xf[m + a * 3 + 1] * v[1]
            t = t + xf[m + a * 3 + 2] * v[2]
            if translate:
                t = t + xf[m + 9 + a]
            r.append(t)
        struct.pack_into('<3f', o, at, *r)

    for i in range(nbp):
        bp = X + bpi + 76 * i
        nm = _r(o, bp + 64)
        _w(o, bp + 72, _r(o, bp + 72) + X)
        mi = _r(o, bp + 72)
        for j in range(nm):
            mo = mi + 112 * j
            nmesh, meshi, nv, vinfo, vi, nn, ninfo, ni = (_r(o, mo + k) for k in (72, 76, 80, 84, 88, 92, 96, 100))
            for k in (76, 84, 88, 96, 100, 108):
                v = _r(o, mo + k)
                if v != 0:
                    _w(o, mo + k, v + X)
            for m in range(nmesh):
                me = X + meshi + 20 * m
                _w(o, me + 4, _r(o, me + 4) + X)
                if _r(o, me + 16) != 0:
                    _w(o, me + 16, _r(o, me + 16) + X)
            for k in range(nv):
                b = o[X + vinfo + k]
                if b >= len(bone_map):
                    raise ValueError('vertex bone')
                point(X + vi + 12 * k, b * 12, True)
                o[X + vinfo + k] = bone_map[b] & 0xFF
            for k in range(nn):
                b = o[X + ninfo + k]
                if b >= len(bone_map):
                    raise ValueError('normal bone')
                point(X + ni + 12 * k, b * 12, False)
                o[X + ninfo + k] = bone_map[b] & 0xFF
    struct.pack_into('<8i', o, 180, nt, X + ti, X + td, nsr, nsf, X + si, nbp, X + bpi)
    o[8:72] = bytes(64)
    o[8:8 + 9] = b'simon.mdl'
    _w(o, 72, len(o))
    renames = {'hoodie_grey.bmp': 'hoodie_coop.bmp', 'jeans.bmp': 'jeanss.bmp', 'head_face.bmp': 'head_coop.bmp'}
    for i in range(nt):
        t = X + ti + 80 * i
        name = bytes(o[t:t + 64]).split(b'\0')[0].decode('ascii', 'replace')
        if name in renames:
            o[t:t + 64] = bytes(64)
            nb = renames[name].encode('ascii')
            o[t:t + len(nb)] = nb
    return bytes(o)


def _quantize(t, w, h, o, at):
    # StoryCoopOutfitModel.Quantize: median cut on 15-bit colours, in integers.
    if t[2] != 2 or t[16] != 24 or struct.unpack_from('<H', t, 12)[0] != w or struct.unpack_from('<H', t, 14)[0] != h:
        raise ValueError('texture')
    start = 18 + t[0]
    top = (t[17] & 0x20) != 0
    count = [0] * 32768
    sr = [0] * 32768
    sg = [0] * 32768
    sb = [0] * 32768
    code = [0] * (w * h)
    for y in range(h):
        row = start + (y if top else h - 1 - y) * w * 3
        for x in range(w):
            p = row + x * 3
            b, g, r = t[p], t[p + 1], t[p + 2]
            c = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3)
            code[y * w + x] = c
            count[c] += 1
            sr[c] += r
            sg[c] += g
            sb[c] += b
    boxes = [[c for c in range(32768) if count[c]]]
    while len(boxes) < 256:
        best, best_axis, best_score = -1, 0, -1
        for i, box in enumerate(boxes):
            if len(box) < 2:
                continue
            lr = lg = lb = 31
            hr = hg = hb = 0
            pop = 0
            for c in box:
                pop += count[c]
                r, g, b = (c >> 10) & 31, (c >> 5) & 31, c & 31
                if r < lr: lr = r
                if r > hr: hr = r
                if g < lg: lg = g
                if g > hg: hg = g
                if b < lb: lb = b
                if b > hb: hb = b
            rr, rg, rb = hr - lr, hg - lg, hb - lb
            mx = max(rr, rg, rb)
            score = pop * mx
            if score > best_score:
                best_score, best = score, i
                best_axis = 0 if rr == mx else 1 if rg == mx else 2
        if best < 0:
            break
        shift = (10, 5, 0)[best_axis]
        keys = sorted(((c >> shift) & 31) * 32768 + c for c in boxes[best])
        total = sum(count[k % 32768] for k in keys)
        cum = 0
        k = 0
        while k < len(keys):
            cum += count[keys[k] % 32768]
            if 2 * cum >= total:
                break
            k += 1
        if k >= len(keys) - 1:
            k = len(keys) - 2
        boxes[best] = [key % 32768 for key in keys[:k + 1]]
        boxes.append([key % 32768 for key in keys[k + 1:]])
    lut = bytearray(32768)
    pal = at + w * h
    o[pal:pal + 768] = bytes(768)
    for i, box in enumerate(boxes):
        n = r = g = b = 0
        for c in box:
            n += count[c]; r += sr[c]; g += sg[c]; b += sb[c]
            lut[c] = i
        o[pal + i * 3] = (r + n // 2) // n
        o[pal + i * 3 + 1] = (g + n // 2) // n
        o[pal + i * 3 + 2] = (b + n // 2) // n
    o[at:at + w * h] = bytes(lut[c] for c in code)


def outfit_model(simon, name, parts, textures):
    o = bytearray(simon)
    nt, ti = _r(o, 180), _r(o, 184)
    wanted = {'hoodie': 'hoodie_coop.bmp', 'jeans': 'jeanss.bmp', 'face': 'head_coop.bmp'}
    for part, texture in zip(parts, textures):
        if part not in wanted:
            raise ValueError('part')
        done = False
        for i in range(nt):
            t = ti + 80 * i
            if bytes(o[t:t + 64]).split(b'\0')[0].decode('ascii', 'replace') != wanted[part]:
                continue
            _quantize(texture, _r(o, t + 68), _r(o, t + 72), o, _r(o, t + 76))
            done = True
        if not done:
            raise ValueError('target')
    o[8:72] = bytes(64)
    nb = (name + '.mdl').encode('ascii')
    o[8:8 + len(nb)] = nb
    return bytes(o)


# ---------------------------------------------------------------- package install
def write_new(path, data):
    tmp = path + '.storycoop-' + uuid.uuid4().hex + '.tmp'
    try:
        with open(tmp, 'xb') as f:
            f.write(data)
        os.replace(tmp, path)
    finally:
        if os.path.exists(tmp):
            os.remove(tmp)


def install_package(game, catalog, verify_only=False, progress=None):
    """install-story-package.ps1 without the RC1-RC3 and old-menu branches (a Linux PC never had those)."""
    maps = find(game, 'maps')
    prepared = []
    for entry in catalog['maps']:
        if not re.fullmatch(r'(c_|s2_|s3_|s4_|cof_campaign_)[a-z0-9_]{1,30}', entry['map']) or len(entry['map']) > 31 \
                or not re.fullmatch(r'c_[a-z0-9_]{1,30}', entry['sourceMap']) or entry['map'] == entry['sourceMap']:
            raise Stop('Unsafe map name.')
        dest = find(maps, entry['map'] + '.bsp')
        present = os.path.isfile(dest)
        if present and file_sha(dest) != entry['outputSha256']:
            raise Stop('Unknown/changed file retained: %s. Nothing installed or removed.' % dest)
        plan_path = os.path.join(HERE, 'plans', entry['map'] + '.json')
        plan_bytes = read(plan_path)
        if sha(plan_bytes) != entry['planSha256']:
            raise Stop('Changed package plan. Nothing installed.')
        plan = json.loads(plan_bytes.decode('utf-8-sig'))
        if plan['schemaVersion'] != catalog['schemaVersion'] or plan['map'] != entry['map'] or plan['sourceSha256'] != entry['sourceSha256'] \
                or plan['outputSha256'] != entry['outputSha256']:
            raise Stop('Plan/catalog mismatch.')
        source = find(maps, entry['sourceMap'] + '.bsp')
        if not present:
            if not os.path.isfile(source):
                raise Stop('Original map missing: %s. Nothing installed.' % source)
            patched_bsp(read(source), plan)  # full preflight; rebuilt again when written
        prepared.append(dict(path=dest, exists=present, source=source, plan=plan, size=plan['fileBytes']))
    adapter = catalog['runtimeAdapter']
    if adapter['file'] != 'storycoop_rc4.dll' or not re.fullmatch(r'[a-f0-9]{64}', adapter['sha256']):
        raise Stop('Invalid RC4 runtime adapter catalog.')
    adapter_dest = find(game, 'cl_dlls/storycoop_rc4.dll')
    present = os.path.isfile(adapter_dest)
    if present and file_sha(adapter_dest) != adapter['sha256']:
        raise Stop('Unknown/changed RC4 adapter retained. Nothing installed or removed.')
    adapter_bytes = read(os.path.join(HERE, 'native', adapter['file']))
    if sha(adapter_bytes) != adapter['sha256'] or len(adapter_bytes) != adapter['fileBytes']:
        raise Stop('Changed RC4 runtime artifact. Nothing installed.')
    prepared.append(dict(path=adapter_dest, exists=present, data=adapter_bytes, size=len(adapter_bytes)))

    simon_built = None
    for extra in [x for x in catalog.get('extras') or [] if x]:
        target = extra['target']
        if extra.get('replaces'):
            raise Stop('This package replaces a game file; the Linux installer does not support that. Nothing installed.')
        if not re.fullmatch(r'(maps/storycoop_[a-z0-9]+\.(custom|coopm)|models/player/simon(_[a-z]+)?/simon(_[a-z]+)?\.mdl|gfx/vgui/menu/sc_(host|join|mdl)(_mo)?\.tga|gfx/vgui/sc_logo\.tga|maps/(Chapter-[0-7](-[A-Za-z]+)+|Load-Save-Slot-[1-5])\.bsp|gfx/vgui/sc_(joinbox|mb[a-z0-9]+)\.tga|sc_setup\.scr|storycoop_(server|comfort)\.cfg|txtfiles/storycoop_hints\.txt)', target) \
                or not re.fullmatch(r'[a-f0-9]{64}', extra['sha256']) or extra['fileBytes'] < 1:
            raise Stop('Invalid extra file catalog.')
        path = find(game, target)
        present = os.path.isfile(path)
        setup_replace = False
        if extra.get('mutable') and target == 'sc_setup.scr':
            # v0.9: an older one without the revive options is replaced once, like v0.7's Unlock extras.
            text = read(path).decode('latin-1') if present else ''
            keep = present and 'sc_extras' in text and 'sv_coopcprtime' in text
            setup_replace = present and not keep
            if keep:
                prepared.append(dict(path=path, exists=True))
                continue
        elif extra.get('mutable'):
            # v0.9: the host's storycoop_server.cfg and each player's storycoop_comfort.cfg are theirs once there.
            if present:
                prepared.append(dict(path=path, exists=True))
                continue
        elif present and file_sha(path) != extra['sha256']:
            if extra.get('build') or extra.get('deltaFrom'):
                say('Kept as it is (not built by this mod or changed since): %s' % path)
                continue
            raise Stop('Unknown/changed file retained: %s. Nothing installed or removed.' % path)
        if present and not setup_replace:
            prepared.append(dict(path=path, exists=True))
            if extra.get('build') == 'simon-player-model':
                simon_built = read(path)
            continue
        data = None
        if extra.get('deltaFrom'):
            source = None
            base = find(game, extra['deltaFrom']['target'])
            for candidate in (base, base + '.storycoop-original'):
                if os.path.isfile(candidate) and file_sha(candidate) == extra['deltaFrom']['sha256']:
                    source = candidate
                    break
            if not source:
                say('Skipped %s: %s is missing or a different version.' % (target, extra['deltaFrom']['target']))
                continue
            try:
                data = xor_delta(read(source), read(os.path.join(HERE, extra['file'])))
            except Exception:
                data = None
            if data is None or sha(data) != extra['sha256']:
                raise Stop('Generated extra mismatch. Nothing installed.')
        elif extra.get('file'):
            if not re.fullmatch(r'extras/(storycoop_[a-z0-9]+\.(custom|coopm)|sc_(host|join|mdl)(_mo)?\.tga|sc_joinbox\.tga|sc_setup\.scr|storycoop_(server|comfort)\.cfg|storycoop_hints\.txt)', extra['file']):
                raise Stop('Invalid extra package file.')
            data = read(os.path.join(HERE, extra['file']))
        elif extra.get('build') == 'simon-outfit-model':
            name = os.path.splitext(os.path.basename(target))[0]
            parts, textures, ok = [], [], simon_built is not None
            for source in extra['sources']:
                if not re.fullmatch(r'models/costumes/[A-Za-z_]+/[A-Za-z_]+\.(tga|TGA)', source['path']) or source['part'] not in ('hoodie', 'jeans', 'face'):
                    raise Stop('Invalid extra file catalog.')
                sp = find(game, source['path'])
                if not os.path.isfile(sp) or file_sha(sp) != source['sha256']:
                    ok = False
                    break
                parts.append(source['part'])
                textures.append(read(sp))
            if ok:
                if progress:
                    progress('Building outfit %s' % name)
                try:
                    data = outfit_model(simon_built, name, parts, textures)
                except Exception:
                    data = None
            if data is None or sha(data) != extra['sha256']:
                say('Outfit model skipped (%s): its costume files are missing or a different version.' % name)
                continue
        elif extra.get('build') == 'simon-player-model':
            s = extra['sources']
            pp, sp = find(game, 'models/player/police1/police1.mdl'), find(game, 'models/cutscene/simon_clothes.mdl')
            if not os.path.isfile(pp) or not os.path.isfile(sp) or file_sha(pp) != s['policeSha256'] or file_sha(sp) != s['simonSha256']:
                if extra.get('optional'):
                    say('Simon player model skipped: police1.mdl or cutscene/simon_clothes.mdl is missing or a different version.')
                    continue
                raise Stop('Required extra source missing or changed. Nothing installed.')
            raw = base64.b64decode(extra['transforms'])
            xf = list(struct.unpack('<%dd' % (len(raw) // 8), raw))
            if progress:
                progress('Building Simon\'s player model')
            try:
                data = simon_model(read(pp), read(sp), [int(b) for b in extra['boneMap']], xf)
            except Exception:
                data = None
            if data is None or sha(data) != extra['sha256']:
                if extra.get('optional'):
                    say('Simon player model skipped: the model built here did not match the expected result.')
                    continue
                raise Stop('Generated extra mismatch. Nothing installed.')
            simon_built = data
        else:
            if extra.get('copyFrom') not in ('models/player.mdl', 'maps/c_difficulty_settings.bsp'):
                raise Stop('Invalid extra copy source.')
            cp = find(game, extra['copyFrom'])
            if not os.path.isfile(cp) or file_sha(cp) != extra['sha256']:
                if extra.get('optional'):
                    say('Skipped %s: %s is missing or a different version.' % (target, extra['copyFrom']))
                    continue
                raise Stop('Required extra source missing or changed. Nothing installed.')
            data = read(cp)
        if sha(data) != extra['sha256'] or len(data) != extra['fileBytes']:
            raise Stop('Changed extra artifact. Nothing installed.')
        item = dict(path=path, exists=False, data=data, size=len(data))
        if setup_replace:
            item['keep'] = path + '.storycoop-old'
        prepared.append(item)

    if file_sha(find(game, 'cl_dlls/hl.dll')) != NATIVE_HL_SHA or catalog['nativeDllSha256'] != NATIVE_HL_SHA:
        raise Stop('Native Steam DLL version mismatch (cl_dlls/hl.dll). Update Cry of Fear in Steam. Nothing installed.')
    if verify_only:
        return prepared
    needed = sum(p['size'] for p in prepared if not p['exists'])
    if shutil.disk_usage(maps).free < needed:
        raise Stop('Not enough disk space: need %d bytes. Nothing installed.' % needed)
    todo = [p for p in prepared if not p['exists']]
    for n, item in enumerate(todo, 1):
        if progress:
            progress('Installing co-op files (%d of %d)' % (n, len(todo)))
        os.makedirs(os.path.dirname(item['path']), exist_ok=True)
        data = item['data'] if 'data' in item else patched_bsp(read(item['source']), item['plan'])
        if 'keep' in item and not os.path.exists(item['keep']):
            write_new(item['keep'], read(item['path']))
        if 'keep' in item:
            os.remove(item['path'])
        write_new(item['path'], data)
    return prepared


# ---------------------------------------------------------------- setup (install / uninstall)
def setup(game, uninstall=False, check_only=False):
    catalog = json.load(open(os.path.join(HERE, 'release.json'), encoding='utf-8-sig'))
    known = json.load(open(os.path.join(HERE, 'known-builds.json'), encoding='utf-8-sig'))
    if catalog.get('schemaVersion') != 2 or catalog.get('release') != 'story-state-rc4' or catalog.get('dependencies') or catalog.get('menu'):
        raise Stop('Unexpected package catalog.')
    version = catalog['coopFixes']['id']
    expected = {'maps/%s.bsp' % m['map']: m['outputSha256'] for m in catalog['maps']}
    expected['cl_dlls/storycoop_rc4.dll'] = catalog['runtimeAdapter']['sha256']
    for x in catalog.get('extras') or []:
        if x:
            expected[x['target']] = x['sha256']
    mutable = {x['target'] for x in catalog.get('extras') or [] if x and x.get('mutable')}
    older = {}
    for b in known['builds']:
        for m in b.get('maps') or []:
            older.setdefault('maps/%s.bsp' % m['map'], set()).add(m['sha256'])
        if b.get('adapter'):
            older.setdefault('cl_dlls/storycoop_rc4.dll', set()).add(b['adapter'])
        for x in b.get('extras') or []:
            if x and not x.get('replaces') and x.get('sha256'):
                older.setdefault(x['target'], set()).add(x['sha256'])
    adapter = find(game, 'cl_dlls/storycoop_rc4.dll')
    installed = None
    if os.path.isfile(adapter):
        s = file_sha(adapter)
        if s == catalog['runtimeAdapter']['sha256']:
            installed = version
        else:
            installed = next((str(b['id']) for b in known['builds'] if b.get('adapter') == s), 'an unknown version')
    if uninstall:
        say('Installed now: co-op %s. Uninstalling.' % installed if installed else 'No co-op adapter installed; setting aside any co-op files left over.')
    elif not installed:
        say('No co-op version installed yet. Installing %s.' % version)
    elif installed == version:
        say('Co-op %s is already installed. Checking every file.' % version)
    else:
        say('Found co-op %s. Updating it to %s.' % (installed, version))
    moves, unknown, skipped, kept = [], [], [], 0
    for rel in sorted(expected):
        path = find(game, rel)
        if not os.path.isfile(path):
            continue
        if rel in mutable:
            if uninstall:
                moves.append(rel)
            else:
                kept += 1
            continue
        s = file_sha(path)
        if s == expected[rel]:
            if uninstall:
                moves.append(rel)
            else:
                kept += 1
        elif s in older.get(rel, ()):
            moves.append(rel)
        elif uninstall:
            skipped.append(rel)
        else:
            unknown.append(rel)
    if unknown:
        for rel in unknown:
            say('Unknown file (not from any co-op build): %s' % find(game, rel))
        raise Stop('Unknown files sit where the co-op files go. Nothing changed. Move them away yourself, or ask for help.')
    for rel in sorted(older):
        if rel in expected:
            continue
        path = find(game, rel)
        if os.path.isfile(path):
            (moves if file_sha(path) in older[rel] else skipped).append(rel)
    for name in known.get('experimentMaps') or []:
        rel = 'maps/%s.bsp' % name
        if os.path.isfile(find(game, rel)) and rel not in moves:
            moves.append(rel)
            if rel in skipped:
                skipped.remove(rel)
    for rel in list(moves):
        m = re.fullmatch(r'maps/([^/]+)\.bsp', rel)
        if not m or (rel in expected and not uninstall):
            continue
        for ext in ('.nod', '.nrp'):
            g = 'maps/graphs/' + m.group(1) + ext
            if os.path.isfile(find(game, g)):
                moves.append(g)
    if not uninstall:
        say('Already current: %d file(s). Old co-op files to set aside: %d.' % (kept, len(moves)))
    for s in skipped:
        say('Left in place (not from this project, or changed since): %s' % s)
    if check_only:
        for rel in moves:
            say('  would set aside %s' % rel)
        say('Check only: nothing changed.')
        return
    if moves:
        stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
        aside = os.path.join(os.path.dirname(os.path.dirname(game)), 'Cry of Fear - removed co-op files', stamp)
        os.makedirs(aside, exist_ok=True)
        for rel in moves:
            target = os.path.join(aside, rel)
            os.makedirs(os.path.dirname(target), exist_ok=True)
            shutil.move(find(game, rel), target)
        with open(os.path.join(aside, 'MOVED FILES.txt'), 'w') as f:
            f.write('Moved here from %s by coop-linux.py (%s%s).\nNothing in this folder is needed to play. Delete the folder whenever you like.\n\n'
                    % (game, version, ', uninstall' if uninstall else ''))
            f.write('\n'.join(moves) + '\n')
        say('Set aside %d co-op file(s) in: %s' % (len(moves), aside))
        say('Delete that folder yourself once you are happy (it frees the disk space).')
    if uninstall:
        say('DONE: the co-op mod is uninstalled. Your co-op saves (storycoop_saves) and logs were kept.')
        return
    say('Installing: the co-op maps are built from your own game files (a few minutes the first time). Please wait...')
    install_package(game, catalog, progress=lambda t: print('  ' + t, flush=True))
    say('Checking the installed files...')
    bad = []
    for rel, want in expected.items():
        path = find(game, rel)
        if os.path.isfile(path) and (file_sha(path) == want or rel in mutable):
            continue
        extra = next((x for x in catalog.get('extras') or [] if x and x['target'] == rel), None)
        if extra and not os.path.isfile(path) and (extra.get('optional') or extra.get('deltaFrom') or extra.get('build')):
            continue
        bad.append(rel)
    if bad:
        for rel in bad:
            say('Not as expected: %s' % rel)
        raise Stop('Verification failed after install.')
    say('DONE: co-op %s installed and verified (%d files). Saves and the game\'s own files were not touched.' % (version, len(expected)))


# ---------------------------------------------------------------- run
def run(game, dry=False):
    if not exists(game, 'cl_dlls/storycoop_rc4.dll'):
        raise Stop('The co-op mod is not installed in %s. Run the installer first (game closed).' % game)
    if game_running() and not dry:
        raise Stop('Close Cry of Fear first.')
    catalog = json.load(open(os.path.join(HERE, 'release.json'), encoding='utf-8-sig'))
    if file_sha(find(game, 'cl_dlls/storycoop_rc4.dll')) != catalog['runtimeAdapter']['sha256']:
        say('Warning: this PC has a different co-op version installed than this folder (%s). Starting it anyway; run the installer to update. '
            'Both players need the same version.' % catalog['coopFixes']['id'])
    addons = find(game, 'addons')
    if os.path.isdir(addons) and [n for n in os.listdir(addons) if n.lower().endswith('.dll')]:
        say('Warning: Cry of Fear add-ons found in %s. RebuiltSimon crashed the game in co-op playtests: move the "addons" folder '
            'somewhere else while playing co-op.' % addons)
    args = ['-condebug', '-insecure']
    name = persona_name()
    if name:
        args += ['+name', '"%s"' % name, '+setinfo', 'sc_name', '"%s"' % name]
    source = os.path.join(os.path.dirname(HERE), 'playermodels')
    if os.path.isdir(source):
        for folder, _, files in os.walk(source):
            for fn in files:
                if not fn.lower().endswith('.mdl'):
                    continue
                stem = os.path.splitext(fn)[0].lower()
                owner = stem
                if len(stem) > 1 and stem.endswith('t') and any(f.lower() == stem[:-1] + '.mdl' for f in files):
                    owner = stem[:-1]
                if not re.fullmatch(r'[a-z0-9_-]{1,32}', owner) or re.fullmatch(r'simon|police[1-4]', owner):
                    continue
                target = os.path.join(find(game, 'models/player'), owner, stem + '.mdl')
                if os.path.exists(find(game, 'models/player/%s/%s.mdl' % (owner, stem))):
                    continue
                try:
                    os.makedirs(os.path.dirname(target), exist_ok=True)
                    shutil.copyfile(os.path.join(folder, fn), target)
                    say('Player model added: %s' % stem)
                except OSError as e:
                    say('Player model %s could not be copied: %s' % (stem, e))
    choice = find(game, 'storycoop_model.txt')
    if os.path.isfile(choice):
        chosen = open(choice, encoding='latin-1').read().strip().lower()
        if re.fullmatch(r'[a-z0-9_-]{1,32}', chosen):
            args += ['+setinfo', 'sc_model', chosen]
    args += ['-dll', 'cl_dlls/storycoop_rc4.dll', '-num_edicts', '4096', '-heapsize', '1536000', '-hw']
    log = os.path.join(os.path.dirname(game), 'qconsole.log')
    if os.path.isfile(log) and not dry:
        archive = os.path.join(game, 'storycoop_logs')
        os.makedirs(archive, exist_ok=True)
        stamp = datetime.datetime.fromtimestamp(os.path.getmtime(log)).strftime('%Y%m%d-%H%M%S')
        shutil.copyfile(log, os.path.join(archive, 'qconsole-%s.log' % stamp))
        for old in sorted((f for f in os.listdir(archive) if f.startswith('qconsole-') and f.endswith('.log')), reverse=True)[20:]:
            os.remove(os.path.join(archive, old))
    if shutil.which('steam'):
        command = ['steam', '-applaunch', APP_ID] + args
    elif shutil.which('flatpak') and subprocess.run(['flatpak', 'info', 'com.valvesoftware.Steam'], capture_output=True).returncode == 0:
        command = ['flatpak', 'run', 'com.valvesoftware.Steam', '-applaunch', APP_ID] + args
    else:
        command = ['xdg-open', 'steam://run/%s//%s/' % (APP_ID, urllib.parse.quote(' '.join(args), safe=''))]
    if dry:
        say(' '.join(command))
        return
    subprocess.Popen(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
    say('Starting Cry of Fear with the co-op mod through Steam. If the game\'s own launcher window opens, click LAUNCH.')
    say('In the main menu choose "Host Simon\'s Story" or "Join Simon\'s Story".')


# --- v0.9: spray logo (the same file the Windows launcher's SPRAY page writes) ---------------
# The engine takes a custom logo of 1024 to 20480 bytes: one miptex of at most 256x256, four mip
# levels, 256 colours; palette entry 255 = 0 0 255 makes it a full-colour decal whose index 255 is
# clear (decomp engine_hw com_custom.c, decals.c, GL_DRAW.C Draw_MiptexTexture).
SPRAY_MAX_PIXELS = 14336


def png_rgba(data):
    if data[:8] != b'\x89PNG\r\n\x1a\n':
        raise Stop('Not a PNG file.')
    pos, chunks, idat = 8, {}, b''
    while pos + 8 <= len(data):
        length, kind = struct.unpack('>I4s', data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if kind == b'IDAT':
            idat += body
        elif kind == b'IEND':
            break
        else:
            chunks[kind] = body
    width, height, depth, colour, _, _, interlace = struct.unpack('>IIBBBBB', chunks[b'IHDR'])
    channels = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}.get(colour)
    if depth != 8 or interlace or channels is None or width * height > 4096 * 4096:
        raise Stop('Use an 8-bit, not interlaced PNG.')
    raw, stride = zlib.decompress(idat), width * channels
    rows, prev, at = [], bytearray(stride), 0
    for _ in range(height):
        kind, line = raw[at], bytearray(raw[at + 1:at + 1 + stride])
        at += 1 + stride
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            up, upleft = prev[i], prev[i - channels] if i >= channels else 0
            if kind == 1:
                line[i] = (line[i] + left) & 255
            elif kind == 2:
                line[i] = (line[i] + up) & 255
            elif kind == 3:
                line[i] = (line[i] + (left + up) // 2) & 255
            elif kind == 4:
                p = left + up - upleft
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - upleft)
                line[i] = (line[i] + (left if pa <= pb and pa <= pc else up if pb <= pc else upleft)) & 255
        rows.append(line)
        prev = line
    palette, alpha = chunks.get(b'PLTE', b''), chunks.get(b'tRNS', b'')
    out = bytearray()
    for line in rows:
        for x in range(width):
            if colour == 6:
                out += line[x * 4:x * 4 + 4]
            elif colour == 2:
                out += line[x * 3:x * 3 + 3] + b'\xff'
            elif colour == 0:
                out += bytes((line[x],) * 3) + b'\xff'
            elif colour == 4:
                out += bytes((line[x * 2],) * 3) + bytes((line[x * 2 + 1],))
            else:
                i = line[x]
                out += palette[i * 3:i * 3 + 3] + bytes((alpha[i] if i < len(alpha) else 255,))
    return width, height, bytes(out)


def bmp_rgba(data):
    if data[:2] != b'BM':
        raise Stop('Not a BMP file.')
    offset, = struct.unpack_from('<I', data, 10)
    width, height, _, bits, compression = struct.unpack_from('<iiHHI', data, 18)
    if bits not in (24, 32) or compression not in (0, 3) or width <= 0 or height == 0:
        raise Stop('Use a 24 or 32-bit BMP.')
    flip, height = height > 0, abs(height)
    stride = (width * bits // 8 + 3) & ~3
    out = bytearray()
    for y in range(height):
        row = offset + (height - 1 - y if flip else y) * stride
        for x in range(width):
            b, g, r = data[row + x * bits // 8:row + x * bits // 8 + 3]
            out += bytes((r, g, b, 255))
    return width, height, bytes(out)


def spray_size(width, height):
    s = min(1.0, 128.0 / max(1, width, height))
    for _ in range(200):
        w, h = max(16, int(width * s) // 16 * 16), max(16, int(height * s) // 16 * 16)
        if w * h <= SPRAY_MAX_PIXELS:
            return w, h
        s *= 0.95
    return 96, 96


def spray_wad(width, height, rgba):
    w, h = spray_size(width, height)
    pixels = []   # area average of the source pixels under each target pixel
    for y in range(h):
        y0, y1 = y * height // h, max(y * height // h + 1, (y + 1) * height // h)
        for x in range(w):
            x0, x1 = x * width // w, max(x * width // w + 1, (x + 1) * width // w)
            r = g = b = a = n = 0
            for sy in range(y0, y1):
                for sx in range(x0, x1):
                    p = (sy * width + sx) * 4
                    pa = rgba[p + 3]
                    r += rgba[p] * pa; g += rgba[p + 1] * pa; b += rgba[p + 2] * pa; a += pa; n += 1
            pixels.append((r // a, g // a, b // a, a // n) if a else (0, 0, 0, 0))
    opaque = [(p[0] << 16) | (p[1] << 8) | p[2] for p in pixels if p[3] >= 128]
    palette = bytearray(768)
    palette[765:768] = b'\x00\x00\xff'
    boxes = [opaque] if opaque else []
    while 0 < len(boxes) < 255:
        best = None
        for i, box in enumerate(boxes):
            if len(box) < 2:
                continue
            for ch in range(3):
                shift = 16 - ch * 8
                vals = [(c >> shift) & 255 for c in box]
                spread = max(vals) - min(vals)
                if spread > 0 and (best is None or spread > best[0]):
                    best = (spread, i, shift)
        if best is None:
            break
        _, i, shift = best
        box = sorted(boxes[i], key=lambda c: (c >> shift) & 255)
        boxes[i], extra = box[:len(box) // 2], box[len(box) // 2:]
        boxes.append(extra)
    for i, box in enumerate(boxes):
        n = len(box)
        palette[i * 3:i * 3 + 3] = bytes((sum((c >> 16) & 255 for c in box) // n, sum((c >> 8) & 255 for c in box) // n, sum(c & 255 for c in box) // n))
    cache, index = {}, bytearray()
    for p in pixels:
        if p[3] < 128:
            index.append(255)
            continue
        key = (p[0], p[1], p[2])
        if key not in cache:
            cache[key] = min(range(len(boxes)), key=lambda k: (p[0] - palette[k * 3]) ** 2 + (p[1] - palette[k * 3 + 1]) ** 2 + (p[2] - palette[k * 3 + 2]) ** 2)
        index.append(cache[key])
    name = b'{LOGO'.ljust(16, b'\0')
    lump, offset, mips = bytearray(name + struct.pack('<II', w, h)), 40, bytearray()
    for m in range(4):
        lump += struct.pack('<I', offset)
        offset += (w >> m) * (h >> m)
        mips += bytes(index[(y << m) * w + (x << m)] for y in range(h >> m) for x in range(w >> m))
    lump += mips + struct.pack('<H', 256) + palette + b'\0\0'
    return b'WAD3' + struct.pack('<ii', 1, 12 + len(lump)) + lump + struct.pack('<iiibbh', 12, len(lump), len(lump), 0x43, 0, 0) + name, (w, h)


def spray(game, picture):
    data = read(picture)
    width, height, rgba = png_rgba(data) if data[:4] == b'\x89PNG' else bmp_rgba(data)
    wad, (w, h) = spray_wad(width, height, rgba)
    if not 1024 <= len(wad) <= 20480:
        raise Stop('This picture does not work as a spray.')
    target = os.path.join(game, 'tempdecal.wad')
    keep = target + '.storycoop-original'
    if os.path.isfile(target) and not os.path.exists(keep):
        shutil.copyfile(target, keep)
    with open(target + '.new', 'wb') as f:
        f.write(wad)
    os.replace(target + '.new', target)
    say('Spray saved (%dx%d, %d bytes): %s. Spray in the game with T (storycoop_comfort.cfg).' % (w, h, len(wad), target))


def main(argv):
    command = argv[1] if len(argv) > 1 else 'help'
    game = None
    if '--game' in argv:
        game = argv[argv.index('--game') + 1]
    if command == 'spray-file' and len(argv) == 4:
        # Tests: spray-file <picture> <out.wad>, no game folder needed.
        data = read(argv[2])
        width, height, rgba = png_rgba(data) if data[:4] == b'\x89PNG' else bmp_rgba(data)
        with open(argv[3], 'wb') as f:
            f.write(spray_wad(width, height, rgba)[0])
        return 0
    if command not in ('install', 'uninstall', 'check', 'run', 'run-dry', 'spray') or (command == 'spray' and (len(argv) < 3 or argv[2] == '--game')):
        print(__doc__ if __doc__ else '', 'Usage: coop-linux.py install|run|uninstall|check|spray <picture.png> [--game <...>/Cry of Fear/cryoffear]')
        return 2
    try:
        if not game:
            game = find_game()
        if not game:
            raise Stop('Cry of Fear was not found in any Steam library. Run again with --game "<...>/steamapps/common/Cry of Fear/cryoffear".')
        game = os.path.realpath(game)
        if not exists(game, 'cl_dlls/hl.dll') or not os.path.isdir(find(game, 'maps')):
            raise Stop('Not a Cry of Fear game folder: %s' % game)
        say('Cry of Fear: %s' % game)
        if command in ('run', 'run-dry'):
            run(game, dry=command == 'run-dry')
            return 0
        if command == 'spray':
            spray(game, argv[2])
            return 0
        if game_running() and command != 'check':
            raise Stop('Close Cry of Fear first. Nothing changed.')
        if not os.access(game, os.W_OK):
            raise Stop('This user cannot write to %s. Nothing changed.' % game)
        setup(game, uninstall=command == 'uninstall', check_only=command == 'check')
        return 0
    except Stop as e:
        say('STOPPED: %s' % e)
        return 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))

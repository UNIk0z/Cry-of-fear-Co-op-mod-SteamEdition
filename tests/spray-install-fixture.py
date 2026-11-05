"""Exercise the real Windows and Linux sound installers in private scratch folders, without running the game."""
import argparse
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
sys.dont_write_bytecode = True

p = argparse.ArgumentParser()
p.add_argument('--game', required=True, type=Path)
p.add_argument('--adapter', required=True, type=Path)
p.add_argument('--scratch', required=True, type=Path)
p.add_argument('--baseline', action='store_true')
a = p.parse_args()
repo = Path(__file__).resolve().parents[1]
a.scratch.mkdir(parents=True, exist_ok=False)
package = a.scratch / 'package'
(package / 'native').mkdir(parents=True)
(package / 'extras').mkdir()
shutil.copyfile(repo / 'scripts/install-story-package.ps1', package / 'install-story-package.ps1')
shutil.copyfile(a.adapter, package / 'native/storycoop_rc4.dll')
wav = subprocess.check_output(['node', '--input-type=module', '-e', "import {spraySound} from './scripts/spray-sound.mjs';process.stdout.write(spraySound());"], cwd=repo)
(package / 'extras/spray.wav').write_bytes(wav)
sha = lambda b: hashlib.sha256(b).hexdigest()
catalog = {'schemaVersion': 2, 'release': 'story-state-rc4', 'maps': [], 'dependencies': [], 'menu': None,
           'nativeDllSha256': sha((a.game / 'cl_dlls/hl.dll').read_bytes()),
           'runtimeAdapter': {'file': 'storycoop_rc4.dll', 'sha256': sha(a.adapter.read_bytes()), 'fileBytes': a.adapter.stat().st_size},
           'extras': [{'target': 'sound/storycoop/spray.wav', 'file': 'extras/spray.wav', 'sha256': sha(wav), 'fileBytes': len(wav)}]}
(package / 'release.json').write_text(json.dumps(catalog), encoding='utf8')
games = {}
for platform in ['windows', 'linux']:
    game = a.scratch / platform
    (game / 'maps').mkdir(parents=True)
    (game / 'cl_dlls').mkdir()
    shutil.copyfile(a.game / 'cl_dlls/hl.dll', game / 'cl_dlls/hl.dll')
    games[platform] = game

def windows(*flags):
    result = subprocess.run(['powershell.exe', '-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass', '-File', str(package / 'install-story-package.ps1'), '-MapsDirectory', str(games['windows'] / 'maps'), '-SkipMenu', *flags], capture_output=True, text=True)
    with (a.scratch / 'windows.log').open('a', encoding='utf8') as log:
        log.write(result.stdout + result.stderr + '\n')
    return result

spec = importlib.util.spec_from_file_location('coop_spray_installer', repo / 'linux/coop-linux.py')
linux = importlib.util.module_from_spec(spec)
spec.loader.exec_module(linux)
linux.HERE = str(package)
win = windows()
linux_error = None
try:
    linux.install_package(str(games['linux']), catalog)
except linux.Stop as e:
    linux_error = str(e)
if a.baseline:
    assert win.returncode != 0 and 'Invalid extra file catalog' in win.stderr
    assert linux_error == 'Invalid extra file catalog.'
    print('FAIL baseline: both real installers reject sound/storycoop/spray.wav before writing it')
    raise SystemExit(1)
assert win.returncode == 0, win.stdout + win.stderr
assert linux_error is None, linux_error
for game in games.values():
    assert (game / 'sound/storycoop/spray.wav').read_bytes() == wav
assert windows('-VerifyOnly').returncode == 0
linux.install_package(str(games['linux']), catalog, verify_only=True)
for platform, game in games.items():
    sound = game / 'sound/storycoop/spray.wav'
    sound.write_bytes(b'user changed this file')
    if platform == 'windows':
        refusal = windows()
        assert refusal.returncode != 0 and 'Unknown/changed file retained' in refusal.stderr
    else:
        try:
            linux.install_package(str(game), catalog)
            raise AssertionError('changed user sound accepted')
        except linux.Stop as e:
            assert 'Unknown/changed file retained' in str(e)
    assert sound.read_bytes() == b'user changed this file'
    sound.write_bytes(wav)
bad = copy.deepcopy(catalog)
bad['extras'][0]['target'] = 'sound/storycoop/other.wav'
(package / 'release.json').write_text(json.dumps(bad), encoding='utf8')
refusal = windows()
assert refusal.returncode != 0 and 'Invalid extra file catalog' in refusal.stderr
try:
    linux.install_package(str(games['linux']), bad)
    raise AssertionError('unexpected sound target accepted')
except linux.Stop as e:
    assert str(e) == 'Invalid extra file catalog.'
(package / 'release.json').write_text(json.dumps(catalog), encoding='utf8')
assert windows('-Remove').returncode == 0
assert not (games['windows'] / 'sound/storycoop/spray.wav').exists()
assert not (games['windows'] / 'cl_dlls/storycoop_rc4.dll').exists()
assert sha((games['windows'] / 'cl_dlls/hl.dll').read_bytes()) == catalog['nativeDllSha256']
print('PASS spray installers: Windows and Linux install and verify exact WAV bytes, preserve changed user files, reject unexpected sound targets, and Windows removes only the fixture mod files. No game launched or installed.')

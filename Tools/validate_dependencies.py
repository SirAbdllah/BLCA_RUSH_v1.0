"""Validate that runtime resource references resolve to this project or installed vanilla data."""
from pathlib import Path
import argparse
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--base-data', type=Path, default=Path(r'C:\Program Files (x86)\Steam\steamapps\common\Arma Reforger\addons\data'))
args = parser.parse_args()
database = args.base_data / 'resourceDatabase.rdb'
if not database.is_file():
    parser.error('Supply --base-data with the installed vanilla addons/data directory.')
data = database.read_bytes()
errors = []
project = (ROOT / 'addon.gproj').read_text(encoding='utf-8-sig')
dependencies = re.search(r'Dependencies\s*\{([^}]+)\}', project)[1]
if re.findall(r'"([A-Fa-f0-9]{16})"', dependencies) != ['58D0FB3206B6F859']:
    errors.append('The project must declare only the vanilla base-game dependency.')

owned = {}
for meta in ROOT.rglob('*.meta'):
    match = re.search(r'Name "\{([A-Fa-f0-9]{16})\}([^\"]+)"', meta.read_text(encoding='utf-8-sig'))
    if match:
        owned[match[2].lower()] = match[1].upper()
refs = set()
extensions = {'.c', '.et', '.ent', '.conf', '.layer', '.layout', '.gproj'}
for path in ROOT.rglob('*'):
    if not path.is_file() or path.suffix.lower() not in extensions:
        continue
    text = path.read_text(encoding='utf-8-sig')
    for guid, relative in re.findall(r'\{([A-Fa-f0-9]{16})\}([A-Za-z0-9_./ -]+\.(?:et|xob|edds|emat|ptc|layout|conf|ent|imageset|fnt|acp))', text):
        refs.add((guid.upper(), relative))
    if re.search(r'\b(?:HFS_|WCS_|NE_)\w*|NewEra|New_Era|Black.?Sh(?:ield|eild|elid)|blcaSquadClient|BLCA_NetFixture', text, re.I):
        errors.append('Unexpected legacy dependency or private fixture in ' + str(path.relative_to(ROOT)))

native = 0
local = 0
for guid, relative in sorted(refs):
    if relative.lower() in owned:
        local += 1
        if owned[relative.lower()] != guid or not (ROOT / relative).is_file():
            errors.append('Local resource mismatch: {' + guid + '}' + relative)
        continue
    # RDBC entries store the resource GUID after the null-terminated resource name and flags.
    encoded = relative.encode('utf-8')
    start = data.find(encoded + b'\0')
    if start < 0:
        errors.append('Resource is not in this project or vanilla data: ' + relative)
        continue
    position = start + len(encoded) + 7
    registered = int.from_bytes(data[position:position + 8], 'little')
    if f'{registered:016X}' != guid:
        errors.append('Vanilla resource GUID mismatch: {' + guid + '}' + relative + ' (installed ' + f'{registered:016X}' + ')')
    native += 1

for path in ROOT.rglob('*.json'):
    try:
        json.loads(path.read_text(encoding='utf-8-sig'))
    except (ValueError, UnicodeError) as exc:
        errors.append(str(path.relative_to(ROOT)) + ': ' + str(exc))
result = {'passed': not errors, 'dependency': '58D0FB3206B6F859', 'localReferences': local, 'vanillaReferences': native, 'errors': errors}
print(json.dumps(result, indent=2))
sys.exit(bool(errors))

"""Filter the five static Central Economy placements removed by RailPilotAI."""

import argparse
from math import dist
from pathlib import Path
import tempfile
from xml.etree import ElementTree as ET


PORT_STOCK_LOOT = {
    'Land_Train_Wagon_Box': (
        (14114.150, 5.070, 13182.092),
        (14117.464, 5.070, 13170.439),
        (14120.896, 5.070, 13158.881),
        (14124.678, 5.070, 13147.435),
    ),
    'Land_Train_742_Blue': ((14129.393, 5.612, 13135.567),),
}


def remove_port_loot(mission, *, apply=True, backup=False):
    """Remove the baked CE placements of the five obstructing port objects."""
    target = mission / 'mapgrouppos.xml'
    parser = ET.XMLParser(target=ET.TreeBuilder(insert_comments=True))
    tree = ET.parse(target, parser=parser)
    groups = tree.getroot()
    if groups.tag != 'map':
        raise ValueError(f'Unrecognized mapgrouppos.xml root: {groups.tag}')
    removed = 0
    for group in groups.findall('group'):
        origins = PORT_STOCK_LOOT.get(group.get('name'))
        if not origins:
            continue
        position = tuple(float(value) for value in group.get('pos', '').split())
        if len(position) == 3 and any(dist(position, origin) < 0.08 for origin in origins):
            groups.remove(group)
            removed += 1
    if removed and apply:
        if backup:
            original = target.with_suffix('.xml.railpilot.bak')
            try:
                with original.open('xb') as saved:
                    saved.write(target.read_bytes())
            except FileExistsError:
                if original.read_bytes() != target.read_bytes():
                    raise ValueError(f'A different backup already exists at {original}')
        with tempfile.NamedTemporaryFile(dir=target.parent, prefix='railpilot-', suffix='.xml', delete=False) as handle:
            temporary = Path(handle.name)
        try:
            tree.write(temporary, encoding='utf-8', xml_declaration=True)
            temporary.replace(target)
        finally:
            temporary.unlink(missing_ok=True)
    return removed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mission', type=Path, help='Chernarus mission directory containing mapgrouppos.xml')
    parser.add_argument('--apply', action='store_true', help='save the filtered XML and keep the original as a backup')
    args = parser.parse_args()
    removed = remove_port_loot(args.mission, apply=args.apply, backup=args.apply)
    if args.apply:
        print(f'Targeted Svetloyarsk placements removed: {removed}')
    else:
        print(f'Targeted Svetloyarsk placements found: {removed}; preview only. Use --apply to save.')


if __name__ == '__main__':
    main()

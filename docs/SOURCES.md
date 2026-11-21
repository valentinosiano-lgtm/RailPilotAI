# Source layout

- `src/addons/` contains the game scripts, add-on configurations and assets. The continuous route and service are in `first-route`. The construction corridor and large-obstacle handling have their own add-ons.
- `src/optional/ai-convoy-traffic/` contains the independent adapter for Moving AI Convoy. It requires RailPilotAI and Triumph's mod.
- `recordings/` contains the project's source recordings. The in-game samples sit beside their add-on configurations.
- `server/` contains the service configuration example and the Svetloyarsk loot filter.
- `src/manifest.json` records the add-on prefixes, hashes of the tested reference PBOs and SHA-256 checksums of the uncompressed geometry datasets.

## Working with route data

The larger generated JSON files are stored as gzip archives. From the repository root, run the following in Python 3 to restore them next to their archives. The script preserves existing files so local edits remain intact.

```python
import gzip
from pathlib import Path
import shutil

for archive in Path("src/addons").rglob("*.json.gz"):
    target = archive.with_suffix("")
    if target.exists():
        continue
    with gzip.open(archive, "rb") as source, target.open("xb") as output:
        shutil.copyfileobj(source, output)
```

When preparing an add-on, use the uncompressed file with its original name. Keep the prefix listed in the manifest. DayZ Tools and the relevant game dependencies are needed to build and run the mod. The tested build came from the project's local packaging tools. A clean-checkout build has yet to be verified.

## Adapting another map

This revision ships as a single mod for Chernarus. A map port needs its own track geometry, stations, crossings and construction corridor. Several scripts also hardcode Chernarus and its route structure. A map-independent Core and a public map extension interface are potential community contributions.

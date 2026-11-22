# Dedicated server setup

These steps apply to the built RailPilotAI mod on a dedicated DayZ Chernarus server.

1. Install the built mod and add `-mod=@RailPilotAI` to the server's existing mod list. Players load the same version. Copy the public `.bikey` to the server's `keys` directory and keep your existing mission and `init.c`.
2. With the server stopped, preview the change to the five Central Economy loot entries for the static rail vehicles cleared at Svetloyarsk. Run the second command to apply it and keep a backup of the original `mapgrouppos.xml`.

   ```sh
   python server/railpilot_port_loot.py "path/to/active-mission"
   python server/railpilot_port_loot.py "path/to/active-mission" --apply
   ```

3. To choose the first stop, copy [Service.example.json](Service.example.json) into the folder specified by `-profiles` as `RailPilot/Service.json`. Set `StartStation` before the first launch. Without this file, the service starts in Komarovo and generates it. Keep `Schema` set to `1`. Set `Enabled` to `false` to disable the service.

Use the station names in the route. Enter `Scalo ovest Myshkino` for Myshkino West Yard and `Berezino Nord` for Berezino North. An existing valid save takes priority over `StartStation`.

Keep `RailPilot/Ring` in the server profiles folder between restarts. On recovery the train waits two minutes at its saved position. Returning passengers recorded in the save are placed on clear ground near the recovery point when they log in.

RailPilotAI announces arrivals, departures and skipped stations to connected players in English. Each arrival includes the planned dwell and following station. If the server restarts during a dwell, the next announcement is sent when the train departs.

When an empty train bypasses an obstacle, it stops at the landing point for two minutes. Passengers can walk to the train and reboard before it gradually resumes the route.

For Moving AI Convoy support, load the built adapter after RailPilotAI and Moving AI Convoy on the server and clients. Install the public keys for both dependencies and the adapter. Road convoy routes are configured in Moving AI Convoy.

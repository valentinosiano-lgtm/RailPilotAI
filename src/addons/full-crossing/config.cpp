class CfgPatches
{
    class RailPilot_FirstRouteFullCrossing
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_FirstRoute", "RailPilot_FullCrossingAssets"};
    };
};
class CfgMods
{
    class RailPilotFirstRouteFullCrossing
    {
        name = "RailPilotAI - quattro sbarre dopo Kamenka";
        type = "mod";
        dependencies[] = {"World"};
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"RailPilot/FirstRouteFullCrossing/4_World"};
            };
        };
    };
};

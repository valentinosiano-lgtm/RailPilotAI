class CfgPatches
{
    class RailPilot_RailCorridorAutomatic
    {
        units[] = {}; weapons[] = {}; requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_RailCorridor"};
    };
};
class CfgMods
{
    class RailPilotRailCorridorAutomatic
    {
        name = "RailPilotAI - protezione integrata"; type = "mod";
        dependencies[] = {"World"};
        class defs { class worldScriptModule { value = ""; files[] = {"RailPilot/RailCorridorAutomatic/4_World"}; }; };
    };
};

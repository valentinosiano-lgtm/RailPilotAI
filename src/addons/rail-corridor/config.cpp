class CfgPatches
{
    class RailPilot_RailCorridor
    {
        units[] = {}; weapons[] = {}; requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_FirstRoute"};
    };
};
class CfgMods
{
    class RailPilotRailCorridor
    {
        name = "RailPilotAI - protezione prima tratta"; type = "mod";
        dependencies[] = {"World"};
        class defs { class worldScriptModule { value = ""; files[] = {"RailPilot/RailCorridor/4_World"}; }; };
    };
};

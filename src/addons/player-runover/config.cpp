class CfgPatches
{
    class RailPilot_PlayerRunOver
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_FirstRoute", "RailPilot_FirstRouteFullCrossing"};
    };
};
class CfgMods
{
    class RailPilotPlayerRunOver
    {
        name = "RailPilotAI - investimento frontale giocatori e animali";
        type = "mod";
        dependencies[] = {"World"};
        class defs
        {
            class worldScriptModule { value = ""; files[] = {"RailPilot/PlayerRunOver/4_World"}; };
        };
    };
};

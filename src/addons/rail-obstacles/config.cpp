class CfgPatches
{
    class RailPilot_RailObstacles
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_RailCorridorAutomatic", "RailPilot_BrakeAir"};
    };
};

class CfgMods
{
    class RailPilotRailObstacles
    {
        name = "RailPilotAI - ostacoli sulla prima tratta";
        type = "mod";
        dependencies[] = {"Game", "World"};
        class defs
        {
            class gameScriptModule { value = ""; files[] = {"RailPilot/RailObstacles/3_Game"}; };
            class worldScriptModule { value = ""; files[] = {"RailPilot/RailObstacles/4_World"}; };
        };
    };
};

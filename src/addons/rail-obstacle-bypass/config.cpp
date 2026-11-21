class CfgPatches
{
    class RailPilot_RailObstacleBypass
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_RailObstacles", "RailPilot_TrainBrakeAudio"};
    };
};

class CfgMods
{
    class RailPilotRailObstacleBypass
    {
        name = "RailPilotAI - recupero del treno vuoto";
        type = "mod";
        dependencies[] = {"Game", "World"};
        class defs
        {
            class gameScriptModule { value = ""; files[] = {"RailPilot/RailObstacleBypass/3_Game"}; };
            class worldScriptModule { value = ""; files[] = {"RailPilot/RailObstacleBypass/4_World"}; };
        };
    };
};

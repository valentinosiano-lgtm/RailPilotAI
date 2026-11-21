class CfgPatches
{
    class RailPilot_Scripts
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};

class CfgMods
{
    class RailPilot
    {
        dir = "RailPilot";
        name = "RailPilot — Original Railway Controller";
        type = "mod";
        dependencies[] = {"Game", "World", "Mission"};

        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] = {"RailPilot/Scripts/3_Game"};
            };
            class worldScriptModule
            {
                value = "";
                files[] = {"RailPilot/Scripts/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"RailPilot/Scripts/5_Mission"};
            };
        };
    };
};

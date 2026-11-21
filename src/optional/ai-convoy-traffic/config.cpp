class CfgPatches
{
    class RailPilot_AIConvoyTraffic
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_FirstRoute", "AIConvoy"};
    };
};
class CfgMods
{
    class RailPilotAIConvoyTraffic
    {
        name = "RailPilotAI - AI Convoy compatibility";
        type = "mod";
        dependencies[] = {"World"};
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"RailPilot/AIConvoyTraffic/4_World"};
            };
        };
    };
};

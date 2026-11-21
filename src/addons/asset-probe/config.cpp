// PRIVATE feasibility addon. Never included in the RailPilot controller PBO.
// References an installed vanilla model; contains no copied model/texture.
class CfgPatches
{
    class RailPilot_AssetProbe
    {
        units[] = {"RP_LocomotiveBodyProbe", "RP_LocomotiveVisualProbe"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data", "RailPilot_Scripts"};
    };
};

class CfgMods
{
    class RailPilotAssetProbe
    {
        type = "mod";
        dependencies[] = {"World", "Mission"};
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"RailPilot/AssetProbe/4_World"};
            };
            class missionScriptModule
            {
                value = "";
                files[] = {"RailPilot/AssetProbe/5_Mission"};
            };
        };
    };
};

class CfgVehicles
{
    class Inventory_Base;
    class RP_LocomotiveBodyProbe : Inventory_Base
    {
        scope = 1;
        displayName = "RailPilot private physics probe - no passengers";
        descriptionShort = "Engineering test only";
        model = "\DZ\structures\Wrecks\Trains\Train_742_Blue.p3d";
        weight = 70000000;
        itemSize[] = {10, 10};
        physLayer = "item_large";
    };
    class RP_LocomotiveVisualProbe : Inventory_Base
    {
        scope = 1;
        displayName = "RailPilot local visual probe";
        model = "\DZ\structures\Wrecks\Trains\Train_742_Blue.p3d";
        itemSize[] = {10, 10};
        physLayer = "item_large";
    };
};

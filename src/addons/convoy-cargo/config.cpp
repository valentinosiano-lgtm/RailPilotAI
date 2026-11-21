class CfgPatches
{
    class RailPilot_ConvoyCargo
    {
        units[] = {"RP_ConvoyCargoStore"}; weapons[] = {}; requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_BrakeAir", "RailPilot_FirstRoute", "DZ_Gear_Camping"};
    };
};
class CfgMods
{
    class RailPilotConvoyCargo
    {
        name = "RailPilotAI"; type = "mod"; dependencies[] = {"World"};
        class defs
        {
            class worldScriptModule { value = ""; files[] = {"RailPilot/ConvoyCargo/4_World"}; };
        };
    };
};
class CfgSlots
{
    class Slot_RP_ConvoyCargo
    {
        name = "RP_ConvoyCargo";
        displayName = "Deposito del vagone";
        ghostIcon = "set:dayz_inventory image:cat_common_cargo";
    };
};
class CfgVehicles
{
    class RP_ConvoyUnit;
    class Container_Base;
    class RP_ConvoyCargoStore : Container_Base
    {
        scope = 1;
        displayName = "Deposito vagone - 200 slot";
        descriptionShort = "Deposito del vagone coperto. Il contenuto dura fino al restart del server.";
        model = "\DZ\gear\containers\Protector_Case.p3d";
        inventorySlot[] = {"RP_ConvoyCargo"};
        itemSize[] = {10, 20};
        itemsCargoSize[] = {10, 20};
        weight = 0;
        openable = 0;
        allowOwnedCargoManipulation = 1;
    };
    class RP_ConvoyBox : RP_ConvoyUnit
    {
        displayName = "RailPilotAI vagone coperto - deposito";
        attachments[] = {"RP_ConvoyCargo"};
    };
};

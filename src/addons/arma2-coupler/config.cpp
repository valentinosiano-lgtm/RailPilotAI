class CfgPatches
{
    class RailPilot_Arma2Coupler
    {
        units[] = {"RP_Arma2ScrewCoupler", "RP_Arma2HookOriginal"}; weapons[] = {}; requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_ConvoyCargo", "RailPilot_FirstRoute", "DZ_Data"};
    };
};
class CfgMods
{
    class RailPilotArma2Coupler
    {
        name="RailPilotAI - ganci Arma2"; type="mod"; dependencies[]={"World","Mission"};
        class defs
        {
            class worldScriptModule { value=""; files[]={"RailPilot/Arma2Coupler/4_World"}; };
            class missionScriptModule { value=""; files[]={"RailPilot/Arma2Coupler/5_Mission"}; };
        };
    };
};
class CfgVehicles
{
    class HouseNoDestruct;
    class RP_Arma2ScrewCoupler : HouseNoDestruct
    {
        scope=1; displayName="RailPilotAI tenditore Arma2";
        model="\RailPilot\Arma2Coupler\models\rp_screw_link.p3d";
        class AnimationSources { class span { source="user"; initPhase=0; animPeriod=0; }; };
    };
    class RP_Arma2HookOriginal : HouseNoDestruct
    {
        scope=1; displayName="RailPilotAI gancio originale Arma2 estratto";
        model="\RailPilot\Arma2Coupler\models\rp_hook_original.p3d";
    };
};

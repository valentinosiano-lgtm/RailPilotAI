class CfgPatches
{
    class RailPilot_Arma2CrossingAssets
    {
        units[] = {"Land_rp_arma2_crossing_barrier"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"DZ_Data"};
    };
};
class CfgVehicles
{
    class HouseNoDestruct;
    class Land_rp_arma2_crossing_barrier : HouseNoDestruct
    {
        scope = 1;
        displayName = "RailPilot Arma 2 crossing barrier";
        model = "\RailPilot\Arma2CrossingAssets\models\rp_arma2_crossing_barrier.p3d";
        forceFarBubble = 1;
        class AnimationSources
        {
            class RPArm { source = "user"; initPhase = 0; animPeriod = 5; };
        };
    };
    class RP_Arma2CrossingVisual : Land_rp_arma2_crossing_barrier {};
};

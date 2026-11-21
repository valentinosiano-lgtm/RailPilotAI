class CfgPatches
{
    class RailPilot_FullCrossingAssets
    {
        units[] = {"Land_rp_full_crossing_barrier"}; weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_FirstRoute"};
    };
};
class CfgVehicles
{
    class Land_rp_crossing_barrier;
    class RP_CrossingBarrierVisual;
    class Land_rp_full_crossing_barrier : Land_rp_crossing_barrier
    {
        model = "\RailPilot\FullCrossingAssets\models\rp_arma2_crossing_barrier.p3d";
    };
    class RP_FullCrossingBarrierVisual : RP_CrossingBarrierVisual
    {
        model = "\RailPilot\FullCrossingAssets\models\rp_arma2_crossing_barrier.p3d";
        class MarkerLights {};
    };
};

class CfgPatches
{
    class RailPilot_Crossings
    {
        units[] = {"Land_rp_crossing_barrier"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_ApproachPreview", "RailPilot_Arma2CrossingAssets"};
    };
};
class CfgMods
{
    class RailPilotCrossings
    {
        type = "mod";
        dependencies[] = {"World", "Mission"};
        class defs
        {
            class worldScriptModule { value = ""; files[] = {"RailPilot/Crossings/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"RailPilot/Crossings/5_Mission"}; };
        };
    };
};
class CfgVehicles
{
    class HouseNoDestruct;
    class Inventory_Base;
    class Land_rp_crossing_barrier : HouseNoDestruct
    {
        scope = 1;
        displayName = "RailPilot automatic level crossing";
        model = "\RailPilot\Arma2CrossingAssets\models\rp_arma2_crossing_barrier.p3d";
        forceFarBubble = 1;
        class AnimationSources
        {
            class RPArm { source = "user"; initPhase = 0; animPeriod = 5; };
        };
    };
    // Local render copy; the authoritative native building owns collision.
    class RP_CrossingBarrierVisual : HouseNoDestruct
    {
        scope = 1;
        model = "\RailPilot\Arma2CrossingAssets\models\rp_arma2_crossing_barrier.p3d";
        class AnimationSources
        {
            class RPArm { source = "user"; initPhase = 0; animPeriod = 5; };
        };
    };
    class RP_CrossingLampVisual : HouseNoDestruct
    {
        scope = 1;
        model = "\RailPilot\Crossings\models\rp_crossing_lamp.p3d";
    };
    // Private test support: vanilla embanked rail collision under flat track.
    // Hidden by script on each machine; no generated collision API is used.
    class RP_CrossingRailGuide : HouseNoDestruct
    {
        scope = 1;
        model = "\DZ\structures\rail\tracks\rail_tracke_40.p3d";
        forceFarBubble = 1;
    };
};
class CfgSoundShaders
{
    class RP_PreviewIdle_Shader;
    class RP_CrossingAlarm_Shader : RP_PreviewIdle_Shader
    {
        samples[] = {{"RailPilot\Crossings\sounds\crossing_alarm", 1}};
        volume = 1;
        frequency = 1;
        range = 300;
    };
};
class CfgSoundSets
{
    class RP_PreviewIdle_SoundSet;
    class RP_CrossingAlarm_SoundSet : RP_PreviewIdle_SoundSet
    {
        soundShaders[] = {"RP_CrossingAlarm_Shader"};
        // Two fixed speakers, one per barrier; preserve the combined gain.
        volumeFactor = 0.5;
        frequencyFactor = 1;
        spatial = 1;
        loop = 1;
        doppler = 0;
        sound3DProcessingType = "default3DProcessingType";
        volumeCurve = "RP_PreviewDistanceCurve";
    };
};

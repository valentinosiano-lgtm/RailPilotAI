// Private straight-track ground/contact audition; references installed models.
class CfgPatches
{
    class RailPilot_ApproachPreview
    {
        units[] = {"RP_WagonBodyProbe", "RP_WagonVisualProbe"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_AudioPreview", "DZ_Sounds_Effects"};
    };
};
class CfgSoundCurves
{
    class RifleShotCurve;
    class RP_ApproachHornDistanceCurve : RifleShotCurve
    {
        // Keep the previous horn's unattenuated first 40m for the requested
        // nearby gain. Join the installed RifleShotCurve at 100m; all farther
        // points are the native rifle curve at a 3000m range.
        points[] = {
            {0,1},{0.013333333,1},{0.033333333,0.374074074},
            {0.1,0.3},{0.15,0.27},{0.2,0.25},{0.25,0.2},
            {0.3,0.15},{0.35,0.107},{0.4,0.0861},{0.45,0.0797},
            {0.5,0.0666},{0.6,0.0575},{0.7,0.0444},{0.8,0.0355},
            {0.9,0.03},{0.95,0.0255},{1,0}
        };
    };
};
class CfgMods
{
    class RailPilotApproachPreview
    {
        type = "mod";
        dependencies[] = {"World", "Mission"};
        class defs
        {
            class worldScriptModule { value = ""; files[] = {"RailPilot/ApproachPreview/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"RailPilot/ApproachPreview/5_Mission"}; };
        };
    };
};
class CfgVehicles
{
    class Inventory_Base;
    class RP_LocomotiveBodyProbe : Inventory_Base
    {
        forceFarBubble = 1;
    };
    class RP_WagonBodyProbe : Inventory_Base
    {
        scope = 1;
        displayName = "RailPilot private wagon - no passengers";
        model = "\DZ\structures\Wrecks\Trains\Train_Wagon_Box.p3d";
        weight = 30000000;
        itemSize[] = {10,10};
        physLayer = "item_large";
        forceFarBubble = 1;
    };
    class RP_WagonVisualProbe : RP_WagonBodyProbe {};
};
class CfgSoundShaders
{
    class RP_PreviewIdle_Shader;
    class RP_PreviewSlow_Shader;
    class RP_PreviewCruise_Shader;
    class RP_PreviewHorn_Shader;
    class RP_ApproachIdle_Shader : RP_PreviewIdle_Shader { range = 450; };
    class RP_ApproachSlow_Shader : RP_PreviewSlow_Shader { range = 450; };
    class RP_ApproachCruise_Shader : RP_PreviewCruise_Shader { range = 450; };
    class RP_ApproachHorn_Shader : RP_PreviewHorn_Shader { range = 3000; };
};
class CfgSoundSets
{
    class RP_PreviewIdle_SoundSet;
    class RP_ApproachIdle_SoundSet : RP_PreviewIdle_SoundSet { soundShaders[] = {"RP_ApproachIdle_Shader"}; volumeFactor = 2; };
    class RP_ApproachSlow_SoundSet : RP_PreviewIdle_SoundSet { soundShaders[] = {"RP_ApproachSlow_Shader"}; volumeFactor = 2; };
    class RP_ApproachCruise_SoundSet : RP_PreviewIdle_SoundSet { soundShaders[] = {"RP_ApproachCruise_Shader"}; volumeFactor = 2; };
    // Inherit installed DayZ rifle attenuation, filtering, occlusion and panner.
    // Retain the private horn recording, without any gunshot sample layers.
    class Rifle_Shot_Base_SoundSet;
    class RP_ApproachHorn_SoundSet : Rifle_Shot_Base_SoundSet
    {
        soundShaders[] = {"RP_ApproachHorn_Shader"};
        volumeFactor = 3;
        volumeCurve = "RP_ApproachHornDistanceCurve";
        frequencyFactor = 1;
        frequencyRandomizer = 0;
        volumeRandomizer = 0;
        spatial = 1;
        loop = 0;
    };
};

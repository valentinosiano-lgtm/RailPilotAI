// Private audition only. Third-party horn is excluded from the public package.
class CfgPatches
{
    class RailPilot_AudioPreview
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_AssetProbe"};
    };
};
class CfgMods
{
    class RailPilotAudioPreview
    {
        type = "mod";
        dependencies[] = {"World", "Mission"};
        class defs
        {
            class worldScriptModule { value = ""; files[] = {"RailPilot/AudioPreview/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"RailPilot/AudioPreview/5_Mission"}; };
        };
    };
};
class CfgSoundShaders
{
    class RP_PreviewIdle_Shader
    {
        samples[] = {{"RailPilot\AudioPreview\samples\engine_idle_loop", 1}};
        volume = 1;
        frequency = 1;
        range = 150;
    };
    class RP_PreviewSlow_Shader : RP_PreviewIdle_Shader
    {
        samples[] = {{"RailPilot\AudioPreview\samples\engine_slow_loop", 1}};
    };
    class RP_PreviewCruise_Shader : RP_PreviewIdle_Shader
    {
        samples[] = {{"RailPilot\AudioPreview\samples\engine_cruise_loop", 1}};
    };
    class RP_PreviewHorn_Shader : RP_PreviewIdle_Shader
    {
        samples[] = {{"RailPilot\AudioPreview\samples\horn_private", 1}};
        range = 500;
    };
};
class CfgSoundCurves
{
    class RP_PreviewDistanceCurve
    {
        points[] = {{0,1},{0.08,1},{0.25,0.65},{0.5,0.25},{0.8,0.07},{1,0}};
    };
};
class CfgSoundSets
{
    class RP_PreviewIdle_SoundSet
    {
        soundShaders[] = {"RP_PreviewIdle_Shader"};
        volumeFactor = 1;
        frequencyFactor = 1;
        frequencyRandomizer = 0;
        volumeRandomizer = 0;
        sound3DProcessingType = "default3DProcessingType";
        volumeCurve = "RP_PreviewDistanceCurve";
        spatial = 1;
        doppler = 0;
        loop = 1;
    };
    class RP_PreviewSlow_SoundSet : RP_PreviewIdle_SoundSet { soundShaders[] = {"RP_PreviewSlow_Shader"}; };
    class RP_PreviewCruise_SoundSet : RP_PreviewIdle_SoundSet { soundShaders[] = {"RP_PreviewCruise_Shader"}; };
    class RP_PreviewHorn_SoundSet : RP_PreviewIdle_SoundSet
    {
        soundShaders[] = {"RP_PreviewHorn_Shader"};
        loop = 0;
    };
};

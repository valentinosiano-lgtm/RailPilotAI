class CfgPatches
{
    class RailPilot_BrakeAir
    {
        units[] = {}; weapons[] = {}; requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_PlayerRunOver", "RailPilot_FirstRoute"};
    };
};
class CfgMods
{
    class RailPilotBrakeAir
    {
        name = "RailPilotAI"; type = "mod"; dependencies[] = {"Game", "World"};
        class defs
        {
            class gameScriptModule { value = ""; files[] = {"RailPilot/BrakeAir/3_Game"}; };
            class worldScriptModule { value = ""; files[] = {"RailPilot/BrakeAir/4_World"}; };
        };
    };
};
class CfgSoundShaders
{
    class RP_BrakeAir_Shader
    {
        samples[] = {{"RailPilot\BrakeAir\samples\brake_air_loop", 1}};
        volume = 1; frequency = 1; range = 80;
    };
};
class CfgSoundSets
{
    class RP_BrakeAir_SoundSet
    {
        soundShaders[] = {"RP_BrakeAir_Shader"};
        volumeFactor = 1; frequencyFactor = 1;
        frequencyRandomizer = 0; volumeRandomizer = 0;
        sound3DProcessingType = "default3DProcessingType";
        volumeCurve = "RP_PreviewDistanceCurve";
        spatial = 1; doppler = 0; loop = 1;
    };
};

class CfgPatches
{
    class RailPilot_TrainBrakeAudio
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_BrakeAir"};
    };
};
class CfgMods
{
    class RailPilotTrainBrakeAudio
    {
        name = "RailPilotAI - train braking audio";
        type = "mod";
        dependencies[] = {"World"};
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] = {"RailPilot/TrainBrakeAudio/4_World"};
            };
        };
    };
};
class CfgSoundShaders
{
    class RP_TrainBrake_Shader
    {
        samples[] = {{"RailPilot\TrainBrakeAudio\samples\train_brake_full", 1}};
        volume = 1;
        frequency = 1;
        range = 150;
    };
    class RP_TrainBrakeMedium_Shader : RP_TrainBrake_Shader
    {
        samples[] = {{"RailPilot\TrainBrakeAudio\samples\train_brake_medium", 1}};
    };
    class RP_TrainBrakeShort_Shader : RP_TrainBrake_Shader
    {
        samples[] = {{"RailPilot\TrainBrakeAudio\samples\train_brake_short", 1}};
    };
};
class CfgSoundSets
{
    class RP_TrainBrake_SoundSet
    {
        soundShaders[] = {"RP_TrainBrake_Shader"};
        volumeFactor = 2;
        frequencyFactor = 1;
        frequencyRandomizer = 0;
        volumeRandomizer = 0;
        sound3DProcessingType = "default3DProcessingType";
        volumeCurve = "RP_PreviewDistanceCurve";
        spatial = 1;
        doppler = 0;
        loop = 0;
    };
    class RP_TrainBrakeMedium_SoundSet : RP_TrainBrake_SoundSet
    {
        soundShaders[] = {"RP_TrainBrakeMedium_Shader"};
    };
    class RP_TrainBrakeShort_SoundSet : RP_TrainBrake_SoundSet
    {
        soundShaders[] = {"RP_TrainBrakeShort_Shader"};
    };
};

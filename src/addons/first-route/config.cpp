class CfgPatches
{
    class RailPilot_FirstRoute
    {
        units[] = {"RP_ConvoyBlue742", "RP_ConvoyTanker", "RP_ConvoyBox", "RP_ConvoyFlat"};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] = {"RailPilot_Crossings", "DZ_Data", "DZ_Structures_Wrecks", "DZ_Vehicles_Parts", "DZ_Sounds_Effects"};
    };
};
class CfgMods
{
    class RailPilotConvoy
    {
        name = "RailPilotAI";
        type = "mod";
        dependencies[] = {"Game", "World", "Mission"};
        class defs
        {
            class gameScriptModule { value = ""; files[] = {"RailPilot/FirstRoute/3_Game"}; };
            class worldScriptModule { value = ""; files[] = {"RailPilot/FirstRoute/4_World"}; };
            class missionScriptModule { value = ""; files[] = {"RailPilot/FirstRoute/5_Mission"}; };
        };
    };
};
class CfgVehicles
{
    class HouseNoDestruct;
    class RP_ConvoyUnit : HouseNoDestruct
    {
        scope = 0;
        animPhysDetachSpeed = 100;
        forceFarBubble = 1;
        hasHistory = 1;
    };
    class RP_ConvoyBlue742 : RP_ConvoyUnit
    {
        scope = 1;
        displayName = "RailPilotAI 742 blu";
        model = "\DZ\structures\Wrecks\Trains\Train_742_Blue.p3d";
    };
    class RP_ConvoyTanker : RP_ConvoyUnit
    {
        scope = 1;
        displayName = "RailPilotAI cisterna benzina";
        model = "\DZ\structures\Wrecks\Trains\Train_Wagon_Tanker.p3d";
        // No unlimited native liquid source: only the finite server action.
        liquidSourceType = 0;
    };
    class RP_ConvoyBox : RP_ConvoyUnit
    {
        scope = 1;
        displayName = "RailPilotAI vagone merci aperto";
        model = "\DZ\structures\Wrecks\Trains\Train_Wagon_Box.p3d";
        // Deliberately no interactive Doors registry. The original model's
        // Door1/Door2 animations are held at phase 1 by RP_ConvoyBox.
        // They are fixed openings, not ordinary lockable building doors.
    };
    class RP_ConvoyFlat : RP_ConvoyUnit
    {
        scope = 1;
        displayName = "RailPilotAI pianale passeggeri";
        model = "\DZ\structures\Wrecks\Trains\Train_Wagon_Flat.p3d";
    };
};

class CfgSoundShaders
{
    class RP_ConvoyHorn_Shader { samples[] = {{"RailPilot\FirstRoute\samples\horn_private",1}}; volume = 1; frequency = 1; range = 3000; };
    class RP_ConvoyIdle_Shader
    {
        samples[] = {{"RailPilot\FirstRoute\samples\engine_idle_loop",1}};
        volume = 1;
        frequency = 1;
        range = 450;
    };
    class RP_ConvoySlow_Shader : RP_ConvoyIdle_Shader { samples[] = {{"RailPilot\FirstRoute\samples\engine_slow_loop",1}}; };
    class RP_ConvoyCruise_Shader : RP_ConvoyIdle_Shader { samples[] = {{"RailPilot\FirstRoute\samples\engine_cruise_loop",1}}; };
};
class CfgSoundCurves
{
    class RifleShotCurve;
    class RP_ConvoyHornDistanceCurve : RifleShotCurve
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

    class RP_ConvoyDistanceCurve { points[] = {{0,1},{0.08,1},{0.25,0.65},{0.5,0.25},{0.8,0.07},{1,0}}; };
};
class CfgSoundSets
{
    class Rifle_Shot_Base_SoundSet;
    class RP_ConvoyHorn_SoundSet : Rifle_Shot_Base_SoundSet
    {
        soundShaders[] = {"RP_ConvoyHorn_Shader"};
        volumeFactor = 3;
        volumeCurve = "RP_ConvoyHornDistanceCurve";
        frequencyFactor = 1; frequencyRandomizer = 0; volumeRandomizer = 0;
        spatial = 1; loop = 0;
    };

    class RP_ConvoyIdle_SoundSet
    {
        soundShaders[] = {"RP_ConvoyIdle_Shader"};
        volumeFactor = 2;
        frequencyFactor = 1;
        frequencyRandomizer = 0;
        volumeRandomizer = 0;
        sound3DProcessingType = "default3DProcessingType";
        volumeCurve = "RP_ConvoyDistanceCurve";
        spatial = 1;
        doppler = 0;
        loop = 1;
    };
    class RP_ConvoySlow_SoundSet : RP_ConvoyIdle_SoundSet { soundShaders[] = {"RP_ConvoySlow_Shader"}; };
    class RP_ConvoyCruise_SoundSet : RP_ConvoyIdle_SoundSet { soundShaders[] = {"RP_ConvoyCruise_Shader"}; };
};

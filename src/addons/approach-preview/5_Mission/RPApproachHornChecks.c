class RPApproachHornChecks
{
    void Check(bool value, string label)
    {
        if (value) Print("[RailPilotApproach] PASS " + label);
        else Print("[RailPilotApproach] FAIL " + label);
    }

    void Run()
    {
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_ApproachIdle_SoundSet volumeFactor") == 2, "nearby idle motor gain doubled");
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_ApproachSlow_SoundSet volumeFactor") == 2, "nearby slow motor gain doubled");
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_ApproachCruise_SoundSet volumeFactor") == 2, "nearby cruise motor gain doubled");
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_ApproachHorn_SoundSet volumeFactor") == 3, "nearby horn gain tripled");
        Check(GetGame().ConfigGetFloat("CfgSoundShaders RP_ApproachHorn_Shader range") == RPApproachHorn.RANGE, "horn world event and shader share the native rifle 3000m range");
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_ApproachHorn_SoundSet spatial") == 1, "distant horn remains a spatial world sound");
        string actual; string nativeSetting;
        GetGame().ConfigGetText("CfgSoundSets RP_ApproachHorn_SoundSet distanceFilter", actual);
        GetGame().ConfigGetText("CfgSoundSets Rifle_Shot_Base_SoundSet distanceFilter", nativeSetting);
        Check(actual == nativeSetting && actual != "", "horn uses installed rifle distance frequency filter");
        GetGame().ConfigGetText("CfgSoundSets RP_ApproachHorn_SoundSet sound3DProcessingType", actual);
        GetGame().ConfigGetText("CfgSoundSets Rifle_Shot_Base_SoundSet sound3DProcessingType", nativeSetting);
        Check(actual == nativeSetting && actual != "", "horn uses installed rifle 3D processor");
        GetGame().ConfigGetText("CfgSoundSets RP_ApproachHorn_SoundSet volumeCurve", actual);
        Check(actual == "RP_ApproachHornDistanceCurve", "horn preserves near gain then follows rifle distance attenuation");
        Check(RPApproachHorn.InRange("100 0 100", "3100 0 100") && !RPApproachHorn.InRange("100 0 100", "3101 0 100"), "distant horn recipient boundary includes 3000m and excludes beyond");
        RPApproachHornGate gate = new RPApproachHornGate;
        Check(gate.Accept(7) && !gate.Accept(7) && !gate.Accept(6) && gate.Accept(8), "live horn plays first event once and rejects duplicate and stale delivery");
        RPApproachHornGate wrap = new RPApproachHornGate;
        Check(wrap.Accept(999999) && wrap.Accept(0) && !wrap.Accept(1000000) && !wrap.Accept(-1), "horn handles sequence rollover and rejects invalid values");
        ScriptReadWriteContext buffer = new ScriptReadWriteContext;
        ParamsWriteContext writer = buffer.GetWriteContext();
        writer.Write(RPApproachHorn.SCHEMA); writer.Write(12); writer.Write(Vector(4854,6,2149));
        int sequence; vector position;
        bool decoded = RPApproachHorn.ReadEvent(buffer.GetReadContext(), sequence, position);
        Check(decoded && sequence == 12 && vector.Distance(position, "4854 6 2149") < 0.001, "global horn payload round-trips without a train object");
        ScriptReadWriteContext malformed = new ScriptReadWriteContext;
        writer = malformed.GetWriteContext(); writer.Write(999);
        Check(!RPApproachHorn.ReadEvent(malformed.GetReadContext(), sequence, position), "horn rejects incompatible event schema");
    }
};

modded class MissionServer
{
    override void OnMissionStart()
    {
        super.OnMissionStart();
        RPApproachHornChecks checks = new RPApproachHornChecks;
        checks.Run();
    }
};

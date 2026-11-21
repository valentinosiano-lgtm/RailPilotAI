modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        RP_LocomotiveBodyProbe.SampleAllRPAudio(timeslice);
    }
};

class RPAudioPreviewChecks
{
    protected int m_Failures;
    void Check(bool valid, string label)
    {
        if (valid)
            Print("[RailPilotAudioTest] PASS " + label);
        else
        {
            m_Failures++;
            Print("[RailPilotAudioTest] FAIL " + label);
        }
    }
    void Run()
    {
        RPAudioMotionGate gate = new RPAudioMotionGate;
        Check(!gate.Update(0, 0.4), "initial idle never greets");
        gate.Update(0.5, 0.1);
        Check(!gate.Update(0, 0.4), "brief motion does not arm a greeting");
        gate.Update(0.5, 0.3);
        Check(!gate.Update(0, 0.1), "greeting waits for sustained standstill");
        Check(gate.Update(0, 0.3), "physical stop produces one greeting");
        Check(!gate.Update(0, 0.5), "remaining stopped never repeats greeting");
        gate.Update(0.5, 0.3);
        Check(gate.Update(0, 0.4), "new movement arms the next greeting");
        RPAudioSequenceGate seq = new RPAudioSequenceGate;
        Check(!seq.Accept(7) && seq.Accept(8) && !seq.Accept(8) && !seq.Accept(7), "join baseline duplicate and stale events are handled");
        RPAudioSequenceGate wrap = new RPAudioSequenceGate;
        Check(!wrap.Accept(999999) && wrap.Accept(0), "sequence rollover remains usable");
        RPAudioEnvelope mix = new RPAudioEnvelope;
        Check(mix.Idle() == 1 && mix.Cruise() == 0, "stationary engine remains at idle");
        for (int i = 0; i < 100; i++)
            mix.Update(1, 0.02);
        float total = mix.Idle() + mix.Slow() + mix.Cruise();
        Check(mix.Cruise() > 0.99 && Math.AbsFloat(total - 1) < 0.0001, "smooth three-layer mix preserves total gain");
        Check(GetGame().ConfigGetFloat("CfgSoundShaders RP_PreviewIdle_Shader range") == 150, "engine range configured at 150m");
        Check(GetGame().ConfigGetFloat("CfgSoundShaders RP_PreviewHorn_Shader range") == 500, "horn range configured at 500m");
        Check(GetGame().ConfigGetFloat("CfgSoundSets RP_PreviewHorn_SoundSet spatial") == 1, "greeting is spatial audio");
        string curve;
        GetGame().ConfigGetText("CfgSoundSets RP_PreviewHorn_SoundSet volumeCurve", curve);
        Check(curve == "RP_PreviewDistanceCurve", "distance attenuation curve attached to greeting");
        Print("[RailPilotAudioTest] COMPLETE failures=" + m_Failures.ToString());
    }
};

modded class MissionServer
{
    override void OnMissionStart()
    {
        super.OnMissionStart();
        RPAudioPreviewChecks checks = new RPAudioPreviewChecks;
        checks.Run();
    }
};

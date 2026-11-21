// Uses the approved barrier classes, assets, alarm, lamp and crossing state machine.
class RPGCHornAudition
{
    protected static ref RPApproachSound s_Sound;
    protected static float s_Elapsed;
    protected static bool s_Decoded;
    protected static int s_Plays;
    protected static bool s_ThisDecoded;
    protected static int s_DecodedPlays;

    static void Play(vector position)
    {
        Release();
        s_Sound = new RPApproachSound;
        s_Sound.SetSoundSet("RP_ApproachHorn_SoundSet");
        s_Sound.SetPosition(position); s_Sound.SetSoundLoop(false); s_Sound.SetSoundAutodestroy(false);
        SEffectManager.EffectRegister(s_Sound); s_Sound.SoundPlay();
        s_Sound.SetCurrentPosition(position); s_Elapsed = 0; s_Plays++; s_ThisDecoded = false;
    }

    static void Update(float dt)
    {
        if (!s_Sound) return;
        s_Elapsed += dt;
        if (!s_ThisDecoded && s_Sound.GetSoundWaveLength() > 1 && s_Sound.IsSoundPlaying())
        {
            s_Decoded = true; s_ThisDecoded = true; s_DecodedPlays++;
        }
        if (s_Elapsed > 15) Release();
    }
    static bool Decoded() { return s_Decoded; }
    static int Plays() { return s_Plays; }
    static int DecodedPlays() { return s_DecodedPlays; }
    static void Release()
    {
        if (!s_Sound) return;
        s_Sound.StopImmediately(); SEffectManager.DestroyEffect(s_Sound); s_Sound = null;
    }
};

// Scenario-specific extension. The approved barrier PBO remains unchanged.
modded class RPApproachSound
{
    float RPGCNativeWaveVolume()
    {
        if (!m_SoundWaveObject) return -1;
        return m_SoundWaveObject.GetVolume();
    }
    void RPGCSetNativeWaveVolume(float volume)
    {
        if (m_SoundWaveObject) m_SoundWaveObject.SetVolume(volume);
    }
};

modded class Land_rp_crossing_barrier
{
    protected bool m_GCInsideClientUpdate;
    protected bool m_GCTailArmed;
    protected int m_GCTailStartMs;
    protected int m_GCTailDeadlineMs;
    protected int m_GCTailCompletions;
    protected float m_GCLastTailSeconds;
    protected float m_GCTailGain = 1;
    protected int m_GCFades;
    protected bool m_GCFadeCounted;
    protected float m_GCFadeWaveStart;
    protected float m_GCLastFadeWaveRatio = 1;

    override void ClientUpdate(float dt)
    {
        m_GCInsideClientUpdate = true;
        super.ClientUpdate(dt);
        m_GCInsideClientUpdate = false;
        if (m_AlarmSound && (m_RPAlarm || m_RenderPhase > 0.0005))
        {
            // A new approach takes priority over a previous sound tail.
            if (m_GCTailDeadlineMs > 0 && m_GCFadeWaveStart > 0) m_AlarmSound.RPGCSetNativeWaveVolume(m_GCFadeWaveStart);
            m_GCTailArmed = true; m_GCTailDeadlineMs = 0;
            m_AlarmSound.SetSoundMaxVolume(1); m_AlarmSound.SetSoundVolume(1);
        }
        else if (m_GCTailDeadlineMs > 0 && GetGame().GetTime() >= m_GCTailDeadlineMs)
        {
            m_GCLastTailSeconds = (GetGame().GetTime() - m_GCTailStartMs) / 1000.0;
            if (m_GCFadeWaveStart > 0) m_GCLastFadeWaveRatio = m_AlarmSound.RPGCNativeWaveVolume() / m_GCFadeWaveStart;
            m_GCTailCompletions++; m_GCTailArmed = false; m_GCTailDeadlineMs = 0;
            Print("[RailPilotGuided] ALARM_TAIL_COMPLETE seconds=" + m_GCLastTailSeconds.ToString());
            Print("[RailPilotGuided] ALARM_FADE nativeWaveRatio=" + m_GCLastFadeWaveRatio.ToString());
            super.StopAlarm();
        }
        else if (m_GCTailDeadlineMs > 0 && m_AlarmSound)
        {
            // Smoothstep fades only the last1.5s of the existing5s tail.
            float remaining = (m_GCTailDeadlineMs - GetGame().GetTime()) / 1000.0;
            float envelope = Math.Clamp(remaining / 1.5,0,1);
            m_GCTailGain = envelope * envelope * (3 - 2 * envelope);
            // AbstractWave.GetVolume reads its absolute level, separately
            // from EffectSound's relative gain. Fade that native level from
            // its measured baseline; keep the approved sound-set gain intact.
            m_AlarmSound.RPGCSetNativeWaveVolume(m_GCFadeWaveStart * m_GCTailGain);
            if (!m_GCFadeCounted && m_GCTailGain < 0.9 && m_GCTailGain > 0.05)
            {
                m_GCFadeCounted = true; m_GCFades++;
            }
        }
    }

    override void UpdateWarningLight(bool active)
    {
        bool finishingSound = m_GCInsideClientUpdate && m_GCTailArmed && m_AlarmSound && !s_MissionEnding;
        finishingSound = finishingSound && GetGame().GetPlayer() != null && m_RPEmitter;
        finishingSound = finishingSound && !m_RPAlarm && m_RenderPhase <= 0.0005;
        if (finishingSound && (m_GCTailDeadlineMs == 0 || GetGame().GetTime() < m_GCTailDeadlineMs)) active = true;
        super.UpdateWarningLight(active);
    }

    override void StopAlarm()
    {
        // Only defer the ordinary fully-open client transition. Deletion,
        // disconnect, world teardown and playback failures stop immediately.
        if (m_GCInsideClientUpdate && !s_MissionEnding && m_GCTailArmed && m_AlarmSound && GetGame().GetPlayer() && m_RPEmitter && !m_RPAlarm && m_RenderPhase <= 0.0005)
        {
            if (m_GCTailDeadlineMs == 0)
            {
                m_GCTailStartMs = GetGame().GetTime();
                m_GCTailDeadlineMs = m_GCTailStartMs + 5000;
                m_GCFadeCounted = false; m_GCTailGain = 1;
                m_GCFadeWaveStart = m_AlarmSound.RPGCNativeWaveVolume();
                Print("[RailPilotGuided] ALARM_TAIL_START fully raised, light blinking, continuous original wave");
            }
            return;
        }
        m_GCTailArmed = false; m_GCTailDeadlineMs = 0;
        super.StopAlarm();
    }

    bool RPGCAlarmDecoded()
    {
        return m_AlarmSound && m_AlarmSound.GetSoundWaveLength() > 20 && m_AlarmSound.IsSoundPlaying();
    }
    bool RPGCAlarmExists() { return m_AlarmSound != null; }
    bool RPGCTailActive() { return m_GCTailDeadlineMs > 0; }
    int RPGCTailCompletions() { return m_GCTailCompletions; }
    float RPGCLastTailSeconds() { return m_GCLastTailSeconds; }
    int RPGCFades() { return m_GCFades; }
    float RPGCLastFadeWaveRatio() { return m_GCLastFadeWaveRatio; }
};

class RPGCCrossingLogic : RPCrossingLogic
{
    protected int m_PreWarningStartMs;
    protected int m_PreWarningDeadlineMs;
    protected int m_PreWarnings;
    protected int m_Lowerings;
    protected float m_MinPreWarning = 100;
    protected float m_MaxPreWarning;

    override void Update(float minimum, float maximum, float signedSpeed, float dt)
    {
        if (m_PreWarningDeadlineMs > 0)
        {
            if (GetGame().GetTime() < m_PreWarningDeadlineMs) return;
            float elapsed = (GetGame().GetTime() - m_PreWarningStartMs) / 1000.0;
            m_MinPreWarning = Math.Min(m_MinPreWarning,elapsed);
            m_MaxPreWarning = Math.Max(m_MaxPreWarning,elapsed);
            m_PreWarningDeadlineMs = 0; m_Lowerings++;
            Print("[RailPilotGuided] PREWARNING_COMPLETE seconds=" + elapsed.ToString());
        }
        bool wasOpen = m_State == 0;
        super.Update(minimum,maximum,signedSpeed,dt);
        if (wasOpen && m_State == 1)
        {
            // Alarm demand is already active; keep the arm target UP for5s.
            m_Phase = 0; m_PreWarnings++;
            m_PreWarningStartMs = GetGame().GetTime();
            m_PreWarningDeadlineMs = m_PreWarningStartMs + 5000;
            Print("[RailPilotGuided] PREWARNING_START sound and light active, arms fully raised");
        }
    }
    int PreWarnings() { return m_PreWarnings; }
    int Lowerings() { return m_Lowerings; }
    float MinPreWarning() { return m_MinPreWarning; }
    float MaxPreWarning() { return m_MaxPreWarning; }
};


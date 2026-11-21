modded class RP_LocomotiveBodyProbe
{
    protected float m_RPAudioSpeed;
    protected int m_RPHornSequence;
    protected ref RPAudioMotionGate m_RPStopGate = new RPAudioMotionGate;
    protected ref RPAudioSequenceGate m_RPSequenceGate = new RPAudioSequenceGate;
    protected ref RPAudioEnvelope m_RPEnvelope = new RPAudioEnvelope;
    protected EffectSound m_RPIdle;
    protected EffectSound m_RPSlow;
    protected EffectSound m_RPCruise;
    protected EffectSound m_RPHorn;
    protected bool m_RPAudioFailed;
    protected bool m_RPHornPending;
    protected float m_RPStartup;
    protected float m_RPDiagnosticClock;
    protected bool m_RPEngineDiagnosed;
    protected float m_RPHornDiagnosticClock;
    protected bool m_RPHornDiagnosed;

    void RP_LocomotiveBodyProbe()
    {
        RegisterNetSyncVariableFloat("m_RPAudioSpeed", 0, 1, 3);
        RegisterNetSyncVariableInt("m_RPHornSequence", 0, 999999);
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (!GetGame().IsServer() && m_RPSequenceGate.Accept(m_RPHornSequence))
            m_RPHornPending = true;
    }

    static void SampleAllRPAudio(float dt)
    {
        for (int i = 0; i < s_MotionBodies.Count(); i++)
        {
            RP_LocomotiveBodyProbe body = s_MotionBodies.Get(i);
            if (body)
                body.UpdateRPAudio(dt);
        }
    }

    protected void UpdateRPAudio(float dt)
    {
        if (dt <= 0)
            return;
        if (GetGame().IsServer())
        {
            float speed;
            if (dBodyIsSet(this))
                speed = GetVelocity(this).Length();
            float normalized = Math.Clamp(speed / 0.5, 0, 1);
            bool changed = Math.AbsFloat(normalized - m_RPAudioSpeed) >= 0.01;
            bool stopped = m_RPStopGate.Update(speed, dt);
            if (changed || stopped)
            {
                m_RPAudioSpeed = normalized;
                if (stopped)
                {
                    m_RPHornSequence = (m_RPHornSequence + 1) % 1000000;
                    Print("[RailPilotAudio] STOP_EVENT sequence=" + m_RPHornSequence.ToString());
                }
                SetSynchDirty();
            }
            return;
        }
        if (m_RPAudioFailed)
            return;
        // Original presentation sampling ran first, creating the local visual.
        Object parent = m_ClientVisual;
        if (!parent)
            parent = this;
        if (!m_RPIdle)
        {
            m_RPIdle = CreateRPLoop("RP_PreviewIdle_SoundSet", parent);
            m_RPSlow = CreateRPLoop("RP_PreviewSlow_SoundSet", parent);
            m_RPCruise = CreateRPLoop("RP_PreviewCruise_SoundSet", parent);
            if (!m_RPIdle || !m_RPSlow || !m_RPCruise)
            {
                StopRPAudio();
                m_RPAudioFailed = true;
                Print("[RailPilotAudio] ERROR: engine sound could not be started.");
                return;
            }
            Print("[RailPilotAudio] Client engine playback requested; awaiting wave validation.");
        }
        m_RPEnvelope.Update(m_RPAudioSpeed, dt);
        m_RPStartup = Math.Min(1, m_RPStartup + dt * 2);
        float gain = 0.65 * m_RPStartup;
        vector soundPosition = parent.GetPosition();
        ApplyRPVolume(m_RPIdle, gain * m_RPEnvelope.Idle(), soundPosition);
        ApplyRPVolume(m_RPSlow, gain * m_RPEnvelope.Slow(), soundPosition);
        ApplyRPVolume(m_RPCruise, gain * m_RPEnvelope.Cruise(), soundPosition);
        m_RPDiagnosticClock += dt;
        if (!m_RPEngineDiagnosed && m_RPDiagnosticClock >= 2)
        {
            m_RPEngineDiagnosed = true;
            PrintRPWaveDiagnostic("idle", m_RPIdle, soundPosition);
            PrintRPWaveDiagnostic("slow", m_RPSlow, soundPosition);
            PrintRPWaveDiagnostic("cruise", m_RPCruise, soundPosition);
        }
        if (m_RPHornPending)
        {
            m_RPHornPending = false;
            if (m_RPHorn)
                SEffectManager.DestroyEffect(m_RPHorn);
            m_RPHorn = SEffectManager.CreateSound("RP_PreviewHorn_SoundSet", parent.GetPosition(), 0, 0, false);
            m_RPHorn.SetSoundAutodestroy(true);
            m_RPHornDiagnosticClock = 0;
            m_RPHornDiagnosed = false;
            if (m_RPHorn.SoundPlay())
            {
                m_RPHorn.SetCurrentPosition(soundPosition);
                Print("[RailPilotAudio] Client greeting requested, sequence=" + m_RPHornSequence.ToString());
            }
            else
                Print("[RailPilotAudio] ERROR: greeting could not be started.");
        }
        if (m_RPHorn)
        {
            m_RPHorn.SetCurrentPosition(soundPosition);
            m_RPHornDiagnosticClock += dt;
            if (!m_RPHornDiagnosed && m_RPHornDiagnosticClock >= 0.3)
            {
                m_RPHornDiagnosed = true;
                PrintRPWaveDiagnostic("horn", m_RPHorn, soundPosition);
            }
        }
    }

    protected EffectSound CreateRPLoop(string name, Object parent)
    {
        EffectSound sound = SEffectManager.CreateSound(name, parent.GetPosition(), 0, 0, true);
        sound.SetSoundVolume(0);
        if (!sound.SoundPlay())
        {
            SEffectManager.DestroyEffect(sound);
            return null;
        }
        return sound;
    }

    protected void ApplyRPVolume(EffectSound sound, float volume, vector position)
    {
        // Update both values: the native start/fade callback uses MaxVolume.
        // Explicit world positions avoid relying on a client-local item's
        // native attachment transform after its physics body was destroyed.
        sound.SetSoundMaxVolume(volume);
        sound.SetSoundVolume(volume);
        sound.SetCurrentPosition(position);
    }

    protected void PrintRPWaveDiagnostic(string role, EffectSound sound, vector expected)
    {
        if (!sound)
            return;
        float length = sound.GetSoundWaveLength();
        vector actual = sound.GetCurrentPosition();
        float positionError = vector.Distance(actual, expected);
        float listenerDistance = -1;
        Man listener = GetGame().GetPlayer();
        if (listener)
            listenerDistance = vector.Distance(actual, listener.GetPosition());
        string message = "[RailPilotAudio] WAVE role=" + role + " length=" + length.ToString();
        message += " playing=" + sound.IsSoundPlaying().ToString();
        message += " volume=" + sound.GetSoundVolume().ToString();
        message += " sourceError=" + positionError.ToString();
        message += " listenerDistance=" + listenerDistance.ToString();
        Print(message);
        if (length <= 0 || positionError > 0.25)
            Print("[RailPilotAudio] ERROR: wave header or source position invalid for " + role);
    }

    protected void StopRPAudio()
    {
        if (m_RPIdle) SEffectManager.DestroyEffect(m_RPIdle);
        if (m_RPSlow) SEffectManager.DestroyEffect(m_RPSlow);
        if (m_RPCruise) SEffectManager.DestroyEffect(m_RPCruise);
        if (m_RPHorn) SEffectManager.DestroyEffect(m_RPHorn);
        m_RPIdle = null;
        m_RPSlow = null;
        m_RPCruise = null;
        m_RPHorn = null;
    }

    override void EEDelete(EntityAI parent)
    {
        if (GetGame() && !GetGame().IsServer())
            StopRPAudio();
        super.EEDelete(parent);
    }
};

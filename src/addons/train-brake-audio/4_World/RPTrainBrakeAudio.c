class RPBrakeWave : RPConvoySound
{
    void FitToRemaining(float seconds)
    {
        if (!m_SoundWaveObject || !m_SoundWaveObject.IsHeaderLoaded()) return;
        float position = Math.Clamp(m_SoundWaveObject.GetCurrPosition(), 0, 1);
        float unplayed = m_SoundWaveObject.GetLength() * (1 - position);
        if (unplayed > 0) m_SoundWaveObject.SetFrequency(Math.Clamp(unplayed / Math.Max(seconds, 0.02), 0.125, 6));
    }
};

modded class RPConvoyEffects
{
    protected ref RPBrakeWave m_TrainBrakeWave;
    protected bool m_TrainBrakeStarted;

    bool TrainBrakeWavePlaying() { return m_TrainBrakeWave != null; }

    void UpdateTrainBrakeAudio(Object loco, float remaining, float duration)
    {
        if (remaining <= 0)
        {
            ReleaseTrainBrakeAudio();
            return;
        }
        if (!m_TrainBrakeStarted)
        {
            m_TrainBrakeStarted = true;
            string soundSet = "RP_TrainBrake_SoundSet";
            if (duration < 5.5) soundSet = "RP_TrainBrakeShort_SoundSet";
            else if (duration < 11.5) soundSet = "RP_TrainBrakeMedium_SoundSet";
            m_TrainBrakeWave = new RPBrakeWave;
            m_TrainBrakeWave.SetSoundSet(soundSet);
            m_TrainBrakeWave.SetPosition(loco.GetPosition());
            m_TrainBrakeWave.SetSoundLoop(false);
            m_TrainBrakeWave.SetSoundAutodestroy(false);
            m_TrainBrakeWave.SetSoundMaxVolume(0.65);
            m_TrainBrakeWave.SetSoundVolume(0.65);
            SEffectManager.EffectRegister(m_TrainBrakeWave);
            if (!m_TrainBrakeWave.SoundPlay())
            {
                SEffectManager.DestroyEffect(m_TrainBrakeWave);
                m_TrainBrakeWave = null;
                Print("[RailPilotTrainBrake] Could not play the braking sound");
            }
        }
        if (m_TrainBrakeWave)
        {
            m_TrainBrakeWave.SetCurrentPosition(loco.GetPosition());
            m_TrainBrakeWave.FitToRemaining(remaining);
        }
    }

    protected void ReleaseTrainBrakeAudio()
    {
        m_TrainBrakeStarted = false;
        if (!m_TrainBrakeWave) return;
        m_TrainBrakeWave.StopImmediately();
        SEffectManager.DestroyEffect(m_TrainBrakeWave);
        m_TrainBrakeWave = null;
    }

    override void Release()
    {
        ReleaseTrainBrakeAudio();
        super.Release();
    }
};

modded class RP_ConvoyUnit
{
    protected int m_TrainBrakeMode;
    protected float m_TrainBrakeRemaining;
    protected float m_TrainBrakeDuration;
    protected float m_TrainBrakeStopClock;
    protected float m_TrainBrakeClockRate = 1;

    void RP_ConvoyUnit()
    {
        RegisterNetSyncVariableInt("m_TrainBrakeMode", 0, 2);
        RegisterNetSyncVariableFloat("m_TrainBrakeRemaining", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_TrainBrakeDuration", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_TrainBrakeStopClock", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_TrainBrakeClockRate", 0, 1, 3);
    }

    bool TrainBrakeAudible() { return m_ServiceEnabled && m_Slot == m_LeadingSlot && m_TrainBrakeMode > 0; }
    bool TrainBrakeBeforeStop(float clock) { return m_TrainBrakeStopClock > clock; }

    protected void PublishBrakeWindow(RPConvoyRoute route, float clock)
    {
        int mode;
        float remaining;
        float duration;
        if (m_Slot == m_LeadingSlot && m_ServiceSpeed > 0)
        {
            remaining = RPObstacleBrakeRemaining(duration);
            if (remaining > 0) mode = 2;
            else
            {
                remaining = route.ScheduledBrakeRemaining(clock, duration);
                if (remaining > 0) mode = 1;
            }
        }
        if (mode > 0)
        {
            float railDistance;
            float routeSpeed;
            int direction;
            route.State(clock, railDistance, routeSpeed, direction);
            if (routeSpeed > 0) m_TrainBrakeClockRate = Math.Clamp(m_ServiceSpeed / routeSpeed, 0, 1);
        }
        m_TrainBrakeRemaining = remaining;
        if (duration > 0) m_TrainBrakeDuration = duration;
        if (mode != m_TrainBrakeMode)
        {
            m_TrainBrakeStopClock = 0;
            if (mode == 0 && m_Slot == m_LeadingSlot && m_ServiceSpeed <= 0) m_TrainBrakeStopClock = clock;
            m_TrainBrakeMode = mode;
            SetSynchDirty();
        }
    }

    override void Present(float clock, float dt)
    {
        super.Present(clock, dt);
        RPConvoyRoute route = Route();
        if (!route) return;
        if (GetGame().IsServer()) PublishBrakeWindow(route, clock);
        if (GetGame().IsDedicatedServer() || !m_Effects || dt <= 0) return;

        float remaining;
        float duration = m_TrainBrakeDuration;
        if (m_ServiceEnabled && m_Slot == m_LeadingSlot && m_Slot == ActiveSlot())
        {
            if (m_TrainBrakeMode == 1) remaining = route.ScheduledBrakeRemaining(clock, duration);
            else if (m_TrainBrakeMode == 2)
            {
                // The rendered train trails the most recent server pose.
                float visualLag = Math.Max(0, m_RouteClock - clock) / Math.Max(0.05, m_TrainBrakeClockRate);
                remaining = m_TrainBrakeRemaining + Math.Min(visualLag, 0.6);
            }
            else if (m_Effects.TrainBrakeWavePlaying() && TrainBrakeBeforeStop(clock))
            {
                remaining = Math.Min((m_TrainBrakeStopClock - clock) / Math.Max(0.05, m_TrainBrakeClockRate), 0.6);
            }
        }
        m_Effects.UpdateTrainBrakeAudio(this, remaining, duration);
    }
};

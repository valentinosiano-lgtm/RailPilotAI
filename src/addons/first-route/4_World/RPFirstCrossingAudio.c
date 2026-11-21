// Audio-only extension: the approved gate, collider and lamp code stays loaded.
modded class Land_rp_crossing_barrier
{
    protected bool m_FirstSoundMuted;
    protected bool m_FirstSoundWake;
    protected bool m_FirstAudioScope;
    protected bool m_FirstLampEmitter;
    protected bool m_FirstLampAlarm;

    void Land_rp_crossing_barrier()
    {
        RegisterNetSyncVariableBool("m_FirstSoundMuted");
        RegisterNetSyncVariableBool("m_FirstSoundWake");
    }
    void ServerAudioState(bool muted, bool wake)
    {
        if (!GetGame().IsServer()) return;
        if (muted == m_FirstSoundMuted && wake == m_FirstSoundWake) return;
        m_FirstSoundMuted = muted; m_FirstSoundWake = wake; SetSynchDirty();
    }
    bool FirstSoundMuted() { return m_FirstSoundMuted; }
    bool FirstSoundWake() { return m_FirstSoundWake; }
    bool FirstLampCommand() { return m_RPEmitter && (m_RPAlarm || m_RPPhase > 0.0005); }
    bool FirstLampActive()
    {
        return m_FirstLampEmitter && (m_FirstLampAlarm || m_RenderPhase > 0.0005) && GetGame().GetPlayer() != null;
    }
    override void ClientUpdate(float dt)
    {
        if ((!m_FirstSoundMuted && !m_FirstSoundWake) || GetGame().IsDedicatedServer() || s_MissionEnding || !m_ClientInitialized)
        {
            super.ClientUpdate(dt); return;
        }
        // The legacy renderer couples sound and light to m_RPEmitter. Scope
        // its audio input locally while the lamp override keeps the original
        // inputs. No animation, sync field or collision target is changed.
        m_FirstLampEmitter = m_RPEmitter; m_FirstLampAlarm = m_RPAlarm;
        m_FirstAudioScope = true;
        m_RPEmitter = !m_FirstSoundMuted;
        if (m_FirstSoundWake) m_RPAlarm = true;
        super.ClientUpdate(dt);
        m_RPEmitter = m_FirstLampEmitter; m_RPAlarm = m_FirstLampAlarm;
        m_FirstAudioScope = false;
    }
    override void UpdateWarningLight(bool active)
    {
        if (m_FirstAudioScope) active = FirstLampActive();
        super.UpdateWarningLight(active);
    }
    override void StopWarningLight()
    {
        // StopAlarm also calls this: keep an existing lamp alive when only
        // the sound is muted. Normal disconnect/delete cleanup remains intact.
        if (m_FirstAudioScope && FirstLampActive()) return;
        super.StopWarningLight();
    }
};

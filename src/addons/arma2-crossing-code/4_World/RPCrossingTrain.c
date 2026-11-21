class RPCrossingStationAudioGate : RPAudioMotionGate
{
    protected bool m_StationDwell;
    protected bool m_PendingStop;

    void SetStationDwell(bool value) { m_StationDwell = value; }

    override bool Update(float speed, float dt)
    {
        if (speed > 0.12) m_PendingStop = false;
        if (super.Update(speed,dt)) m_PendingStop = true;
        if (!m_StationDwell || !m_PendingStop) return false;
        m_PendingStop = false;
        return true;
    }
};

modded class RP_LocomotiveBodyProbe
{
    protected ref RPCrossingStationAudioGate m_CrossingStationGate;

    void RPSetCrossingStationDwell(bool value)
    {
        if (!m_CrossingStationGate)
        {
            m_CrossingStationGate = new RPCrossingStationAudioGate;
            m_RPStopGate = m_CrossingStationGate;
        }
        m_CrossingStationGate.SetStationDwell(value);
    }

    void RPSoundCrossingHorn()
    {
        if (!GetGame().IsServer()) return;
        // Share the authoritative sequence with station greetings. Separate
        // counters would make the existing RPC deduplication drop real horns.
        m_RPHornSequence = (m_RPHornSequence + 1) % 1000000;
        RPApproachHorn.Emit(GetPosition(),m_RPHornSequence);
        SetSynchDirty();
        Print("[RailPilotCrossing] HORN sequence=" + m_RPHornSequence.ToString());
    }
};

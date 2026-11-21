modded class RPConvoyClock
{
    protected int m_BypassEpoch;

    void RPReceiveBypass(int epoch, float clock, int lap = 0)
    {
        if (lap < m_ClockLap || epoch <= m_BypassEpoch) return;
        m_BypassEpoch = epoch;
        Push(clock, lap);
        // A discontinuity is an authoritative pose, never an interpolation
        // through the obstructed stretch. All six units sample this same clock.
        m_Presentation = Math.Max(m_Presentation, m_Latest);
        m_LastSample = GetGame().GetTime() / 1000.0;
        m_Started = true;
    }
};

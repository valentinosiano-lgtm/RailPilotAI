modded class RPConvoyClock
{
    protected float m_ObstacleClockRate = 1;
    protected float m_ObstaclePlaybackRate = 1;

    void RPApplyClockRate(float clock, float rate, int lap = 0)
    {
        if (lap >= m_ClockLap && clock >= m_Latest) m_ObstacleClockRate = rate;
    }

    override float Sample(float now)
    {
        float dt = Math.Clamp(now - m_LastSample, 0, 0.1);
        m_LastSample = now;
        if (m_Latest <= 0) return 0;

        // The shared client pose may approach the last authoritative sample.
        // Extrapolating past that sample could cross a newly applied stop.
        float target = Math.Min(m_Latest, m_Latest + (now - m_ReceivedAt - 0.25) * m_ObstacleClockRate);
        if (!m_Started)
        {
            if (target <= 0) return 0;
            m_Presentation = target;
            m_Started = true;
        }
        float rate = Math.Clamp(m_ObstacleClockRate + (target - m_Presentation) * 4, 0, 1.25);
        m_ObstaclePlaybackRate += (rate - m_ObstaclePlaybackRate) * dt / (0.10 + dt);
        float next = m_Presentation + dt * m_ObstaclePlaybackRate;
        if (m_ObstacleClockRate == 0 && m_Latest - next < 0.001) next = m_Latest;
        m_Presentation = Math.Min(next, m_Latest);
        return m_Presentation;
    }
};

// Approved Rider phase-lock, shared by ALL six native floors. No packet jitter
// applied independently to individual wagons; no hard resets while moving.
class RPConvoyClock
{
    protected float m_Latest;
    protected float m_ReceivedAt;
    protected float m_LastSample;
    protected float m_Presentation;
    protected float m_FilteredError;
    protected bool m_Started;
    protected int m_ClockLap;

    void Push(float clock, int lap = 0)
    {
        if (lap < m_ClockLap) return;
        if (lap > m_ClockLap)
        {
            // A lap starts in the same parking pose at Komarovo. Rebase the
            // single shared client clock and reject packets from the old lap.
            m_ClockLap = lap;
            m_Latest = clock;
            m_Presentation = Math.Max(0, clock - 0.25);
            m_FilteredError = 0;
            m_ReceivedAt = GetGame().GetTime() / 1000.0;
            m_LastSample = m_ReceivedAt;
            return;
        }
        if (clock <= m_Latest) return;
        m_Latest = clock;
        m_ReceivedAt = GetGame().GetTime() / 1000.0;
    }

    float Sample(float now)
    {
        float dt = Math.Clamp(now - m_LastSample, 0, 0.1); m_LastSample = now;
        if (m_Latest <= 0) return 0;
        float target = m_Latest + now - m_ReceivedAt - 0.25;
        if (!m_Started)
        {
            if (target <= 0) return 0;
            m_Started = true; m_Presentation = target;
        }
        float error = target - m_Presentation;
        m_FilteredError += (error - m_FilteredError) * dt / (0.75 + dt);
        float rate = 1 + Math.Clamp(m_FilteredError * 0.25, -0.02, 0.02);
        float next = Math.Min(m_Presentation + dt * rate, m_Latest + 0.1);
        m_Presentation = Math.Max(m_Presentation, next);
        return m_Presentation;
    }
};

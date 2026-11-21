modded class RP_ConvoyUnit
{
    protected float m_RPObstacleBrakeDuration;
    protected float m_RPObstacleBrakeRemaining;

    void RPSetObstacleBrakeWindow(float duration, float remaining)
    {
        m_RPObstacleBrakeDuration = duration;
        m_RPObstacleBrakeRemaining = remaining;
    }

    float RPObstacleBrakeRemaining(out float duration)
    {
        duration = m_RPObstacleBrakeDuration;
        return m_RPObstacleBrakeRemaining;
    }
};

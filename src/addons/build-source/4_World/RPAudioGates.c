class RPAudioMotionGate
{
    protected float m_Moving;
    protected float m_Stopped;
    protected bool m_Armed;

    bool Update(float speed, float dt)
    {
        if (!RPLimits.InRange(speed, 0, 10) || !RPLimits.InRange(dt, 0.000001, 0.5))
            return false;
        if (speed > 0.12)
        {
            m_Moving += dt;
            m_Stopped = 0;
            if (m_Moving >= 0.25)
                m_Armed = true;
        }
        else if (speed < 0.03 && m_Armed)
        {
            m_Stopped += dt;
            if (m_Stopped >= 0.35)
            {
                m_Armed = false;
                m_Moving = 0;
                m_Stopped = 0;
                return true;
            }
        }
        else
        {
            m_Stopped = 0;
        }
        return false;
    }
};

class RPAudioSequenceGate
{
    protected bool m_HasBaseline;
    protected int m_Last;

    bool Accept(int sequence)
    {
        if (sequence < 0 || sequence >= 1000000)
            return false;
        if (!m_HasBaseline)
        {
            m_Last = sequence;
            m_HasBaseline = true;
            return false;
        }
        bool wrapped = m_Last > 999000 && sequence < 1000;
        if (sequence <= m_Last && !wrapped)
            return false;
        m_Last = sequence;
        return true;
    }
};

class RPAudioEnvelope
{
    protected float m_Speed;
    void Update(float target, float dt)
    {
        if (!RPLimits.InRange(dt, 0.000001, 1))
            return;
        target = Math.Clamp(target, 0, 1);
        m_Speed += (target - m_Speed) * dt / (0.25 + dt);
    }
    float Idle() { return Math.Max(0, 1 - 2 * m_Speed); }
    float Slow() { return 1 - Math.AbsFloat(2 * m_Speed - 1); }
    float Cruise() { return Math.Max(0, 2 * m_Speed - 1); }
};

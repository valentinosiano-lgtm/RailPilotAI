// Original, presentation-only interpolation. Never advances server physics.
class RPVisualMotionSample
{
    float Time;
    vector Position;
};

class RPVisualMotionBuffer
{
    protected ref array<ref RPVisualMotionSample> m_Samples = new array<ref RPVisualMotionSample>;
    protected float m_DelaySeconds = 0.12;

    bool Push(float receivedAt, vector position)
    {
        if (!(receivedAt >= 0 && receivedAt < 1000000))
            return false;
        int count = m_Samples.Count();
        if (count > 0)
        {
            RPVisualMotionSample last = m_Samples.Get(count - 1);
            if (receivedAt <= last.Time)
                return false;
            // Teleports/very stale streams must not produce a long sweep.
            if (vector.Distance(position, last.Position) > 5 || receivedAt - last.Time > 1)
                m_Samples.Clear();
        }
        RPVisualMotionSample sample = new RPVisualMotionSample;
        sample.Time = receivedAt;
        sample.Position = position;
        m_Samples.Insert(sample);
        if (m_Samples.Count() > 64)
            m_Samples.RemoveOrdered(0);
        return true;
    }

    vector Sample(float now)
    {
        int count = m_Samples.Count();
        if (count == 0)
            return vector.Zero;
        float renderTime = now - m_DelaySeconds;
        while (m_Samples.Count() > 2 && m_Samples.Get(1).Time <= renderTime)
            m_Samples.RemoveOrdered(0);
        RPVisualMotionSample first = m_Samples.Get(0);
        RPVisualMotionSample last = m_Samples.Get(m_Samples.Count() - 1);
        if (renderTime <= first.Time)
            return first.Position;
        if (renderTime >= last.Time)
            return last.Position; // Hold on packet loss; never extrapolate.
        RPVisualMotionSample next = m_Samples.Get(1);
        float alpha = (renderTime - first.Time) / (next.Time - first.Time);
        return first.Position + (next.Position - first.Position) * alpha;
    }
};

// Client-local visual only: no physics, inventory transfer or passengers.
class RP_LocomotiveVisualProbe : ItemBase
{
    override bool CanPutInCargo(EntityAI parent)
    {
        return false;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return false;
    }
};

modded class RPRunOverController
{
    protected int m_BypassSweepEpoch;

    override bool Bind(Object body, int direction)
    {
        if (!super.Bind(body, direction)) return false;
        m_BypassSweepEpoch = 0;
        RP_ConvoyUnit unit = RP_ConvoyUnit.Cast(body);
        if (unit) m_BypassSweepEpoch = unit.RPBypassEpoch();
        return true;
    }

    override void Tick(float timeslice, int direction, float serviceSpeed)
    {
        RP_ConvoyUnit unit = RP_ConvoyUnit.Cast(m_Body);
        if (unit && m_BypassSweepEpoch != unit.RPBypassEpoch())
        {
            // Never apply a movement sweep across a deliberately skipped
            // stretch of railway, even if a curve makes the chord short.
            m_BypassSweepEpoch = unit.RPBypassEpoch();
            vector pose[4];
            unit.GetTransform(pose);
            Reset(pose, direction);
            ResetPlayerSweep(pose, direction);
            ResetAnimalSweep(pose, direction);
            SkippedDiscontinuities++;
            PlayerSkippedDiscontinuities++;
            AnimalSkippedDiscontinuities++;
            return;
        }
        super.Tick(timeslice, direction, serviceSpeed);
    }
};

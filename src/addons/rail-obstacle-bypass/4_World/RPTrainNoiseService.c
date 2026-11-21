modded class RPConvoyService
{
    protected ref RPTrainAINoise m_TrainAINoise;
    protected int m_NoiseLeadingSlot = -1;

    override void Tick(float dt)
    {
        super.Tick(dt);
        if (!GetGame().IsServer() || dt <= 0) return;
        RP_ConvoyUnit loco = Unit(LeadingSlot());
        if (!loco || !m_Route) return;
        if (!m_TrainAINoise || m_NoiseLeadingSlot != LeadingSlot())
        {
            m_TrainAINoise = new RPTrainAINoise;
            m_NoiseLeadingSlot = LeadingSlot();
        }
        // The service clock can stop advancing while the locomotive keeps running.
        m_TrainAINoise.Update(RPTrainAINoise.SourcePosition(loco), loco.Speed(), m_Route.Speed,
            loco.TrainBrakeAudible(), loco.BrakeAirRequested(Clock()), dt);
    }

    override void Stop()
    {
        super.Stop();
        m_TrainAINoise = null;
    }
};

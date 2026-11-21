modded class RPFirstRun
{
    protected ref RPInfectedTrainBoundary m_InfectedBoundary = new RPInfectedTrainBoundary;

    override void Update(float dt)
    {
        super.Update(dt);
        if (!Done && Service && GetGame().IsServer()) m_InfectedBoundary.Tick(Service, dt);
    }
};

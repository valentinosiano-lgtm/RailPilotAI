// Sample once per mission frame on both server and client. Inventory objects
// do not reliably receive EOnFrame; no physics/event-mask changes are needed.
modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        RP_LocomotiveBodyProbe.SampleAllMotion(timeslice);
    }
};

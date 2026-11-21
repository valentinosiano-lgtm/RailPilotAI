modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        RPArma2Couplers.UpdateAll();
    }
};
modded class MissionGameplay
{
    override void OnMissionFinish()
    {
        RPArma2Couplers.Clear();
        super.OnMissionFinish();
    }
};

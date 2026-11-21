modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (!GetGame().IsDedicatedServer()) Land_rp_crossing_barrier.UpdateAll(timeslice);
    }
};

modded class MissionGameplay
{
    override void OnMissionStart()
    {
        Land_rp_crossing_barrier.BeginClientMission();
        super.OnMissionStart();
    }

    override void OnMissionFinish()
    {
        Land_rp_crossing_barrier.EndClientMission();
        super.OnMissionFinish();
    }
};

modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        RP_WagonBodyProbe.SampleAllWagons(timeslice);
        if (!GetGame().IsServer())
        {
            RPApproachAudioBus.Sweep();
            RPApproachHorn.Update(timeslice);
            RPTrainHeadlightRig.Sweep();
        }
    }

    override void OnMissionFinish()
    {
        if (!GetGame().IsServer()) RPApproachAudioBus.Clear();
        super.OnMissionFinish();
    }
};

// Vanilla MissionGameplay.OnMissionFinish does not call the base method.
// Handle the actual client mission so disconnecting to the menu releases audio.
modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();
        RPApproachHorn.StartReceiving();
    }

    override void OnMissionFinish()
    {
        RPTrainHeadlightRig.ClearAll();
        RPApproachHorn.StopReceiving();
        RPApproachAudioBus.Clear();
        super.OnMissionFinish();
    }
};

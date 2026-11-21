class RPRingSettings
{
    int Schema = 1;
    bool Enabled = true;
    string StartStation = "Komarovo";

    static RPRingSettings Load()
    {
        string world;
        GetGame().GetWorldName(world);
        world.ToLower();
        if (world != "chernarusplus") return null;
        string path = "$profile:RailPilot/Service.json";
        string error;
        RPRingSettings settings;
        if (FileExist(path))
        {
            if (!JsonFileLoader<RPRingSettings>.LoadFile(path, settings, error) || !settings || settings.Schema != 1)
            {
                Print("[RailPilotRing] ERROR invalid Service.json " + error);
                return null;
            }
            return settings;
        }
        MakeDirectory("$profile:RailPilot");
        settings = new RPRingSettings;
        if (!JsonFileLoader<RPRingSettings>.SaveFile(path, settings, error))
        {
            Print("[RailPilotRing] ERROR cannot create Service.json " + error);
            return null;
        }
        return settings;
    }
};

modded class MissionServer
{
    protected ref RPFirstRun m_RPRing;
    RPFirstRun RPRingRun() { return m_RPRing; }

    override void OnMissionStart()
    {
        super.OnMissionStart();
        if (!GetGame().IsDedicatedServer() || m_RPRing) return;
        RPRingSettings settings = RPRingSettings.Load();
        if (!settings || !settings.Enabled) return;
        m_RPRing = new RPFirstRun;
        if (!m_RPRing.Start(false, true, "$profile:RailPilot/Ring", settings.StartStation, true))
        {
            Print("[RailPilotRing] ERROR service could not start; check the station and saved state");
            m_RPRing.Release();
            m_RPRing = null;
            return;
        }
        DayZGame.Cast(GetGame()).Event_OnRPC.Insert(RPRingPortRequest);
        PrintFormat("[RailPilotRing] READY lap=%1 time=%2 recovered=%3", m_RPRing.Service.Lap(),
            m_RPRing.Service.Clock(), m_RPRing.Recovered);
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (!m_RPRing || timeslice <= 0) return;
        m_RPRing.Update(timeslice);
        if (m_RPRing.Done) RPRingRelease();
    }

    protected void RPRingPortRequest(PlayerIdentity sender, Object target, int id, ParamsReadContext ctx)
    {
        if (id != RPSvetloyarskPortClearance.REQUEST_RPC || target || !sender || sender.GetId() == "") return;
        if (!m_RPRing || !m_RPRing.BerezinoPortActive() || !RPSvetloyarskPortClearance.ReadRequest(ctx)) return;
        ScriptRPC rpc = new ScriptRPC;
        rpc.Write(RPSvetloyarskPortClearance.RPC_VERSION);
        rpc.Send(null, RPSvetloyarskPortClearance.ACTIVATION_RPC, true, sender);
    }

    override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
    {
        super.OnClientReadyEvent(identity, player);
        if (!m_RPRing || !identity || !player) return;
        RPFirstPassengers passengers = m_RPRing.Passengers();
        if (passengers.Restore(identity.GetId(), player, true)) m_RPRing.SaveNow();
    }

    override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
    {
        if (m_RPRing && identity && player && !authFailed)
        {
            m_RPRing.Passengers().Observe(identity.GetId(), player);
            m_RPRing.SaveNow();
        }
        super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
    }

    override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
    {
        if (m_RPRing && identity && player)
        {
            m_RPRing.Passengers().Forget(identity.GetId());
            m_RPRing.SaveNow();
        }
        super.OnClientRespawnEvent(identity, player);
    }

    protected void RPRingRelease()
    {
        if (!m_RPRing) return;
        DayZGame.Cast(GetGame()).Event_OnRPC.Remove(RPRingPortRequest);
        m_RPRing.Release();
        m_RPRing = null;
    }

    override void OnMissionFinish()
    {
        RPRingRelease();
        super.OnMissionFinish();
    }
};


modded class MissionBase
{
    override void OnUpdate(float timeslice)
    {
        RP_ConvoyUnit.UpdateAll(timeslice);
        if (!GetGame().IsDedicatedServer()) RPConvoyHorn.Update(timeslice);
        super.OnUpdate(timeslice);
    }
};

modded class MissionGameplay
{
    override void OnMissionStart()
    {
        super.OnMissionStart(); RPConvoyHorn.StartReceiving();
    }
    override void OnMissionFinish()
    {
        RPConvoyHorn.StopReceiving(); RP_ConvoyUnit.ResetPresentation();
        RPConvoyHeadlightRig.ClearAll();
        super.OnMissionFinish();
    }
};

modded class RPFirstRun
{
    protected ref RPSvetloyarskPortClearance m_BerezinoPort;

    bool BerezinoPortActive() { return m_BerezinoPort && m_BerezinoPort.Active(); }

    override protected bool PrepareRouteWorld()
    {
        if (!super.PrepareRouteWorld()) return false;
        if (!Route.HasBerezino()) return true;
        m_BerezinoPort = new RPSvetloyarskPortClearance;
        return m_BerezinoPort.Clear();
    }

    override protected void ReleaseRouteWorld()
    {
        if (m_BerezinoPort) m_BerezinoPort.Restore();
        m_BerezinoPort = null;
        super.ReleaseRouteWorld();
    }
};

modded class RPFirstServerMission
{
    override void OnMissionStart()
    {
        super.OnMissionStart();
        DayZGame.Cast(GetGame()).Event_OnRPC.Insert(OnBerezinoPortRequest);
    }

    void OnBerezinoPortRequest(PlayerIdentity sender, Object target, int id, ParamsReadContext context)
    {
        if (id != RPSvetloyarskPortClearance.REQUEST_RPC || target || !sender || sender.GetId() == "") return;
        if (!m_Run || !m_Run.BerezinoPortActive() || !RPSvetloyarskPortClearance.ReadRequest(context)) return;
        ScriptRPC rpc = new ScriptRPC;
        rpc.Write(RPSvetloyarskPortClearance.RPC_VERSION);
        rpc.Send(null, RPSvetloyarskPortClearance.ACTIVATION_RPC, true, sender);
        Print("[RailPilotPort] SERVER clearance confirmed to connected client");
    }

    override void OnMissionFinish()
    {
        DayZGame.Cast(GetGame()).Event_OnRPC.Remove(OnBerezinoPortRequest);
        super.OnMissionFinish();
    }
};

modded class MissionGameplay
{
    protected ref RPSvetloyarskPortClearance m_ClientBerezinoPort;
    protected float m_ClientPortRetry;

    override void OnMissionStart()
    {
        super.OnMissionStart();
        DayZGame.Cast(GetGame()).Event_OnRPC.Insert(ReceiveBerezinoPort);
    }

    void ReceiveBerezinoPort(PlayerIdentity sender, Object target, int id, ParamsReadContext ctx)
    {
        if (GetGame().IsServer() || target || id != RPSvetloyarskPortClearance.ACTIVATION_RPC) return;
        if (m_ClientBerezinoPort || !RPSvetloyarskPortClearance.ReadRequest(ctx)) return;
        m_ClientBerezinoPort = new RPSvetloyarskPortClearance;
        m_ClientPortRetry = 0;
        Print("[RailPilotPort] CLIENT received clearance confirmation");
    }

    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
        if (!player || GetGame().IsServer() || !GetGame().IsMultiplayer()) return;
        if (!RPSvetloyarskPortClearance.NearPort(player.PhysicsGetPositionWS())) return;
        m_ClientPortRetry -= timeslice;
        if (m_ClientPortRetry > 0) return;
        if (!m_ClientBerezinoPort)
        {
            m_ClientPortRetry = 2;
            ScriptRPC rpc = new ScriptRPC;
            rpc.Write(RPSvetloyarskPortClearance.RPC_VERSION);
            rpc.Send(null, RPSvetloyarskPortClearance.REQUEST_RPC, true, null);
            return;
        }
        m_ClientPortRetry = 0.5;
        if (m_ClientBerezinoPort.Active()) m_ClientBerezinoPort.Refresh();
        else if (m_ClientBerezinoPort.Clear())
            Print("[RailPilotPort] CLIENT removed the five exact stock models and their collision");
    }

    override void OnMissionFinish()
    {
        DayZGame.Cast(GetGame()).Event_OnRPC.Remove(ReceiveBerezinoPort);
        if (m_ClientBerezinoPort) m_ClientBerezinoPort.Restore();
        m_ClientBerezinoPort = null;
        super.OnMissionFinish();
    }
};

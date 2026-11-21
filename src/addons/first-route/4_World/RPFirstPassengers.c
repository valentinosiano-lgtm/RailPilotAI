// Passenger identities and local poses belong to the same verified generation
// as the train. Missing/offline players retain their own pending boarding pose.
class RPFirstPassengers
{
    protected ref RPConvoyService m_Service;
    protected ref array<ref RPFirstPassengerState> m_Entries = new array<ref RPFirstPassengerState>;

    void Configure(RPConvoyService service, array<ref RPFirstPassengerState> saved = null)
    {
        m_Service = service;
        if (saved) m_Entries = saved;
    }
    static bool ValidEntry(RPFirstPassengerState entry)
    {
        if (!entry || entry.PlayerId == "" || entry.PlayerId.Length() > 128) return false;
        if (entry.Slot < 2 || entry.Slot > 4) return false;
        for (int i = 0; i < 3; i++)
        {
            if (!RPFirstRecoveryPhases.FiniteRange(entry.LocalPosition[i], -25, 25)) return false;
            if (!RPFirstRecoveryPhases.FiniteRange(entry.LocalDirection[i], -1.001, 1.001)) return false;
        }
        return Math.AbsFloat(entry.LocalDirection.Length() - 1) < 0.01 && Math.AbsFloat(entry.LocalDirection[1]) < 0.01;
    }
    static bool ValidRoster(array<ref RPFirstPassengerState> entries)
    {
        if (!entries || entries.Count() > 256) return false;
        for (int i = 0; i < entries.Count(); i++)
        {
            if (!ValidEntry(entries[i])) return false;
            for (int j = 0; j < i; j++) if (entries[j].PlayerId == entries[i].PlayerId) return false;
        }
        return true;
    }
    RPFirstPassengerState Find(string id)
    {
        for (int i = 0; i < m_Entries.Count(); i++) if (m_Entries[i].PlayerId == id) return m_Entries[i];
        return null;
    }
    void Forget(string id)
    {
        for (int i = m_Entries.Count() - 1; i >= 0; i--) if (m_Entries[i].PlayerId == id) m_Entries.RemoveOrdered(i);
    }
    static bool Inside(RP_ConvoyUnit unit, vector relativePos)
    {
        if (!unit || !unit.IsPassengerWagon() || !unit.PrepareBounds()) return false;
        vector low = unit.Low(); vector high = unit.High();
        return relativePos[0] >= low[0] && relativePos[0] <= high[0] && relativePos[2] >= low[2] && relativePos[2] <= high[2] && relativePos[1] >= low[1] && relativePos[1] <= high[1] + 0.4;
    }
    // Trusted server caller supplies an authenticated GetId in ordinary play.
    // Native fixtures use explicit test identities, never a client-supplied ID.
    void Observe(string id, PlayerBase player)
    {
        if (id == "" || !player) return;
        if (!player.IsAlive()) { Forget(id); return; }
        if (player.FirstRecoveryWaiting()) return;
        RP_ConvoyUnit unit = ParentWagon(player);
        if (!unit || !unit.IsPassengerWagon() || m_Service.Unit(unit.Slot()) != unit)
        {
            Forget(id); return;
        }
        vector relativePos = unit.WorldToModel(player.PhysicsGetPositionWS());
        if (!Inside(unit, relativePos)) { Forget(id); return; }
        RPFirstPassengerState entry = Find(id);
        if (!entry)
        {
            if (m_Entries.Count() >= 256) return;
            entry = new RPFirstPassengerState; entry.PlayerId = id; m_Entries.Insert(entry);
        }
        entry.Slot = unit.Slot(); entry.LocalPosition = relativePos;
        vector ahead = unit.WorldToModel(player.PhysicsGetPositionWS() + player.GetDirection());
        vector direction = ahead - relativePos; direction[1] = 0; direction.Normalize();
        entry.LocalDirection = direction;
    }
    array<ref RPFirstPassengerState> Capture()
    {
        array<Man> players = new array<Man>; GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players[i]);
            if (player && player.GetIdentity()) Observe(player.GetIdentity().GetId(), player);
        }
        return m_Entries;
    }
    bool SpawnPose(string id, out vector pos, out vector direction)
    {
        RPFirstPassengerState entry = Find(id);
        if (!ValidEntry(entry)) return false;
        RP_ConvoyUnit unit = m_Service.Unit(entry.Slot);
        if (!Inside(unit, entry.LocalPosition)) return false;
        pos = unit.ModelToWorld(entry.LocalPosition);
        direction = unit.ModelToWorld(entry.LocalPosition + entry.LocalDirection) - pos;
        return true;
    }
    static RP_ConvoyUnit ParentWagon(PlayerBase player)
    {
        if (!player) return null;
        // Human documents that PhysicsGetLinkedEntity is not valid while
        // GetParent exists. Explicit restart linking uses that native parent.
        RP_ConvoyUnit unit = RP_ConvoyUnit.Cast(player.GetParent());
        if (!unit) unit = RP_ConvoyUnit.Cast(player.PhysicsGetLinkedEntity());
        return unit;
    }
    static bool Place(PlayerBase player, RP_ConvoyUnit unit, vector relativePos, vector direction)
    {
        if (!player || !player.IsAlive() || !Inside(unit, relativePos)) return false;
        vector relative[4]; vector parent[4]; vector world[4];
        Math3D.DirectionAndUpMatrix(direction, Vector(0, 1, 0), relative);
        relative[3] = relativePos; unit.GetTransform(parent); Math3D.MatrixMultiply4(parent, relative, world);
        if (player.GetParent() || player.PhysicsGetLinkedEntity()) player.UnlinkFromLocalSpace();
        player.SetTransform(world); player.StartCommand_Move();
        player.LinkToLocalSpaceOf(unit, relative);
        return ParentWagon(player) == unit;
    }
    bool Restore(string id, PlayerBase player, bool requestClient = false)
    {
        RPFirstPassengerState entry = Find(id);
        if (!ValidEntry(entry) || !player || player.FirstRecoveryIssued()) return false;
        RP_ConvoyUnit unit = m_Service.Unit(entry.Slot);
        if (!Place(player, unit, entry.LocalPosition, entry.LocalDirection)) return false;
        if (requestClient) player.FirstRequestRecovery(entry);
        Print("[RailPilotRecovery] PASSENGER restored slot=" + entry.Slot.ToString());
        return true;
    }
};

// One placement per connection, after the client's own wagon presentation is
// ready. Retry only readiness; never teleport a player once per moving frame.
modded class RP_ConvoyUnit
{
    bool FirstPresentationReady() { return m_ServiceEnabled && m_HavePose; }
    override void ReleaseRiders()
    {
        super.ReleaseRiders();
        array<Man> players = new array<Man>; GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(players[i]);
            if (player && player.GetParent() == this) player.UnlinkFromLocalSpace();
        }
    }
};
modded class PlayerBase
{
    protected int m_FirstRecoverySlot = -1;
    protected int m_FirstRecoveryToken;
    protected int m_FirstRecoveryAck;
    protected int m_FirstRecoveryPlaced;
    protected float m_FirstRecoveryX;
    protected float m_FirstRecoveryY;
    protected float m_FirstRecoveryZ;
    protected float m_FirstRecoveryDX;
    protected float m_FirstRecoveryDZ;
    protected int m_FirstRecoveryRetries;
    protected bool m_FirstRecoveryQueued;
    static const int FIRST_RECOVERY_ACK_RPC = 4729182;

    void PlayerBase()
    {
        RegisterNetSyncVariableInt("m_FirstRecoverySlot", -1, 4);
        RegisterNetSyncVariableInt("m_FirstRecoveryToken", 0, 1000000);
        RegisterNetSyncVariableFloat("m_FirstRecoveryX", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_FirstRecoveryY", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_FirstRecoveryZ", 0, 0, 4);
        RegisterNetSyncVariableFloat("m_FirstRecoveryDX", -1, 1, 4);
        RegisterNetSyncVariableFloat("m_FirstRecoveryDZ", -1, 1, 4);
    }
    bool FirstRecoveryWaiting() { return m_FirstRecoveryToken > m_FirstRecoveryAck; }
    bool FirstRecoveryIssued() { return m_FirstRecoveryToken > 0; }
    void FirstRequestRecovery(RPFirstPassengerState entry)
    {
        if (!GetGame().IsServer() || !entry || FirstRecoveryIssued()) return;
        m_FirstRecoverySlot = entry.Slot;
        vector relativePos = entry.LocalPosition; vector direction = entry.LocalDirection;
        m_FirstRecoveryX = relativePos[0]; m_FirstRecoveryY = relativePos[1]; m_FirstRecoveryZ = relativePos[2];
        m_FirstRecoveryDX = direction[0]; m_FirstRecoveryDZ = direction[2];
        m_FirstRecoveryToken = 1; SetSynchDirty();
    }
    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (GetGame().IsServer() || m_FirstRecoveryToken <= m_FirstRecoveryPlaced || m_FirstRecoveryQueued) return;
        if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_REMOTE) return;
        m_FirstRecoveryQueued = true;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(FirstTryRecovery, 100, false);
    }
    void FirstTryRecovery()
    {
        m_FirstRecoveryQueued = false;
        if (!GetGame() || GetGame().IsServer() || !IsAlive() || m_FirstRecoveryToken <= m_FirstRecoveryPlaced) return;
        if (GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_REMOTE) return;
        RP_ConvoyUnit unit = RP_ConvoyUnit.AtSlot(m_FirstRecoverySlot);
        if (GetGame().GetPlayer() == this && unit && unit.FirstPresentationReady())
        {
            vector relativePos = Vector(m_FirstRecoveryX, m_FirstRecoveryY, m_FirstRecoveryZ);
            vector direction = Vector(m_FirstRecoveryDX, 0, m_FirstRecoveryDZ); direction.Normalize();
            if (RPFirstPassengers.Place(this, unit, relativePos, direction))
            {
                m_FirstRecoveryPlaced = m_FirstRecoveryToken;
                GetGame().RPCSingleParam(this, FIRST_RECOVERY_ACK_RPC, new Param1<int>(m_FirstRecoveryToken), true);
                Print("[RailPilotRecovery] CLIENT passenger placed once on streamed wagon"); return;
            }
        }
        m_FirstRecoveryRetries++;
        if (m_FirstRecoveryRetries >= 600)
        {
            Print("[RailPilotRecovery] ERROR passenger wagon not ready after60seconds; pending pose retained"); return;
        }
        m_FirstRecoveryQueued = true;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(FirstTryRecovery, 100, false);
    }
    override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
    {
        super.OnRPC(sender, rpc_type, ctx);
        if (rpc_type != FIRST_RECOVERY_ACK_RPC || !GetGame().IsServer() || !sender || !GetIdentity()) return;
        Param1<int> ack;
        if (sender.GetId() == GetIdentity().GetId() && ctx.Read(ack) && ack.param1 == m_FirstRecoveryToken)
            m_FirstRecoveryAck = ack.param1;
    }
    override void EEDelete(EntityAI parent)
    {
        if (GetGame()) GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(FirstTryRecovery);
        super.EEDelete(parent);
    }
};

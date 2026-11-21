// Visible geometry, native passenger floor and network identity stay together,
// preserving the approved Rider implementation (no player teleports/proxies).
class RP_ConvoyUnit : House
{
    protected static ref array<RP_ConvoyUnit> s_Units = new array<RP_ConvoyUnit>;
    protected static ref RPConvoyClock s_Clock = new RPConvoyClock;
    protected static ref RPConvoyRoute s_Route;
    protected static int s_PresentationLeading = -1;
    protected int m_Slot = -1;
    protected float m_Offset;
    protected float m_RouteClock;
    protected int m_ServiceLap;
    protected bool m_ServiceEnabled;
    protected int m_LeadingSlot;
    protected int m_Night;
    protected float m_ServiceSpeed;
    protected float m_Bottom;
    protected vector m_Low;
    protected vector m_High;
    protected bool m_HaveBounds;
    protected bool m_HavePose;
    protected vector m_Pose[4];
    protected vector m_AnglesRadians;
    protected ref RPConvoyEffects m_Effects;

    void RP_ConvoyUnit()
    {
        RegisterNetSyncVariableInt("m_Slot", -1, 5);
        RegisterNetSyncVariableFloat("m_Offset", 0, 250, 4);
        RegisterNetSyncVariableFloat("m_RouteClock", 0, 0, 4);
        RegisterNetSyncVariableInt("m_ServiceLap", 0, 1000000000);
        RegisterNetSyncVariableBool("m_ServiceEnabled");
        RegisterNetSyncVariableInt("m_LeadingSlot", 0, 5);
        RegisterNetSyncVariableInt("m_Night", 0, 1);
        RegisterNetSyncVariableFloat("m_ServiceSpeed", 0, 50, 4);
    }

    override void EEInit()
    {
        super.EEInit();
        s_Units.Insert(this);
        SetFlags(EntityFlags.ACTIVE, false);
        ClearFlags(EntityFlags.STATIC, false);
        SetAllowDamage(false);
        PrepareBounds();
        KeepDoorsOpen();
    }

    bool PrepareBounds()
    {
        if (m_HaveBounds) return true;
        vector bounds[2];
        if (!GetCollisionBox(bounds)) return false;
        m_Low = bounds[0]; m_High = bounds[1]; m_Bottom = m_Low[1];
        m_HaveBounds = m_High[2] > m_Low[2] && m_High[0] > m_Low[0];
        return m_HaveBounds;
    }

    void Configure(int slot, float offset)
    {
        m_Slot = slot; m_Offset = offset;
    }

    // Extents expressed in the route's positive direction (rear loco reversed).
    float BackExtent()
    {
        if (m_Slot == 5) return m_High[2];
        return -m_Low[2];
    }
    float FrontExtent()
    {
        if (m_Slot == 5) return -m_Low[2];
        return m_High[2];
    }
    int Slot() { return m_Slot; }
    float Offset() { return m_Offset; }
    float Speed() { return m_ServiceSpeed; }
    RPConvoyEffects Effects() { return m_Effects; }
    bool Stopped() { return m_ServiceEnabled && m_ServiceSpeed <= 0.001; }
    vector Low() { return m_Low; }
    vector High() { return m_High; }
    bool IsPassengerWagon() { return m_Slot == 2 || m_Slot == 3 || m_Slot == 4; }

    void SetServiceLap(int lap) { m_ServiceLap = lap; }

    void ServerState(float clock, float speed, int leading, int night, bool synchronize = true)
    {
        m_RouteClock = clock; m_ServiceSpeed = speed;
        m_LeadingSlot = leading; m_Night = night;
        m_ServiceEnabled = true;
        if (synchronize) SetSynchDirty();
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (m_ServiceEnabled) s_Clock.Push(m_RouteClock, m_ServiceLap);
        KeepDoorsOpen();
    }

    static void SetRoute(RPConvoyRoute route) { s_Route = route; }

    static RPConvoyRoute Route()
    {
        if (!s_Route) s_Route = RPConvoyRoute.Load();
        return s_Route;
    }

    void Present(float clock, float dt)
    {
        RPConvoyRoute route = Route();
        if (!route || m_Slot < 0 || !PrepareBounds()) return;
        float distance; float speed; int direction;
        route.State(clock, distance, speed, direction);
        vector pose[4];
        float bogieHalf = 4.5;
        if (m_Slot == 0 || m_Slot == 5) bogieHalf = 4;
        RPFirstAlignment.PoseAtClock(route, m_Slot, clock, m_Offset, pose);
        SetTransform(pose); Update();
        for (int i = 0; i < 4; i++) m_Pose[i] = pose[i];
        m_AnglesRadians = Math3D.MatrixToAngles(pose) * Math.DEG2RAD;
        m_HavePose = true;
        KeepDoorsOpen();
        if (!GetGame().IsDedicatedServer() && (m_Slot == 0 || m_Slot == 5))
        {
            if (!m_Effects) m_Effects = new RPConvoyEffects;
            bool active = m_ServiceEnabled && m_Slot == RPConvoySpec.LeadingSlot(direction);
            // Prediction and server must agree before enabling a new source.
            if (GetGame().IsMultiplayer()) active = active && m_Slot == m_LeadingSlot;
            bool night = GetGame().GetWorld().IsNight();
            if (GetGame().IsMultiplayer()) night = night && m_Night == 1;
            m_Effects.Update(this, active, night, speed, route.Speed, dt);
        }
    }

    override bool OnNetworkTransformUpdate(out vector pos, out vector ypr)
    {
        if (GetGame().IsServer() || !m_ServiceEnabled || !m_HavePose) return false;
        pos = m_Pose[3]; ypr = m_AnglesRadians;
        return true;
    }

    void KeepDoorsOpen() {}

    override bool CanDoorBeClosed(int doorIndex) { return false; }
    override bool CanDoorBeClosed(int doorIndex, Object caller) { return false; }
    override bool CanDoorBeLocked(int doorIndex) { return false; }

    bool Contains(vector point, float margin = 0)
    {
        if (!PrepareBounds()) return false;
        vector p = WorldToModel(point);
        return p[0] >= m_Low[0] - margin && p[0] <= m_High[0] + margin && p[1] >= m_Low[1] - 0.5 && p[1] <= m_High[1] + 3 && p[2] >= m_Low[2] - margin && p[2] <= m_High[2] + margin;
    }

    static RP_ConvoyUnit AtSlot(int slot)
    {
        for (int i = 0; i < s_Units.Count(); i++)
            if (s_Units[i] && s_Units[i].m_Slot == slot) return s_Units[i];
        return null;
    }

    static array<RP_ConvoyUnit> All() { return s_Units; }

    static void UpdateAll(float dt)
    {
        if (GetGame().IsServer()) return;
        float clock = s_Clock.Sample(GetGame().GetTime() / 1000.0);
        RPConvoyRoute route = Route();
        if (!route) return;
        float distance; float speed; int direction;
        route.State(clock, distance, speed, direction);
        SelectLeading(RPConvoySpec.LeadingSlot(direction));
        for (int i = 0; i < s_Units.Count(); i++)
            if (s_Units[i] && s_Units[i].m_ServiceEnabled) s_Units[i].Present(clock, dt);
    }

    static void ReleaseAllEffects()
    {
        for (int i = 0; i < s_Units.Count(); i++)
            if (s_Units[i] && s_Units[i].m_Effects) s_Units[i].m_Effects.Release();
    }

    static void SelectLeading(int slot)
    {
        if (slot == s_PresentationLeading) return;
        ReleaseAllEffects(); RPConvoyHorn.Stop(); s_PresentationLeading = slot;
    }

    static int ActiveSlot() { return s_PresentationLeading; }

    static void ResetPresentation()
    {
        ReleaseAllEffects(); s_Clock = new RPConvoyClock;
        s_Route = null; s_PresentationLeading = -1;
    }

    void ReleaseRiders()
    {
        array<Man> players = new array<Man>; GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            Human human = Human.Cast(players[i]);
            if (human && human.PhysicsGetLinkedEntity() == this) human.UnlinkFromLocalSpace();
        }
    }

    override void EEDelete(EntityAI parent)
    {
        if (m_Effects) m_Effects.Release();
        s_Units.RemoveItem(this);
        if (s_Units.Count() == 0) ResetPresentation();
        super.EEDelete(parent);
    }
};

class RP_ConvoyBlue742 : RP_ConvoyUnit {};
class RP_ConvoyFlat : RP_ConvoyUnit {};

class RP_ConvoyBox : RP_ConvoyUnit
{
    override void KeepDoorsOpen()
    {
        SetAnimationPhase("Door1", 1); SetAnimationPhase("Door2", 1);
    }
};

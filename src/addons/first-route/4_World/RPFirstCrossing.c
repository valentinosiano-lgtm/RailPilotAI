// Approved alarm/prewarning/arm timing, with the explicit parked start.
class RPFirstCrossingLogic : RPGCCrossingLogic
{
    RPFirstGateState Capture(string id)
    {
        RPFirstGateState state = new RPFirstGateState;
        state.Id = id; state.State = m_State; state.Phase = m_Phase; state.Direction = m_Direction;
        state.PrewarningRemaining = Math.Clamp((m_PreWarningDeadlineMs - GetGame().GetTime()) / 1000.0, 0, 5);
        return state;
    }
    void Restore(RPFirstGateState state, bool occupied)
    {
        m_State = state.State; m_Phase = state.Phase; m_Direction = state.Direction;
        m_HornFired = true; m_HornEvent = false; m_PreWarningDeadlineMs = 0;
        if (state.PrewarningRemaining > 0)
        {
            m_PreWarningDeadlineMs = GetGame().GetTime() + Math.Round(state.PrewarningRemaining * 1000);
            m_PreWarningStartMs = m_PreWarningDeadlineMs - 5000;
        }
        // Revalidate the complete world footprint before enabling motion.
        if (occupied)
        {
            m_State = 1; m_Phase = 1; m_PreWarningDeadlineMs = 0;
        }
    }
    void InitiallyClosed()
    {
        m_State = 1; m_Phase = 1; m_Direction = 1;
        m_PreWarningDeadlineMs = 0; m_HornFired = true;
    }
};
class RPFirstCrossing
{
    protected ref RPConvoyService m_Service;
    protected ref RPFirstCrossingSite m_Site;
    protected ref RPFirstCrossingLogic m_Logic;
    protected Land_rp_crossing_barrier m_A;
    protected Land_rp_crossing_barrier m_B;
    protected bool m_Initial;
    protected bool m_Offline;
    protected float m_MinClosed = 1;
    protected float m_OpeningClearance = 1000;
    protected bool m_Silent;
    protected float m_Tick;

    bool Configure(RPConvoyService service, RPFirstCrossingSite site, bool initial, bool offline)
    {
        m_Service = service; m_Site = site; m_Initial = initial; m_Offline = offline;
        m_Logic = new RPFirstCrossingLogic;
        if (!m_Logic.ConfigureRoadWidth(site.RoadHalfWidth)) return false;
        if (initial) m_Logic.InitiallyClosed();
        for (int i = 0; i < 2; i++)
        {
            vector p = site.BarrierPositions[i];
            // The Arma 2 replacement uses a ground-level origin. The vanilla
            // model's higher origin is only useful for locating its instance.
            p[1] = GetGame().SurfaceY(p[0], p[2]) + 0.06;
            vector orientation = site.BarrierOrientations[i];
            if (site.Segment != 0)
            {
                orientation[1] = 0;
                orientation[2] = 0;
            }
            Land_rp_crossing_barrier gate = Land_rp_crossing_barrier.Cast(GetGame().CreateObjectEx("Land_rp_crossing_barrier", p, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
            if (!gate) { Release(); return false; }
            if (i == 0) m_A = gate; else m_B = gate;
            gate.SetOrientation(orientation); gate.HideVanillaOriginal(); gate.SetAllowDamage(false);
            if (site.Segment != 0 && !gate.OriginalBarrier()) { Release(); return false; }
        }
        Apply(); return true;
    }

    void Apply()
    {
        RPConvoyRoute route = m_Service.Route(); float clock = m_Service.Clock();
        // Preserve the existing lamp/arm commands byte for byte in meaning.
        bool initialLightsOff = m_Initial && clock < route.DepartureAt(0);
        bool alarm = m_Logic.Alarm() && !initialLightsOff;
        m_Silent = m_Site.Segment == 0 && !OnReturn() && SoundMutedAt(route, clock);
        bool wake = initialLightsOff && !m_Silent;
        m_A.ServerAudioState(m_Silent, wake);
        m_B.ServerAudioState(m_Silent, wake);
        m_A.ServerApply(m_Logic.Phase(), alarm, !initialLightsOff);
        m_B.ServerApply(m_Logic.Phase(), alarm, !initialLightsOff);
        if (m_Offline) { m_A.OnVariablesSynchronized(); m_B.OnVariablesSynchronized(); }
    }
    static bool SoundMutedAt(RPConvoyRoute route, float clock)
    {
        int phase = RPFirstRecoveryPhases.At(route, Math.Min(clock, route.TotalSeconds()));
        // The final stop has no scheduled onward departure.
        if (phase >= 6) return true;
        if (phase == 0 || phase == 2 || phase == 4)
            return RPFirstRecoveryPhases.Begin(route, phase + 1) - clock > 5;
        return false;
    }
    void ProjectPose(RP_ConvoyUnit unit, vector pose[4], inout float minimum, inout float maximum)
    {
        vector lo = unit.Low(); vector hi = unit.High();
        for (int corner = 0; corner < 8; corner++)
        {
            vector p = lo;
            if ((corner & 1) != 0) p[0] = hi[0];
            if ((corner & 2) != 0) p[1] = hi[1];
            if ((corner & 4) != 0) p[2] = hi[2];
            float along = vector.Dot(p.Multiply4(pose) - m_Site.Pivot, m_Site.TrackAxis);
            minimum = Math.Min(minimum, along); maximum = Math.Max(maximum, along);
        }
    }
    protected bool OnReturn()
    {
        return m_Site.ReturnDistance > 0 && m_Service.Route().SegmentAt(m_Service.Clock()) == m_Site.ReturnSegment;
    }
    protected bool OnSiteSegment()
    {
        return OnReturn() || m_Service.Route().SegmentAt(m_Service.Clock()) == m_Site.Segment;
    }
    void Interval(out float minimum, out float maximum)
    {
        m_Service.OccupiedInterval(minimum, maximum);
        float crossingDistance = m_Site.Distance;
        if (OnReturn()) crossingDistance = m_Site.ReturnDistance;
        minimum -= crossingDistance; maximum -= crossingDistance;
        // Use world footprints locally and route coordinates elsewhere, so
        // distant bends cannot falsely project onto this road a second time.
        if (minimum > 350 || maximum < -350) return;
        minimum = 100000; maximum = -100000;
        float lagDistance; float lagSpeed; int lagDirection;
        RPConvoyRoute route = m_Service.Route();
        route.State(Math.Max(0, m_Service.Clock() - m_Site.PresentationAllowanceSeconds), lagDistance, lagSpeed, lagDirection);
        for (int i = 0; i < 6; i++)
        {
            RP_ConvoyUnit unit = m_Service.Unit(i); vector pose[4]; unit.GetTransform(pose);
            ProjectPose(unit, pose, minimum, maximum);
            RPFirstAlignment.PoseAtClock(route, i, Math.Max(0, m_Service.Clock() - m_Site.PresentationAllowanceSeconds), unit.Offset(), pose);
            ProjectPose(unit, pose, minimum, maximum);
        }
    }
    void Update(float dt)
    {
        m_Tick += dt; if (m_Tick < 0.05) return;
        dt = m_Tick; m_Tick = 0;
        if (!OnSiteSegment()) { Apply(); return; }
        float minimum; float maximum; Interval(minimum, maximum);
        float distance; float speed; int direction;
        RPConvoyRoute route = m_Service.Route(); route.State(m_Service.Clock(), distance, speed, direction);
        int before = m_Logic.State();
        float logicalMinimum = minimum; float logicalMaximum = maximum;
        if (m_Site.Name == "Kamenka" && m_Service.Clock() >= route.ArrivalAt(1) && m_Service.Clock() <= route.DepartureAt(1))
        {
            logicalMinimum = Math.Min(minimum, -m_Site.RoadHalfWidth);
            logicalMaximum = Math.Max(maximum, m_Site.RoadHalfWidth);
        }
        m_Logic.Update(logicalMinimum, logicalMaximum, speed, dt);
        if (before != m_Logic.State())
        {
            if (m_Logic.State() == 2) m_OpeningClearance = Math.Min(m_OpeningClearance, minimum);
            string log = "[RailPilotGuided] FIRST_CROSSING " + m_Site.Name + " state=" + m_Logic.State().ToString();
            log += " clock=" + m_Service.Clock().ToString() + " tailClearance=" + minimum.ToString(); Print(log);
        }
        if (minimum <= m_Site.RoadHalfWidth && maximum >= -m_Site.RoadHalfWidth) m_MinClosed = Math.Min(m_MinClosed, m_Logic.Phase());
        Apply();
    }
    float Phase() { return m_Logic.Phase(); }
    RPFirstGateState Capture() { return m_Logic.Capture(m_Site.Name); }
    void Restore(RPFirstGateState state)
    {
        bool occupied;
        RPConvoyRoute route = m_Service.Route(); float clock = m_Service.Clock();
        if (OnSiteSegment())
        {
            float minimum; float maximum; Interval(minimum, maximum);
            occupied = minimum <= m_Site.RoadHalfWidth && maximum >= -m_Site.RoadHalfWidth;
            if (m_Site.Name == "Kamenka" && clock >= route.ArrivalAt(1) && clock <= route.DepartureAt(1)) occupied = true;
        }
        m_Logic.Restore(state, occupied); Apply();
    }
    bool Alarm() { return (m_Logic.Alarm() || (m_Initial && m_Service.Clock() < m_Service.Route().DepartureAt(0))) && !m_Silent; }
    bool SoundMuted() { return m_Silent; }
    float MinClosed() { return m_MinClosed; }
    float OpeningClearance() { return m_OpeningClearance; }
    RPFirstCrossingLogic Logic() { return m_Logic; }
    Land_rp_crossing_barrier GateA() { return m_A; }
    Land_rp_crossing_barrier GateB() { return m_B; }
    void Release()
    {
        if (m_A) GetGame().ObjectDelete(m_A);
        if (m_B) GetGame().ObjectDelete(m_B);
        m_A = null; m_B = null;
    }
};

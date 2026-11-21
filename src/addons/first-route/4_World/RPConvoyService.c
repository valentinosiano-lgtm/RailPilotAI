class RPConvoyService
{
    protected ref array<RP_ConvoyUnit> m_Units = new array<RP_ConvoyUnit>;
    protected ref RPConvoyRoute m_Route;
    protected float m_Clock;
    protected int m_Lap;
    protected float m_Sync;
    protected float m_Length;
    protected bool m_Moved;
    protected float m_StoppedFor;
    protected int m_HornSequence;
    protected int m_LeadingSlot;
    protected bool m_StopGreetings = true;
    void SetStopGreetings(bool enabled) { m_StopGreetings = enabled; }

    bool Start(RPConvoyRoute route, float initialClock = 0, int initialLap = 0)
    {
        if (!GetGame().IsServer() || !route || m_Units.Count() > 0) return false;
        // Refuse a second service instead of duplicating the consist/fuel.
        if (RP_ConvoyUnit.All().Count() > 0) return false;
        m_Clock = initialClock; m_Lap = initialLap; m_Sync = 0; m_LeadingSlot = 0;
        m_Moved = false; m_StoppedFor = 0;
        m_Route = route; RP_ConvoyUnit.SetRoute(route);
        float offset;
        for (int i = 0; i < RPConvoySpec.COUNT; i++)
        {
            RP_ConvoyUnit unit = RP_ConvoyUnit.Cast(GetGame().CreateObjectEx(RPConvoySpec.TypeAt(i), route.At(route.Start), ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
            if (!unit) { Stop(); return false; }
            m_Units.Insert(unit); unit.Configure(i, 0);
            if (!unit.PrepareBounds()) { Stop(); return false; }
            if (i > 0)
                offset += m_Units[i - 1].BackExtent() + RPConvoySpec.COUPLING_GAP + unit.FrontExtent();
            unit.Configure(i, offset);
        }
        m_Length = offset + m_Units[5].BackExtent() + m_Units[0].FrontExtent();
        // Never silently clamp rear wagons onto the start of a short route.
        if (route.Start - offset - m_Units[5].BackExtent() < 5 || route.End + m_Units[0].FrontExtent() > route.Length - 1)
        {
            Print("[RailPilotConvoy] ERROR route lacks space for the complete six-unit consist");
            Stop(); return false;
        }
        RP_ConvoyTanker tank = RP_ConvoyTanker.Cast(m_Units[1]); tank.RefreshFuel();
        Tick(0);
        Print("[RailPilotConvoy] READY six units, length=" + m_Length.ToString() + "m, blue742-tanker-box-box-flat-reversedBlue742");
        return true;
    }

    void Tick(float dt)
    {
        if (!GetGame().IsServer() || m_Units.Count() != RPConvoySpec.COUNT) return;
        m_Clock += Math.Max(0, dt); m_Sync += dt;
        if (m_Route.Cyclic && m_Clock >= m_Route.CycleDuration())
        {
            m_Clock = m_Route.LapClock(m_Clock);
            m_Lap++;
            m_Sync = 0.1;
        }
        float distance; float speed; int direction;
        m_Route.State(m_Clock, distance, speed, direction);
        int nextLeading = RPConvoySpec.LeadingSlot(direction);
        if (nextLeading != m_LeadingSlot)
        {
            RPConvoyHorn.Stop();
            m_LeadingSlot = nextLeading;
        }
        RP_ConvoyUnit.SelectLeading(nextLeading);
        bool sync = m_Sync >= 0.1 || dt == 0;
        int night;
        if (GetGame().GetWorld().IsNight()) night = 1;
        for (int i = 0; i < m_Units.Count(); i++)
        {
            RP_ConvoyUnit unit = m_Units[i];
            if (!unit) { Stop(); return; }
            unit.SetServiceLap(m_Lap);
            unit.ServerState(m_Clock, speed, m_LeadingSlot, night, sync);
            unit.Present(m_Clock, dt);
        }
        if (sync) m_Sync = 0;
        if (speed > 1) { m_Moved = true; m_StoppedFor = 0; }
        else if (speed <= 0.001 && m_Moved)
        {
            m_StoppedFor += dt;
            if (m_StopGreetings && m_StoppedFor >= 0.7)
            {
                m_HornSequence++; RPConvoyHorn.Emit(m_Units[m_LeadingSlot], m_HornSequence);
                m_Moved = false; m_StoppedFor = 0;
            }
        }
    }

    RP_ConvoyUnit Unit(int slot)
    {
        if (slot < 0 || slot >= m_Units.Count()) return null;
        return m_Units[slot];
    }
    int LeadingSlot() { return m_LeadingSlot; }
    float Clock() { return m_Clock; }
    int Lap() { return m_Lap; }
    float Length() { return m_Length; }
    RPConvoyRoute Route() { return m_Route; }
    int RPBlockedStopIndex() { return -1; }
    float RPBlockedStopTime() { return 0; }
    bool RPRestoreBlockedStop(int stop, float elapsed) { return stop < 0; }

    // For crossing integration: complete occupied interval, independent of
    // travel direction. Do not reuse the old two-unit crossing footprint.
    void OccupiedInterval(out float back, out float front)
    {
        float distance; float speed; int direction;
        m_Route.State(m_Clock, distance, speed, direction);
        back = distance - m_Units[5].Offset() - m_Units[5].BackExtent();
        front = distance + m_Units[0].FrontExtent();
    }

    void Stop()
    {
        for (int i = 0; i < m_Units.Count(); i++)
            if (m_Units[i]) { m_Units[i].ReleaseRiders(); GetGame().ObjectDelete(m_Units[i]); }
        m_Units.Clear(); RPConvoyHorn.Stop();
        RP_ConvoyUnit.ResetPresentation();
    }
};

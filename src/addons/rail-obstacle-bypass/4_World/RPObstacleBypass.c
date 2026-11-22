modded class RP_ConvoyUnit
{
    protected int m_RPBypassEpoch;

    void RP_ConvoyUnit()
    {
        RegisterNetSyncVariableInt("m_RPBypassEpoch", 0, 1000000);
    }

    void RPSetBypassEpoch(int epoch) { m_RPBypassEpoch = epoch; }
    int RPBypassEpoch() { return m_RPBypassEpoch; }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (!GetGame().IsServer() && m_ServiceEnabled)
            s_Clock.RPReceiveBypass(m_RPBypassEpoch, m_RouteClock, m_ServiceLap);
    }
};

modded class RPConvoyService
{
    static const float EMPTY_SECONDS = 3.0;
    static const float RETRY_SECONDS = 1.0;
    static const float PASSENGER_POLL = 0.10;
    static const float POST_JUMP_DWELL_SECONDS = 120.0;

    protected ref RPBypassPassengers m_BypassPassengers = new RPBypassPassengers;
    protected ref RPBypassDestination m_BypassDestination = new RPBypassDestination;
    protected int m_BypassEpoch;
    protected bool m_BypassWaiting;
    protected bool m_BypassEmpty;
    protected float m_BypassElapsed;
    protected float m_BypassPoll;
    protected float m_BypassRetry;
    protected string m_BypassDenied;
    protected int m_BlockedStop = -1;
    protected float m_BlockedStopTime;
    protected float m_StationSearch;
    protected float m_PostJumpDwell;
    protected float m_PostJumpSync;

    int RPBypasses() { return m_BypassEpoch; }
    float RPEmptySeconds() { return m_BypassElapsed; }
    string RPBypassDenied() { return m_BypassDenied; }
    override int RPBlockedStopIndex() { return m_BlockedStop; }
    override float RPBlockedStopTime() { return m_BlockedStopTime; }

    override bool RPRestoreBlockedStop(int stop, float elapsed)
    {
        if (stop < 0) return true;
        if (m_Route.StationApproach(m_Clock, m_Length) != stop) return false;
        m_BlockedStop = stop;
        m_BlockedStopTime = elapsed;
        m_ObstacleRate = 0;
        foreach (RP_ConvoyUnit unit : m_Units) unit.RPSetObstacleMotionRate(0);
        super.Tick(0);
        return true;
    }

    protected void RPResetStationWait()
    {
        m_BlockedStop = -1;
        m_BlockedStopTime = 0;
        m_StationSearch = 0;
    }

    protected void RPResetBypassWait()
    {
        m_BypassWaiting = false;
        m_BypassEmpty = false;
        m_BypassElapsed = 0;
        m_BypassPoll = 0;
        m_BypassRetry = 0;
        m_BypassDenied = "";
    }

    protected void RPCommitBypass(float destination)
    {
        float previous = m_Clock;
        m_BypassPassengers.Notify(m_Units, "The train is moving past the obstacle. It will wait for 2 minutes so you can catch up.");
        m_PostJumpDwell = POST_JUMP_DWELL_SECONDS;
        m_PostJumpSync = 0;
        m_BypassEpoch++;
        m_Clock = destination;
        m_ObstacleRate = 0;
        m_ObstacleDetected = false;
        m_ObstacleClearSeconds = 0;
        m_ObstacleUntilScan = 0;
        m_ObstacleTravelSpeed = 0;
        m_BrakeDuration = 0;
        foreach (RP_ConvoyUnit unit : m_Units)
        {
            unit.RPSetObstacleMotionRate(0);
            unit.RPSetObstacleBrakeWindow(0, 0);
            unit.RPSetBypassEpoch(m_BypassEpoch);
        }
        // One zero-speed authoritative snapshot for the whole consist. The
        // existing service publishes poses and the mission updates its gates.
        super.Tick(0);
        RPResetBypassWait();
        RPResetStationWait();
        Print("[RailPilotBypass] JUMP from=" + previous.ToString() + " to=" + destination.ToString());
    }

    protected void RPAwaitEmptyTrain(float elapsed)
    {
        if (!m_BypassWaiting)
        {
            m_BypassWaiting = true;
            m_BypassPassengers.Notify(m_Units, "Track blocked. Clear the track or have everyone get off so the train can continue.");
        }
        m_BypassPoll -= elapsed;
        if (m_BypassPoll <= 0)
        {
            bool empty = !m_BypassPassengers.HasRiders(m_Units, m_Length);
            if (m_BypassEmpty != empty)
            {
                m_BypassElapsed = 0;
                m_BypassDenied = "";
                m_BypassRetry = 0;
                if (empty) m_BypassPassengers.Notify(m_Units, "Everyone is off the train. It can move past the obstacle in 3 seconds.");
                else m_BypassPassengers.Notify(m_Units, "The train is waiting for all passengers to get off before bypassing the obstacle.");
                m_BypassEmpty = empty;
                m_BypassPoll = PASSENGER_POLL;
                return;
            }
            m_BypassEmpty = empty;
            m_BypassPoll = PASSENGER_POLL;
        }
        if (!m_BypassEmpty) return;
        m_BypassElapsed += elapsed;
        m_BypassRetry -= elapsed;
        if (m_BypassElapsed < EMPTY_SECONDS || m_BypassRetry > 0) return;
        m_BypassRetry = RETRY_SECONDS;
        string reason;
        float destination = m_BypassDestination.FindClock(m_Route, m_Units, m_Clock, m_Length, reason);
        if (destination <= m_Clock)
        {
            if (reason != m_BypassDenied)
            {
                Print("[RailPilotBypass] WAIT " + reason);
                if (reason != "immediate obstruction changed")
                    m_BypassPassengers.Notify(m_Units, "The train is waiting for a clear stretch of track ahead.");
                m_BypassDenied = reason;
            }
            return;
        }
        if (m_BypassPassengers.HasRiders(m_Units, m_Length))
        {
            m_BypassEmpty = false;
            m_BypassElapsed = 0;
            return;
        }
        RPCommitBypass(destination);
    }

    protected void RPAwaitStation(int station, float elapsed)
    {
        if (m_BlockedStop != station)
        {
            RPResetBypassWait();
            RPResetStationWait();
            m_BlockedStop = station;
            m_BypassPassengers.NotifyRiders(m_Units, "Track blocked. Clear the track or have everyone get off so the train can continue.");
        }
        if (m_BlockedStopTime < RPConvoyRoute.FORCED_STOP_SECONDS)
        {
            m_BlockedStopTime = Math.Min(m_BlockedStopTime + elapsed, RPConvoyRoute.FORCED_STOP_SECONDS);
            if (m_BlockedStopTime >= RPConvoyRoute.FORCED_STOP_SECONDS)
            {
                Print("[RailPilotBypass] FORCED_STOP_COMPLETE " + m_Route.Stops[station].Name);
                m_BypassPassengers.NotifyRiders(m_Units, "The train is waiting for all passengers to get off before bypassing the obstacle.");
            }
            // Let the mission persist completion before the next update can jump.
            return;
        }
        m_BypassRetry -= elapsed;
        if (m_BypassRetry > 0) return;
        m_BypassRetry = RETRY_SECONDS;
        if (m_BypassPassengers.HasRiders(m_Units, m_Length))
        {
            m_StationSearch = 0;
            return;
        }

        string reason;
        float destination = m_BypassDestination.FindStationClock(m_Route, m_Units, station, m_Clock, m_Length, m_StationSearch, reason);
        if (destination <= m_Clock)
        {
            if (reason != m_BypassDenied) Print("[RailPilotBypass] STATION_WAIT " + reason);
            m_BypassDenied = reason;
            return;
        }
        // An observer can climb back on during the wait for a clear landing.
        if (m_BypassPassengers.HasRiders(m_Units, m_Length))
        {
            m_StationSearch = 0;
            return;
        }
        Print("[RailPilotBypass] STATION_DEPARTURE " + m_Route.Stops[station].Name);
        RPCommitBypass(destination);
    }

    override void Tick(float dt)
    {
        bool active = GetGame().IsServer() && dt > 0 && m_Route && m_Units.Count() == RPConvoySpec.COUNT;
        if (active && m_PostJumpDwell > 0)
        {
            // Time for riders to reach the train runs independently of the route clock.
            float held = Math.Min(dt, m_PostJumpDwell);
            m_PostJumpDwell -= held;
            dt -= held;
            m_PostJumpSync += held;
            if (m_PostJumpSync >= 0.1 || m_PostJumpDwell == 0)
            {
                super.Tick(0);
                m_PostJumpSync = 0;
            }
            if (m_PostJumpDwell > 0) return;
            Print("[RailPilotBypass] POST_JUMP_DWELL_COMPLETE");
            if (dt <= 0) return;
        }
        float elapsed = Math.Min(dt, 0.25);
        if (active && m_BlockedStop >= 0 && (m_Route.Cyclic || m_BlockedStop < m_Route.Stops.Count() - 1) && m_BlockedStopTime >= RPConvoyRoute.FORCED_STOP_SECONDS)
        {
            RPAwaitStation(m_BlockedStop, elapsed);
            return;
        }
        float previous = m_Clock;
        super.Tick(dt);
        if (!active || m_Units.Count() != RPConvoySpec.COUNT) return;
        if (!m_ObstacleDetected || m_ObstacleRate > 0 || m_ObstacleClearSeconds > 0)
        {
            RPResetBypassWait();
            // Keep time already spent stopped if the blocker moves within
            // the same approach. Clear it after leaving the approach.
            if (m_BlockedStop >= 0 && m_Route.StationApproach(m_Clock, m_Length) != m_BlockedStop) RPResetStationWait();
            return;
        }
        float departure = RPBypassDestination.DwellDeparture(m_Route, m_Clock);
        if (departure > m_Clock)
        {
            // An obstruction within the stand-off must not freeze an existing
            // station dwell before it is time to leave.
            if (previous == m_Clock)
            {
                m_Clock = Math.Min(m_Clock + elapsed, departure);
                super.Tick(0);
            }
            RPResetBypassWait();
            RPResetStationWait();
            return;
        }
        int station = m_Route.StationApproach(m_Clock, m_Length);
        if (station >= 0)
        {
            RPAwaitStation(station, elapsed);
            return;
        }
        RPResetStationWait();
        if (RPBypassDestination.LegEndClock(m_Route, m_Clock) < 0) return;
        RPAwaitEmptyTrain(elapsed);
    }

    override void Stop()
    {
        super.Stop();
        m_BypassEpoch = 0;
        m_PostJumpDwell = 0;
        m_PostJumpSync = 0;
        RPResetBypassWait();
        RPResetStationWait();
    }
};

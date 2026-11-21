// Copyright 2026 RailPilot project contributors.
// Independent server-side proof-of-concept. No third-party train-mod code.

class RPService
{
    protected static const string RP_DIR = "$profile:RailPilot";
    protected static const string RP_SETTINGS_FILE = "$profile:RailPilot/Settings.json";
    protected static const string RP_ROUTE_FILE = "$profile:RailPilot/Route.json";

    protected ref RPSettings m_Settings;
    protected ref RPRoute m_Route;
    protected ref RPState m_State;
    protected ref array<float> m_MetersToPoint;

    protected float m_RouteLength;
    protected float m_SaveCountdown;
    protected int m_LastTickTimeMs;
    protected int m_Started;

    protected ref RPStateStore m_StateStore;
    protected ref RPVisualAdapter m_Visual;

    void RPService()
    {
        m_MetersToPoint = new array<float>;
        m_Settings = new RPSettings;
        m_Route = new RPRoute;
        m_State = new RPState;
        m_StateStore = new RPStateStore(RP_DIR);
        m_Visual = new RPVisualAdapter;
    }

    protected void Log(string message)
    {
        Print("[RailPilot] " + message);
    }

    void Start()
    {
        if (!GetGame() || !GetGame().IsDedicatedServer() || m_Started == 1)
            return;

        if (!FileExist(RP_DIR) && !MakeDirectory(RP_DIR))
        {
            Log("Cannot create profile directory; startup refused.");
            return;
        }
        string error;
        if (FileExist(RP_SETTINGS_FILE))
        {
            m_Settings.SchemaVersion = -1;
            m_Settings.Enabled = -1;
            m_Settings.CruiseSpeedKph = -1;
            m_Settings.AccelerationMps2 = -1;
            m_Settings.ServiceBrakeMps2 = -1;
            m_Settings.TickSeconds = -1;
            m_Settings.SaveEverySeconds = -1;
            m_Settings.DefaultDwellSeconds = -1;
            if (!JsonFileLoader<RPSettings>.LoadFile(RP_SETTINGS_FILE, m_Settings, error))
            {
                Log("Invalid Settings.json; startup refused. " + error);
                return;
            }
        }
        else
        {
            if (!JsonFileLoader<RPSettings>.SaveFile(RP_SETTINGS_FILE, m_Settings, error))
            {
                Log("Cannot write Settings.json. " + error);
                return;
            }
        }
        if (!m_Settings || m_Settings.SchemaVersion != 1 || !RPLimits.Flag(m_Settings.Enabled))
        {
            Log("Invalid settings schema or Enabled flag; startup refused.");
            return;
        }

        if (!FileExist(RP_ROUTE_FILE))
        {
            if (!JsonFileLoader<RPRoute>.SaveFile(RP_ROUTE_FILE, m_Route, error))
            {
                Log("Cannot write Route.json. " + error);
                return;
            }
        }

        if (m_Settings.Enabled != 1)
        {
            Log("Disabled. Templates created under $profile:RailPilot. Configure a surveyed route and restart.");
            return;
        }

        m_Route.SchemaVersion = -1;
        m_Route.RouteId = "";
        m_Route.Map = "";
        m_Route.ServiceMode = "";
        if (!JsonFileLoader<RPRoute>.LoadFile(RP_ROUTE_FILE, m_Route, error))
        {
            Log("Invalid Route.json; startup refused. " + error);
            return;
        }

        if (!ValidateConfiguration())
        {
            Log("Route/settings failed validation. No objects spawned or scheduled.");
            return;
        }

        BuildDistances();
        if (!m_Visual.Start(m_Settings) || !LoadState())
            return;

        m_Started = 1;
        m_LastTickTimeMs = GetGame().GetTime();
        m_SaveCountdown = m_Settings.SaveEverySeconds;

        int intervalMs = (int)(m_Settings.TickSeconds * 1000.0);
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Tick, intervalMs, true);
        Log("Server controller active: " + m_Route.RouteId + ", length " + m_RouteLength.ToString() + "m");
    }

    void Stop()
    {
        if (m_Started == 1)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
            SaveState();
            m_Started = 0;
        }

        m_Visual.Stop();
    }

    protected bool ValidateConfiguration()
    {
        if (!m_Settings || !m_Route || !m_Route.Points)
            return false;

        if (m_Settings.SchemaVersion != 1 || m_Route.SchemaVersion != 2 || m_Route.ServiceMode != "Shuttle")
            return false;

        string world;
        GetGame().GetWorldName(world);
        world.ToLower();
        if (m_Route.Map != "chernarusplus" || world != m_Route.Map || m_Route.RouteId == "")
        {
            Log("Only surveyed ChernarusPlus routes are accepted in this prototype.");
            return false;
        }

        if (!RPLimits.InRange(m_Settings.CruiseSpeedKph, 0.01, 35))
            return false;
        if (!RPLimits.InRange(m_Settings.AccelerationMps2, 0.00001, 10) || !RPLimits.InRange(m_Settings.ServiceBrakeMps2, 0.00001, 10))
            return false;
        if (!RPLimits.InRange(m_Settings.TickSeconds, 0.1, 1))
            return false;
        if (!RPLimits.InRange(m_Settings.SaveEverySeconds, 1, 3600) || !RPLimits.InRange(m_Settings.DefaultDwellSeconds, 0, 3600))
            return false;
        if (!RPLimits.Flag(m_Settings.DebugLogs) || !RPLimits.Flag(m_Settings.EnableDebugVisual))
            return false;
        if (m_Route.Points.Count() < 2 || m_Route.Points.Count() > 50000)
        {
            Log("No surveyed rail points in Route.json.");
            return false;
        }

        int stationCount = 0;
        for (int i = 0; i < m_Route.Points.Count(); i++)
        {
            RPWaypoint point = m_Route.Points.Get(i);
            if (!point)
                return false;
            if (!RPLimits.Flag(point.IsStation))
                return false;
            if (point.IsStation == 1)
            {
                if (point.Name == "")
                    return false;
                stationCount++;
            }
            if (!RPLimits.InRange(point.DwellSeconds, 0, 3600))
                return false;
            if (!RPLimits.InRange(point.X, 0, 15360) || !RPLimits.InRange(point.Z, 0, 15360) || !RPLimits.InRange(point.Y, -100, 2000))
                return false;

            if (i > 0)
            {
                float length = Distance3D(m_Route.Points.Get(i - 1).GetWorldPosition(), point.GetWorldPosition());
                if (length < 0.1 || length > 40.0)
                {
                    Log("Waypoint segment out of safe survey range at index " + i.ToString());
                    return false;
                }
                vector previous = m_Route.Points.Get(i - 1).GetWorldPosition();
                float horizontal = Math.Sqrt(Math.SqrFloat(point.X - previous[0]) + Math.SqrFloat(point.Z - previous[2]));
                if (horizontal < 0.1)
                    return false;
            }
        }

        if (stationCount < 2)
        {
            Log("A route must contain at least two named stations.");
            return false;
        }
        int lastIndex = m_Route.Points.Count() - 1;
        if (m_Route.Points.Get(0).IsStation != 1 || m_Route.Points.Get(lastIndex).IsStation != 1)
        {
            Log("Shuttle endpoints must be surveyed station stops.");
            return false;
        }
        return true;
    }

    protected float Distance3D(vector first, vector second)
    {
        float dx = second[0] - first[0];
        float dy = second[1] - first[1];
        float dz = second[2] - first[2];
        return Math.Sqrt(dx * dx + dy * dy + dz * dz);
    }

    protected void BuildDistances()
    {
        m_MetersToPoint.Clear();
        m_MetersToPoint.Insert(0.0);
        m_RouteLength = 0.0;

        for (int i = 1; i < m_Route.Points.Count(); i++)
        {
            m_RouteLength += Distance3D(m_Route.Points.Get(i - 1).GetWorldPosition(), m_Route.Points.Get(i).GetWorldPosition());
            m_MetersToPoint.Insert(m_RouteLength);
        }
    }

    protected bool LoadState()
    {
        bool fresh;
        if (!m_StateStore.Load(m_State, fresh))
            return false;
        string routeData;
        string error;
        if (!JsonFileLoader<RPRoute>.MakeData(m_Route, routeData, error))
            return false;
        int fingerprint = routeData.Hash();
        if (fresh)
        {
            m_State.RouteId = m_Route.RouteId;
            m_State.RouteFingerprint = fingerprint;
            ChooseNextStation(0);
            return SaveState();
        }

        if (m_State.RouteId != m_Route.RouteId || m_State.RouteFingerprint != fingerprint || !RPLimits.InRange(m_State.DistanceMeters, 0, m_RouteLength) || !RPLimits.InRange(m_State.SpeedMps, 0, m_Settings.CruiseSpeedKph / 3.6))
        {
            Log("Saved state does not match route/settings; startup refused. Archive snapshots before an intentional new run.");
            return false;
        }
        int target = m_State.NextStopPointIndex;
        if (target < 0 || target >= m_Route.Points.Count())
            return false;
        float targetOffset = m_MetersToPoint.Get(target) - m_State.DistanceMeters;
        if (m_Route.Points.Get(target).IsStation != 1 || targetOffset * m_State.Direction < -0.0001)
            return false;
        if (m_State.Phase == 1)
        {
            if (m_State.SpeedMps != 0 || Math.AbsFloat(targetOffset) > 0.0001)
                return false;
        }
        else if (m_State.DwellRemainingSeconds != 0)
        {
            return false;
        }
        // A saved target may not skip an unvisited station.
        int expected = -1;
        int start = 0;
        if (m_State.Direction == -1)
            start = m_Route.Points.Count() - 1;
        for (int i = start; i >= 0 && i < m_Route.Points.Count(); i += m_State.Direction)
        {
            float ahead = (m_MetersToPoint.Get(i) - m_State.DistanceMeters) * m_State.Direction;
            if (m_Route.Points.Get(i).IsStation == 1 && ahead > 0.0001)
            {
                expected = i;
                break;
            }
        }
        bool atTarget = Math.AbsFloat(targetOffset) <= 0.0001;
        if (m_State.Phase == 0 && atTarget && m_State.SpeedMps != 0)
            return false;
        if (!atTarget && target != expected)
            return false;
        Log("Restored state generation " + m_State.Generation.ToString());
        return true;
    }

    protected void ChooseNextStation(int startIndex)
    {
        m_State.NextStopPointIndex = -1;
        for (int i = startIndex; i >= 0 && i < m_Route.Points.Count(); i += m_State.Direction)
        {
            float ahead = (m_MetersToPoint.Get(i) - m_State.DistanceMeters) * m_State.Direction;
            if (m_Route.Points.Get(i).IsStation == 1 && ahead >= -0.0001)
            {
                m_State.NextStopPointIndex = i;
                return;
            }
        }
    }

    protected float TargetDistance()
    {
        if (m_State.NextStopPointIndex >= 0)
            return m_MetersToPoint.Get(m_State.NextStopPointIndex);
        if (m_State.Direction == -1)
            return 0;
        return m_RouteLength;
    }

    protected void Tick()
    {
        if (m_Started != 1)
            return;

        int nowMs = GetGame().GetTime();
        float dt = (nowMs - m_LastTickTimeMs) / 1000.0;
        m_LastTickTimeMs = nowMs;
        if (dt <= 0.0)
            return;
        if (dt > 2.0)
            dt = 2.0; // Slow simulation after a severe stall; never leap the route.
        float pending = dt;
        while (pending > 0 && m_Started == 1)
        {
            float slice = Math.Min(pending, 0.05);
            AdvanceController(slice);
            pending -= slice;
        }

        m_SaveCountdown -= dt;
        if (m_SaveCountdown <= 0.0)
        {
            SaveState();
            m_SaveCountdown = m_Settings.SaveEverySeconds;
        }
    }

    // "Step" is already a vanilla test-framework class (2_GameLib).
    // An unqualified Step(...) call resolved to its constructor in runtime tests.
    protected void AdvanceController(float dt)
    {
        if (!m_State || !RPLimits.InRange(dt, 0.000001, 0.05))
            return;

        if (m_State.Phase == 1)
        {
            m_State.DwellRemainingSeconds -= dt;
            m_State.SpeedMps = 0.0;
            if (m_State.DwellRemainingSeconds <= 0.0)
            {
                m_State.DwellRemainingSeconds = 0.0;
                Depart();
            }
            return;
        }

        float remaining = (TargetDistance() - m_State.DistanceMeters) * m_State.Direction;
        if (remaining <= 0.01)
        {
            Arrive();
            return;
        }

        float cruise = m_Settings.CruiseSpeedKph / 3.6;
        float brakingSpeed = Math.Sqrt(2.0 * m_Settings.ServiceBrakeMps2 * remaining);
        float desired = Math.Min(cruise, brakingSpeed);

        if (m_State.SpeedMps > desired)
            m_State.SpeedMps = Math.Max(0.0, m_State.SpeedMps - m_Settings.ServiceBrakeMps2 * dt);
        else
            m_State.SpeedMps = Math.Min(desired, m_State.SpeedMps + m_Settings.AccelerationMps2 * dt);

        float movement = m_State.SpeedMps * dt;
        if (movement >= remaining)
        {
            m_State.DistanceMeters = TargetDistance();
            Arrive();
        }
        else
        {
            m_State.DistanceMeters += movement * m_State.Direction;
        }
    }

    protected void Depart()
    {
        int served = m_State.NextStopPointIndex;
        int lastIndex = m_Route.Points.Count() - 1;
        if ((served == lastIndex && m_State.Direction == 1) || (served == 0 && m_State.Direction == -1))
        {
            m_State.Direction = -m_State.Direction;
            Log("Terminal dwell complete; returning over the surveyed route.");
        }
        m_State.Phase = 0;
        m_State.DwellRemainingSeconds = 0;
        ChooseNextStation(served + m_State.Direction);
        SaveState();
    }

    protected void Arrive()
    {
        m_State.SpeedMps = 0.0;
        if (m_State.NextStopPointIndex < 0)
        {
            Log("Missing station target; controller stopped for configuration review.");
            Stop();
            return;
        }

        RPWaypoint station = m_Route.Points.Get(m_State.NextStopPointIndex);
        m_State.DistanceMeters = m_MetersToPoint.Get(m_State.NextStopPointIndex);
        m_State.Phase = 1;
        m_State.DwellRemainingSeconds = station.DwellSeconds;
        if (m_State.DwellRemainingSeconds <= 0.0)
            m_State.DwellRemainingSeconds = m_Settings.DefaultDwellSeconds;

        Log("Arrived: " + station.Name + "; dwell " + m_State.DwellRemainingSeconds.ToString() + "s");
        SaveState();
    }

    protected vector PositionAt(float distanceMeters)
    {
        if (distanceMeters <= 0.0)
            return m_Route.Points.Get(0).GetWorldPosition();
        if (distanceMeters >= m_RouteLength)
            return m_Route.Points.Get(m_Route.Points.Count() - 1).GetWorldPosition();

        for (int i = 1; i < m_MetersToPoint.Count(); i++)
        {
            float to = m_MetersToPoint.Get(i);
            if (distanceMeters > to)
                continue;

            float from = m_MetersToPoint.Get(i - 1);
            float t = (distanceMeters - from) / (to - from);
            vector start = m_Route.Points.Get(i - 1).GetWorldPosition();
            vector end = m_Route.Points.Get(i).GetWorldPosition();
            return Vector(start[0] + (end[0] - start[0]) * t, start[1] + (end[1] - start[1]) * t, start[2] + (end[2] - start[2]) * t);
        }
        return m_Route.Points.Get(m_Route.Points.Count() - 1).GetWorldPosition();
    }

    protected bool SaveState()
    {
        if (m_StateStore.Save(m_State))
            return true;
        Log("Snapshot could not be verified; stopping headless controller.");
        if (m_Started == 1)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(Tick);
            m_Started = 0;
            m_Visual.Stop();
        }
        return false;
    }
};

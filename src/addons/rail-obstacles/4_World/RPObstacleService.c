modded class RP_ConvoyUnit
{
    protected float m_ObstacleMotionRate = 1;

    void RP_ConvoyUnit()
    {
        RegisterNetSyncVariableFloat("m_ObstacleMotionRate", 0, 1, 3);
    }

    void RPSetObstacleMotionRate(float rate)
    {
        m_ObstacleMotionRate = rate;
    }

    override void ServerState(float clock, float speed, int leading, int night, bool synchronize = true)
    {
        super.ServerState(clock, speed * m_ObstacleMotionRate, leading, night, synchronize);
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        if (!GetGame().IsServer() && m_ServiceEnabled)
            s_Clock.RPApplyClockRate(m_RouteClock, m_ObstacleMotionRate, m_ServiceLap);
    }
};

modded class RPConvoyEffects
{
    override void Update(Object loco, bool active, bool night, float speed, float cruise, float dt)
    {
        RP_ConvoyUnit unit = RP_ConvoyUnit.Cast(loco);
        if (unit) speed = unit.Speed();
        super.Update(loco, active, night, speed, cruise, dt);
    }
};

modded class RPConvoyService
{
    static const float OBSTACLE_SCAN_INTERVAL = 0.10;
    static const float OBSTACLE_STANDOFF = 3.0;
    static const float OBSTACLE_DECELERATION = 0.85;
    static const float OBSTACLE_CLEAR_SECONDS = 0.5;
    static const float OBSTACLE_TARGET_TOLERANCE = 0.35;
    static const float OBSTACLE_STOP_TOLERANCE = 0.015;

    protected ref RPObstacleSensor m_ObstacleSensor = new RPObstacleSensor;
    protected float m_ObstacleUntilScan;
    protected float m_ObstacleClearSeconds;
    protected float m_ObstacleRate = 1;
    protected float m_ObstaclePosition;
    protected int m_ObstacleDirection;
    protected bool m_ObstacleDetected;
    protected float m_ObstacleTravelSpeed;
    protected float m_ObstacleWallSync;
    protected float m_BrakeOrigin;
    protected float m_BrakeLimit;
    protected float m_BrakeDuration;
    protected float m_BrakeElapsed;
    protected float m_BrakeStopAt;

    // Seed the published and physical speeds for a stationary recovery.
    void RPPrepareRestartHold()
    {
        m_ObstacleRate = 0;
        m_ObstacleTravelSpeed = 0;
        foreach (RP_ConvoyUnit unit : m_Units) unit.RPSetObstacleMotionRate(0);
        Tick(0);
    }

    bool RPObstacleDetected() { return m_ObstacleDetected; }
    float RPObstacleRate() { return m_ObstacleRate; }

    protected float RPBoundObstacleStep(float step, float limit, int direction)
    {
        float distance;
        float speed;
        int branch;
        m_Route.State(m_Clock + step, distance, speed, branch);
        int segment = m_Route.SegmentAt(m_Clock);
        if (m_Route.SegmentAt(m_Clock + step) == segment && branch == direction && (distance - limit) * direction <= 0) return step;
        float low;
        float high = step;
        for (int i = 0; i < 14; i++)
        {
            float middle = (low + high) * 0.5;
            m_Route.State(m_Clock + middle, distance, speed, branch);
            if (m_Route.SegmentAt(m_Clock + middle) == segment && branch == direction && (distance - limit) * direction <= 0) low = middle;
            else high = middle;
        }
        return low;
    }

    protected float RPComfortDistance(float speed)
    {
        // Cubic ease-out velocity has peak deceleration 0.75 * vÂ² / d.
        return 0.75 * speed * speed / OBSTACLE_DECELERATION;
    }

    protected float RPDepartureStep(float dt, float speed)
    {
        if (m_ObstacleRate >= 0.999 || speed < 0.01)
        {
            m_ObstacleTravelSpeed = speed;
            return dt;
        }
        float before = Math.Min(m_ObstacleTravelSpeed, speed);
        if (m_ObstacleRate <= 0) before = 0;
        float after = Math.Min(speed, before + m_Route.Acceleration * dt);
        float ramp = (after - before) / m_Route.Acceleration;
        float travel = (before + after) * 0.5 * ramp + after * (dt - ramp);
        m_ObstacleTravelSpeed = after;
        return Math.Min(dt, travel / speed);
    }

    protected float RPBrakingStep(float dt, float distance, float speed, int direction)
    {
        float remaining = (m_ObstaclePosition - distance) * direction;
        if (remaining < OBSTACLE_STOP_TOLERANCE)
        {
            m_ObstacleTravelSpeed = 0;
            m_BrakeDuration = 0;
            return 0;
        }

        // Keep a continuous profile for fixed obstacles. Replan only when
        // the detected obstruction has meaningfully changed.
        if (m_BrakeDuration > 0 && Math.AbsFloat(m_ObstaclePosition - m_BrakeLimit) > OBSTACLE_TARGET_TOLERANCE)
            m_BrakeDuration = 0;
        float velocity = Math.Min(m_ObstacleTravelSpeed, speed);
        if (m_ObstacleRate >= 0.999) velocity = speed;
        if (m_ObstacleRate <= 0) velocity = 0;
        if (m_BrakeDuration <= 0 && velocity > 0.15 && remaining <= RPComfortDistance(velocity) + velocity * dt + 0.4)
        {
            m_BrakeOrigin = distance;
            m_BrakeLimit = m_ObstaclePosition;
            m_BrakeDuration = 2 * remaining / velocity;
            m_BrakeElapsed = 0;
            // The same easing curve reaches the controller's stop tolerance
            // before its theoretical endpoint. Publish that braking window.
            float low = 0;
            float high = 1;
            for (int i = 0; i < 16; i++)
            {
                float phase = (low + high) * 0.5;
                float rest = 1 - phase;
                float residual = remaining * rest * rest * rest * (1 + phase);
                if (residual > OBSTACLE_STOP_TOLERANCE) low = phase;
                else high = phase;
            }
            m_BrakeStopAt = high * m_BrakeDuration;
            Print("[RailPilotObstacles] PROGRESSIVE_BRAKE speed=" + velocity.ToString() + " distance=" + remaining.ToString());
        }
        if (m_BrakeDuration <= 0)
            return RPBoundObstacleStep(RPDepartureStep(dt, speed), m_ObstaclePosition, direction);

        m_BrakeElapsed += dt;
        float u = Math.Clamp(m_BrakeElapsed / m_BrakeDuration, 0, 1);
        float u2 = u * u;
        float u3 = u2 * u;
        float range = m_BrakeLimit - m_BrakeOrigin;
        float wanted = m_BrakeOrigin + range * (2 * u - 2 * u3 + u3 * u);
        if ((wanted - m_ObstaclePosition) * direction > 0) wanted = m_ObstaclePosition;
        float advance = (wanted - distance) * direction;
        m_ObstacleTravelSpeed = Math.Min(speed, Math.AbsFloat(range) * 2 / m_BrakeDuration * (1 - u) * (1 - u) * (1 + 2 * u));
        if (advance <= 0) return 0;
        return RPBoundObstacleStep(dt, wanted, direction);
    }

    override void Tick(float dt)
    {
        if (!GetGame().IsServer() || dt <= 0 || !m_Route || m_Units.Count() != RPConvoySpec.COUNT)
        {
            super.Tick(dt);
            return;
        }
        foreach (RP_ConvoyUnit member : m_Units)
        {
            if (!member) { super.Tick(0); return; }
        }

        // Large wall-clock gaps cannot turn one tick into a long teleport.
        dt = Math.Min(dt, 0.25);
        float distance;
        float speed;
        int direction;
        m_Route.State(m_Clock, distance, speed, direction);
        m_ObstacleUntilScan -= dt;
        if (m_ObstacleUntilScan <= 0 || direction != m_ObstacleDirection)
        {
            float cruise = m_Route.Speed;
            if (direction < 0) cruise = m_Route.ReverseSpeed;
            float horizon = RPComfortDistance(cruise) + 12;
            float gap = m_ObstacleSensor.Gap(m_Route, m_Units, distance, m_Route.SegmentAt(m_Clock), horizon);
            if (gap >= 0)
            {
                m_ObstaclePosition = distance + Math.Max(gap - OBSTACLE_STANDOFF, 0) * direction;
                m_ObstacleClearSeconds = 0;
                if (!m_ObstacleDetected)
                    Print("[RailPilotObstacles] DETECTED ahead=" + gap.ToString() + " direction=" + direction.ToString());
                m_ObstacleDetected = true;
            }
            else if (m_ObstacleDetected)
            {
                m_ObstacleClearSeconds += OBSTACLE_SCAN_INTERVAL;
                if (m_ObstacleClearSeconds >= OBSTACLE_CLEAR_SECONDS || direction != m_ObstacleDirection)
                {
                    m_ObstacleDetected = false;
                    Print("[RailPilotObstacles] CLEAR");
                }
            }
            m_ObstacleDirection = direction;
            m_ObstacleUntilScan = OBSTACLE_SCAN_INTERVAL;
        }

        float step;
        if (m_ObstacleDetected)
            step = RPBrakingStep(dt, distance, speed, direction);
        else
        {
            m_BrakeDuration = 0;
            step = RPDepartureStep(dt, speed);
        }
        m_ObstacleRate = step / dt;
        foreach (RP_ConvoyUnit unit : m_Units)
        {
            if (!unit) continue;
            unit.RPSetObstacleMotionRate(m_ObstacleRate);
            if (m_BrakeDuration > 0 && m_ObstacleRate > 0)
                unit.RPSetObstacleBrakeWindow(m_BrakeStopAt, Math.Max(dt, m_BrakeStopAt - m_BrakeElapsed + dt));
            else unit.RPSetObstacleBrakeWindow(0, 0);
        }

        // RPConvoyService accumulates route time in m_Sync. Keep the same
        // publication cadence in real seconds when its clock is slowed.
        m_ObstacleWallSync += dt;
        if (m_ObstacleWallSync >= 0.1)
        {
            m_Sync = 0.1;
            m_ObstacleWallSync = 0;
        }
        super.Tick(step);
    }

    override void Stop()
    {
        super.Stop();
        m_ObstacleUntilScan = 0;
        m_ObstacleClearSeconds = 0;
        m_ObstacleRate = 1;
        m_ObstacleDirection = 0;
        m_ObstacleDetected = false;
        m_ObstacleTravelSpeed = 0;
        m_ObstacleWallSync = 0;
        m_BrakeDuration = 0;
    }
};

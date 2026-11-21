// Original crossing cycle; coordinates are track-distance offsets from road.
class RPCrossingLogic
{
    static const float APPROACH = 200;
    static const float HORN = 10;
    static const float ROAD_HALF = 3.5;
    static const float CLEARANCE = 4;
    static const float TRAVEL_SECONDS = 5;
    protected int m_State; // 0 open, 1 closing/closed, 2 opening
    protected int m_Direction = 1;
    protected float m_Phase; // 0 fully up, 1 fully down
    protected bool m_HornFired;
    protected bool m_HornEvent;
    protected int m_Activations;
    protected int m_Horns;
    protected int m_Openings;
    protected int m_Completions;
    protected float m_RoadHalfWidth = ROAD_HALF;
    protected float m_Clearance = CLEARANCE;

    bool ConfigureRoadWidth(float halfWidth)
    {
        if (m_State != 0 || halfWidth < 1 || halfWidth > 20) return false;
        m_RoadHalfWidth = halfWidth;
        m_Clearance = halfWidth + 0.5;
        return true;
    }

    void Update(float minimum, float maximum, float signedSpeed, float dt)
    {
        m_HornEvent = false;
        if (dt <= 0 || minimum > maximum) return;
        int direction = m_Direction;
        if (signedSpeed > 0.2) direction = 1;
        if (signedSpeed < -0.2) direction = -1;
        float lead = maximum;
        float tail = minimum;
        if (direction < 0) { lead = -minimum; tail = -maximum; }
        bool occupied = minimum <= m_RoadHalfWidth && maximum >= -m_RoadHalfWidth;
        bool approaching = Math.AbsFloat(signedSpeed) > 0.2 && lead >= -APPROACH && lead <= m_RoadHalfWidth;
        bool demand = occupied || approaching;
        if (m_State == 0 && demand)
        {
            m_State = 1;
            m_HornFired = false;
            m_Activations++;
        }
        if (m_State == 2 && demand)
        {
            m_State = 1;
            m_HornFired = false;
            m_Activations++;
        }
        if (direction != m_Direction)
        {
            m_Direction = direction;
            m_HornFired = false;
        }
        if (m_State == 1)
        {
            // Remain closed at standstill, including during a reversal while
            // any part of the consist occupies the road. Clear by its tail.
            if (!occupied && (tail > m_Clearance || lead < -APPROACH - 5))
            {
                m_State = 2;
                m_Openings++;
            }
            else if (!m_HornFired && Math.AbsFloat(signedSpeed) > 0.2 && lead >= -HORN && tail <= m_Clearance)
            {
                m_HornFired = true;
                m_HornEvent = true;
                m_Horns++;
            }
        }
        if (m_State == 1) m_Phase = Math.Min(1, m_Phase + dt / TRAVEL_SECONDS);
        if (m_State == 2)
        {
            m_Phase = Math.Max(0, m_Phase - dt / TRAVEL_SECONDS);
            if (m_Phase <= 0)
            {
                m_State = 0;
                m_Completions++;
            }
        }
    }

    bool Alarm() { return m_State != 0; }
    bool HornEvent() { return m_HornEvent; }
    float Phase() { return m_Phase; }
    int State() { return m_State; }
    int Activations() { return m_Activations; }
    int Horns() { return m_Horns; }
    int Openings() { return m_Openings; }
    int Completions() { return m_Completions; }
};

class RPCrossingFootprint
{
    static bool Project(array<Object> bodies, vector origin, vector axis, out float minimum, out float maximum)
    {
        if (!bodies || bodies.Count() == 0) return false;
        minimum = 1000000; maximum = -1000000;
        for (int i = 0; i < bodies.Count(); i++)
        {
            Object body = bodies.Get(i);
            vector bounds[2];
            if (!body || body.ToDelete() || !body.GetCollisionBox(bounds)) return false;
            // All corners, including rotation/tilt; never just locomotive centre.
            for (int x = 0; x < 2; x++)
            for (int y = 0; y < 2; y++)
            for (int z = 0; z < 2; z++)
            {
                vector boundX = bounds[x];
                vector boundY = bounds[y];
                vector boundZ = bounds[z];
                vector corner = Vector(boundX[0],boundY[1],boundZ[2]);
                float along = vector.Dot(body.ModelToWorld(corner) - origin,axis);
                minimum = Math.Min(minimum,along); maximum = Math.Max(maximum,along);
            }
        }
        return true;
    }
};

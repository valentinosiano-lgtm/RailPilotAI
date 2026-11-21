// Original RailPilot code. No Triumph source/classes are replaced or copied.
// Experimental: only the three documented convoy types with an AI driver.
class RPTrafficSignal
{
    string Id;
    vector Center;
    vector RoadAxis;
    float StopRadius = 12;
    float CorridorHalfWidth = 8;
    bool Blocked;

    float SignedAlong(vector position)
    {
        return vector.Dot(position - Center, RoadAxis);
    }

    bool InCorridor(vector position)
    {
        vector delta = position - Center;
        if (Math.AbsFloat(delta[1]) > 6) return false;
        delta[1] = 0;
        vector normal = Vector(-RoadAxis[2], 0, RoadAxis[0]);
        return Math.AbsFloat(vector.Dot(delta, normal)) <= CorridorHalfWidth;
    }
};

class RPTrafficRegistry
{
    static ref array<ref RPTrafficSignal> Signals = new array<ref RPTrafficSignal>;

    static void Add(RPTrafficSignal signal)
    {
        if (Signals.Find(signal) < 0) Signals.Insert(signal);
    }

    static void Remove(RPTrafficSignal signal)
    {
        int index = Signals.Find(signal);
        if (index >= 0) Signals.RemoveOrdered(index);
    }

    static bool ConvoyType(string kind)
    {
        return kind == "AIConvoy_Car" || kind == "AIConvoy_M1025Desert" || kind == "AIConvoy_Truck";
    }

    static float BrakingHorizon(float speed)
    {
        // Conservative starting assumption, NOT a measured vehicle capability.
        // Actual stopping margin must be observed with each original vehicle.
        return Math.Clamp(speed * speed / 5.0 + speed * 0.5 + 6, 8, 140);
    }

    static vector RoadAxis(RPFirstCrossingSite site)
    {
        if (site.RoadGateRows && site.RoadGateRows.Count() > 0)
        {
            vector road = vector.Zero;
            foreach (RPFirstRoadGateRow row : site.RoadGateRows)
            {
                vector across = row.FarPost - row.NearPost;
                vector rowHeading = Vector(across[2], 0, -across[0]);
                rowHeading.Normalize();
                if (road.LengthSq() > 0.1 && vector.Dot(road, rowHeading) < 0) rowHeading = -rowHeading;
                road = road + rowHeading;
            }
            road.Normalize();
            return road;
        }
        // Opposite barrier posts are staggered across the road. Their joining
        // diagonal is NOT the road heading, especially at the four-arm site.
        // The approved native barrier poses orient local Z along the road.
        vector sum = vector.Zero;
        foreach (vector orientation: site.BarrierOrientations)
        {
            float yaw = orientation[0] * Math.DEG2RAD;
            vector heading = Vector(Math.Sin(yaw), 0, Math.Cos(yaw));
            if (sum.LengthSq() > 0.1 && vector.Dot(sum, heading) < 0) heading = -heading;
            sum = sum + heading;
        }
        sum.Normalize();
        return sum;
    }
};

modded class RPFirstCrossing
{
    protected ref RPTrafficSignal m_RPTrafficSignal;

    override void Apply()
    {
        super.Apply();
        if (!GetGame().IsServer() || !m_Site || !m_Logic) return;
        if (!m_RPTrafficSignal)
        {
            m_RPTrafficSignal = new RPTrafficSignal;
            m_RPTrafficSignal.Id = m_Site.Name;
            m_RPTrafficSignal.Center = m_Site.Pivot;
            vector axis = RPTrafficRegistry.RoadAxis(m_Site);
            m_RPTrafficSignal.RoadAxis = axis;
            float extent;
            float halfWidth;
            if (m_Site.RoadGateRows && m_Site.RoadGateRows.Count() > 0)
            {
                vector across = Vector(-axis[2], 0, axis[0]);
                foreach (RPFirstRoadGateRow row : m_Site.RoadGateRows)
                {
                    extent = Math.Max(extent, Math.AbsFloat(vector.Dot(row.NearPost - m_Site.Pivot, axis)));
                    extent = Math.Max(extent, Math.AbsFloat(vector.Dot(row.FarPost - m_Site.Pivot, axis)));
                    halfWidth = Math.Max(halfWidth, Math.AbsFloat(vector.Dot(row.NearPost - m_Site.Pivot, across)));
                    halfWidth = Math.Max(halfWidth, Math.AbsFloat(vector.Dot(row.FarPost - m_Site.Pivot, across)));
                }
            }
            else
            {
                foreach (vector pole: m_Site.BarrierPositions)
                    extent = Math.Max(extent, Math.AbsFloat(vector.Dot(pole - m_Site.Pivot, axis)));
                halfWidth = m_Site.RoadHalfWidth;
            }
            m_RPTrafficSignal.StopRadius = extent + 2;
            m_RPTrafficSignal.CorridorHalfWidth = halfWidth + 3;
            RPTrafficRegistry.Add(m_RPTrafficSignal);
        }
        m_RPTrafficSignal.Blocked = m_Logic.State() != 0 || m_Logic.Phase() > 0.0005;
        if (!m_A || !m_B) m_RPTrafficSignal.Blocked = true;
    }

    override void Release()
    {
        if (m_RPTrafficSignal) RPTrafficRegistry.Remove(m_RPTrafficSignal);
        m_RPTrafficSignal = null;
        super.Release();
    }
};

modded class CarScript
{
    protected ref RPTrafficSignal m_RPTrafficHeld;
    protected float m_RPTrafficApproachSign;
    protected bool m_RPTrafficOwnBrake;
    protected bool m_RPTrafficOwnHandbrake;
    protected float m_RPTrafficPreviousBrake;
    protected float m_RPTrafficPreviousHandbrake;
    int RPTrafficInputCallbacks;
    int RPTrafficControllerSuppressed;
    int RPTrafficRecoveryResets;
    int RPTrafficApplied;
    int RPTrafficReleased;
    float RPTrafficInputThrottle;
    float RPTrafficInputBrake;

    bool RPTrafficEligible()
    {
        if (!RPTrafficRegistry.ConvoyType(GetType())) return false;
        PlayerBase driver = PlayerBase.Cast(CrewDriver());
        return driver && driver.IsAlive() && driver.IsAI() && !IsDamageDestroyed() && RPTrafficRecovery.FindActiveUnit(this) != null;
    }

    float RPTrafficHalfLength()
    {
        vector bounds[2];
        if (!GetCollisionBox(bounds)) return 6;
        vector low = bounds[0]; vector high = bounds[1];
        return Math.Max(Math.AbsFloat(low[2]), Math.AbsFloat(high[2]));
    }

    RPTrafficSignal RPTrafficHeldSignal() { return m_RPTrafficHeld; }

    RPTrafficSignal RPTrafficQueueSignal(vector position, vector heading)
    {
        // The original follower reference was identified by runtime metadata.
        // Queued vehicles may stop far beyond their own speed-based horizon.
        CarScript front = m_AIConvoy_FollowVehicle;
        if (!front || front == this || !front.RPTrafficEligible()) return null;
        RPTrafficSignal signal = front.RPTrafficHeldSignal();
        if (!signal || !signal.Blocked || RPTrafficRegistry.Signals.Find(signal) < 0) return null;
        if (!signal.InCorridor(position)) return null;
        float along = signal.SignedAlong(position);
        float frontAlong = signal.SignedAlong(front.GetPosition());
        if (along * frontAlong <= 0 || Math.AbsFloat(along) < Math.AbsFloat(frontAlong)) return null;
        float sign = 1;
        if (along < 0) sign = -1;
        if (vector.Dot(heading, signal.RoadAxis) * sign > -0.6) return null;
        if (Math.AbsFloat(along) < signal.StopRadius + RPTrafficHalfLength()) return null;
        return signal;
    }

    void RPTrafficRelease()
    {
        bool ordinaryDriving = RPTrafficEligible();
        if (m_RPTrafficHeld && ordinaryDriving)
        {
            if (RPTrafficRecovery.ResetFor(this)) RPTrafficRecoveryResets++;
        }
        // Restore only values still matching our last injection. Never command
        // throttle on release: the original controller owns the next departure.
        // After ambush, destruction or driver change, the new controller owns
        // the inputs. Do not clear a brake it may have deliberately applied.
        if (ordinaryDriving)
        {
            if (m_RPTrafficOwnBrake && GetBrake() >= 0.999) SetBrake(m_RPTrafficPreviousBrake);
            if (m_RPTrafficOwnHandbrake && GetHandbrake() >= 0.999) SetHandbrake(m_RPTrafficPreviousHandbrake);
        }
        if (m_RPTrafficHeld)
        {
            RPTrafficReleased++;
            Print("[RailPilotTraffic] RELEASE " + GetType() + " signal=" + m_RPTrafficHeld.Id);
        }
        m_RPTrafficOwnBrake = false;
        m_RPTrafficOwnHandbrake = false;
        m_RPTrafficHeld = null;
    }

    void RPTrafficUpdate()
    {
        if (!RPTrafficEligible()) { RPTrafficRelease(); return; }
        vector position = GetPosition();
        float speed = GetVelocity(this).Length();
        if (m_RPTrafficHeld)
        {
            bool registered = RPTrafficRegistry.Signals.Find(m_RPTrafficHeld) >= 0;
            float alongHeld = m_RPTrafficHeld.SignedAlong(position) * m_RPTrafficApproachSign;
            // A vehicle that has crossed the stop line must clear the rails.
            float line = m_RPTrafficHeld.StopRadius + RPTrafficHalfLength();
            if (!registered || !m_RPTrafficHeld.Blocked || !m_RPTrafficHeld.InCorridor(position) || alongHeld < line - 1)
                RPTrafficRelease();
        }
        if (!m_RPTrafficHeld)
        {
            float bestGap = 100000;
            vector heading = GetDirection(); heading[1] = 0; heading.Normalize();
            RPTrafficSignal queued = RPTrafficQueueSignal(position, heading);
            if (queued)
            {
                m_RPTrafficHeld = queued;
                m_RPTrafficApproachSign = 1;
                if (queued.SignedAlong(position) < 0) m_RPTrafficApproachSign = -1;
                bestGap = Math.AbsFloat(queued.SignedAlong(position)) - queued.StopRadius - RPTrafficHalfLength();
            }
            foreach (RPTrafficSignal signal: RPTrafficRegistry.Signals)
            {
                if (queued) break;
                if (!signal || !signal.Blocked || !signal.InCorridor(position)) continue;
                float along = signal.SignedAlong(position);
                float sign = 1;
                if (along < 0) sign = -1;
                if (vector.Dot(heading, signal.RoadAxis) * sign > -0.6) continue;
                float gap = Math.AbsFloat(along) - signal.StopRadius - RPTrafficHalfLength();
                if (gap < 0 || gap > RPTrafficRegistry.BrakingHorizon(speed) || gap >= bestGap) continue;
                m_RPTrafficHeld = signal; m_RPTrafficApproachSign = sign; bestGap = gap;
            }
            if (m_RPTrafficHeld)
            {
                m_RPTrafficPreviousBrake = GetBrake();
                m_RPTrafficPreviousHandbrake = GetHandbrake();
                Print("[RailPilotTraffic] HOLD " + GetType() + " signal=" + m_RPTrafficHeld.Id + " gap=" + bestGap.ToString());
            }
        }
        if (!m_RPTrafficHeld) return;
        if (RPTrafficRecovery.ResetFor(this)) RPTrafficRecoveryResets++;
        RPTrafficInputThrottle = GetThrottle(); RPTrafficInputBrake = GetBrake();
        SetThrottle(0);
        SetBrake(1.0); m_RPTrafficOwnBrake = true;
        if (speed < 0.4) { SetHandbrake(1.0); m_RPTrafficOwnHandbrake = true; }
        RPTrafficApplied++;
    }

    // Preserve ordinary controller updates, then impose traffic inputs and
    // refresh only this actively held unit's observed recovery counters.
    override void OnInput(float dt)
    {
        super.OnInput(dt);
        if (GetGame().IsServer())
        {
            RPTrafficInputCallbacks++;
            RPTrafficUpdate();
        }
    }
};

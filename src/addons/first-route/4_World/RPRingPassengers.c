// Retain the passenger manifest until each living character has returned.
class RPRingPassengers : RPFirstPassengers
{
    protected ref array<string> m_Returning = new array<string>;
    protected float m_RecoveryClock;

    void BeginRecovery(float clock)
    {
        m_RecoveryClock = clock;
        foreach (RPFirstPassengerState entry : m_Entries) m_Returning.Insert(entry.PlayerId);
    }

    override void Forget(string id)
    {
        m_Returning.RemoveItem(id);
        super.Forget(id);
    }

    override void Observe(string id, PlayerBase player)
    {
        // A character can be present in the world before its login completes.
        if (m_Returning.Find(id) >= 0) return;
        super.Observe(id, player);
    }

    protected bool GroundClear(vector point, float railHeight, float heightLimit, array<Object> excluded, array<Man> players, PlayerBase returning)
    {
        if (Math.AbsFloat(point[1] - railHeight) > heightLimit) return false;
        if (GetGame().SurfaceIsSea(point[0], point[2]) || GetGame().SurfaceIsPond(point[0], point[2])) return false;
        vector normal = GetGame().SurfaceGetNormal(point[0], point[2]);
        if (normal[1] < 0.82) return false;
        foreach (Man other : players)
        {
            if (other && other != returning && other.IsAlive() && vector.Distance(other.GetPosition(), point) < 1.05)
                return false;
        }
        return !GetGame().IsBoxCollidingGeometry(point + "0 0.95 0", "0 0 0", "0.85 1.8 0.85",
            ObjIntersectView, ObjIntersectGeom, excluded);
    }

    protected bool FindGround(RPFirstPassengerState entry, PlayerBase player, out vector ground)
    {
        array<Object> excluded = new array<Object>;
        excluded.Insert(player);
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        // Prefer the passenger's own wagon and the closest usable verge.
        // Try the full consist before widening the search for busy platforms or bridges.
        array<float> gaps = {1.5, 2.5, 3.75, 5.5, 8, 12, 18, 26};
        array<int> offsets = {0, -1, 1, -2, 2, -3, 3, -4, 4};
        array<float> longitudinal = {0, -2, 2, -4, 4, -6, 6};
        foreach (float gap : gaps)
        {
            foreach (int offset : offsets)
            {
                RP_ConvoyUnit unit = m_Service.Unit(entry.Slot + offset);
                if (!unit) continue;
                vector pose[4];
                RPFirstAlignment.PoseAtClock(m_Service.Route(), unit.Slot(), m_RecoveryClock, unit.Offset(), pose);
                vector low = unit.Low();
                vector high = unit.High();
                float center;
                if (offset == 0) center = entry.LocalPosition[2];
                float heightLimit = 4;
                if (gap >= 12) heightLimit = 8;
                foreach (float shift : longitudinal)
                {
                    float along = Math.Clamp(center + shift, low[2], high[2]);
                    for (int side = -1; side <= 1; side += 2)
                    {
                        float lateral = low[0] - gap;
                        if (side > 0) lateral = high[0] + gap;
                        vector point = pose[3] + pose[0] * lateral + pose[2] * along;
                        float railHeight = point[1];
                        point[1] = GetGame().SurfaceRoadY(point[0], point[2]) + 0.12;
                        if (GroundClear(point, railHeight, heightLimit, excluded, players, player))
                        {
                            ground = point;
                            return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    override bool Restore(string id, PlayerBase player, bool requestClient = false)
    {
        if (m_Returning.Find(id) < 0) return super.Restore(id, player, requestClient);
        RPFirstPassengerState entry = Find(id);
        if (!ValidEntry(entry) || !player || !player.IsAlive()) return false;
        vector ground;
        if (!FindGround(entry, player, ground))
        {
            Print("[RailPilotRing] PASSENGER ground unavailable near the recovery point; claim retained");
            return false;
        }
        if (player.GetParent() || player.PhysicsGetLinkedEntity()) player.UnlinkFromLocalSpace();
        player.SetPosition(ground);
        PrintFormat("[RailPilotRing] PASSENGER ground slot=%1 position=%2", entry.Slot, ground);
        Forget(id);
        return true;
    }
};

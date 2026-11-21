modded class RPFirstCrossing
{
    protected Land_rp_crossing_barrier MakeRoadGate(vector position, float yaw, bool longArm)
    {
        position[1] = GetGame().SurfaceY(position[0], position[2]) + 0.06;
        string type = "Land_rp_crossing_barrier";
        if (longArm) type = "Land_rp_full_crossing_barrier";
        Land_rp_crossing_barrier gate = Land_rp_crossing_barrier.Cast(GetGame().CreateObjectEx(type, position, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
        if (gate)
        {
            gate.SetOrientation(Vector(yaw, 0, 0));
            gate.SetAllowDamage(false);
        }
        return gate;
    }

    override bool Configure(RPConvoyService service, RPFirstCrossingSite site, bool initial, bool offline)
    {
        if (!site.RoadGateRows || site.RoadGateRows.Count() == 0) return super.Configure(service, site, initial, offline);

        m_Service = service;
        m_Site = site;
        m_Initial = initial && site.PredepartureWarningSeconds <= 0;
        m_Offline = offline;
        m_Logic = new RPFirstCrossingLogic;
        if (!m_Logic.ConfigureRoadWidth(site.RoadHalfWidth)) return false;
        if (m_Initial) m_Logic.InitiallyClosed();
        m_FullRoadGates = new array<Land_rp_crossing_barrier>;
        for (int i = 0; i < site.RoadGateRows.Count(); i++)
        {
            RPFirstRoadGateRow row = site.RoadGateRows[i];
            Land_rp_crossing_barrier primary = MakeRoadGate(row.NearPost, row.Yaw, row.LongNearArm);
            if (!primary) { Release(); return false; }
            if (i == 0) m_A = primary; else m_B = primary;
            if (site.BarrierPositions.Count() == 2)
            {
                primary.UseOriginalRoadPosition(site.BarrierPositions[i]);
                primary.HideVanillaOriginal();
                if (!primary.OriginalBarrier()) { Release(); return false; }
            }
            if (row.SingleArm) continue;
            Land_rp_crossing_barrier other = MakeRoadGate(row.FarPost, row.Yaw + 180, row.LongFarArm);
            if (!other) { Release(); return false; }
            m_FullRoadGates.Insert(other);
        }
        Apply();
        return true;
    }

    override void Update(float dt)
    {
        if (m_Site.PredepartureWarningSeconds > 0)
        {
            RPConvoyRoute route = m_Service.Route();
            float departure = route.DepartureAt(Math.Max(0, m_Site.Segment - 1));
            float clock = m_Service.Clock();
            if (clock >= departure - m_Site.PredepartureWarningSeconds && clock < departure)
            {
                // Awaiting the scheduled departure just beside this road.
                // Give the normal audible warning and closing cycle its full time.
                float minimum; float maximum;
                Interval(minimum, maximum);
                m_Logic.Update(minimum, maximum, route.Speed, dt);
                Apply();
                return;
            }
        }
        super.Update(dt);
    }
};

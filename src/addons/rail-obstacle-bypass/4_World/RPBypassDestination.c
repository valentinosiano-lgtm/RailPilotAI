class RPBypassDestination : RPObstacleSensor
{
    static const float NOSE_BEYOND = 100.0;
    static const float MAX_DISTANCE = 230.0;
    static const float MAX_EVENT_DISTANCE = 300.0;
    static const float RUNWAY = 5.0;
    static const float ROAD_CLEARANCE = 2.0;
    static const float GATE_APPROACH_SECONDS = 20.0;

    protected ref array<RPCCBox> m_Blockers = new array<RPCCBox>;
    protected ref array<ref RPCCBox> m_Landing = new array<ref RPCCBox>;
    protected ref array<Man> m_Players = new array<Man>;

    static float LegEndClock(RPConvoyRoute route, float clock)
    {
        int south = route.MovingSouthCoastSegment(clock);
        if (south >= RPConvoyRoute.FIRST_SOUTH_COAST_STOP) return route.ArrivalAt(south);
        if (route.HasBerezino() && clock >= route.DepartureAt(7) && clock < route.ArrivalAt(8)) return route.ArrivalAt(8);
        if (route.HasSvetloyarsk() && clock >= route.DepartureAt(6) && clock < route.ArrivalAt(7)) return route.ArrivalAt(7);
        if (route.HasNovodmitrovsk() && clock >= route.DepartureAt(5) && clock < route.ArrivalAt(6)) return route.ArrivalAt(6);
        if (route.HasSeverograd() && clock >= route.DepartureAt(4) && clock < route.ArrivalAt(5)) return route.ArrivalAt(5);
        if (route.HasNovaya() && clock >= route.DepartureAt(3) && clock < route.ArrivalAt(4)) return route.ArrivalAt(4);
        if (route.HasWest())
        {
            if (!route.WestDirect && clock >= route.YardReverseAt() && clock < route.ArrivalAt(3)) return route.ArrivalAt(3);
            if (clock >= route.DepartureAt(2) && clock < route.WestRunEndAt()) return route.WestRunEndAt();
        }
        if (clock >= route.ReversalAt() && clock < route.ArrivalAt(2)) return route.ArrivalAt(2);
        if (clock >= route.DepartureAt(1) && clock < route.ManoeuvreArrivalAt()) return route.ManoeuvreArrivalAt();
        if (clock >= route.DepartureAt(0) && clock < route.ArrivalAt(1)) return route.ArrivalAt(1);
        return -1;
    }

    static float DwellDeparture(RPConvoyRoute route, float clock)
    {
        if (route.HasSouthCoast())
            for (int s = 8; s < route.Stops.Count() - 1; s++)
                if (clock >= route.ArrivalAt(s) && clock < route.DepartureAt(s)) return route.DepartureAt(s);
        if (route.HasBerezino() && clock >= route.ArrivalAt(7) && clock < route.DepartureAt(7)) return route.DepartureAt(7);
        if (route.HasSvetloyarsk() && clock >= route.ArrivalAt(6) && clock < route.DepartureAt(6)) return route.DepartureAt(6);
        if (route.HasNovodmitrovsk() && clock >= route.ArrivalAt(5) && clock < route.DepartureAt(5)) return route.DepartureAt(5);
        if (route.HasSeverograd() && clock >= route.ArrivalAt(4) && clock < route.DepartureAt(4)) return route.DepartureAt(4);
        if (route.HasNovaya() && clock >= route.ArrivalAt(3) && clock < route.DepartureAt(3)) return route.DepartureAt(3);
        if (route.HasWest())
        {
            if (clock >= route.ArrivalAt(2) && clock < route.DepartureAt(2)) return route.DepartureAt(2);
            if (!route.WestDirect && clock >= route.WestTurnAt() && clock < route.YardReverseAt()) return route.YardReverseAt();
        }
        if (clock < route.DepartureAt(0)) return route.DepartureAt(0);
        if (clock >= route.ArrivalAt(1) && clock < route.DepartureAt(1)) return route.DepartureAt(1);
        if (clock >= route.ManoeuvreArrivalAt() && clock < route.ReversalAt()) return route.ReversalAt();
        return -1;
    }

    protected RPCCBox UnitBox(RPConvoyRoute route, RP_ConvoyUnit unit, float distance, int segment, float margin)
    {
        vector pose[4];
        RPFirstAlignment.BuildSegmentPose(route, unit.Slot(), distance - unit.Offset(), segment, pose);
        vector padding = Vector(margin, margin, margin);
        return RPCorridor.MakeBox(unit.Low() - padding, unit.High() + padding, pose);
    }

    protected void FindBlockers(RPConvoyRoute route, array<RP_ConvoyUnit> units, float start, int direction, float available, int segment)
    {
        // A genuine stop is about 3 m before the first contact. Match only
        // boxes that still block that immediate part of the route.
        m_Blockers.Clear();
        float near = Math.Min(5.0, available);
        for (float advance = 0; advance <= near; advance += SAMPLE_SPACING)
        {
            foreach (RP_ConvoyUnit unit : units)
            {
                RPCCBox train = UnitBox(route, unit, start + direction * advance, segment, SWEEP_MARGIN);
                foreach (RPCCBox obstacle : m_Obstacles)
                {
                    if (m_Blockers.Find(obstacle) < 0 && train.Intersects(obstacle))
                        m_Blockers.Insert(obstacle);
                }
            }
        }
    }

    protected float LastContact(RPConvoyRoute route, array<RP_ConvoyUnit> units, float start, int direction, float limit, int segment)
    {
        // Search backwards for the final contact with the original blockage.
        // This accounts for gaps between wagons and either end leading.
        for (float advance = limit; advance >= 0; advance -= SAMPLE_SPACING)
        {
            foreach (RP_ConvoyUnit unit : units)
            {
                RPCCBox train = UnitBox(route, unit, start + direction * advance, segment, SWEEP_MARGIN);
                foreach (RPCCBox obstacle : m_Blockers)
                {
                    if (train.Intersects(obstacle)) return advance;
                }
            }
        }
        return -1;
    }

    protected void AddLandingBody(Object object, bool includeRailStock = false)
    {
        if (!object || RP_ConvoyUnit.Cast(object)) return;
        if (includeRailStock)
        {
            string model = object.GetShapeName();
            model.ToLower();
            if (model.Contains("train_wagon") || model.Contains("train_742") || model.Contains("wreck_train"))
            {
                vector stockBounds[2];
                vector stockPose[4];
                if (object.GetCollisionBox(stockBounds))
                {
                    object.GetTransform(stockPose);
                    m_Landing.Insert(RPCorridor.MakeBox(stockBounds[0], stockBounds[1], stockPose));
                }
                return;
            }
        }
        EntityAI entity = EntityAI.Cast(object);
        if (!entity || entity.GetHierarchyParent() || entity.IsHologram()) return;
        if (!Man.Cast(object) && !object.IsDayZCreature() && !Transport.Cast(object) && !ItemBase.Cast(object)) return;
        // Built-part boxes are already in the base sensor's candidate list.
        if (BaseBuildingBase.Cast(object)) return;
        vector bounds[2];
        vector pose[4];
        object.GetTransform(pose);
        if (object.IsMan() || object.IsDayZCreature())
        {
            // Native character bounds can disappear mid-animation. Reserve
            // space around their actual world position as well.
            PlayerBase player = PlayerBase.Cast(object);
            if (player) pose[3] = player.PhysicsGetPositionWS();
            m_Landing.Insert(RPCorridor.MakeBox("-0.45 -0.2 -0.45", "0.45 2 0.45", pose));
            return;
        }
        if (object.GetCollisionBox(bounds))
            m_Landing.Insert(RPCorridor.MakeBox(bounds[0], bounds[1], pose));
    }

    protected bool LandingClear(RPConvoyRoute route, array<RP_ConvoyUnit> units, float distance, int direction, int segment, bool includeRailStock = false)
    {
        m_Landing.Clear();
        foreach (Object object : m_Objects) AddLandingBody(object, includeRailStock);
        m_Players.Clear();
        GetGame().GetPlayers(m_Players);
        foreach (Man person : m_Players)
        {
            if (m_Objects.Find(person) < 0) AddLandingBody(person);
        }
        for (float advance = 0; advance <= RUNWAY; advance += SAMPLE_SPACING)
        {
            foreach (RP_ConvoyUnit unit : units)
            {
                RPCCBox train = UnitBox(route, unit, distance + direction * advance, segment, 0.5);
                foreach (RPCCBox obstacle : m_Obstacles)
                    if (train.Intersects(obstacle)) return false;
                foreach (RPCCBox body : m_Landing)
                    if (train.Intersects(body)) return false;
            }
        }
        return true;
    }

    static bool RoadsClear(RPConvoyRoute route, float back, float front, int direction, int segment = 0)
    {
        if (direction < 0) return true;
        foreach (RPFirstCrossingSite site : route.Crossings)
        {
            if (site.Segment != segment) continue;
            float near = site.Distance - site.RoadHalfWidth - ROAD_CLEARANCE;
            float far = site.Distance + site.RoadHalfWidth + ROAD_CLEARANCE;
            if (back <= far && front + RUNWAY >= near) return false;
            // Preserve room for the normal prewarning/arm movement when
            // arriving before a road, regardless of its current gate state.
            if (front < near && near - front < route.Speed * GATE_APPROACH_SECONDS) return false;
        }
        return true;
    }

    float FindClock(RPConvoyRoute route, array<RP_ConvoyUnit> units, float clock, float length, out string reason)
    {
        float endClock = LegEndClock(route, clock);
        reason = "scheduled stop or manoeuvre";
        if (endClock < 0) return -1;
        float start;
        float speed;
        int direction;
        route.State(clock, start, speed, direction);
        float end;
        float endSpeed;
        int endDirection;
        route.State(endClock, end, endSpeed, endDirection);
        if (route.Cyclic && endClock == route.CycleDuration()) end = route.Stops[route.Stops.Count() - 1].Distance;
        float available = (end - start) * direction;
        float offsetBeyondLastContact = NOSE_BEYOND - length + SAMPLE_SPACING;
        // Reserve enough room to stand fully beyond the original blockage
        // while staying in the current leg and leaving a short clear runway.
        int segment = route.SegmentAt(clock);
        float searchDistance = MAX_DISTANCE;
        if (segment >= 4) searchDistance = MAX_EVENT_DISTANCE;
        float limit = Math.Min(MAX_DISTANCE, available) - offsetBeyondLastContact - RUNWAY;
        if (limit <= 5 || offsetBeyondLastContact < 1) return -1;
        // Most of the site sweep can be rejected before collecting geometry.
        vector center = route.SegmentPoint(start - units[5].Offset() * 0.5, segment);
        Collect(center, units[5].Offset() * 0.5 + Math.Min(searchDistance, available) + 24, segment >= 4);
        FindBlockers(route, units, start, direction, available, segment);
        reason = "immediate obstruction changed";
        if (m_Blockers.Count() == 0) return -1;
        bool eventBlock;
        foreach (RPCCBox blocker : m_Blockers)
            if (m_EventStock.Find(blocker) >= 0) eventBlock = true;
        if (eventBlock)
        {
            // A train event can place several consecutive wagons on the rail.
            foreach (RPCCBox eventBody : m_EventStock)
                if (m_Blockers.Find(eventBody) < 0) m_Blockers.Insert(eventBody);
            limit = Math.Min(MAX_EVENT_DISTANCE, available) - offsetBeyondLastContact - RUNWAY;
        }
        float contact = LastContact(route, units, start, direction, limit, segment);
        reason = "obstruction too large or next stop too close";
        if (contact < 0 || contact >= limit - SAMPLE_SPACING) return -1;
        float destination = start + (contact + offsetBeyondLastContact) * direction;
        float rear = destination - units[5].Offset() - units[5].BackExtent();
        float front = destination + units[0].FrontExtent();
        reason = "road crossing clearance";
        if (!RoadsClear(route, rear, front, direction, segment)) return -1;
        reason = "occupied destination";
        if (!LandingClear(route, units, destination, direction, segment)) return -1;
        // Invert the monotonic route clock within this leg, using the same
        // route-state function that positions every unit and the sbarre.
        reason = "";
        return ClockAtDistance(route, clock, endClock, destination, direction);
    }

    protected float ClockAtDistance(RPConvoyRoute route, float low, float endClock, float destination, int direction)
    {
        float high = endClock;
        for (int i = 0; i < 25; i++)
        {
            float mid = (low + high) * 0.5;
            float at;
            float sampleSpeed;
            int sampleDirection;
            route.State(mid, at, sampleSpeed, sampleDirection);
            if ((at - destination) * direction < 0) low = mid;
            else high = mid;
        }
        return high;
    }

    float FindStationClock(RPConvoyRoute route, array<RP_ConvoyUnit> units, int station, float clock, float length, inout float cursor, out string reason)
    {
        reason = "no surveyed onward leg";
        int last = route.Stops.Count() - 1;
        if (station < 0 || station > last || (station == last && !route.Cyclic)) return -1;
        float depart = route.DepartureAt(station);
        float endClock;
        if (station == last) endClock = route.CycleDuration() + route.ArrivalAt(1);
        else endClock = LegEndClock(route, depart);
        if (endClock <= depart || clock >= endClock) return -1;

        float start;
        float speed;
        int direction;
        route.State(depart, start, speed, direction);
        float end;
        float endSpeed;
        int endDirection;
        route.State(endClock, end, endSpeed, endDirection);
        float from = length + SAMPLE_SPACING;
        if (clock > depart)
        {
            // On the first departure the train may already be moving out of Komarovo.
            float current;
            float currentSpeed;
            int currentDirection;
            route.State(clock, current, currentSpeed, currentDirection);
            from += (current - start) * direction;
        }
        float limit = (end - start) * direction - RUNWAY;
        reason = "no clear position on the onward leg";
        if (limit < from) return -1;
        cursor = Math.Max(cursor, from);
        int segment = route.SegmentAt(depart);
        // Check successive positions in bounded batches so a long blockage
        // cannot monopolize the server tick.
        for (int attempt = 0; attempt < 24 && cursor <= limit; attempt++)
        {
            float destination = start + cursor * direction;
            cursor += 1;
            float rear = destination - units[5].Offset() - units[5].BackExtent();
            float front = destination + units[0].FrontExtent();
            if (!RoadsClear(route, rear, front, direction, segment)) continue;
            vector center = route.SegmentPoint(destination - units[5].Offset() * 0.5, segment);
            Collect(center, length * 0.5 + RUNWAY + 24, segment >= 4);
            if (!LandingClear(route, units, destination, direction, segment, true)) continue;
            reason = "";
            return ClockAtDistance(route, depart, endClock, destination, direction);
        }
        if (cursor > limit) cursor = 0;
        return -1;
    }
};

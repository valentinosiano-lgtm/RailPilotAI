// First finite itinerary. Geometry/Pose follows the preserved convoy code.
class RPConvoyRail
{
    string Model;
    vector Position;
    vector Orientation;
};
class RPFirstStop
{
    string Name;
    float Distance;
    float Dwell;
};
class RPFirstRoadGateRow
{
    vector NearPost;
    vector FarPost;
    float Yaw;
    bool LongNearArm;
    bool LongFarArm;
    bool SingleArm;
};
class RPFirstCrossingSite
{
    string Name;
    int Segment;
    float Distance;
    vector Pivot;
    vector TrackAxis;
    ref array<vector> BarrierPositions;
    ref array<vector> BarrierOrientations;
    ref array<ref RPFirstRoadGateRow> RoadGateRows;
    float RoadHalfWidth;
    float PresentationAllowanceSeconds;
    int ReturnSegment;
    float ReturnDistance;
    float PredepartureWarningSeconds;
};
class RPFirstGeometry
{
    float Length;
    ref array<float> Distances;
    ref array<vector> Points;
    ref array<vector> Tangents;

    bool ValidGeometry()
    {
        if (!Points || !Distances || !Tangents || Points.Count() < 4) return false;
        if (Points.Count() != Distances.Count() || Points.Count() != Tangents.Count()) return false;
        if (Distances[0] != 0 || Math.AbsFloat(Distances[Distances.Count() - 1] - Length) > 0.01) return false;
        for (int i = 1; i < Distances.Count(); i++) if (Distances[i] <= Distances[i - 1]) return false;
        return true;
    }
    vector At(float distance)
    {
        distance = Math.Clamp(distance, 0, Length);
        int lo = 0; int hi = Distances.Count() - 1;
        while (hi - lo > 1)
        {
            int middle = (lo + hi) / 2;
            if (Distances[middle] <= distance) lo = middle; else hi = middle;
        }
        float span = Distances[hi] - Distances[lo];
        float u = (distance - Distances[lo]) / span;
        float u2 = u * u; float u3 = u2 * u;
        vector result = Points[lo] * (2 * u3 - 3 * u2 + 1);
        result += Tangents[lo] * ((u3 - 2 * u2 + u) * span);
        result += Points[hi] * (-2 * u3 + 3 * u2);
        result += Tangents[hi] * ((u3 - u2) * span);
        return result;
    }
};
class RPConvoyRoute : RPFirstGeometry
{
    static const float FORCED_STOP_SECONDS = 300.0;
    int Schema;
    float Start;
    float End;
    float Speed;
    float Acceleration;
    float Dwell;
    vector Observer;
    vector LookAt;
    ref array<ref RPConvoyRail> ExpectedRails;
    ref array<ref RPFirstStop> Stops;
    ref array<ref RPFirstCrossingSite> Crossings;
    ref RPFirstGeometry ReverseGeometry;
    float ReverseStart;
    float ReverseEnd;
    float ReverseSpeed;
    float ManoeuvreDwell;
    float SharedFrom;
    float SharedOffset;
    float JunctionDistance;
    ref RPFirstGeometry WestGeometry;
    float WestEnd;
    float YardEnd;
    bool WestDirect;
    ref RPFirstGeometry NovayaGeometry;
    float NovayaEnd;
    ref RPFirstGeometry SeverogradGeometry;
    float SeverogradEnd;
    ref RPFirstGeometry NovodmitrovskGeometry;
    float NovodmitrovskEnd;
    ref RPFirstGeometry SvetloyarskGeometry;
    float SvetloyarskEnd;
    ref RPFirstGeometry BerezinoGeometry;
    float BerezinoEnd;
    ref RPFirstGeometry SouthCoastGeometry;
    bool Cyclic;
    protected float m_CycleDuration;

    float CycleDuration()
    {
        if (!Cyclic) return 0;
        if (m_CycleDuration <= 0) m_CycleDuration = TotalSeconds();
        return m_CycleDuration;
    }
    float LapClock(float clock)
    {
        if (!Cyclic) return clock;
        float cycle = CycleDuration();
        if (clock < cycle) return clock;
        return clock - Math.Floor(clock / cycle) * cycle;
    }
    static const int FIRST_SOUTH_COAST_STOP = 9;

    bool HasSouthCoast() { return SouthCoastGeometry != null; }

    bool ValidSouthCoast()
    {
        if (!HasSouthCoast()) return !Cyclic && Stops.Count() <= FIRST_SOUTH_COAST_STOP;
        if (!HasBerezino() || Stops.Count() != 15 || !SouthCoastGeometry.ValidGeometry()) return false;
        array<string> names = {"Solnechny", "Kamyshovo", "Elektrozavodsk", "Chernogorsk", "Balota", "Komarovo"};
        float previous = BerezinoEnd;
        for (int i = FIRST_SOUTH_COAST_STOP; i < Stops.Count(); i++)
        {
            RPFirstStop stop = Stops[i];
            if (!stop || stop.Name != names[i - FIRST_SOUTH_COAST_STOP]) return false;
            if (stop.Distance - previous < Speed * Speed / Acceleration || stop.Distance > SouthCoastGeometry.Length) return false;
            previous = stop.Distance;
        }
        if (vector.Distance(BerezinoGeometry.At(BerezinoEnd), SouthCoastGeometry.At(BerezinoEnd)) >= 0.005) return false;
        if (!Cyclic) return true;
        for (float offset = -82; offset <= 7; offset += 0.5)
            if (vector.Distance(At(Start + offset), SouthCoastGeometry.At(previous + offset)) > 0.045) return false;
        return true;
    }

    int SouthCoastSegment(float clock)
    {
        clock = LapClock(clock);
        if (!HasSouthCoast() || clock < DepartureAt(8)) return -1;
        for (int i = FIRST_SOUTH_COAST_STOP; i < Stops.Count() - 1; i++)
            if (clock < DepartureAt(i)) return i;
        return Stops.Count() - 1;
    }

    int MovingSouthCoastSegment(float clock)
    {
        clock = LapClock(clock);
        int segment = SouthCoastSegment(clock);
        if (segment >= FIRST_SOUTH_COAST_STOP && clock < ArrivalAt(segment)) return segment;
        return -1;
    }

    bool HasWest() { return WestGeometry != null; }
    bool HasNovaya() { return NovayaGeometry != null; }
    bool HasSeverograd() { return SeverogradGeometry != null; }
    bool HasNovodmitrovsk() { return NovodmitrovskGeometry != null; }
    bool HasSvetloyarsk() { return SvetloyarskGeometry != null; }
    bool HasBerezino() { return BerezinoGeometry != null; }

    bool ValidWest()
    {
        if (!HasWest()) return Stops.Count() == 3 && !HasNovaya();
        if (Stops.Count() < 4 || Stops.Count() > 15) return false;
        if (!WestGeometry.ValidGeometry()) return false;
        if (!Stops[3] || Stops[3].Name == "" || Math.AbsFloat(Stops[3].Distance - YardEnd) > 0.001) return false;
        if (WestEnd <= ReverseEnd + Speed * Speed / Acceleration || WestEnd + 10 > WestGeometry.Length) return false;
        return YardEnd >= ReverseEnd && WestEnd > YardEnd + 85;
    }

    bool ValidNovaya()
    {
        if (!HasNovaya()) return Stops.Count() <= 4;
        if (!HasWest() || !WestDirect || Stops.Count() < 5 || !NovayaGeometry.ValidGeometry()) return false;
        if (!Stops[4] || Stops[4].Name != "Novaya Petrovka" || Math.AbsFloat(Stops[4].Distance - NovayaEnd) > 0.002) return false;
        if (NovayaEnd - YardEnd < Speed * Speed / Acceleration || NovayaEnd + 10 > NovayaGeometry.Length) return false;
        return vector.Distance(WestGeometry.At(YardEnd), NovayaGeometry.At(YardEnd)) < 0.005;
    }

    bool ValidSeverograd()
    {
        if (!HasSeverograd()) return Stops.Count() <= 5;
        if (!HasNovaya() || Stops.Count() < 6 || !SeverogradGeometry.ValidGeometry()) return false;
        if (!Stops[5] || Stops[5].Name != "Severograd" || Math.AbsFloat(Stops[5].Distance - SeverogradEnd) > 0.002) return false;
        if (SeverogradEnd - NovayaEnd < Speed * Speed / Acceleration || SeverogradEnd + 10 > SeverogradGeometry.Length) return false;
        return vector.Distance(NovayaGeometry.At(NovayaEnd), SeverogradGeometry.At(NovayaEnd)) < 0.005;
    }

    bool ValidNovodmitrovsk()
    {
        if (!HasNovodmitrovsk()) return Stops.Count() <= 6;
        if (!HasSeverograd() || Stops.Count() < 7 || !NovodmitrovskGeometry.ValidGeometry()) return false;
        if (!Stops[6] || Stops[6].Name != "Novodmitrovsk" || Math.AbsFloat(Stops[6].Distance - NovodmitrovskEnd) > 0.002) return false;
        if (NovodmitrovskEnd - SeverogradEnd < Speed * Speed / Acceleration || NovodmitrovskEnd + 10 > NovodmitrovskGeometry.Length) return false;
        return vector.Distance(SeverogradGeometry.At(SeverogradEnd), NovodmitrovskGeometry.At(SeverogradEnd)) < 0.005;
    }

    bool ValidSvetloyarsk()
    {
        if (!HasSvetloyarsk()) return Stops.Count() <= 7;
        if (!HasNovodmitrovsk() || Stops.Count() < 8 || !SvetloyarskGeometry.ValidGeometry()) return false;
        if (!Stops[7] || Stops[7].Name != "Svetloyarsk" || Math.AbsFloat(Stops[7].Distance - SvetloyarskEnd) > 0.002) return false;
        if (SvetloyarskEnd - NovodmitrovskEnd < Speed * Speed / Acceleration || SvetloyarskEnd + 10 > SvetloyarskGeometry.Length) return false;
        return vector.Distance(NovodmitrovskGeometry.At(NovodmitrovskEnd), SvetloyarskGeometry.At(NovodmitrovskEnd)) < 0.005;
    }

    bool ValidBerezino()
    {
        if (!HasBerezino()) return Stops.Count() <= 8;
        if (!HasSvetloyarsk() || Stops.Count() < 9 || !BerezinoGeometry.ValidGeometry()) return false;
        if (!Stops[8] || Stops[8].Name != "Berezino Nord" || Math.AbsFloat(Stops[8].Distance - BerezinoEnd) > 0.002) return false;
        if (BerezinoEnd - SvetloyarskEnd < Speed * Speed / Acceleration || BerezinoEnd + 10 > BerezinoGeometry.Length) return false;
        return vector.Distance(SvetloyarskGeometry.At(SvetloyarskEnd), BerezinoGeometry.At(SvetloyarskEnd)) < 0.005;
    }

    static RPConvoyRoute Load()
    {
        RPConvoyRoute route; string error;
        if (!JsonFileLoader<RPConvoyRoute>.LoadFile("RailPilot\\FirstRoute\\route.json", route, error))
        {
            Print("[RailPilotGuided] ERROR first route " + error); return null;
        }
        if (!route || route.Schema != 3 || !route.ValidGeometry()) return null;
        if (!route.ExpectedRails || !route.Stops || !route.Crossings || !route.ReverseGeometry) return null;
        if (!route.ReverseGeometry.ValidGeometry()) return null;
        if (!route.ValidWest() || !route.ValidNovaya() || !route.ValidSeverograd() || !route.ValidNovodmitrovsk()) return null;
        if (!route.ValidSvetloyarsk() || !route.ValidBerezino() || !route.ValidSouthCoast()) return null;
        if (route.Crossings.Count() < 2) return null;
        if (route.Start < 100 || route.End > route.Length - 10 || route.End - route.Start < 7000) return null;
        if (Math.AbsFloat(route.Speed - 40.0 / 3.6) > 0.001 || route.Acceleration <= 0) return null;
        if (route.Stops[0].Name != "Komarovo" || route.Stops[1].Name != "Kamenka" || route.Stops[2].Name != "Zelenogorsk") return null;
        if (Math.AbsFloat(route.Stops[0].Distance - route.Start) > 0.001 || Math.AbsFloat(route.Stops[2].Distance - route.ReverseEnd) > 0.001) return null;
        for (int i = 0; i < route.Stops.Count(); i++)
        {
            RPFirstStop stop = route.Stops[i];
            if (!stop || stop.Dwell != 300) return null;
        }
        if (route.Stops[1].Distance - route.Start < route.Speed * route.Speed / route.Acceleration) return null;
        if (route.End - route.Stops[1].Distance < route.Speed * route.Speed / route.Acceleration) return null;
        if (Math.AbsFloat(route.ReverseSpeed - 10.0 / 3.6) > 0.001 || route.ManoeuvreDwell != 5) return null;
        if (route.ReverseEnd < 100 || route.ReverseStart - route.ReverseEnd < 100) return null;
        if (route.ReverseStart > route.ReverseGeometry.Length - 10) return null;
        if (Math.AbsFloat(route.ReverseStart - route.SharedFrom - 90) > 0.02) return null;
        if (Math.AbsFloat(route.End - route.ReverseStart - route.SharedOffset) > 0.01) return null;
        for (int k = 0; k < route.Crossings.Count(); k++)
        {
            RPFirstCrossingSite site = route.Crossings[k];
            if (!site || !site.BarrierPositions || !site.BarrierOrientations) return null;
            int originalCount = site.BarrierPositions.Count();
            if (originalCount != site.BarrierOrientations.Count()) return null;
            if (originalCount != 2)
            {
                if (originalCount != 0 || !site.RoadGateRows || site.RoadGateRows.Count() != 2) return null;
            }
            if (site.RoadGateRows && site.RoadGateRows.Count() > 0)
            {
                if (site.RoadGateRows.Count() != 2 || !site.RoadGateRows[0] || !site.RoadGateRows[1]) return null;
            }
            if (site.Segment == 4 && (!route.HasNovaya() || site.Distance <= route.YardEnd || site.Distance >= route.NovayaEnd)) return null;
            if (site.Segment == 5 && (!route.HasSeverograd() || site.Distance <= route.NovayaEnd || site.Distance >= route.SeverogradEnd)) return null;
            if (site.Segment == 6 && (!route.HasNovodmitrovsk() || site.Distance <= route.SeverogradEnd || site.Distance >= route.NovodmitrovskEnd)) return null;
            if (site.Segment == 7 && (!route.HasSvetloyarsk() || site.Distance <= route.NovodmitrovskEnd || site.Distance >= route.SvetloyarskEnd)) return null;
            if (site.Segment == 8 && (!route.HasBerezino() || site.Distance <= route.SvetloyarskEnd || site.Distance >= route.BerezinoEnd)) return null;
            if (site.Segment != 0 && (site.Segment < 4 || site.Segment >= route.Stops.Count())) return null;
            if (site.Segment >= FIRST_SOUTH_COAST_STOP)
            {
                if (!route.HasSouthCoast()) return null;
                if (site.Distance <= route.Stops[site.Segment - 1].Distance || site.Distance >= route.Stops[site.Segment].Distance) return null;
            }
            if (site.ReturnDistance != 0)
            {
                if (!route.HasSouthCoast() || site.Segment != 0) return null;
                if (site.ReturnSegment < FIRST_SOUTH_COAST_STOP || site.ReturnSegment >= route.Stops.Count()) return null;
                if (site.ReturnDistance <= route.Stops[site.ReturnSegment - 1].Distance || site.ReturnDistance >= route.Stops[site.ReturnSegment].Distance) return null;
            }
        }
        return route;
    }

    float LegDuration(int index)
    {
        if (index == 0) return Duration(Start, Stops[1].Distance, Speed);
        return Duration(Stops[1].Distance, End, Speed);
    }
    float Duration(float a, float b, float limit) { return Math.AbsFloat(b - a) / limit + limit / Acceleration; }
    float ManoeuvreArrivalAt() { return DepartureAt(1) + LegDuration(1); }
    float ReversalAt() { return ManoeuvreArrivalAt() + ManoeuvreDwell; }
    float ReverseDuration() { return Duration(ReverseStart, ReverseEnd, ReverseSpeed); }
    float ArrivalAt(int index)
    {
        if (index == 0) return 0;
        if (index == 1) return Stops[0].Dwell + LegDuration(0);
        if (index >= FIRST_SOUTH_COAST_STOP && HasSouthCoast())
        {
            float clock = ArrivalAt(8);
            for (int i = FIRST_SOUTH_COAST_STOP; i <= index; i++)
                clock += Stops[i - 1].Dwell + Duration(Stops[i - 1].Distance, Stops[i].Distance, Speed);
            return clock;
        }
        if (index == 8 && HasBerezino()) return DepartureAt(7) + Duration(SvetloyarskEnd, BerezinoEnd, Speed);
        if (index == 7 && HasSvetloyarsk()) return DepartureAt(6) + Duration(NovodmitrovskEnd, SvetloyarskEnd, Speed);
        if (index == 6 && HasNovodmitrovsk()) return DepartureAt(5) + Duration(SeverogradEnd, NovodmitrovskEnd, Speed);
        if (index == 5 && HasSeverograd()) return DepartureAt(4) + Duration(NovayaEnd, SeverogradEnd, Speed);
        if (index == 4 && HasNovaya()) return DepartureAt(3) + Duration(YardEnd, NovayaEnd, Speed);
        if (index == 3 && HasWest())
        {
            if (WestDirect) return WestRunEndAt();
            return YardReverseAt() + Duration(WestEnd, YardEnd, ReverseSpeed);
        }
        return ReversalAt() + ReverseDuration();
    }
    float DepartureAt(int index) { return ArrivalAt(index) + Stops[index].Dwell; }
    float WestTurnAt() { return DepartureAt(2) + Duration(ReverseEnd, WestEnd, Speed); }
    float YardReverseAt() { return WestTurnAt() + ManoeuvreDwell; }
    float WestRunEndAt()
    {
        if (WestDirect) return DepartureAt(2) + Duration(ReverseEnd, YardEnd, Speed);
        return WestTurnAt();
    }

    float ScheduledBrakeRemaining(float clock, out float duration)
    {
        clock = LapClock(clock);
        duration = 0;
        float end;
        float speedLimit = Speed;
        int southSegment = MovingSouthCoastSegment(clock);
        if (HasSouthCoast() && clock >= DepartureAt(8))
        {
            if (southSegment < 0) return 0;
            end = ArrivalAt(southSegment);
        }
        else if (clock < DepartureAt(0)) return 0;
        else if (clock < ArrivalAt(1)) end = ArrivalAt(1);
        else if (clock >= DepartureAt(1) && clock < ManoeuvreArrivalAt()) end = ManoeuvreArrivalAt();
        else if (clock >= ReversalAt() && clock < ArrivalAt(2))
        {
            end = ArrivalAt(2);
            speedLimit = ReverseSpeed;
        }
        else if (HasWest() && clock >= DepartureAt(2) && clock < WestRunEndAt()) end = WestRunEndAt();
        else if (HasWest() && !WestDirect && clock >= YardReverseAt() && clock < ArrivalAt(3))
        {
            end = ArrivalAt(3);
            speedLimit = ReverseSpeed;
        }
        else if (HasNovaya() && clock >= DepartureAt(3) && clock < ArrivalAt(4)) end = ArrivalAt(4);
        else if (HasSeverograd() && clock >= DepartureAt(4) && clock < ArrivalAt(5)) end = ArrivalAt(5);
        else if (HasNovodmitrovsk() && clock >= DepartureAt(5) && clock < ArrivalAt(6)) end = ArrivalAt(6);
        else if (HasSvetloyarsk() && clock >= DepartureAt(6) && clock < ArrivalAt(7)) end = ArrivalAt(7);
        else if (HasBerezino() && clock >= DepartureAt(7) && clock < ArrivalAt(8)) end = ArrivalAt(8);
        else return 0;
        float ramp = speedLimit / Acceleration;
        float remaining = end - clock;
        if (remaining > ramp) return 0;
        duration = ramp;
        return remaining;
    }

    int StationApproach(float clock, float consistLength)
    {
        clock = LapClock(clock);
        float distance;
        float speed;
        int direction;
        State(clock, distance, speed, direction);
        float reach = consistLength + 3.0;
        int southSegment = MovingSouthCoastSegment(clock);
        if (southSegment >= FIRST_SOUTH_COAST_STOP)
        {
            if (Stops[southSegment].Distance - distance <= reach) return southSegment;
            return -1;
        }
        if (clock >= DepartureAt(0) && clock < ArrivalAt(1))
        {
            // Komarovo starts with its stop already served in this itinerary.
            if (distance - Start <= reach) return 0;
            if (Stops[1].Distance - distance <= reach) return 1;
        }
        if (clock >= ReversalAt() && clock < ArrivalAt(2) && distance - ReverseEnd <= reach) return 2;
        if (HasWest() && clock < ArrivalAt(3))
        {
            if (WestDirect && clock >= DepartureAt(2) && YardEnd - distance <= reach) return 3;
            if (!WestDirect && clock >= YardReverseAt() && distance - YardEnd <= reach) return 3;
        }
        if (HasNovaya() && clock >= DepartureAt(3) && clock < ArrivalAt(4) && NovayaEnd - distance <= reach) return 4;
        if (HasSeverograd() && clock >= DepartureAt(4) && clock < ArrivalAt(5) && SeverogradEnd - distance <= reach) return 5;
        if (HasNovodmitrovsk() && clock >= DepartureAt(5) && clock < ArrivalAt(6) && NovodmitrovskEnd - distance <= reach) return 6;
        if (HasSvetloyarsk() && clock >= DepartureAt(6) && clock < ArrivalAt(7) && SvetloyarskEnd - distance <= reach) return 7;
        if (HasBerezino() && clock >= DepartureAt(7) && clock < ArrivalAt(8) && BerezinoEnd - distance <= reach) return 8;
        return -1;
    }

    float TotalSeconds()
    {
        if (HasSouthCoast()) return ArrivalAt(Stops.Count() - 1);
        if (HasBerezino()) return ArrivalAt(8);
        if (HasSvetloyarsk()) return ArrivalAt(7);
        if (HasNovodmitrovsk()) return ArrivalAt(6);
        if (HasSeverograd()) return ArrivalAt(5);
        if (HasNovaya()) return ArrivalAt(4);
        if (HasWest()) return ArrivalAt(3);
        return DepartureAt(2);
    }

    int SegmentAt(float clock)
    {
        clock = LapClock(clock);
        int southSegment = SouthCoastSegment(clock);
        if (southSegment >= FIRST_SOUTH_COAST_STOP) return southSegment;
        if (HasBerezino() && clock >= DepartureAt(7)) return 8;
        if (HasSvetloyarsk() && clock >= DepartureAt(6)) return 7;
        if (HasNovodmitrovsk() && clock >= DepartureAt(5)) return 6;
        if (HasSeverograd() && clock >= DepartureAt(4)) return 5;
        if (HasNovaya() && clock >= DepartureAt(3)) return 4;
        if (HasWest() && clock >= DepartureAt(2))
        {
            if (!WestDirect && clock >= YardReverseAt()) return 3;
            return 2;
        }
        if (clock >= ReversalAt()) return 1;
        return 0;
    }

    float TrackRemaining(float distance, int segment)
    {
        if (segment >= FIRST_SOUTH_COAST_STOP && HasSouthCoast()) return Math.Max(0, Stops[segment].Distance - distance);
        if (segment == 8) return Math.Max(0, BerezinoEnd - distance);
        if (segment == 7) return Math.Max(0, SvetloyarskEnd - distance);
        if (segment == 6) return Math.Max(0, NovodmitrovskEnd - distance);
        if (segment == 5) return Math.Max(0, SeverogradEnd - distance);
        if (segment == 4) return Math.Max(0, NovayaEnd - distance);
        if (segment == 3) return Math.Max(0, distance - YardEnd);
        if (segment == 2)
        {
            if (WestDirect) return Math.Max(0, YardEnd - distance);
            return Math.Max(0, WestEnd - distance);
        }
        if (segment == 1) return Math.Max(0, distance - ReverseEnd);
        return Math.Max(0, End - distance);
    }
    void Motion(float a, float b, float limit, float time, out float distance, out float speed)
    {
        float leg = Duration(a, b, limit); float ramp = limit / Acceleration;
        float t = Math.Clamp(time, 0, leg); float traveled;
        speed = 0;
        if (t < ramp)
        {
            traveled = 0.5 * Acceleration * t * t; speed = Acceleration * t;
        }
        else if (t < leg - ramp) { traveled = limit * (t - 0.5 * ramp); speed = limit; }
        else
        {
            float remaining = leg - t; traveled = Math.AbsFloat(b - a) - 0.5 * Acceleration * remaining * remaining;
            speed = Acceleration * remaining;
        }
        if (b >= a) distance = a + traveled; else distance = a - traveled;
    }
    void State(float clock, out float distance, out float speed, out int direction)
    {
        clock = LapClock(clock);
        direction = 1; speed = 0; distance = Start;
        if (clock <= DepartureAt(0)) return;
        if (clock < ArrivalAt(1)) { Motion(Start, Stops[1].Distance, Speed, clock - DepartureAt(0), distance, speed); return; }
        distance = Stops[1].Distance;
        if (clock <= DepartureAt(1)) return;
        if (clock < ManoeuvreArrivalAt()) { Motion(Stops[1].Distance, End, Speed, clock - DepartureAt(1), distance, speed); return; }
        distance = End;
        if (clock < ReversalAt()) return;
        direction = -1; distance = ReverseEnd;
        if (clock < ArrivalAt(2))
        {
            Motion(ReverseStart, ReverseEnd, ReverseSpeed, clock - ReversalAt(), distance, speed);
            return;
        }
        if (!HasWest() || clock < DepartureAt(2)) return;
        direction = 1;
        if (WestDirect)
        {
            int southSegment = SouthCoastSegment(clock);
            if (southSegment >= FIRST_SOUTH_COAST_STOP)
            {
                distance = Stops[southSegment].Distance;
                if (clock < ArrivalAt(southSegment))
                    Motion(Stops[southSegment - 1].Distance, distance, Speed, clock - DepartureAt(southSegment - 1), distance, speed);
                return;
            }
            if (HasBerezino() && clock >= DepartureAt(7))
            {
                distance = BerezinoEnd;
                if (clock < ArrivalAt(8)) Motion(SvetloyarskEnd, BerezinoEnd, Speed, clock - DepartureAt(7), distance, speed);
                return;
            }
            if (HasSvetloyarsk() && clock >= DepartureAt(6))
            {
                distance = SvetloyarskEnd;
                if (clock < ArrivalAt(7)) Motion(NovodmitrovskEnd, SvetloyarskEnd, Speed, clock - DepartureAt(6), distance, speed);
                return;
            }
            if (HasNovodmitrovsk() && clock >= DepartureAt(5))
            {
                distance = NovodmitrovskEnd;
                if (clock < ArrivalAt(6)) Motion(SeverogradEnd, NovodmitrovskEnd, Speed, clock - DepartureAt(5), distance, speed);
                return;
            }
            if (HasSeverograd() && clock >= DepartureAt(4))
            {
                distance = SeverogradEnd;
                if (clock < ArrivalAt(5)) Motion(NovayaEnd, SeverogradEnd, Speed, clock - DepartureAt(4), distance, speed);
                return;
            }
            distance = YardEnd;
            if (clock < WestRunEndAt()) Motion(ReverseEnd, YardEnd, Speed, clock - DepartureAt(2), distance, speed);
            else if (HasNovaya() && clock >= DepartureAt(3))
            {
                distance = NovayaEnd;
                if (clock < ArrivalAt(4)) Motion(YardEnd, NovayaEnd, Speed, clock - DepartureAt(3), distance, speed);
            }
            return;
        }
        distance = WestEnd;
        if (clock < WestTurnAt())
        {
            Motion(ReverseEnd, WestEnd, Speed, clock - DepartureAt(2), distance, speed);
            return;
        }
        if (clock < YardReverseAt()) return;
        direction = -1;
        distance = YardEnd;
        if (clock < ArrivalAt(3)) Motion(WestEnd, YardEnd, ReverseSpeed, clock - YardReverseAt(), distance, speed);
    }

    vector SegmentPoint(float distance, int segment)
    {
        if (segment >= FIRST_SOUTH_COAST_STOP && HasSouthCoast()) return SouthCoastGeometry.At(distance);
        if (segment == 8) return BerezinoGeometry.At(distance);
        if (segment == 7) return SvetloyarskGeometry.At(distance);
        if (segment == 6) return NovodmitrovskGeometry.At(distance);
        if (segment == 5) return SeverogradGeometry.At(distance);
        if (segment == 4) return NovayaGeometry.At(distance);
        if (segment >= 2) return WestGeometry.At(distance);
        return RailPoint(distance, segment == 1);
    }

    vector RailPoint(float distance, bool reversing)
    {
        if (!reversing || !ReverseGeometry) return At(distance);
        // Entire consist uses the exact same mainline geometry across the
        // clock/role transition. Only once nearer the fork does its branch
        // become distinct; the small native resampling residual blends C1.
        float u = Math.Clamp((distance - SharedFrom - 5) / 5, 0, 1);
        if (u <= 0) return ReverseGeometry.At(distance);
        if (u >= 1) return At(distance + SharedOffset);
        float blend = u * u * (3 - 2 * u);
        return ReverseGeometry.At(distance) * (1 - blend) + At(distance + SharedOffset) * blend;
    }
    void Pose(float distance, float bottom, float bogieHalf, out vector matrix[4])
    {
        vector back = At(distance - bogieHalf); vector front = At(distance + bogieHalf);
        vector forward = front - back; forward.Normalize();
        Math3D.DirectionAndUpMatrix(forward, vector.Up, matrix);
        matrix[3] = (front + back) * 0.5;
        vector origin = matrix[3]; origin[1] = origin[1] - bottom; matrix[3] = origin;
    }
};

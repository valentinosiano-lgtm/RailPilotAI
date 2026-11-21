class RPFirstSectionIdentity
{
    string Id;
    string Fingerprint;
};
class RPFirstRecoveryManifest
{
    int Schema;
    string RouteId;
    string ConvoyId;
    string Map;
    string Revision;
    ref array<ref RPFirstSectionIdentity> Sections;

    static RPFirstRecoveryManifest Load()
    {
        RPFirstRecoveryManifest manifest; string error;
        if (!JsonFileLoader<RPFirstRecoveryManifest>.LoadFile("RailPilot\\FirstRoute\\recovery-route.json", manifest, error)) return null;
        if (!manifest || manifest.Schema != 1 || manifest.RouteId == "" || manifest.ConvoyId == "" || manifest.Map != "chernarusplus" || !manifest.Sections || manifest.Sections.Count() < 3) return null;
        for (int i = 0; i < manifest.Sections.Count(); i++)
        {
            if (!manifest.Sections[i] || manifest.Sections[i].Id == "" || manifest.Sections[i].Fingerprint.Length() != 64) return null;
            for (int j = 0; j < i; j++) if (manifest.Sections[j].Id == manifest.Sections[i].Id) return null;
        }
        return manifest;
    }
};
class RPFirstGateState
{
    string Id;
    int State = -1;
    float Phase = -1;
    int Direction;
    float PrewarningRemaining = -1;
};
class RPFirstPassengerState
{
    string PlayerId;
    int Slot = -1;
    vector LocalPosition;
    vector LocalDirection;
};
// Exact schema 2 representation, including field order for the checksum.
class RPFirstRecoveryStateV2
{
    int Schema = -1;
    int Generation = -1;
    string RouteId;
    string ConvoyId;
    string Map;
    string Revision;
    ref array<ref RPFirstSectionIdentity> Sections;
    string SegmentId;
    string PhaseId;
    float PhaseSeconds = -1;
    float Distance = -1;
    float Speed = -1;
    int Direction;
    int LeadingSlot = -1;
    float DwellRemaining = -1;
    int NextHorn = -1;
    int Horns = -1;
    int LastStop = -2;
    int Lap = -1;
    ref array<ref RPFirstGateState> Gates;
    ref array<ref RPFirstPassengerState> Passengers = new array<ref RPFirstPassengerState>;
    string Checksum;
};

class RPFirstRecoveryState : RPFirstRecoveryStateV2
{
    int BlockedStop = -1;
    float BlockedStopSeconds;
};

// Exact schema1 serialization, retained only to verify and migrate older saves.
class RPFirstLegacyRecoveryState
{
    int Schema = -1;
    int Generation = -1;
    string RouteId;
    string ConvoyId;
    string Map;
    string Revision;
    ref array<ref RPFirstSectionIdentity> Sections;
    string SegmentId;
    string PhaseId;
    float PhaseSeconds = -1;
    float Distance = -1;
    float Speed = -1;
    int Direction;
    int LeadingSlot = -1;
    float DwellRemaining = -1;
    int NextHorn = -1;
    int Horns = -1;
    int LastStop = -2;
    int Lap = -1;
    ref array<ref RPFirstGateState> Gates;
    string Checksum;
};

// Stable phase identities, local time and rail distance are canonical.
// No global service-clock value is persisted across itinerary revisions.
class RPFirstRecoveryPhases
{
    static string Id(int phase)
    {
        array<string> ids = {"komarovo:hold", "komarovo-kamenka:run", "kamenka:hold", "kamenka-runout:run", "runout:reverse-pause", "zelenogorsk:reverse", "zelenogorsk:hold", "zelenogorsk:terminal", "west:run", "west-yard:reverse-pause", "west-yard:reverse", "west-yard:terminal", "myshkino:hold", "myshkino-novaya:run", "novaya:terminal", "novaya:hold", "novaya-severograd:run", "severograd:terminal", "severograd:hold", "severograd-novodmitrovsk:run", "novodmitrovsk:terminal",
            "novodmitrovsk:hold", "novodmitrovsk-svetloyarsk:run", "svetloyarsk:terminal",
            "svetloyarsk:hold", "svetloyarsk-berezino:run", "berezino:terminal",
            "berezino:hold", "berezino-solnechny:run", "solnechny:terminal",
            "solnechny:hold", "solnechny-kamyshovo:run", "kamyshovo:terminal",
            "kamyshovo:hold", "kamyshovo-elektrozavodsk:run", "elektrozavodsk:terminal",
            "elektrozavodsk:hold", "elektrozavodsk-chernogorsk:run", "chernogorsk:terminal",
            "chernogorsk:hold", "chernogorsk-balota:run", "balota:terminal",
            "balota:hold", "balota-komarovo:run", "komarovo-return:terminal"};
        if (phase < 0 || phase >= ids.Count()) return "";
        return ids[phase];
    }
    static string Segment(int phase)
    {
        if (phase <= 2) return "komarovo-kamenka";
        if (phase <= 4) return "kamenka-runout";
        if (phase <= 7) return "zelenogorsk-reverse";
        if (phase >= 43) return "balota-komarovo";
        if (phase >= 40) return "chernogorsk-balota";
        if (phase >= 37) return "elektrozavodsk-chernogorsk";
        if (phase >= 34) return "kamyshovo-elektrozavodsk";
        if (phase >= 31) return "solnechny-kamyshovo";
        if (phase >= 28) return "berezino-solnechny";
        if (phase >= 25) return "svetloyarsk-berezino";
        if (phase >= 22) return "novodmitrovsk-svetloyarsk";
        if (phase >= 19) return "severograd-novodmitrovsk";
        if (phase >= 16) return "novaya-severograd";
        if (phase >= 13) return "myshkino-novaya";
        return "zelenogorsk-west-yard";
    }
    static float Begin(RPConvoyRoute route, int phase)
    {
        if (phase == 0) return 0;
        if (phase == 1) return route.DepartureAt(0);
        if (phase == 2) return route.ArrivalAt(1);
        if (phase == 3) return route.DepartureAt(1);
        if (phase == 4) return route.ManoeuvreArrivalAt();
        if (phase == 5) return route.ReversalAt();
        if (phase == 6) return route.ArrivalAt(2);
        if (phase == 7 || phase == 8) return route.DepartureAt(2);
        if (phase == 9) return route.WestRunEndAt();
        if (phase == 10) return route.YardReverseAt();
        if (phase == 11 || phase == 12) return route.ArrivalAt(3);
        if (phase == 13) return route.DepartureAt(3);
        if (phase == 14 || phase == 15) return route.ArrivalAt(4);
        if (phase == 16) return route.DepartureAt(4);
        if (phase == 17 || phase == 18) return route.ArrivalAt(5);
        if (phase == 19) return route.DepartureAt(5);
        if (phase == 20 || phase == 21) return route.ArrivalAt(6);
        if (phase == 22) return route.DepartureAt(6);
        if (phase == 23 || phase == 24) return route.ArrivalAt(7);
        if (phase == 25) return route.DepartureAt(7);
        if (phase == 26) return route.ArrivalAt(8);
        if (phase >= 27 && route.HasSouthCoast())
        {
            int origin = 8 + (phase - 27) / 3;
            int kind = (phase - 27) % 3;
            if (kind == 0) return route.ArrivalAt(origin);
            if (kind == 1) return route.DepartureAt(origin);
            return route.ArrivalAt(origin + 1);
        }
        return route.TotalSeconds();
    }
    static int At(RPConvoyRoute route, float clock)
    {
        for (int i = 0; i < 7; i++) if (clock < Begin(route, i + 1)) return i;
        if (!route.HasWest()) return 7;
        if (route.WestDirect)
        {
            if (clock < route.ArrivalAt(3)) return 8;
            if (route.HasNovaya())
            {
                if (clock < route.DepartureAt(3)) return 12;
                if (clock < route.ArrivalAt(4)) return 13;
                if (route.HasSeverograd())
                {
                    if (clock < route.DepartureAt(4)) return 15;
                    if (clock < route.ArrivalAt(5)) return 16;
                    if (route.HasNovodmitrovsk())
                    {
                        if (clock < route.DepartureAt(5)) return 18;
                        if (clock < route.ArrivalAt(6)) return 19;
                        if (route.HasSvetloyarsk())
                        {
                            if (clock < route.DepartureAt(6)) return 21;
                            if (clock < route.ArrivalAt(7)) return 22;
                            if (route.HasBerezino())
                            {
                                if (clock < route.DepartureAt(7)) return 24;
                                if (clock < route.ArrivalAt(8)) return 25;
                                if (!route.HasSouthCoast()) return 26;
                                for (int s = 8; s < route.Stops.Count() - 1; s++)
                                {
                                    int phase = 27 + (s - 8) * 3;
                                    if (clock < route.DepartureAt(s)) return phase;
                                    if (clock < route.ArrivalAt(s + 1)) return phase + 1;
                                }
                                return 26 + (route.Stops.Count() - 9) * 3;
                            }
                            return 23;
                        }
                        return 20;
                    }
                    return 17;
                }
                return 14;
            }
            return 11;
        }
        if (clock < route.WestTurnAt()) return 8;
        if (clock < route.YardReverseAt()) return 9;
        if (clock < route.ArrivalAt(3)) return 10;
        return 11;
    }
    static int HornCount(RPConvoyRoute route, float clock, bool legacy = false)
    {
        int limit = route.Stops.Count() - 1;
        if (legacy) limit = 2;
        int count;
        for (int i = 0; i < limit; i++) if (clock >= route.DepartureAt(i) - 0.35) count++;
        return count;
    }
    static float Remaining(RPConvoyRoute route, int phase, float elapsed)
    {
        if (phase == 12) return Math.Max(0, route.Stops[3].Dwell - elapsed);
        if (phase == 15) return Math.Max(0, route.Stops[4].Dwell - elapsed);
        if (phase == 18) return Math.Max(0, route.Stops[5].Dwell - elapsed);
        if (phase == 21) return Math.Max(0, route.Stops[6].Dwell - elapsed);
        if (phase == 24) return Math.Max(0, route.Stops[7].Dwell - elapsed);
        if (phase >= 27 && (phase - 27) % 3 == 0)
            return Math.Max(0, route.Stops[8 + (phase - 27) / 3].Dwell - elapsed);
        if (phase != 0 && phase != 2 && phase != 4 && phase != 6 && phase != 9) return 0;
        return Begin(route, phase + 1) - Begin(route, phase) - elapsed;
    }
    static bool FiniteRange(float value, float lo, float hi)
    {
        return value >= lo && value <= hi; // Also rejects NaN/infinity.
    }

    protected static bool ProgressMatches(RPConvoyRoute route, RPFirstRecoveryState state, float clock, bool legacy)
    {
        if (state.Horns != state.NextHorn || state.NextHorn != HornCount(route, clock, legacy)) return false;
        if (state.Lap < 0 || state.Lap > 1000000000 || (!route.Cyclic && state.Lap != 0)) return false;
        int expectedStop = -1;
        for (int s = 0; s < route.Stops.Count(); s++) if (clock >= route.ArrivalAt(s)) expectedStop = s;
        // Fresh snapshot is written before the first arrival event at clock0.
        return state.LastStop == expectedStop || (clock == 0 && state.LastStop == -1);
    }

    protected static bool MigrateWestClock(RPConvoyRoute route, RPFirstRecoveryState state, int phase, out float clock)
    {
        if (!FiniteRange(state.Distance, 0, 100000) || !FiniteRange(state.Speed, 0, 15)) return false;
        if (state.DwellRemaining != 0 || state.BlockedStop >= 0) return false;
        if (phase == 11)
        {
            // The same six units occupy the same pose; only the active cab changes.
            clock = route.ArrivalAt(3);
            if (state.PhaseSeconds != 0 || state.Direction != -1 || state.LeadingSlot != 5 || state.Speed != 0) return false;
            return Math.AbsFloat(state.Distance - route.YardEnd) <= 0.02 && ProgressMatches(route, state, clock, false);
        }
        if (phase != 8 || state.Distance >= route.YardEnd)
        {
            Print("[RailPilotRecovery] snapshot is in the removed northern manoeuvre");
            return false;
        }
        float oldDuration = route.Duration(route.ReverseEnd, route.WestEnd, route.Speed);
        if (!FiniteRange(state.PhaseSeconds, 0, oldDuration) || state.Direction != 1 || state.LeadingSlot != 0) return false;
        float oldDistance;
        float oldSpeed;
        route.Motion(route.ReverseEnd, route.WestEnd, route.Speed, state.PhaseSeconds, oldDistance, oldSpeed);
        if (Math.AbsFloat(state.Distance - oldDistance) > 0.02 || Math.AbsFloat(state.Speed - oldSpeed) > 0.002) return false;

        // Recalculate the time at this rail position under the new braking curve.
        float low = route.DepartureAt(2);
        float high = route.ArrivalAt(3);
        for (int i = 0; i < 25; i++)
        {
            float middle = (low + high) * 0.5;
            float at;
            float speed;
            int direction;
            route.State(middle, at, speed, direction);
            if (at < oldDistance) low = middle;
            else high = middle;
        }
        clock = high;
        return At(route, clock) == 8 && ProgressMatches(route, state, clock, false);
    }

    static bool Clock(RPConvoyRoute route, RPFirstRecoveryState state, out float clock)
    {
        int phase = -1;
        bool legacy = route.HasWest() && state.Sections && state.Sections.Count() == 3;
        int phaseCount = 8;
        if (route.HasWest() && !legacy) phaseCount = 12;
        if (route.HasNovaya() && state.Sections && state.Sections.Count() >= 5) phaseCount = 15;
        if (route.HasSeverograd() && state.Sections && state.Sections.Count() >= 6) phaseCount = 18;
        if (route.HasNovodmitrovsk() && state.Sections && state.Sections.Count() >= 7) phaseCount = 21;
        if (route.HasSvetloyarsk() && state.Sections && state.Sections.Count() >= 8) phaseCount = 24;
        if (route.HasBerezino() && state.Sections && state.Sections.Count() >= 9) phaseCount = 27;
        if (route.HasSouthCoast() && state.Sections && state.Sections.Count() >= 10)
            phaseCount += (state.Sections.Count() - 9) * 3;
        for (int i = 0; i < phaseCount; i++) if (state.PhaseId == Id(i)) phase = i;
        if (phase < 0 || state.SegmentId != Segment(phase)) return false;
        bool previousWest = state.Revision == "west-stop-1" || state.Revision == "west-station-rules-1";
        if (route.WestDirect && previousWest && phase >= 8) return MigrateWestClock(route, state, phase, clock);
        bool oldTerminal = legacy && phase == 7;
        bool oldMyshkino = route.HasNovaya() && phase == 11 && phaseCount == 12 && state.Revision == "west-direct-stop-1";
        bool oldNovaya = route.HasSeverograd() && phase == 14 && phaseCount == 15 && state.Revision == "novaya-forward-1";
        bool oldSeverograd = route.HasNovodmitrovsk() && phase == 17 && phaseCount == 18 && state.Revision == "severograd-forward-1";
        bool oldNovodmitrovsk = route.HasSvetloyarsk() && phase == 20 && phaseCount == 21 && state.Revision == "novodmitrovsk-forward-1";
        bool oldSvetloyarsk = route.HasBerezino() && phase == 23 && phaseCount == 24 && state.Revision == "svetloyarsk-forward-1";
        bool oldBerezino = route.HasSouthCoast() && phase == 26 && phaseCount == 27 && state.Revision == "berezino-forward-1";
        if (phaseCount >= 12 && phase == 7) return false;
        float duration;
        if (phase < phaseCount - 1) duration = Begin(route, phase + 1) - Begin(route, phase);
        if (!FiniteRange(state.PhaseSeconds, 0, duration)) return false;
        clock = Begin(route, phase) + state.PhaseSeconds;
        if (route.Cyclic && clock >= route.CycleDuration()) return false;
        if (!oldTerminal && !oldMyshkino && !oldNovaya && !oldSeverograd && !oldNovodmitrovsk && !oldSvetloyarsk && !oldBerezino && At(route, clock) != phase) return false;
        float distance; float speed; int direction;
        if (oldTerminal) { distance = route.ReverseEnd; direction = -1; }
        else route.State(clock, distance, speed, direction);
        if (!FiniteRange(state.Distance, 0, 100000) || !FiniteRange(state.Speed, 0, 15) || !FiniteRange(state.DwellRemaining, 0, 300)) return false;
        if (Math.AbsFloat(state.Distance - distance) > 0.02 || Math.AbsFloat(state.Speed - speed) > 0.002) return false;
        if (state.Direction != direction || state.LeadingSlot != RPConvoySpec.LeadingSlot(direction)) return false;
        if (Math.AbsFloat(state.DwellRemaining - Remaining(route, phase, state.PhaseSeconds)) > 0.002) return false;
        return ProgressMatches(route, state, clock, legacy);
    }
};

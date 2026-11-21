// Derive a sustained state from the same replicated itinerary clock as motion.
// No transient RPC/event: late joins and recovered stops need no event replay.
class RPBrakeAirState
{
    static const float LEAD_SECONDS = 3.0;
    static bool Active(RPConvoyRoute route, float clock)
    {
        if (!route || clock < 0) return false;
        if (clock < route.DepartureAt(0)) return true;
        if (clock >= route.ArrivalAt(1) - LEAD_SECONDS && clock < route.DepartureAt(1)) return true;
        if (clock >= route.ManoeuvreArrivalAt() - LEAD_SECONDS && clock < route.ReversalAt()) return true;
        if (!route.HasWest()) return clock >= route.ArrivalAt(2) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(2) - LEAD_SECONDS && clock < route.DepartureAt(2)) return true;
        if (!route.WestDirect && clock >= route.WestTurnAt() - LEAD_SECONDS && clock < route.YardReverseAt()) return true;
        if (!route.HasNovaya()) return clock >= route.ArrivalAt(3) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(3) - LEAD_SECONDS && clock < route.DepartureAt(3)) return true;
        if (!route.HasSeverograd()) return clock >= route.ArrivalAt(4) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(4) - LEAD_SECONDS && clock < route.DepartureAt(4)) return true;
        if (!route.HasNovodmitrovsk()) return clock >= route.ArrivalAt(5) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(5) - LEAD_SECONDS && clock < route.DepartureAt(5)) return true;
        if (!route.HasSvetloyarsk()) return clock >= route.ArrivalAt(6) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(6) - LEAD_SECONDS && clock < route.DepartureAt(6)) return true;
        if (!route.HasBerezino()) return clock >= route.ArrivalAt(7) - LEAD_SECONDS;
        if (clock >= route.ArrivalAt(7) - LEAD_SECONDS && clock < route.DepartureAt(7)) return true;
        if (!route.HasSouthCoast()) return clock >= route.ArrivalAt(8) - LEAD_SECONDS;
        for (int s = 8; s < route.Stops.Count() - 1; s++)
            if (clock >= route.ArrivalAt(s) - LEAD_SECONDS && clock < route.DepartureAt(s)) return true;
        return clock >= route.ArrivalAt(route.Stops.Count() - 1) - LEAD_SECONDS;
    }
};

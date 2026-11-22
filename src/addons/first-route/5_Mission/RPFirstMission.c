class RPFirstRun
{
    ref RPConvoyRoute Route;
    ref RPConvoyService Service;
    ref array<ref RPFirstCrossing> Crossings = new array<ref RPFirstCrossing>;
    protected ref RPRunOverController m_RunOver;
    int Failures;
    int Horns;
    bool Done;
    protected int m_NextHorn;
    protected int m_LastStop = -1;
    protected int m_RunOverSlot;
    protected ref RPFirstPersistence m_Persistence;
    protected ref RPFirstPassengers m_Passengers;
    protected float m_SaveElapsed;
    protected bool m_Started;
    protected bool m_SaveFailed;
    static const float RESTART_HOLD_SECONDS = 120;
    protected float m_RestartWait;
    protected float m_RestartSync;
    bool Recovered;
    float RecoveredClock;

    RPFirstPersistence Persistence() { return m_Persistence; }
    RPFirstPassengers Passengers() { return m_Passengers; }
    float RestartWaitRemaining() { return m_RestartWait; }
    protected bool PrepareRouteWorld() { return true; }
    protected void ReleaseRouteWorld() {}

    protected void BroadcastStationAnnouncement(string message)
    {
        Print("[RailPilotRing] STATION_ANNOUNCEMENT " + message);
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        foreach (Man character : players)
        {
            PlayerBase player = PlayerBase.Cast(character);
            if (player && player.GetIdentity()) player.MessageStatus(message);
        }
    }

    protected void AnnounceStation(string eventName, int index)
    {
        // The final timetable entry is Komarovo at the start of the next lap.
        if (index == Route.Stops.Count() - 1) index = 0;
        RPFirstStop station = Route.Stops[index];
        string next = Route.Stops[index + 1].Name;
        string message;
        if (eventName == "arrival")
        {
            int minutes = Math.Round(station.Dwell / 60);
            message = string.Format("The train has arrived at %1. It will stop for %2 minutes before continuing to %3.", station.Name, minutes, next);
        }
        else if (eventName == "departure")
            message = string.Format("The train has departed from %1. Next stop: %2.", station.Name, next);
        else
            message = string.Format("The train skipped %1 because the track was blocked. Next stop: %2.", station.Name, next);
        BroadcastStationAnnouncement(message);
    }

    void Check(bool ok, string text)
    {
        if (ok) Print("[RailPilotGuided] PASS " + text);
        else { Failures++; Print("[RailPilotGuided] FAIL " + text); }
    }
    bool Start(bool offline, bool persistent = false, string stateDirectory = "", string initialStation = "", bool holdAfterRestart = false)
    {
        if (m_Started || Service) return false;
        Route = RPConvoyRoute.Load();
        if (!Route) { Check(false, "first route schema loads"); return false; }
        bool valid = true;
        for (int i = 0; i < Route.ExpectedRails.Count(); i++)
        {
            RPConvoyRail rail = Route.ExpectedRails[i];
            array<Object> objects = new array<Object>; array<CargoBase> cargo = new array<CargoBase>;
            GetGame().GetObjectsAtPosition3D(rail.Position, 0.1, objects, cargo);
            bool found;
            float positionTolerance = 0.005;
            if (Route.HasWest()) positionTolerance = 0.012;
            for (int j = 0; j < objects.Count(); j++)
            {
                Object obj = objects[j];
                string model = obj.GetShapeName(); model.ToLower(); model.Replace("/", "\\");
                if (model == rail.Model && vector.Distance(obj.GetPosition(), rail.Position) < positionTolerance && vector.Distance(obj.GetOrientation(), rail.Orientation) < 0.01) found = true;
            }
            if (!found) { valid = false; Print("[RailPilotGuided] ERROR missing native rail " + rail.Model + " at " + rail.Position.ToString()); }
        }
        Check(valid, "all first-route native rail positions and orientations verified at runtime");
        if (!valid) return false;
        RPFirstRecoveryState restored; bool fresh; float startClock; int startLap;
        if (persistent)
        {
            if (stateDirectory == "") stateDirectory = "$profile:RailPilotFirstRouteRecovery";
            m_Persistence = new RPFirstPersistence;
            if (!m_Persistence.Configure(Route, stateDirectory) || !m_Persistence.Load(restored, fresh, startClock))
            {
                Print("[RailPilotRecovery] REFUSED missing, corrupt or incompatible state; original files preserved"); return false;
            }
            Recovered = !fresh; RecoveredClock = startClock;
            if (Recovered)
            {
                m_NextHorn = RPFirstRecoveryPhases.HornCount(Route, startClock);
                Horns = m_NextHorn;
                m_LastStop = restored.LastStop;
                startLap = restored.Lap;
            }
        }
        if (!Recovered && initialStation != "")
        {
            int station = -1;
            for (int s = 0; s < Route.Stops.Count() - 1; s++)
                if (Route.Stops[s].Name == initialStation) station = s;
            if (station < 0) { Check(false, "configured initial station is part of the timetable"); return false; }
            startClock = Route.ArrivalAt(station);
            m_NextHorn = RPFirstRecoveryPhases.HornCount(Route, startClock);
            Horns = m_NextHorn;
            m_LastStop = station;
        }
        Service = new RPConvoyService;
        Service.SetStopGreetings(false);
        if (!PrepareRouteWorld()) { Check(false, "route world changes were applied"); return false; }
        if (!Service.Start(Route, startClock, startLap)) { Check(false, "first route six-unit service starts"); ReleaseRouteWorld(); return false; }
        if (Recovered && !Service.RPRestoreBlockedStop(restored.BlockedStop, restored.BlockedStopSeconds))
        {
            Check(false, "saved station block can restore on current route");
            Service.Stop();
            ReleaseRouteWorld();
            return false;
        }
        if (Recovered && Route.Cyclic && holdAfterRestart)
        {
            RPRingPassengers returning = new RPRingPassengers;
            returning.Configure(Service, restored.Passengers);
            returning.BeginRecovery(startClock);
            m_Passengers = returning;
            m_RestartWait = RESTART_HOLD_SECONDS;
            Service.RPPrepareRestartHold();
        }
        else
        {
            m_Passengers = new RPFirstPassengers;
            if (Recovered) m_Passengers.Configure(Service, restored.Passengers);
            else m_Passengers.Configure(Service);
        }
        for (int k = 0; k < Route.Crossings.Count(); k++)
        {
            RPFirstCrossing crossing = new RPFirstCrossing; Crossings.Insert(crossing);
            if (!crossing.Configure(Service, Route.Crossings[k], Route.Crossings[k].Name == "Komarovo", offline)) { Check(false, "first route crossing creates"); return false; }
            if (Recovered && k < restored.Gates.Count()) crossing.Restore(restored.Gates[k]);
        }
        float distance; float speed; int direction; Route.State(startClock, distance, speed, direction);
        m_RunOverSlot = RPConvoySpec.LeadingSlot(direction);
        m_RunOver = new RPRunOverController;
        Check(m_RunOver.Bind(Service.Unit(m_RunOverSlot), 1), "active locomotive retains isolated infected runover module");
        Check(RP_ConvoyUnit.All().Count() == 6 && Service.Length() > 80 && Service.Length() < 82, "single complete81m consist");
        Check(Route.Stops[0].Dwell == 300 && Route.Stops[1].Dwell == 300 && Route.Stops[2].Dwell == 300, "all three specified stops retain300seconds");
        m_Started = Failures == 0;
        if (m_Started && m_Persistence)
        {
            Print("[RailPilotRecovery] START recovered=" + Recovered.ToString() + " clock=" + startClock.ToString() + " head=" + m_RunOverSlot.ToString());
            if (!SaveNow()) return false;
            if (m_RestartWait > 0)
                PrintFormat("[RailPilotRing] RECOVERY_WAIT seconds=%1", RESTART_HOLD_SECONDS);
        }
        return m_Started;
    }
    void Update(float dt)
    {
        if (Done || !Service) return;
        if (m_RestartWait > 0)
        {
            float elapsed = Math.Min(dt, m_RestartWait);
            m_RestartWait -= elapsed;
            dt -= elapsed;
            m_RestartSync += elapsed;
            if (m_RestartSync >= 0.1 || m_RestartWait <= 0)
            {
                Service.Tick(0);
                m_RestartSync = 0;
            }
            if (m_RestartWait <= 0)
                Print("[RailPilotRing] RECOVERY_WAIT_COMPLETE");
        }
        if (dt <= 0) return;
        float previous = Service.Clock();
        int previousLap = Service.Lap();
        int oldPhase = RPFirstRecoveryPhases.At(Route, previous); int oldHorn = m_NextHorn;
        int oldBlockedStop = Service.RPBlockedStopIndex();
        bool forcedStopComplete = Service.RPBlockedStopTime() >= RPConvoyRoute.FORCED_STOP_SECONDS;
        Service.Tick(dt);
        float clock = Service.Clock();
        bool newLap = Service.Lap() != previousLap;
        bool skippedStop = forcedStopComplete && oldBlockedStop >= 0 && Service.RPBlockedStopIndex() < 0;
        int arrivedStation = -1;
        int departedStation = -1;
        if (newLap)
        {
            previous = 0;
            m_LastStop = 0;
            m_NextHorn = RPFirstRecoveryPhases.HornCount(Route, clock);
            Horns = m_NextHorn;
            if (!skippedStop) arrivedStation = 0;
            Print("[RailPilotRing] LAP " + Service.Lap().ToString() + " arrives Komarovo");
        }
        if (!newLap && forcedStopComplete && oldBlockedStop >= 0 && clock > previous + dt && clock > Route.DepartureAt(oldBlockedStop))
        {
            m_LastStop = Math.Max(m_LastStop, oldBlockedStop);
            m_NextHorn = RPFirstRecoveryPhases.HornCount(Route, clock);
            Horns = m_NextHorn;
        }
        if (Route.Cyclic && !newLap && !skippedStop && m_LastStop >= 0 && m_LastStop < Route.Stops.Count() - 1)
        {
            float departure = Route.DepartureAt(m_LastStop);
            // The horn is a warning. Announce only once the train starts its leg.
            if (previous <= departure && clock > departure) departedStation = m_LastStop;
        }
        if (m_NextHorn < Route.Stops.Count() - 1)
        {
            float hornAt = Route.DepartureAt(m_NextHorn) - 0.35;
            if (previous <= hornAt && clock >= hornAt)
            {
                Horns++;
                int sequence = (Service.Lap() % 1000000) * (Route.Stops.Count() - 1) + Horns;
                RPConvoyHorn.Emit(Service.Unit(0), sequence);
                Print("[RailPilotGuided] FIRST_DEPARTURE_HORN " + Route.Stops[m_NextHorn].Name + " clock=" + clock.ToString());
                m_NextHorn++;
            }
        }
        for (int i = 0; i < Crossings.Count(); i++) Crossings[i].Update(dt);
        float distance; float speed; int direction; Route.State(clock, distance, speed, direction);
        int activeSlot = RPConvoySpec.LeadingSlot(direction);
        if (activeSlot != m_RunOverSlot)
        {
            m_RunOver = new RPRunOverController;
            Check(m_RunOver.Bind(Service.Unit(activeSlot), 1), "infected sweep follows the active reverse locomotive");
            m_RunOverSlot = activeSlot;
            Print("[RailPilotGuided] MANOEUVRE_REVERSE_HEAD slot=" + activeSlot.ToString() + " clock=" + clock.ToString());
        }
        if (m_RunOver) m_RunOver.Tick(dt, 1, Service.Unit(activeSlot).Speed());
        for (int s = 0; s < Route.Stops.Count(); s++)
        {
            if (s <= m_LastStop) continue;
            float arrival = Route.ArrivalAt(s);
            if (clock >= arrival)
            {
                m_LastStop = s;
                if (Route.Cyclic && !skippedStop && previous < arrival) arrivedStation = s;
                Print("[RailPilotGuided] FIRST_ARRIVAL " + Route.Stops[s].Name + " clock=" + clock.ToString());
            }
        }
        if (m_Persistence)
        {
            m_SaveElapsed += dt;
            bool completeNow = Service.RPBlockedStopTime() >= RPConvoyRoute.FORCED_STOP_SECONDS;
            bool announcementDue = arrivedStation >= 0 || departedStation >= 0;
            bool saveDue = announcementDue || newLap || m_SaveElapsed >= 1 || oldPhase != RPFirstRecoveryPhases.At(Route, clock) || oldHorn != m_NextHorn || oldBlockedStop != Service.RPBlockedStopIndex() || forcedStopComplete != completeNow;
            if (saveDue && !SaveNow()) return;
        }
        if (!Route.Cyclic) return;
        if (skippedStop) AnnounceStation("skip", oldBlockedStop);
        if (departedStation >= 0) AnnounceStation("departure", departedStation);
        if (arrivedStation >= 0) AnnounceStation("arrival", arrivedStation);
    }
    RPFirstRecoveryState Capture()
    {
        RPFirstRecoveryState state = new RPFirstRecoveryState;
        float clock = Math.Min(Service.Clock(), Route.TotalSeconds());
        int phase = RPFirstRecoveryPhases.At(Route, clock);
        state.PhaseId = RPFirstRecoveryPhases.Id(phase); state.SegmentId = RPFirstRecoveryPhases.Segment(phase);
        state.PhaseSeconds = clock - RPFirstRecoveryPhases.Begin(Route, phase);
        Route.State(clock, state.Distance, state.Speed, state.Direction);
        state.LeadingSlot = Service.LeadingSlot();
        state.DwellRemaining = RPFirstRecoveryPhases.Remaining(Route, phase, state.PhaseSeconds);
        state.NextHorn = m_NextHorn; state.Horns = Horns; state.LastStop = m_LastStop; state.Lap = Service.Lap();
        state.BlockedStop = Service.RPBlockedStopIndex();
        state.BlockedStopSeconds = Service.RPBlockedStopTime();
        state.Gates = new array<ref RPFirstGateState>;
        for (int i = 0; i < Crossings.Count(); i++) state.Gates.Insert(Crossings[i].Capture());
        state.Passengers = m_Passengers.Capture();
        return state;
    }
    bool SaveNow()
    {
        if (!m_Persistence || !m_Started || m_SaveFailed) return false;
        if (!m_Persistence.Save(Capture()))
        {
            m_SaveFailed = true; Done = true;
            Print("[RailPilotRecovery] REFUSED state write/verification failed; stopping service, keeping last verified snapshot");
            // Release the ring cleanly without stopping the host mission.
            // The previous verified generation stays available to the owner.
            if (!Route.Cyclic) GetGame().RequestExit(1);
            return false;
        }
        m_SaveElapsed = 0; return true;
    }
    void Audit()
    {
        Check(Horns == 2, "exactly two predeparture horns, no automatic arrival or terminal horn");
        Check(Service.LeadingSlot() == 5, "east-facing locomotive remains selected on final reverse arrival");
        Check(Crossings[0].Logic().Openings() == 1 && Crossings[0].Logic().Completions() == 1, "initial Komarovo crossing opens once after entire consist");
        Check(Crossings[1].Logic().PreWarnings() == 1 && Crossings[1].Logic().Lowerings() == 1, "Kamenka retains one five-second prewarning");
        for (int i = 0; i < Crossings.Count(); i++)
        {
            RPFirstCrossing c = Crossings[i];
            Check(c.MinClosed() > 0.999, "all six units protected across road " + i.ToString());
            Check(c.OpeningClearance() > Route.Crossings[i].RoadHalfWidth + 0.5, "last locomotive and delayed client clear road before reopening " + i.ToString());
            Check(c.Phase() == 0 && !c.Alarm(), "crossing open after passage " + i.ToString());
        }
        Check(RP_ConvoyUnit.All().Count() == 6, "no duplicate rolling stock after complete first itinerary");
    }
    void Release()
    {
        if (m_Started && m_Persistence && !m_SaveFailed) SaveNow();
        m_Started = false;
        Done = true;
        for (int i = 0; i < Crossings.Count(); i++) Crossings[i].Release();
        Crossings.Clear(); m_RunOver = null;
        if (Service) Service.Stop(); Service = null;
        ReleaseRouteWorld();
    }
};

class RPFirstServerMission : MissionServer
{
    protected ref RPFirstRun m_Run;
    protected vector m_Observer;
    protected vector m_Look;
    protected float m_Sweep;

    protected bool ClearInfectedForTest() { return false; }

    override void OnMissionStart()
    {
        super.OnMissionStart(); m_Run = new RPFirstRun;
        if (!m_Run.Start(false, true)) { GetGame().RequestExit(1); return; }
        m_Observer = m_Run.Route.Observer;
        m_Observer[1] = GetGame().SurfaceY(m_Observer[0], m_Observer[2]) + 0.15;
        m_Look = m_Run.Route.LookAt - m_Observer; m_Look[1] = 0; m_Look.Normalize();
        if (ClearInfectedForTest()) RPStationEnvironment.ClearInfected(m_Observer);
        Print("[RailPilotGuided] READY: first route with persistent recovery; day only; restored clock=" + m_Run.Service.Clock().ToString());
    }
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        if (!m_Run || m_Run.Done || timeslice <= 0) return;
        // Service clock advances without players; no interactive test override.
        m_Run.Update(timeslice);
        if (!ClearInfectedForTest()) return;
        m_Sweep += timeslice;
        if (m_Sweep >= 1)
        {
            m_Sweep = 0; RPStationEnvironment.ClearInfected(m_Run.Service.Unit(0).GetPosition());
            array<Man> players = new array<Man>; GetGame().GetPlayers(players);
            for (int i = 0; i < players.Count(); i++) RPStationEnvironment.ClearInfected(players[i].GetPosition());
        }
    }
    override PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
    {
        vector spawn = m_Observer; vector direction = m_Look;
        if (identity) m_Run.Passengers().SpawnPose(identity.GetId(), spawn, direction);
        Entity entity = GetGame().CreatePlayer(identity, characterName, spawn, 0, "NONE");
        Class.CastTo(m_player, entity); GetGame().SelectPlayer(identity, m_player);
        if (!m_player) return null;
        m_player.SetAllowDamage(false); m_player.SetDirection(direction);
        if (identity) m_Run.Passengers().Restore(identity.GetId(), m_player, true);
        return m_player;
    }
    override void OnClientPrepareEvent(PlayerIdentity identity, out bool useDB, out vector pos, out float yaw, out int preloadTimeout)
    {
        useDB = false; pos = m_Observer; yaw = Math.Atan2(m_Look[0], m_Look[2]) * Math.RAD2DEG; preloadTimeout = 60;
        vector direction;
        if (identity && m_Run.Passengers().SpawnPose(identity.GetId(), pos, direction)) yaw = Math.Atan2(direction[0], direction[2]) * Math.RAD2DEG;
    }
    override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
    {
        super.OnClientReadyEvent(identity, player);
        if (identity && player && !player.FirstRecoveryIssued()) m_Run.Passengers().Restore(identity.GetId(), player, true);
    }
    override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
    {
        if (m_Run && identity && player && !authFailed)
        {
            m_Run.Passengers().Observe(identity.GetId(), player); m_Run.SaveNow();
        }
        super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
    }
    override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
    {
        super.OnClientRespawnEvent(identity, player);
        if (m_Run && identity) { m_Run.Passengers().Forget(identity.GetId()); m_Run.SaveNow(); }
    }
    override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
    {
        // Existing equipment helper retained. Owner deferred the occupied-hands bug.
        player.GetInventory().CreateInInventory("TShirt_Green");
        player.GetInventory().CreateInInventory("Jeans_Blue");
        player.GetInventory().CreateInInventory("AthleticShoes_Blue");
        RPStationEquipment.EmptyCan(player);
    }
    override void OnMissionFinish()
    {
        if (m_Run) m_Run.Release(); super.OnMissionFinish();
    }
};

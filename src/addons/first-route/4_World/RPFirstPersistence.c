// Alternating verified generations. Never truncates the current valid slot.
// Checksum detects accidental corruption; it is not a security signature.
class RPFirstPersistence
{
    protected string m_Directory;
    protected string m_LastSlot;
    protected int m_Generation;
    protected ref RPFirstRecoveryManifest m_Manifest;
    protected ref RPConvoyRoute m_Route;
    bool RecoveredFallback;
    bool Loaded;

    bool Configure(RPConvoyRoute route, string directory)
    {
        m_Route = route; m_Directory = directory;
        m_Manifest = RPFirstRecoveryManifest.Load();
        if (!m_Route || !m_Manifest) return false;
        MakeDirectory(m_Directory);
        return FileExist(m_Directory);
    }
    string Directory() { return m_Directory; }
    RPFirstRecoveryManifest Manifest() { return m_Manifest; }
    static string Digest(RPFirstRecoveryState state)
    {
        string old = state.Checksum; state.Checksum = "";
        string data; string error;
        bool ok = JsonFileLoader<RPFirstRecoveryState>.MakeData(state, data, error);
        state.Checksum = old;
        if (!ok) return "";
        if (state.Schema == 2)
        {
            RPFirstRecoveryStateV2 legacy;
            if (!JsonFileLoader<RPFirstRecoveryStateV2>.LoadData(data, legacy, error) || !legacy) return "";
            if (!JsonFileLoader<RPFirstRecoveryStateV2>.MakeData(legacy, data, error)) return "";
        }
        return data.Hash().ToString();
    }
    bool Read(string path, out RPFirstRecoveryState state)
    {
        if (!FileExist(path)) return false;
        state = new RPFirstRecoveryState; string error;
        if (!JsonFileLoader<RPFirstRecoveryState>.LoadFile(path, state, error)) return false;
        if (!state || state.Schema < 1 || state.Schema > 3 || state.Generation < 1 || state.Generation >= 2000000000 || state.Checksum == "") return false;
        if (state.Schema == 1)
        {
            RPFirstLegacyRecoveryState legacy; string legacyData;
            if (!JsonFileLoader<RPFirstLegacyRecoveryState>.LoadFile(path, legacy, error)) return false;
            string expected = legacy.Checksum; legacy.Checksum = "";
            if (!JsonFileLoader<RPFirstLegacyRecoveryState>.MakeData(legacy, legacyData, error) || legacyData.Hash().ToString() != expected) return false;
            // Old snapshots contain no passenger record. They never invent one.
            state.Passengers = new array<ref RPFirstPassengerState>;
            state.BlockedStop = -1; state.BlockedStopSeconds = 0;
            return true;
        }
        if (state.Schema == 2)
        {
            RPFirstRecoveryStateV2 legacyV2;
            string legacyDataV2;
            if (!JsonFileLoader<RPFirstRecoveryStateV2>.LoadFile(path, legacyV2, error) || !legacyV2) return false;
            string expectedV2 = legacyV2.Checksum;
            legacyV2.Checksum = "";
            if (!JsonFileLoader<RPFirstRecoveryStateV2>.MakeData(legacyV2, legacyDataV2, error) || legacyDataV2.Hash().ToString() != expectedV2) return false;
            state.BlockedStop = -1; state.BlockedStopSeconds = 0;
            return true;
        }
        return Digest(state) == state.Checksum;
    }
    bool Compatible(RPFirstRecoveryState state, out float clock)
    {
        if (!RPFirstPassengers.ValidRoster(state.Passengers)) return false;
        if (state.RouteId != m_Manifest.RouteId || state.ConvoyId != m_Manifest.ConvoyId || state.Map != m_Manifest.Map) return false;
        if (!state.Sections || state.Sections.Count() < 3 || state.Sections.Count() > m_Manifest.Sections.Count()) return false;
        // Appended sections are allowed. Changed/reordered existing sections
        // are refused; migration must be explicit in a future candidate.
        for (int i = 0; i < state.Sections.Count(); i++)
        {
            if (!state.Sections[i] || state.Sections[i].Id != m_Manifest.Sections[i].Id || state.Sections[i].Fingerprint != m_Manifest.Sections[i].Fingerprint) return false;
        }
        if (!RPFirstRecoveryPhases.Clock(m_Route, state, clock)) return false;
        if (!RPFirstRecoveryPhases.FiniteRange(state.BlockedStopSeconds, 0, RPConvoyRoute.FORCED_STOP_SECONDS)) return false;
        if (state.BlockedStop < -1 || state.BlockedStop >= m_Route.Stops.Count()) return false;
        if (state.BlockedStop < 0 && state.BlockedStopSeconds != 0) return false;
        if (state.BlockedStop >= 0 && m_Route.StationApproach(clock, 82) != state.BlockedStop) return false;
        if (!state.Gates) return false;
        int expectedGates = m_Route.Crossings.Count();
        while (expectedGates > 0 && m_Route.Crossings[expectedGates - 1].Segment >= state.Sections.Count()) expectedGates--;
        if (state.Gates.Count() != expectedGates) return false;
        for (int g = 0; g < state.Gates.Count(); g++)
        {
            RPFirstGateState gate = state.Gates[g];
            if (!gate || gate.Id != m_Route.Crossings[g].Name || gate.State < 0 || gate.State > 2 || gate.Direction != 1) return false;
            if (!RPFirstRecoveryPhases.FiniteRange(gate.Phase, 0, 1) || !RPFirstRecoveryPhases.FiniteRange(gate.PrewarningRemaining, 0, 5)) return false;
            if (gate.State == 0 && (gate.Phase != 0 || gate.PrewarningRemaining != 0)) return false;
            if (gate.PrewarningRemaining > 0 && (gate.State != 1 || gate.Phase != 0)) return false;
        }
        return true;
    }
    bool Load(out RPFirstRecoveryState state, out bool fresh, out float clock)
    {
        fresh = false; clock = 0;
        string aPath = m_Directory + "/State.A.json";
        string bPath = m_Directory + "/State.B.json";
        bool hasA = FileExist(aPath); bool hasB = FileExist(bPath);
        if (!hasA && !hasB)
        {
            if (FileExist(m_Directory + "/Service.started")) return false;
            fresh = true; Loaded = true; return true;
        }
        RPFirstRecoveryState a; RPFirstRecoveryState b;
        bool validA = Read(aPath, a); bool validB = Read(bPath, b);
        if (!validA && !validB) return false;
        if (validA && validB && a.Generation == b.Generation && a.Checksum != b.Checksum) return false;
        if (validA && (!validB || a.Generation >= b.Generation)) { state = a; m_LastSlot = aPath; }
        else { state = b; m_LastSlot = bPath; }
        // An intact but incompatible newest snapshot must not be replaced
        // by an older state from a different itinerary.
        if (!Compatible(state, clock)) return false;
        m_Generation = state.Generation;
        RecoveredFallback = (hasA && !validA) || (hasB && !validB);
        Loaded = true;
        if (RecoveredFallback) Print("[RailPilotRecovery] FALLBACK previous valid snapshot; recent progress may be lost");
        return true;
    }
    bool Save(RPFirstRecoveryState state)
    {
        if (!Loaded || !state || m_Generation >= 1999999999) return false;
        state.Schema = 3; state.Generation = m_Generation + 1;
        state.RouteId = m_Manifest.RouteId; state.ConvoyId = m_Manifest.ConvoyId;
        state.Map = m_Manifest.Map; state.Revision = m_Manifest.Revision;
        state.Sections = m_Manifest.Sections;
        float clock;
        if (!Compatible(state, clock)) return false;
        state.Checksum = Digest(state);
        string path = m_Directory + "/State.A.json";
        if (m_LastSlot == path) path = m_Directory + "/State.B.json";
        string error;
        if (!JsonFileLoader<RPFirstRecoveryState>.SaveFile(path, state, error)) return false;
        RPFirstRecoveryState verified;
        if (!Read(path, verified) || verified.Checksum != state.Checksum || !Compatible(verified, clock)) return false;
        m_LastSlot = path; m_Generation = verified.Generation;
        if (!FileExist(m_Directory + "/Service.started"))
        {
            FileHandle marker = OpenFile(m_Directory + "/Service.started", FileMode.WRITE);
            if (!marker) return false;
            FPrintln(marker, "RailPilot recovery initialized; missing snapshots require review."); CloseFile(marker);
        }
        return true;
    }
};

// Two alternating snapshots: never truncate the most recent valid slot.
// Not an atomic-rename or power-loss durability claim. On corruption, recover
// the previous verified slot or refuse startup; never silently reset a train.
class RPStateStore
{
    protected string m_Directory;
    protected string m_LastSlot;
    protected int m_Generation;

    void RPStateStore(string directory)
    {
        m_Directory = directory;
    }

    bool Load(out RPState state, out bool fresh)
    {
        fresh = false;
        string first = m_Directory + "/State.A.json";
        string second = m_Directory + "/State.B.json";
        bool hasFirst = FileExist(first);
        bool hasSecond = FileExist(second);
        if (!hasFirst && !hasSecond)
        {
            if (FileExist(m_Directory + "/State.json"))
            {
                Print("[RailPilot] Legacy State.json requires manual review; startup refused. Original file preserved.");
                return false;
            }
            state = new RPState;
            fresh = true;
            return true;
        }

        RPState a;
        RPState b;
        bool validA = ReadSlot(first, a);
        bool validB = ReadSlot(second, b);
        if (!validA && !validB)
        {
            Print("[RailPilot] No valid state snapshot; startup refused. Both files preserved.");
            return false;
        }
        if (validA && (!validB || a.Generation >= b.Generation))
        {
            state = a;
            m_LastSlot = first;
        }
        else
        {
            state = b;
            m_LastSlot = second;
        }
        m_Generation = state.Generation;
        if ((hasFirst && !validA) || (hasSecond && !validB))
            Print("[RailPilot] Recovered previous valid snapshot; recent progress may have been lost.");
        return true;
    }

    protected bool ReadSlot(string path, out RPState state)
    {
        if (!FileExist(path))
            return false;
        string error;
        state = new RPState;
        // Deserialization preserves initializer defaults for absent fields.
        // Sentinels prevent a truncated but syntactically valid snapshot passing.
        state.SchemaVersion = -1;
        state.Direction = 0;
        state.DistanceMeters = -1;
        state.SpeedMps = -1;
        state.DwellRemainingSeconds = -1;
        state.NextStopPointIndex = -2;
        state.Phase = -1;
        if (!JsonFileLoader<RPState>.LoadFile(path, state, error))
            return false;
        if (!state || state.SchemaVersion != 3 || state.Generation < 1 || state.Generation >= 2000000000 || state.RouteId == "")
            return false;
        bool directionValid = state.Direction == 1 || state.Direction == -1;
        if (!directionValid || !RPLimits.Flag(state.Phase) || state.NextStopPointIndex < 0 || state.NextStopPointIndex >= 50000)
            return false;
        return RPLimits.InRange(state.DistanceMeters, 0, 1000000) && RPLimits.InRange(state.SpeedMps, 0, 10) && RPLimits.InRange(state.DwellRemainingSeconds, 0, 3600);
    }

    bool Save(RPState state)
    {
        if (!state || state.RouteId == "" || m_Generation >= 1999999999)
            return false;
        string path = m_Directory + "/State.A.json";
        if (m_LastSlot == path)
            path = m_Directory + "/State.B.json";
        state.Generation = m_Generation + 1;
        string error;
        if (!JsonFileLoader<RPState>.SaveFile(path, state, error))
        {
            Print("[RailPilot] State write failed: " + error);
            return false;
        }
        RPState verified;
        string expected;
        string actual;
        if (!ReadSlot(path, verified))
            return false;
        if (!JsonFileLoader<RPState>.MakeData(state, expected, error) || !JsonFileLoader<RPState>.MakeData(verified, actual, error) || expected != actual)
            return false;
        m_Generation = verified.Generation;
        m_LastSlot = path;
        return true;
    }
};

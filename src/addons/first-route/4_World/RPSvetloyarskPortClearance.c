class RPSvetloyarskPortClearance
{
    static const int ACTIVATION_RPC = 1473623441;
    static const int REQUEST_RPC = 1473623442;
    static const int RPC_VERSION = 2;
    static const float CLIENT_RANGE = 600;
    protected ref array<vector> m_Origins = {"14114.150 5.070 13182.092", "14117.464 5.070 13170.439",
        "14120.896 5.070 13158.881", "14124.678 5.070 13147.435", "14129.393 5.612 13135.567"};
    protected ref array<vector> m_Positions = new array<vector>;
    protected ref array<EntityFlags> m_Flags = new array<EntityFlags>;
    protected bool m_Cleared;

    static bool NearPort(vector position)
    {
        return vector.Distance(position, "14122 5 13158") < CLIENT_RANGE;
    }

    bool Active() { return m_Cleared; }

    static bool ReadRequest(ParamsReadContext context)
    {
        int version;
        return context.Read(version) && version == RPC_VERSION;
    }

    protected Object Find(vector position, int index)
    {
        string model = "dz\\structures\\wrecks\\trains\\train_wagon_box.p3d";
        string stockType = "Land_Train_Wagon_Box";
        if (index == 4)
        {
            model = "dz\\structures\\wrecks\\trains\\train_742_blue.p3d";
            stockType = "Land_Train_742_Blue";
        }
        array<Object> objects = new array<Object>;
        array<CargoBase> cargo = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(position, 1.0, objects, cargo);
        foreach (Object candidate : objects)
        {
            if (!candidate || RP_ConvoyUnit.Cast(candidate) || vector.Distance(position, candidate.GetPosition()) >= 0.08) continue;
            // The moving consist shares these P3Ds and passes directly through the surveyed origins.
            if (candidate.GetType() != stockType) continue;
            string actual = candidate.GetShapeName();
            actual.ToLower();
            actual.Replace("/", "\\");
            if (actual == model) return candidate;
        }
        return null;
    }

    bool Clear()
    {
        if (m_Cleared) return true;
        string world;
        GetGame().GetWorldName(world);
        world.ToLower();
        if (world != "chernarusplus") return false;
        m_Positions.Clear();
        m_Flags.Clear();
        for (int i = 0; i < m_Origins.Count(); i++)
        {
            Object stock = Find(m_Origins[i], i);
            if (!stock) return false;
            m_Positions.Insert(stock.GetPosition());
            m_Flags.Insert(stock.GetFlags());
        }
        for (int j = 0; j < m_Positions.Count(); j++)
        {
            Object original = Find(m_Positions[j], j);
            if (!original) { Restore(); return false; }
            Hide(original, m_Positions[j]);
        }
        m_Cleared = true;
        return true;
    }

    protected void Hide(Object stock, vector original)
    {
        stock.SetPosition(original - "0 200 0");
        stock.ClearFlags(EntityFlags.VISIBLE | EntityFlags.SOLID, true);
    }

    void Refresh()
    {
        if (!m_Cleared) return;
        // The client can stream the terrain cell again after leaving the port.
        for (int i = 0; i < m_Positions.Count(); i++)
        {
            Object stock = Find(m_Positions[i], i);
            if (stock) Hide(stock, m_Positions[i]);
        }
    }

    void Restore()
    {
        for (int i = 0; i < m_Positions.Count(); i++)
        {
            Object original = Find(m_Positions[i] - "0 200 0", i);
            if (!original) continue;
            original.SetPosition(m_Positions[i]);
            original.SetFlags(m_Flags[i], true);
        }
        m_Positions.Clear();
        m_Flags.Clear();
        m_Cleared = false;
    }
};

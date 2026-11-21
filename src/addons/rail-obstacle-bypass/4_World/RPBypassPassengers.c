class RPBypassPassengers
{
    protected ref array<Man> m_Connected = new array<Man>;
    protected ref array<Object> m_Nearby = new array<Object>;
    protected ref array<CargoBase> m_Cargo = new array<CargoBase>;

    protected bool OnTrain(Object object, array<RP_ConvoyUnit> units)
    {
        Man person = Man.Cast(object);
        if (!person || !person.IsAlive()) return false;
        PlayerBase player = PlayerBase.Cast(person);
        RP_ConvoyUnit parent;
        RP_ConvoyUnit floor;
        vector position = person.GetPosition();
        if (player)
        {
            parent = RPFirstPassengers.ParentWagon(player);
            floor = RP_ConvoyUnit.Cast(player.PhysicsGetFloorEntity());
            position = player.PhysicsGetPositionWS();
        }
        if (parent && units.Find(parent) >= 0) return true;
        if (floor && units.Find(floor) >= 0) return true;

        // Keep riders in the bounds, on roofs and briefly in the air.
        foreach (RP_ConvoyUnit unit : units)
        {
            vector relative = unit.WorldToModel(position);
            vector low = unit.Low();
            vector high = unit.High();
            bool withinX = relative[0] >= low[0] - 0.35 && relative[0] <= high[0] + 0.35;
            bool withinZ = relative[2] >= low[2] - 0.35 && relative[2] <= high[2] + 0.35;
            bool withinY = relative[1] >= low[1] - 0.15 && relative[1] <= high[1] + 1.5;
            if (withinX && withinZ && withinY) return true;
        }
        return false;
    }

    bool HasRiders(array<RP_ConvoyUnit> units, float length)
    {
        m_Connected.Clear();
        GetGame().GetPlayers(m_Connected);
        foreach (Man player : m_Connected)
        {
            if (OnTrain(player, units)) return true;
        }
        m_Nearby.Clear();
        m_Cargo.Clear();
        GetGame().GetObjectsAtPosition3D(units[2].GetPosition(), length, m_Nearby, m_Cargo);
        foreach (Object object : m_Nearby)
        {
            if (OnTrain(object, units)) return true;
        }
        return false;
    }

    void Notify(array<RP_ConvoyUnit> units, string message)
    {
        m_Connected.Clear();
        GetGame().GetPlayers(m_Connected);
        foreach (Man person : m_Connected)
        {
            PlayerBase player = PlayerBase.Cast(person);
            if (!player || !player.GetIdentity() || !player.IsAlive()) continue;
            foreach (RP_ConvoyUnit unit : units)
            {
                if (vector.Distance(unit.GetPosition(), player.GetPosition()) <= 25)
                {
                    player.MessageStatus(message);
                    break;
                }
            }
        }
    }

    void NotifyRiders(array<RP_ConvoyUnit> units, string message)
    {
        m_Connected.Clear();
        GetGame().GetPlayers(m_Connected);
        foreach (Man person : m_Connected)
        {
            PlayerBase player = PlayerBase.Cast(person);
            if (player && player.GetIdentity() && OnTrain(player, units)) player.MessageStatus(message);
        }
    }
};

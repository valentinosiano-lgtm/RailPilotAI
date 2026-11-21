class RPStationSpec
{
    int Interactive;
    int Night;
    int NoInfected;
    float TestDwellSeconds;
};

class RPStationEquipment
{
    static ItemBase EmptyCan(PlayerBase player)
    {
        if (!player) return null;
        ItemBase can = ItemBase.Cast(player.GetHumanInventory().CreateInHands("CanisterGasoline"));
        if (can) can.SetQuantity(0);
        return can;
    }
};

class RPStationEnvironment
{
    static int ClearInfected(vector centre)
    {
        // This helper is used only by this opt-in isolated test mission.
        // No economy is initialized here; also remove any incidental spawn.
        array<Object> objects = new array<Object>; array<CargoBase> cargo = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D(centre, 500, objects, cargo);
        int removed;
        for (int i = 0; i < objects.Count(); i++)
        {
            ZombieBase infected = ZombieBase.Cast(objects.Get(i));
            if (infected) { GetGame().ObjectDelete(infected); removed++; }
        }
        return removed;
    }
};


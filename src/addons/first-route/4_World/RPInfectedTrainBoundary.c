class RPInfectedTrainBoundary
{
    static const float BODY_RADIUS = 0.30;
    static const float BODY_HEIGHT = 1.75;
    static const float UPDATE_INTERVAL = 0.05;
    protected float m_Elapsed;
    protected ref array<Object> m_Objects = new array<Object>;
    protected ref array<CargoBase> m_Cargo = new array<CargoBase>;

    static bool Intersects(RP_ConvoyUnit unit, vector foot)
    {
        if (!unit || !unit.PrepareBounds() || vector.DistanceSq(foot, unit.GetPosition()) > 100) return false;
        vector relative = unit.WorldToModel(foot);
        vector low = unit.Low();
        vector high = unit.High();
        bool across = relative[0] > low[0] - BODY_RADIUS && relative[0] < high[0] + BODY_RADIUS;
        bool along = relative[2] > low[2] - BODY_RADIUS && relative[2] < high[2] + BODY_RADIUS;
        return across && along && relative[1] < high[1] && relative[1] + BODY_HEIGHT > low[1];
    }

    protected bool TryShoulder(RP_ConvoyUnit unit, ZombieBase infected, vector relative, int side)
    {
        if (side < 0) relative[0] = unit.Low()[0] - BODY_RADIUS - 0.12;
        else relative[0] = unit.High()[0] + BODY_RADIUS + 0.12;
        vector target = unit.ModelToWorld(relative);
        vector hit;
        vector normal;
        Object obstruction;
        float fraction;
        // Accept native walkable support near the current height, including bridges and platforms.
        int support = PhxInteractionLayers.TERRAIN | PhxInteractionLayers.ROADWAY;
        if (!DayZPhysics.RayCastBullet(target + "0 0.65 0", target - "0 1.35 0", support, infected,
            obstruction, hit, normal, fraction)) return false;
        if (normal[1] < 0.65) return false;
        target[1] = hit[1] + 0.04;
        foreach (RP_ConvoyUnit other : RP_ConvoyUnit.All())
            if (Intersects(other, target)) return false;

        int layers = PhxInteractionLayers.BUILDING | PhxInteractionLayers.DOOR | PhxInteractionLayers.VEHICLE | PhxInteractionLayers.ITEM_LARGE;
        if (DayZPhysics.SphereCastBullet(target + "0 0.45 0", target + "0 1.4 0", BODY_RADIUS, layers,
            infected, obstruction, hit, normal, fraction)) return false;
        infected.SetPosition(target);
        return true;
    }

    protected void MoveOut(RP_ConvoyUnit unit, ZombieBase infected, vector relative, int side)
    {
        if (TryShoulder(unit, infected, relative, side) || TryShoulder(unit, infected, relative, -side)) return;
        relative[2] = relative[2] + 0.85;
        if (TryShoulder(unit, infected, relative, side) || TryShoulder(unit, infected, relative, -side)) return;
        relative[2] = relative[2] - 1.70;
        if (!TryShoulder(unit, infected, relative, side)) TryShoulder(unit, infected, relative, -side);
    }

    void Tick(RPConvoyService service, float dt)
    {
        m_Elapsed += dt;
        if (m_Elapsed < UPDATE_INTERVAL || !service.Unit(5)) return;
        m_Elapsed = 0;
        vector front = service.Unit(0).GetPosition();
        vector rear = service.Unit(5).GetPosition();
        m_Objects.Clear();
        m_Cargo.Clear();
        GetGame().GetObjectsAtPosition3D((front + rear) * 0.5, service.Length() * 0.5 + 2, m_Objects, m_Cargo);
        foreach (Object object : m_Objects)
        {
            ZombieBase infected = ZombieBase.Cast(object);
            if (!infected || !infected.IsAlive()) continue;
            vector foot = infected.GetPosition();
            foreach (RP_ConvoyUnit unit : RP_ConvoyUnit.All())
            {
                if (!Intersects(unit, foot)) continue;
                vector relative = unit.WorldToModel(foot);
                // Leave the moving leading face to the existing frontal impact sweep.
                if (unit.Slot() == service.LeadingSlot() && unit.Speed() > 0.05 && relative[2] > unit.High()[2] - 0.8) continue;
                int side = 1;
                if (relative[0] < (unit.Low()[0] + unit.High()[0]) * 0.5) side = -1;
                MoveOut(unit, infected, relative, side);
                break;
            }
        }
    }

    static bool BlocksAttack(ZombieBase infected, PlayerBase player)
    {
        RP_ConvoyUnit unit = RPFirstPassengers.ParentWagon(player);
        if (!unit) unit = RP_ConvoyUnit.Cast(player.PhysicsGetFloorEntity());
        if (!unit) return false;
        vector attacker = infected.GetPosition();
        if (Intersects(unit, attacker)) return true;

        set<Object> hits = new set<Object>;
        vector contact;
        vector normal;
        int component;
        vector playerPoint = player.PhysicsGetPositionWS() + "0 0.85 0";
        DayZPhysics.RaycastRV(attacker + "0 1.15 0", playerPoint, contact, normal, component,
            hits, player, infected, true, false, ObjIntersectGeom);
        foreach (Object blocker : hits)
            if (RP_ConvoyUnit.Cast(blocker)) return true;
        return false;
    }
};

modded class PlayerBase
{
    override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source,
        int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        ZombieBase infected = ZombieBase.Cast(source);
        if (GetGame().IsServer() && infected && RPInfectedTrainBoundary.BlocksAttack(infected, this)) return false;
        return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
    }
};

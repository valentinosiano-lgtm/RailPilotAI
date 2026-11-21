class RPObstacleSensor
{
    static const float SAMPLE_SPACING = 0.5;
    static const float SWEEP_MARGIN = 0.27;

    protected ref array<ref RPCCBox> m_Obstacles = new array<ref RPCCBox>;
    protected ref array<Object> m_Objects = new array<Object>;
    protected ref array<CargoBase> m_Cargo = new array<CargoBase>;
    protected ref array<RPCCBox> m_EventStock = new array<RPCCBox>;

    protected void AddBuiltParts(BaseBuildingBase building, vector pose[4])
    {
        Construction construction = building.GetConstruction();
        if (!construction) return;
        map<string, ref ConstructionPart> parts = construction.GetConstructionParts();
        if (!parts) return;
        foreach (string name, ConstructionPart part : parts)
        {
            if (!part || !part.IsBuilt()) continue;
            vector low;
            vector high;
            if (RPBCBounds.Part(building, part, low, high))
                m_Obstacles.Insert(RPCorridor.MakeBox(low, high, pose));
        }
    }

    protected bool IsTrainEventBody(Object object)
    {
        string type = object.GetType();
        if (!type.Contains("_DE")) return false;
        if (type.IndexOf("StaticObj_Wreck_Train_") == 0 || type.IndexOf("Land_Train_Wagon_") == 0) return true;
        if (type.IndexOf("StaticObj_Train_Wagon_") == 0 || type.IndexOf("Land_Container_1") == 0) return true;
        if (type == "StaticObj_Wreck_Ural_DE" || type == "StaticObj_Wreck_BMP2_DE") return true;
        return type == "StaticObj_Wreck_T72_Chassis_DE" || type == "StaticObj_Wreck_T72_Turret_DE";
    }

    protected void Collect(vector center, float radius, bool includeTrainEvents = false)
    {
        m_EventStock.Clear();
        m_Obstacles.Clear();
        m_Objects.Clear();
        m_Cargo.Clear();
        GetGame().GetObjectsAtPosition3D(center, radius, m_Objects, m_Cargo);
        foreach (Object object : m_Objects)
        {
            if (!object) continue;
            BaseBuildingBase building = BaseBuildingBase.Cast(object);
            bool container = Barrel_ColorBase.Cast(object) || SeaChest.Cast(object) || WoodenCrate.Cast(object);
            bool trainEvent = includeTrainEvents && IsTrainEventBody(object);
            if (!building && !container && !CarScript.Cast(object) && !trainEvent) continue;
            EntityAI entity = EntityAI.Cast(object);
            if (!trainEvent && !entity) continue;
            if (entity && (entity.GetHierarchyParent() || entity.IsHologram())) continue;
            vector pose[4];
            object.GetTransform(pose);
            if (building)
            {
                AddBuiltParts(building, pose);
                continue;
            }
            vector bounds[2];
            if (!object.GetCollisionBox(bounds)) continue;
            RPCCBox box = RPCorridor.MakeBox(bounds[0], bounds[1], pose);
            m_Obstacles.Insert(box);
            if (trainEvent) m_EventStock.Insert(box);
        }
    }

    protected bool Touches(RPConvoyRoute route, array<RP_ConvoyUnit> units, float at, int segment)
    {
        vector padding = Vector(SWEEP_MARGIN, SWEEP_MARGIN, SWEEP_MARGIN);
        foreach (RP_ConvoyUnit unit : units)
        {
            vector pose[4];
            RPFirstAlignment.BuildSegmentPose(route, unit.Slot(), at - unit.Offset(), segment, pose);
            RPCCBox train = RPCorridor.MakeBox(unit.Low() - padding, unit.High() + padding, pose);
            foreach (RPCCBox obstacle : m_Obstacles)
            {
                if (train.Intersects(obstacle)) return true;
            }
        }
        return false;
    }

    float Gap(RPConvoyRoute route, array<RP_ConvoyUnit> units, float distance, int segment, float horizon)
    {
        vector center = route.SegmentPoint(distance - units[5].Offset() * 0.5, segment);
        Collect(center, units[5].Offset() * 0.5 + horizon + 24, segment >= 4);
        if (m_Obstacles.Count() == 0) return -1;

        float direction = 1;
        float available = route.TrackRemaining(distance, segment);
        if (segment == 1 || segment == 3) direction = -1;
        horizon = Math.Clamp(horizon, 0, Math.Max(available, 0));
        if (Touches(route, units, distance, segment)) return 0;

        // Anchor samples in route coordinates so a stationary obstruction
        // does not move within the sample grid as the train approaches it.
        float grid = (Math.Floor(distance / SAMPLE_SPACING) + 1) * SAMPLE_SPACING;
        if (direction < 0) grid = (Math.Ceil(distance / SAMPLE_SPACING) - 1) * SAMPLE_SPACING;
        float first = (grid - distance) * direction;
        float previous = 0;
        float advance = Math.Min(first, horizon);
        while (advance > previous)
        {
            if (Touches(route, units, distance + direction * advance, segment))
            {
                // Return the clear side of the contact bracket. The original
                // stand-off is applied afterwards by the motion controller.
                float blocked = advance;
                for (int i = 0; i < 5; i++)
                {
                    float mid = (previous + blocked) * 0.5;
                    if (Touches(route, units, distance + direction * mid, segment)) blocked = mid;
                    else previous = mid;
                }
                return previous;
            }
            previous = advance;
            advance = Math.Min(advance + SAMPLE_SPACING, horizon);
        }
        return -1;
    }
};

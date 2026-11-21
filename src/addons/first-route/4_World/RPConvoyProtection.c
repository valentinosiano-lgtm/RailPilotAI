// Local preview + authoritative checks at action completion. Restrictions are
// limited to this consist, including space beside doors and the coupled gaps.
class RPConvoyProtection
{
    static bool Riding(PlayerBase player)
    {
        if (!player) return false;
        if (RP_ConvoyUnit.Cast(player.PhysicsGetLinkedEntity())) return true;
        if (RP_ConvoyUnit.Cast(player.PhysicsGetFloorEntity())) return true;
        array<RP_ConvoyUnit> units = RP_ConvoyUnit.All();
        for (int i = 0; i < units.Count(); i++)
            if (units[i] && units[i].Contains(player.GetPosition(), 0.2)) return true;
        return false;
    }

    static bool Overlaps(Object object, vector position, vector orientation)
    {
        if (!object) return false;
        vector bounds[2];
        if (!object.GetCollisionBox(bounds)) { bounds[0] = "-0.5 -0.5 -0.5"; bounds[1] = "0.5 0.5 0.5"; }
        vector lo = bounds[0]; vector hi = bounds[1];
        vector transform[4]; Math3D.YawPitchRollMatrix(orientation, transform); transform[3] = position;
        array<RP_ConvoyUnit> units = RP_ConvoyUnit.All();
        for (int i = 0; i < units.Count(); i++)
        {
            RP_ConvoyUnit unit = units[i];
            if (!unit || !unit.PrepareBounds()) continue;
            vector minLocal = "100000 100000 100000";
            vector maxLocal = "-100000 -100000 -100000";
            for (int corner = 0; corner < 8; corner++)
            {
                vector p = lo;
                if ((corner & 1) != 0) p[0] = hi[0];
                if ((corner & 2) != 0) p[1] = hi[1];
                if ((corner & 4) != 0) p[2] = hi[2];
                vector localPoint = unit.WorldToModel(p.Multiply4(transform));
                for (int axis = 0; axis < 3; axis++)
                {
                    minLocal[axis] = Math.Min(minLocal[axis], localPoint[axis]);
                    maxLocal[axis] = Math.Max(maxLocal[axis], localPoint[axis]);
                }
            }
            vector ul = unit.Low(); vector uh = unit.High();
            // Includes side-door approaches, roof and underfloor fence posts.
            if (minLocal[0] <= uh[0] + 2 && maxLocal[0] >= ul[0] - 2 && minLocal[1] <= uh[1] + 3 && maxLocal[1] >= ul[1] - 1.5 && minLocal[2] <= uh[2] + 1 && maxLocal[2] >= ul[2] - 1) return true;
        }
        return false;
    }

    static bool BuildBlocked(PlayerBase player, ActionTarget target)
    {
        if (Riding(player)) return true;
        if (!target || !target.GetObject()) return false;
        Object object = target.GetObject();
        return Overlaps(object, object.GetPosition(), object.GetOrientation());
    }

    static bool DeploymentBlocked(ActionData data)
    {
        if (!data || !data.m_Player) return true;
        if (Riding(data.m_Player)) return true;
        PlaceObjectActionData place = PlaceObjectActionData.Cast(data);
        if (!place || !place.m_MainItem) return false;
        Hologram hologram = data.m_Player.GetHologramServer();
        Object object = place.m_MainItem;
        if (hologram && hologram.GetProjectionEntity()) object = hologram.GetProjectionEntity();
        vector position = place.m_Position;
        vector orientation = place.m_Orientation;
        ActionDeployBase action = ActionDeployBase.Cast(data.m_Action);
        if (action && !action.ActionUsesHologram())
        {
            position = data.m_Player.GetPosition() + data.m_Player.GetDirection() * 0.5;
            orientation = data.m_Player.GetOrientation();
        }
        return Overlaps(object, position, orientation);
    }
};

modded class Hologram
{
    override void EvaluateCollision(ItemBase action_item = null)
    {
        super.EvaluateCollision(action_item);
        Object projection = GetProjectionEntity();
        if (RPConvoyProtection.Riding(m_Player) || (projection && RPConvoyProtection.Overlaps(projection, projection.GetPosition(), projection.GetOrientation()))) SetIsColliding(true);
    }
};

modded class ActionDeployBase
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (RPConvoyProtection.DeploymentBlocked(action_data)) return;
        super.OnFinishProgressServer(action_data);
    }
};

modded class ActionBuildPart
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (RPConvoyProtection.BuildBlocked(player, target)) return false;
        return super.ActionCondition(player, target, item);
    }
    override bool ActionConditionContinue(ActionData action_data)
    {
        if (RPConvoyProtection.BuildBlocked(action_data.m_Player, action_data.m_Target)) return false;
        return super.ActionConditionContinue(action_data);
    }
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (RPConvoyProtection.BuildBlocked(action_data.m_Player, action_data.m_Target)) return;
        super.OnFinishProgressServer(action_data);
    }
};

// Covers instant/debug building and other native callers as well as the UI.
modded class Construction
{
    override void BuildPartServerEx(Man player, string part_name, int action_id)
    {
        if (RPConvoyProtection.Riding(PlayerBase.Cast(player))) return;
        if (m_Parent && RPConvoyProtection.Overlaps(m_Parent, m_Parent.GetPosition(), m_Parent.GetOrientation())) return;
        super.BuildPartServerEx(player, part_name, action_id);
    }
};

modded class ActionCloseDoors
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (target && RP_ConvoyUnit.Cast(target.GetObject())) return false;
        return super.ActionCondition(player, target, item);
    }
    override void OnStartServer(ActionData action_data)
    {
        if (action_data.m_Target && RP_ConvoyUnit.Cast(action_data.m_Target.GetObject())) return;
        super.OnStartServer(action_data);
    }
};

modded class ActionLockDoors
{
    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (target && RP_ConvoyUnit.Cast(target.GetObject())) return false;
        return super.ActionCondition(player, target, item);
    }
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (action_data.m_Target && RP_ConvoyUnit.Cast(action_data.m_Target.GetObject())) return;
        super.OnFinishProgressServer(action_data);
    }
};

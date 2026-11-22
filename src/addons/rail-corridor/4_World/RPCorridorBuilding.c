// Keep all vanilla checks; our restriction remains effective with vanilla
// collision checks disabled. No dismantle, damage, map cleanup or loot changes.
modded class RPConvoyProtection
{
    // Replace the approved consist's wider moving exclusion only inside
    // this opt-in candidate. Holograms, deployment and parts are checked
    // below against the same static swept volume. Riding() is unchanged.
    override static bool Overlaps(Object object,vector position,vector orientation)
    {
        return false;
    }
    override static bool BuildBlocked(PlayerBase player,ActionTarget target)
    {
        return Riding(player);
    }
};
modded class ActionDeployBase
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        PlaceObjectActionData place=PlaceObjectActionData.Cast(action_data);
        if (place && place.m_Player && place.m_MainItem)
        {
            Hologram hologram=place.m_Player.GetHologramServer();
            if (ActionUsesHologram() && hologram && hologram.RPCorridorBlocked()) return;
            if (!ActionUsesHologram())
            {
                vector bounds[2];vector pose[4];
                if (place.m_MainItem.GetCollisionBox(bounds))
                {
                    Math3D.YawPitchRollMatrix(place.m_Player.GetOrientation(),pose);
                    pose[3]=place.m_Player.GetPosition()+place.m_Player.GetDirection()*0.5;
                    if (RPCorridor.Blocks(RPCorridor.MakeBox(bounds[0],bounds[1],pose))) return;
                }
            }
        }
        super.OnFinishProgressServer(action_data);
    }
};
modded class Hologram
{
    bool RPCorridorBlocked()
    {
        if (!m_Projection) return false;
        vector bounds[2];GetProjectionCollisionBox(bounds);
        vector pose[4];m_Projection.GetTransform(pose);
        return RPCorridor.Blocks(RPCorridor.MakeBox(bounds[0],bounds[1],pose));
    }
    override void EvaluateCollision(ItemBase action_item=null)
    {
        super.EvaluateCollision(action_item);
        if (RPCorridorBlocked()) SetIsColliding(true);
    }
};
// The vanilla Construction subclass emits build events after its base call.
// Reject before entering it, so an aborted base cannot still emit those events.
modded class Construction
{
    override void BuildPartServerEx(Man player,string part_name,int action_id)
    {
        if (RPCorridorPartBlocked(part_name)) return;
        super.BuildPartServerEx(player,part_name,action_id);
    }
};
modded class ActionDeployObject
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (action_data && action_data.m_Player)
        {
            Hologram hologram=action_data.m_Player.GetHologramServer();
            if (hologram && hologram.RPCorridorBlocked())
            {
                action_data.m_Player.MessageStatus("You cannot place objects in the railway corridor.");
                return;
            }
        }
        super.OnFinishProgressServer(action_data);
    }
};
modded class ConstructionBase
{
    bool RPCorridorPartBlocked(string partName)
    {
        if (!m_EntityParent) return false;
        ConstructionPart part=GetConstructionPart(partName);
        if (!part) return false;
        vector bounds[2];vector pose[4];m_EntityParent.GetTransform(pose);
        array<string> data=part.GetCollisionData();
        if (data && data.Count()==2 && m_EntityParent.MemoryPointExists(data[0]) && m_EntityParent.MemoryPointExists(data[1]))
            GetCollisionBoxData(partName,bounds);
        else if (!m_EntityParent.GetCollisionBox(bounds)) return false;
        vector.SanitizeMinsMaxs(bounds[0],bounds[1]);
        return RPCorridor.Blocks(RPCorridor.MakeBox(bounds[0],bounds[1],pose));
    }
    override bool IsCollidingEx(CollisionCheckData check_data)
    {
        if (check_data && RPCorridorPartBlocked(check_data.m_PartName)) return true;
        return super.IsCollidingEx(check_data);
    }
    override bool CanBuildPart(string part_name,ItemBase tool,bool use_tool)
    {
        if (RPCorridorPartBlocked(part_name)) return false;
        return super.CanBuildPart(part_name,tool,use_tool);
    }
    override void BuildPartServerEx(Man player,string part_name,int action_id)
    {
        if (RPCorridorPartBlocked(part_name)) return;
        super.BuildPartServerEx(player,part_name,action_id);
    }
};

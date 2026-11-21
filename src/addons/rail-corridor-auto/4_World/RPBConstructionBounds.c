// Bounds are measured via public native model/config interfaces. No BBP
// source, assets or private fields are used. The corridor itself is unchanged.
class RPBCPartBounds
{
    string PartName;
    vector Low;
    vector High;
};
class RPBCModelBounds
{
    string ObjectType;
    string HologramType;
    vector Low;
    vector High;
    ref array<ref RPBCPartBounds> Parts;
};
class RPBCBoundsData
{
    int Schema;
    ref array<ref RPBCModelBounds> Items;
};
class RPBCBounds
{
    protected static ref RPBCBoundsData s_Data;
    protected static bool s_Attempted;
    static bool Load()
    {
        if (s_Attempted) return s_Data!=null;
        s_Attempted=true;string error;
        if (!JsonFileLoader<RPBCBoundsData>.LoadFile("RailPilot\\RailCorridorAutomatic\\planned-bounds.json",s_Data,error))
        { Print("[RailPilotConstruction] ERROR bounds "+error);s_Data=null; }
        return s_Data && s_Data.Schema==1;
    }
    static RPBCModelBounds Find(string type,bool projection=false)
    {
        if (!Load()) return null;
        type.ToLower();
        foreach (RPBCModelBounds item:s_Data.Items)
        {
            string candidate=item.ObjectType;if (projection) candidate=item.HologramType;
            candidate.ToLower();if (candidate==type) return item;
        }
        return null;
    }
    static bool Part(Object object,ConstructionPart part,out vector low,out vector high)
    {
        array<string> data=part.GetCollisionData();
        if (data && data.Count()==2 && object.MemoryPointExists(data[0]) && object.MemoryPointExists(data[1]))
        { low=object.GetMemoryPointPos(data[0]);high=object.GetMemoryPointPos(data[1]);vector.SanitizeMinsMaxs(low,high);return true; }
        string name=part.GetPartName();
        if (object.MemoryPointExists(name+"_min") && object.MemoryPointExists(name+"_max"))
        { low=object.GetMemoryPointPos(name+"_min");high=object.GetMemoryPointPos(name+"_max");vector.SanitizeMinsMaxs(low,high);return true; }
        RPBCModelBounds known=Find(object.GetType());
        if (known)
        {
            low=known.Low;high=known.High;
            if (known.Parts) foreach (RPBCPartBounds overrideBounds:known.Parts)
            {
                if (overrideBounds.PartName==name) { low=overrideBounds.Low;high=overrideBounds.High;break; }
            }
            return true;
        }
        vector bounds[2];
        if (object.GetCollisionBox(bounds)) { low=bounds[0];high=bounds[1];return true; }
        if (object.MemoryPointExists("boundingbox_min") && object.MemoryPointExists("boundingbox_max"))
        { low=object.GetMemoryPointPos("boundingbox_min");high=object.GetMemoryPointPos("boundingbox_max");vector.SanitizeMinsMaxs(low,high);return true; }
        return false;
    }
};
modded class ConstructionBase
{
    override bool CanBuildPartEx(string partName,ItemBase tool,out EConstructionInfoCategories failureReasonsMask)
    {
        bool allowed=super.CanBuildPartEx(partName,tool,failureReasonsMask);
        if (RPCorridorPartBlocked(partName)) return false;
        return allowed;
    }
    override bool RPCorridorPartBlocked(string partName)
    {
        if (!m_EntityParent) return false;
        ConstructionPart part=GetConstructionPart(partName);if (!part) return false;
        vector low;vector high;
        if (!RPBCBounds.Part(m_EntityParent,part,low,high)) return super.RPCorridorPartBlocked(partName);
        vector pose[4];m_EntityParent.GetTransform(pose);
        return RPCorridor.Blocks(RPCorridor.MakeBox(low,high,pose));
    }
};
modded class Hologram
{
    override bool RPCorridorBlocked()
    {
        if (!m_Projection) return false;
        RPBCModelBounds known=RPBCBounds.Find(m_Projection.GetType(),true);
        if (!known) return super.RPCorridorBlocked();
        vector pose[4];m_Projection.GetTransform(pose);
        return RPCorridor.Blocks(RPCorridor.MakeBox(known.Low,known.High,pose));
    }
};
modded class ActionBuildPart
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        BuildPartActionData build=BuildPartActionData.Cast(action_data);
        if (build && build.m_Target)
        {
            EntityAI object=EntityAI.Cast(build.m_Target.GetObject());
            ConstructionBase construction;
            if (object) construction=ConstructionBase.Cast(object.GetConstructionBasic());
            if (construction && construction.RPCorridorPartBlocked(build.m_PartType)) return;
        }
        super.OnFinishProgressServer(action_data);
    }
};
// Guard the native dispatcher before a mod's leaf callback can apply tool
// wear after a refused construction. No compensation/refund of health.
modded class ActionContinuousBase
{
    override void OnFinishProgress(ActionData action_data)
    {
        BuildPartActionData build=BuildPartActionData.Cast(action_data);
        if (build && ActionBuildPart.Cast(build.m_Action) && build.m_Target)
        {
            EntityAI object=EntityAI.Cast(build.m_Target.GetObject());
            ConstructionBase construction;
            if (object) construction=ConstructionBase.Cast(object.GetConstructionBasic());
            if (construction && construction.RPCorridorPartBlocked(build.m_PartType))
            { action_data.m_WasExecuted=false;return; }
        }
        super.OnFinishProgress(action_data);
    }
};

class RPConvoyFuelReserve
{
    // Process/session lifetime, deliberately NOT stored in persistence.
    // Recreating the convoy during this session cannot reset the allowance.
    protected static float s_Remaining = RPConvoySpec.FUEL_ML;

    static float Remaining() { return s_Remaining; }

    static float Take(float requested)
    {
        if (!GetGame().IsServer() || requested <= 0) return 0;
        float amount = Math.Min(requested, s_Remaining);
        s_Remaining = Math.Max(0, s_Remaining - amount);
        return amount;
    }
};

class RP_ConvoyTanker : RP_ConvoyUnit
{
    protected float m_FuelML;

    void RP_ConvoyTanker()
    {
        RegisterNetSyncVariableFloat("m_FuelML", 0, 100000, 1);
    }

    void RefreshFuel()
    {
        if (!GetGame().IsServer()) return;
        m_FuelML = RPConvoyFuelReserve.Remaining(); SetSynchDirty();
    }

    float FuelML()
    {
        if (GetGame().IsServer()) return RPConvoyFuelReserve.Remaining();
        return m_FuelML;
    }

    bool CanDraw(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!Stopped() || FuelML() <= 0 || !player || !target || !item || item.IsRuined()) return false;
        if (target.GetObject() != this || !item.IsInherited(Bottle_Base)) return false;
        if (item.GetQuantity() >= item.GetQuantityMax()) return false;
        if (!Liquid.CanFillContainer(item, LIQUID_GASOLINE)) return false;
        vector hit = target.GetCursorHitPos();
        if (vector.Distance(player.GetPosition(), hit) > UAMaxDistances.DEFAULT) return false;
        // A cursor position must also belong to the actual tank body.
        if (!Contains(hit, 0.25)) return false;
        return true;
    }

    float Draw(PlayerBase player, ActionTarget target, ItemBase item, float requested)
    {
        if (!GetGame().IsServer() || !CanDraw(player, target, item)) return 0;
        float amount = Math.Min(requested, item.GetQuantityMax() - item.GetQuantity());
        amount = RPConvoyFuelReserve.Take(amount);
        if (amount <= 0) return 0;
        Liquid.FillContainerEnviro(item, LIQUID_GASOLINE, amount);
        RefreshFuel();
        return amount;
    }
};

class RPConvoyFuelComponent : CAContinuousBase
{
    protected float m_Pending;
    protected float m_Time;
    protected float m_Filled;
    protected float m_Target;
    protected float m_Rate;

    override void Setup(ActionData action_data)
    {
        m_Pending = 0; m_Time = 0; m_Filled = 0;
        m_Target = action_data.m_MainItem.GetQuantityMax() - action_data.m_MainItem.GetQuantity();
        RP_ConvoyTanker tank = RP_ConvoyTanker.Cast(action_data.m_Target.GetObject());
        if (tank) m_Target = Math.Min(m_Target, tank.FuelML());
        m_Rate = UAQuantityConsumed.FUEL * action_data.m_MainItem.GetLiquidThroughputCoef();
    }

    override int Execute(ActionData action_data)
    {
        RP_ConvoyTanker tank = RP_ConvoyTanker.Cast(action_data.m_Target.GetObject());
        if (!tank || !tank.CanDraw(action_data.m_Player, action_data.m_Target, action_data.m_MainItem)) return UA_FINISHED;
        float dt = action_data.m_Player.GetDeltaT();
        m_Pending += m_Rate * dt; m_Time += dt;
        if (m_Time >= 0.25)
        {
            float amount = Math.Min(m_Pending, m_Target - m_Filled);
            if (GetGame().IsServer()) amount = tank.Draw(action_data.m_Player, action_data.m_Target, action_data.m_MainItem, amount);
            m_Filled += amount; m_Pending = 0; m_Time = 0;
            if (amount <= 0 || m_Filled >= m_Target)
            {
                OnCompletePogress(action_data); return UA_FINISHED;
            }
        }
        return UA_PROCESSING;
    }

    override int Cancel(ActionData action_data)
    {
        // No delayed/residual delivery after the train starts moving.
        m_Pending = 0;
        return UA_CANCEL;
    }

    override float GetProgress()
    {
        if (m_Target <= 0) return 1;
        return Math.Clamp(m_Filled / m_Target, 0, 1);
    }
};

class ActionRPConvoyFuelCB : ActionContinuousBaseCB
{
    override void CreateActionComponent() { m_ActionData.m_ActionComponent = new RPConvoyFuelComponent; }
};

class ActionRPConvoyFuel : ActionContinuousBase
{
    void ActionRPConvoyFuel()
    {
        m_CallbackClass = ActionRPConvoyFuelCB;
        m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_FILLBOTTLEWELL;
        m_FullBody = true;
        m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
        m_Text = "Preleva benzina dalla cisterna";
    }

    override void CreateConditionComponents()
    {
        m_ConditionItem = new CCINonRuined;
        m_ConditionTarget = new CCTCursor;
    }

    override bool ActionCondition(PlayerBase player, ActionTarget target, ItemBase item)
    {
        if (!target) return false;
        RP_ConvoyTanker tank = RP_ConvoyTanker.Cast(target.GetObject());
        return tank && tank.CanDraw(player, target, item);
    }

    override bool ActionConditionContinue(ActionData action_data)
    {
        return ActionCondition(action_data.m_Player, action_data.m_Target, action_data.m_MainItem);
    }

    override bool IsLockTargetOnUse() { return false; }
};

modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions); actions.Insert(ActionRPConvoyFuel);
    }
};

modded class Bottle_Base
{
    override void SetActions()
    {
        super.SetActions(); AddAction(ActionRPConvoyFuel);
    }
};

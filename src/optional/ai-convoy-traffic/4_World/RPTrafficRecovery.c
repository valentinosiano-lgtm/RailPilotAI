// Original addon code, using names/types observed through runtime metadata.
// Specific to the exact original 1.4.1 tested by the isolated runner.
// No original script bodies or assets are copied into this module.
class RPTrafficRecovery
{
    static AIConvoyManager Manager;

    static AIConvoyVehicleUnit FindActiveUnit(CarScript target)
    {
        if (!Manager || !target) return null;
        array<ref AIConvoySlot> slots = Manager.RPRecoverySlots();
        if (!slots) return null;
        foreach (AIConvoySlot slot: slots)
        {
            if (!slot) continue;
            AIConvoyEvent convoyEvent = slot.RPRecoveryEvent();
            if (!convoyEvent || !convoyEvent.RPRecoveryDriving()) continue;
            array<ref AIConvoyVehicleUnit> units = convoyEvent.RPRecoveryUnits();
            if (!units) continue;
            foreach (AIConvoyVehicleUnit unit: units)
            {
                if (unit && unit.RPRecoveryVehicle() == target) return unit;
            }
        }
        return null;
    }

    static bool ResetFor(CarScript target)
    {
        AIConvoyVehicleUnit unit = FindActiveUnit(target);
        if (!unit) return false;
        return unit.RPResetTrafficBlock(target);
    }
};

modded class AIConvoyManager
{
    void AIConvoyManager() { RPTrafficRecovery.Manager = this; }
    array<ref AIConvoySlot> RPRecoverySlots() { return m_Slots; }
};

modded class AIConvoySlot
{
    AIConvoyEvent RPRecoveryEvent() { return m_Event; }
};

modded class AIConvoyEvent
{
    bool RPRecoveryDriving() { return m_State == STATE_DRIVING; }
    array<ref AIConvoyVehicleUnit> RPRecoveryUnits() { return m_Units; }
};

modded class AIConvoyVehicleUnit
{
    Transport RPRecoveryVehicle() { return m_Vehicle; }
    bool RPResetTrafficBlock(CarScript target)
    {
        if (m_Vehicle != target) return false;
        m_StuckTime = 0;
        m_StuckStrikes = 0;
        return true;
    }
};

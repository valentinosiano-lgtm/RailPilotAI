// Native houses have attachment inventory, not item cargo. A fixed invisible
// native container supplies the grid without replacing the approved wagon.
class RP_ConvoyCargoStore : Container_Base
{
    override void EEInit()
    {
        super.EEInit();
        SetInvisible(true);
        SetAllowDamage(false);
    }
    override bool CanPutInCargo(EntityAI parent) { return false; }
    override bool CanPutIntoHands(EntityAI parent) { return false; }
};

modded class RP_ConvoyBox
{
    override void EEInit()
    {
        super.EEInit();
        if (GetGame().IsServer())
        {
            GameInventory inventory = GetInventory();
            if (!FindAttachmentBySlotName("RP_ConvoyCargo"))
            {
                // Use the engine's explicit persistence flags. No cargo state
                // is written to train snapshots or the world/character store.
                int flags = ECE_IN_INVENTORY | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD | ECE_NOPERSISTENCY_CHAR;
                EntityAI store = EntityAI.Cast(GetGame().CreateObjectEx("RP_ConvoyCargoStore", GetPosition(), flags));
                int slot = InventorySlots.GetSlotIdFromString("RP_ConvoyCargo");
                if (store && !inventory.TakeEntityAsTargetAttachmentEx(InventoryMode.LOCAL, this, store, slot))
                {
                    GetGame().ObjectDelete(store);
                    Print("[RailPilotCargo] ERROR session-only cargo could not attach to wagon");
                }
            }
        }
    }
    RP_ConvoyCargoStore CargoStore() { return RP_ConvoyCargoStore.Cast(FindAttachmentBySlotName("RP_ConvoyCargo")); }
    override bool IsInventoryVisible() { return true; }
    override bool IsDetectableByVicinityCone() { return true; }
    override bool DisableVicinityIcon() { return false; }
    override bool CanDisplayCargo() { return GetInventory().GetCargo() != null; }
    override bool CanPutInCargo(EntityAI parent) { return false; }
    override bool CanPutIntoHands(EntityAI parent) { return false; }
    override bool CanReleaseAttachment(EntityAI attachment)
    {
        if (RP_ConvoyCargoStore.Cast(attachment)) return false;
        return super.CanReleaseAttachment(attachment);
    }

};

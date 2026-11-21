// Applies the railway restriction through vanilla interfaces. No BBP
// dependency, macros, classes, settings changes or load-list selection.
modded class Hologram
{
    override bool IsColliding()
    {
        if (RPCorridorBlocked()) return true;
        return super.IsColliding();
    }
};
modded class ActionDeployBase
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (action_data && action_data.m_Player)
        {
            Hologram hologram=action_data.m_Player.GetHologramServer();
            if (hologram && hologram.RPCorridorBlocked()) return;
        }
        super.OnFinishProgressServer(action_data);
    }
};
modded class ActionDeployObject
{
    override void OnFinishProgressServer(ActionData action_data)
    {
        if (action_data && action_data.m_Player)
        {
            Hologram hologram=action_data.m_Player.GetHologramServer();
            if (hologram && hologram.RPCorridorBlocked()) return;
        }
        super.OnFinishProgressServer(action_data);
    }
};

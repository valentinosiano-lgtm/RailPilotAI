// The audit found no validated rideable entity. Keep the controller headless.
// Asset inspection belongs to the isolated test mission, never to gameplay.
class RPVisualAdapter
{
    bool Start(RPSettings settings)
    {
        if (settings.EnableDebugVisual == 1)
        {
            Print("[RailPilot] Moving-wreck visual refused: a safe mobile entity has not been validated. Use the private asset probe.");
            return false;
        }
        return true;
    }

    void Stop()
    {
        // No entity exists in the headless prototype.
    }
};

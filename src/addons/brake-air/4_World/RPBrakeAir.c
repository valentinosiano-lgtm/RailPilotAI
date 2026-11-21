modded class RPConvoyEffects
{
    protected ref RPConvoySound m_BrakeAir;
    protected float m_AirRetryAt;

    bool BrakeAirPlaying() { return m_BrakeAir != null; }
    void UpdateBrakeAir(Object loco, bool enabled)
    {
        if (GetGame().IsDedicatedServer()) return;
        if (!enabled || !loco) { StopBrakeAir(); return; }
        if (!m_BrakeAir)
        {
            float now = GetGame().GetTime() / 1000.0;
            if (now < m_AirRetryAt) return;
            m_BrakeAir = new RPConvoySound;
            m_BrakeAir.SetSoundSet("RP_BrakeAir_SoundSet");
            m_BrakeAir.SetPosition(loco.GetPosition());
            m_BrakeAir.SetSoundLoop(true); m_BrakeAir.SetSoundAutodestroy(false);
            m_BrakeAir.SetSoundMaxVolume(0.35); m_BrakeAir.SetSoundVolume(0.35);
            SEffectManager.EffectRegister(m_BrakeAir);
            if (!m_BrakeAir.SoundPlay())
            {
                StopBrakeAir(); m_AirRetryAt = now + 5;
                Print("[RailPilotBrakeAir] ERROR sound start failed; retry in5seconds");
                return;
            }
        }
        // RPConvoySound updates the actual native wave's world position too.
        m_BrakeAir.SetCurrentPosition(loco.GetPosition());
    }
    void StopBrakeAir()
    {
        if (!m_BrakeAir) return;
        m_BrakeAir.StopImmediately(); SEffectManager.DestroyEffect(m_BrakeAir);
        m_BrakeAir = null;
    }
    override void Release()
    {
        StopBrakeAir(); m_AirRetryAt = 0; super.Release();
    }
};
modded class RP_ConvoyUnit
{
    bool BrakeAirRequested(float clock)
    {
        RPConvoyRoute route = Route();
        if (!m_ServiceEnabled || !route || (m_Slot != 0 && m_Slot != 5)) return false;
        float distance; float speed; int direction;
        route.State(clock, distance, speed, direction);
        if (m_Slot != RPConvoySpec.LeadingSlot(direction)) return false;
        if (GetGame().IsMultiplayer() && m_Slot != m_LeadingSlot) return false;
        return RPBrakeAirState.Active(route, clock);
    }
    override void Present(float clock, float dt)
    {
        super.Present(clock, dt);
        if (!GetGame().IsDedicatedServer() && m_Effects)
            m_Effects.UpdateBrakeAir(this, BrakeAirRequested(clock));
    }
};

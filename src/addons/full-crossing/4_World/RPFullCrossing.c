// New long-arm variant shares the approved renderer, light and collider logic.
class Land_rp_full_crossing_barrier : Land_rp_crossing_barrier
{
    override void ClientUpdate(float dt)
    {
        if (!s_MissionEnding && !GetGame().IsDedicatedServer() && m_ClientInitialized && !m_LocalVisual)
        {
            m_LocalVisual = Entity.Cast(GetGame().CreateObjectEx("RP_FullCrossingBarrierVisual", GetPosition(), ECE_LOCAL | ECE_KEEPHEIGHT));
            if (!m_LocalVisual) return;
            m_LocalVisual.SetOrientation(GetOrientation());
            if (dBodyIsSet(m_LocalVisual)) dBodyDestroy(m_LocalVisual);
            m_LocalVisual.ClearFlags(EntityFlags.SOLID, false);
            m_LocalVisual.SetFlags(EntityFlags.ACTIVE, true);
            m_LocalVisual.SetInvisible(false);
            m_LocalVisual.SetAnimationPhaseNow("RPArm", m_RPPhase);
        }
        super.ClientUpdate(dt);
    }
};

// Add only the two opposing gates at the main-road crossing after Kamenka.
// One existing controller/snapshot owns all four arms. Base packages stay intact.
modded class RPFirstCrossing
{
    protected ref array<Land_rp_crossing_barrier> m_FullRoadGates;

    override bool Configure(RPConvoyService service, RPFirstCrossingSite site, bool initial, bool offline)
    {
        if (!super.Configure(service, site, initial, offline)) return false;
        if (site.Name != "Crossing_2871") return true;
        // Refuse an unexpected site rather than adding barriers elsewhere.
        vector expected = "1324.72778 6.49009 2387.77499";
        if (vector.Distance(site.Pivot, expected) > 0.05) { Release(); return false; }
        m_FullRoadGates = new array<Land_rp_crossing_barrier>;
        for (int i = 0; i < 2; i++)
        {
            vector position = site.BarrierPositions[i];
            float across = vector.Dot(position - site.Pivot, site.TrackAxis);
            position = position - site.TrackAxis * (2 * across);
            position[1] = GetGame().SurfaceY(position[0], position[2]) + 0.06;
            vector orientation = site.BarrierOrientations[i];
            orientation[0] = orientation[0] + 180;
            Land_rp_crossing_barrier gate = Land_rp_crossing_barrier.Cast(GetGame().CreateObjectEx("Land_rp_full_crossing_barrier", position, ECE_CREATEPHYSICS | ECE_KEEPHEIGHT));
            if (!gate) { Release(); return false; }
            m_FullRoadGates.Insert(gate);
            gate.SetOrientation(orientation); gate.SetAllowDamage(false);
        }
        Apply();
        Print("[RailPilotFullCrossing] Four gates at Crossing_2871 only; original two alarm speakers retained");
        return true;
    }

    override void Apply()
    {
        super.Apply();
        if (!m_FullRoadGates) return;
        for (int i = 0; i < m_FullRoadGates.Count(); i++)
        {
            Land_rp_crossing_barrier gate = m_FullRoadGates[i];
            if (!gate) continue;
            // The existing audio-only extension preserves the lamp while muted.
            // Keep the two approved speakers rather than doubling alarm volume.
            gate.ServerAudioState(true, false);
            gate.ServerApply(m_Logic.Phase(), m_Logic.Alarm(), true);
            if (m_Offline) gate.OnVariablesSynchronized();
        }
    }

    int FullGateCount()
    {
        if (!m_FullRoadGates) return 2;
        return 2 + m_FullRoadGates.Count();
    }

    Land_rp_crossing_barrier FullGate(int index)
    {
        if (index == 0) return m_A;
        if (index == 1) return m_B;
        if (!m_FullRoadGates || index < 2 || index >= FullGateCount()) return null;
        return m_FullRoadGates[index - 2];
    }

    override void Release()
    {
        if (m_FullRoadGates)
        {
            for (int i = 0; i < m_FullRoadGates.Count(); i++)
                if (m_FullRoadGates[i]) GetGame().ObjectDelete(m_FullRoadGates[i]);
            m_FullRoadGates.Clear();
        }
        super.Release();
    }
};

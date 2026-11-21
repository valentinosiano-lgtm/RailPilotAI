class Land_rp_crossing_barrier : BuildingSuper
{
    protected static ref array<Land_rp_crossing_barrier> s_Barriers = new array<Land_rp_crossing_barrier>;
    protected static bool s_MissionEnding;
    protected float m_RPPhase;
    protected bool m_RPAlarm;
    protected bool m_RPEmitter;
    protected int m_RPTarget;
    protected float m_RenderPhase;
    protected bool m_ClientInitialized;
    protected ref RPApproachSound m_AlarmSound;
    protected float m_WaveDiagnostic;
    protected Entity m_LocalVisual;
    protected Object m_OriginalBarrier;
    protected bool m_OriginalHidden;
    protected bool m_UseOriginalRoadPosition;
    protected float m_OriginalRoadX;
    protected float m_OriginalRoadY;
    protected float m_OriginalRoadZ;
    protected EntityFlags m_OriginalFlags;
    protected vector m_OriginalPosition;
    protected vector m_OriginalParkedPosition;
    protected RPCrossingWarningLight m_WarningLight;
    protected Entity m_WarningLens;

    void Land_rp_crossing_barrier()
    {
        RegisterNetSyncVariableFloat("m_RPPhase",0,1,3);
        RegisterNetSyncVariableBool("m_RPAlarm");
        RegisterNetSyncVariableBool("m_RPEmitter");
        RegisterNetSyncVariableInt("m_RPTarget",0,1);
        RegisterNetSyncVariableBool("m_UseOriginalRoadPosition");
        RegisterNetSyncVariableFloat("m_OriginalRoadX", 0, 16384, 3);
        RegisterNetSyncVariableFloat("m_OriginalRoadY", -50, 1000, 3);
        RegisterNetSyncVariableFloat("m_OriginalRoadZ", 0, 16384, 3);
        s_Barriers.Insert(this);
    }

    void ServerApply(float phase, bool alarm, bool emitter)
    {
        if (!GetGame().IsServer()) return;
        int target = m_RPTarget;
        if (phase > m_RPPhase) target = 1;
        if (phase < m_RPPhase) target = 0;
        if (Math.AbsFloat(phase-m_RPPhase) < 0.0001 && alarm == m_RPAlarm && emitter == m_RPEmitter && target == m_RPTarget) return;
        m_RPTarget = target;
        m_RPPhase = phase; m_RPAlarm = alarm; m_RPEmitter = emitter;
        SetAnimationPhaseNow("RPArm",phase);
        SetSynchDirty();
    }

    override void OnVariablesSynchronized()
    {
        super.OnVariablesSynchronized();
        // The local player's collider must use the replicated gate pose too.
        // A smooth visual alone leaves the client and server collision apart.
        SetAnimationPhaseNow("RPArm",m_RPPhase);
        if (!m_ClientInitialized)
        {
            m_ClientInitialized = true;
            m_RenderPhase = m_RPPhase;
        }
    }

    void ClientUpdate(float dt)
    {
        if (s_MissionEnding || GetGame().IsDedicatedServer() || !m_ClientInitialized) return;
        HideVanillaOriginal();
        if (!m_LocalVisual)
        {
            m_LocalVisual = Entity.Cast(GetGame().CreateObjectEx("RP_CrossingBarrierVisual",GetPosition(),ECE_LOCAL | ECE_KEEPHEIGHT));
            if (!m_LocalVisual) return;
            m_LocalVisual.SetOrientation(GetOrientation());
            if (dBodyIsSet(m_LocalVisual)) dBodyDestroy(m_LocalVisual);
            m_LocalVisual.ClearFlags(EntityFlags.SOLID,false);
            m_LocalVisual.SetFlags(EntityFlags.ACTIVE,true);
            m_LocalVisual.SetInvisible(false);
            m_LocalVisual.SetAnimationPhaseNow("RPArm",m_RPPhase);
            SetInvisible(true);
            ClearFlags(EntityFlags.VISIBLE,true);
            Print("[RailPilotCrossing] CLIENT local Arma 2 visual, continuous 5s render pose");
        }
        // Keep the local renderer's transform current independently of
        // authoritative building visibility and network animation snapshots.
        m_LocalVisual.SetPosition(GetPosition());
        m_LocalVisual.SetOrientation(GetOrientation());
        SetInvisible(true);
        ClearFlags(EntityFlags.VISIBLE,true);
        if (m_RPTarget == 1) m_RenderPhase = Math.Min(1,m_RenderPhase + dt / 5);
        else m_RenderPhase = Math.Max(0,m_RenderPhase - dt / 5);
        m_LocalVisual.SetAnimationPhaseNow("RPArm",m_RenderPhase);
        m_LocalVisual.Update();
        bool audible = m_RPEmitter && (m_RPAlarm || m_RenderPhase > 0.0005);
        UpdateWarningLight(audible && GetGame().GetPlayer() != null);
        if (!audible || !GetGame().GetPlayer()) { StopAlarm(); return; }
        if (!m_AlarmSound)
        {
            m_AlarmSound = new RPApproachSound;
            m_AlarmSound.SetSoundSet("RP_CrossingAlarm_SoundSet");
            m_AlarmSound.SetPosition(AlarmPosition());
            m_AlarmSound.SetSoundLoop(true);
            m_AlarmSound.SetSoundAutodestroy(false);
            SEffectManager.EffectRegister(m_AlarmSound);
            if (!m_AlarmSound.SoundPlay())
            {
                StopAlarm(); Print("[RailPilotCrossing] ERROR alarm playback failed"); return;
            }
            Print("[RailPilotCrossing] CLIENT alarm ON");
            m_WaveDiagnostic = 0;
        }
        m_AlarmSound.SetCurrentPosition(AlarmPosition());
        m_WaveDiagnostic += dt;
        if (m_WaveDiagnostic >= 5)
        {
            m_WaveDiagnostic = 0;
            string message = "[RailPilotCrossing] CLIENT phase=" + m_RenderPhase.ToString();
            message += " waveLength=" + m_AlarmSound.GetSoundWaveLength().ToString();
            message += " playing=" + m_AlarmSound.IsSoundPlaying().ToString();
            Print(message);
        }
    }

    vector AlarmPosition()
    {
        // Fixed cabinet speaker, never parented to or positioned on a train.
        if (MemoryPointExists("bily pozicni blik")) return ModelToWorld(GetMemoryPointPos("bily pozicni blik"));
        return ModelToWorld(Vector(0,-1,0));
    }

    Entity ClientVisual() { return m_LocalVisual; }
    Object OriginalBarrier() { return FindVanillaOriginal(); }
    RPCrossingWarningLight WarningLight() { return m_WarningLight; }

    vector WarningLightPosition()
    {
        // Upper red lens on the original fixed roadside signal cabinet.
        return ModelToWorld(GetMemoryPointPos("rp_red_lamp"));
    }

    void UpdateWarningLight(bool active)
    {
        if (!active) { StopWarningLight(); return; }
        if (!m_WarningLight)
        {
            m_WarningLight = RPCrossingWarningLight.Cast(ScriptedLightBase.CreateLight(RPCrossingWarningLight,WarningLightPosition()));
            if (!m_WarningLight) { Print("[RailPilotCrossing] ERROR warning light creation failed"); return; }
        }
        if (!m_WarningLens)
        {
            m_WarningLens = Entity.Cast(GetGame().CreateObjectEx("RP_CrossingLampVisual",WarningLightPosition(),ECE_LOCAL | ECE_KEEPHEIGHT));
            if (m_WarningLens) m_WarningLens.SetOrientation(GetOrientation());
        }
        m_WarningLight.SetPosition(WarningLightPosition());
        // Both fixed cabinets share the client clock: 0.5s on, 0.5s off.
        bool on = (GetGame().GetTime() % 1000) < 500;
        m_WarningLight.SetEnabled(on);
        if (m_WarningLens) m_WarningLens.SetInvisible(!on);
    }

    void StopWarningLight()
    {
        Entity lens = m_WarningLens; m_WarningLens = null;
        if (lens && !s_MissionEnding) GetGame().ObjectDelete(lens);
        if (!m_WarningLight) return;
        RPCrossingWarningLight lamp = m_WarningLight; m_WarningLight = null;
        lamp.SetEnabled(false); lamp.Destroy();
    }

    void HideVanillaOriginal()
    {
        Object original = FindVanillaOriginal();
        if (!original) return;
        // Terrain Object wrappers can expire between queries. Preserve the
        // original flags once, then reacquire the native object by transform.
        if (!m_OriginalHidden)
        {
            m_OriginalFlags = original.GetFlags();
            m_OriginalPosition = original.GetPosition();
            m_OriginalParkedPosition = m_OriginalPosition - Vector(0,200,0);
            m_OriginalHidden = true;
            Print("[RailPilotCrossing] original terrain barrier replaced at its exact native transform");
        }
        m_OriginalBarrier = original;
        original.SetPosition(m_OriginalParkedPosition);
        original.ClearFlags(EntityFlags.VISIBLE | EntityFlags.SOLID,true);
    }

    void UseOriginalRoadPosition(vector position)
    {
        m_UseOriginalRoadPosition = true;
        m_OriginalRoadX = position[0];
        m_OriginalRoadY = position[1];
        m_OriginalRoadZ = position[2];
        SetSynchDirty();
    }

    Object FindVanillaOriginal()
    {
        array<Object> objects = new array<Object>;
        array<CargoBase> cargo = new array<CargoBase>;
        vector searchPosition = GetPosition();
        if (m_OriginalHidden) searchPosition = m_OriginalParkedPosition;
        else if (m_UseOriginalRoadPosition) searchPosition = Vector(m_OriginalRoadX, m_OriginalRoadY, m_OriginalRoadZ);
        GetGame().GetObjectsAtPosition3D(searchPosition,10,objects,cargo);
        for (int i = 0; i < objects.Count(); i++)
        {
            Object candidate = objects.Get(i);
            if (!candidate || candidate == this) continue;
            string model = candidate.GetShapeName(); model.ToLower(); model.Replace("/","\\");
            if (model != "dz\\structures\\rail\\misc\\rail_crossing_barrier.p3d") continue;
            vector delta = candidate.GetPosition() - searchPosition;
            delta[1] = 0;
            if (delta.Length() > 0.1) continue;
            return candidate;
        }
        return null;
    }

    void ReleaseVisual()
    {
        if (!GetGame()) return;
        // Detach first: native deletion can synchronously re-enter cleanup.
        Entity visual = m_LocalVisual; m_LocalVisual = null;
        Object original;
        if (m_OriginalHidden) original = FindVanillaOriginal();
        m_OriginalBarrier = null; m_OriginalHidden = false;
        if (visual) GetGame().ObjectDelete(visual);
        if (original)
        {
            original.SetPosition(m_OriginalPosition);
            original.SetFlags(m_OriginalFlags,true);
        }
    }

    void StopAlarm()
    {
        StopWarningLight();
        if (!m_AlarmSound) return;
        m_AlarmSound.StopImmediately();
        SEffectManager.DestroyEffect(m_AlarmSound);
        m_AlarmSound = null;
        Print("[RailPilotCrossing] CLIENT alarm OFF, source released");
    }

    override void EEDelete(EntityAI parent)
    {
        if (!s_MissionEnding) { StopAlarm(); ReleaseVisual(); }
        else { m_LocalVisual = null; m_OriginalBarrier = null; }
        if (s_Barriers) s_Barriers.RemoveItem(this);
        super.EEDelete(parent);
    }

    void ~Land_rp_crossing_barrier()
    {
        // World objects may already have been freed when script destructors run.
        // Native cleanup belongs to EEDelete / OnMissionFinish, never here.
        m_LocalVisual = null; m_OriginalBarrier = null;
        if (s_Barriers) s_Barriers.RemoveItem(this);
    }

    static void UpdateAll(float dt)
    {
        int last = s_Barriers.Count() - 1;
        for (int i = last; i >= 0; i--)
        {
            Land_rp_crossing_barrier barrier = s_Barriers.Get(i);
            if (!barrier) s_Barriers.Remove(i);
            else if (!barrier.ToDelete()) barrier.ClientUpdate(dt);
        }
    }

    static void StopAllSounds()
    {
        for (int i = 0; i < s_Barriers.Count(); i++)
        {
            Land_rp_crossing_barrier barrier = s_Barriers.Get(i);
            if (barrier) { barrier.StopAlarm(); barrier.ReleaseVisual(); }
        }
    }

    static void BeginClientMission() { s_MissionEnding = false; }

    static void EndClientMission()
    {
        s_MissionEnding = true;
        for (int i = 0; i < s_Barriers.Count(); i++)
        {
            Land_rp_crossing_barrier barrier = s_Barriers.Get(i);
            if (!barrier) continue;
            barrier.StopAlarm();
            // The closing world owns these entities and removes them itself.
            barrier.m_LocalVisual = null; barrier.m_OriginalBarrier = null;
        }
    }
};

class RP_CrossingRailGuide : BuildingSuper
{
    override void EEInit()
    {
        super.EEInit();
        ClearFlags(EntityFlags.VISIBLE,false);
        SetAllowDamage(false);
    }
};

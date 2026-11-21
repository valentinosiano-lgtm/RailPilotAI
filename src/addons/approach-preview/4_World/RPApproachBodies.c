class RPApproachContactDiagnostic
{
    string LastBlocking;

    void Capture(Object body, IEntity other, Contact contact)
    {
        Object obstacle = Object.Cast(other);
        if (!obstacle || Math.AbsFloat(vector.Dot(contact.Normal, body.GetTransformAxis(2))) < 0.4)
            return;
        LastBlocking = obstacle.GetType() + " model=" + obstacle.GetShapeName();
        LastBlocking += " normal=" + contact.Normal.ToString();
        LastBlocking += " point=" + contact.Position.ToString();
        LastBlocking += " penetration=" + contact.PenetrationDepth.ToString();
    }
};

modded class RP_LocomotiveBodyProbe
{
    protected int m_RPGroundContacts;
    protected bool m_RPApproachMoving;
    protected ref RPApproachContactDiagnostic m_RPContacts = new RPApproachContactDiagnostic;
    protected float m_RPFollowDiagnostic;
    protected int m_RPHeadlightsNight;
    protected int m_RPHeadlightDirection = 1;
    protected float m_RPHeadlightRefresh;
    protected ref RPTrainHeadlightRig m_RPHeadlightRig;

    void RP_LocomotiveBodyProbe()
    {
        RegisterNetSyncVariableInt("m_RPHeadlightsNight", 0, 1);
        RegisterNetSyncVariableInt("m_RPHeadlightDirection", -1, 1);
    }

    void RPRefreshHeadlightState()
    {
        if (!GetGame().IsServer()) return;
        int night;
        if (GetGame().GetWorld().IsNight()) night = 1;
        vector velocity;
        if (dBodyIsSet(this)) velocity = GetVelocity(this);
        int direction = RPTrainHeadlightRules.Direction(vector.Dot(velocity, GetTransformAxis(2)), m_RPHeadlightDirection);
        if (night != m_RPHeadlightsNight || direction != m_RPHeadlightDirection)
        {
            m_RPHeadlightsNight = night;
            m_RPHeadlightDirection = direction;
            SetSynchDirty();
            string message = "[RailPilotLights] SERVER night=" + night.ToString();
            message += " direction=" + direction.ToString();
            Print(message);
        }
    }

    int RPHeadlightsNight() { return m_RPHeadlightsNight; }
    int RPHeadlightDirection() { return m_RPHeadlightDirection; }

    override protected void UpdateClientVisual(float timeSlice)
    {
        super.UpdateClientVisual(timeSlice);
        if (GetGame().IsServer() || timeSlice <= 0) return;
        if (!m_RPHeadlightRig || m_RPHeadlightRig.IsReleased())
            m_RPHeadlightRig = new RPTrainHeadlightRig(this);
        bool enabled = RPTrainHeadlightRules.Enabled(m_RPHeadlightsNight, GetGame().GetWorld().IsNight());
        m_RPHeadlightRig.Update(m_ClientVisual, enabled, m_RPHeadlightDirection, timeSlice);
    }

    protected void ReleaseRPHeadlights()
    {
        if (m_RPHeadlightRig) m_RPHeadlightRig.Release();
        m_RPHeadlightRig = null;
    }

    int RPGroundContacts() { return m_RPGroundContacts; }
    string RPBlockingContact() { return m_RPContacts.LastBlocking; }

    void RPEnableMotionAudio()
    {
        m_RPStopGate = new RPAudioMotionGate;
        m_RPApproachMoving = true;
    }

    override protected void UpdateRPAudio(float dt)
    {
        if (!GetGame().IsServer())
        {
            UpdateRPApproachClientAudio(dt);
            return;
        }
        if (dt <= 0) return;
        m_RPHeadlightRefresh += dt;
        if (m_RPHeadlightRefresh >= 0.5)
        {
            m_RPHeadlightRefresh = 0;
            RPRefreshHeadlightState();
        }
        if (!m_RPApproachMoving)
            return;
        vector velocity = GetVelocity(this);
        velocity[1] = 0;
        float speed = velocity.Length();
        float normalized = Math.Clamp(speed / (30.0 / 3.6), 0, 1);
        bool changed = Math.AbsFloat(normalized - m_RPAudioSpeed) >= 0.01;
        bool stopped = m_RPStopGate.Update(speed, dt);
        if (changed || stopped)
        {
            m_RPAudioSpeed = normalized;
            if (stopped)
            {
                m_RPHornSequence = (m_RPHornSequence + 1) % 1000000;
                Print("[RailPilotAudio] STOP_EVENT sequence=" + m_RPHornSequence.ToString());
                RPApproachHorn.Emit(GetPosition(), m_RPHornSequence);
            }
            SetSynchDirty();
        }
    }

    override void EOnContact(IEntity other, Contact extra)
    {
        super.EOnContact(other, extra);
        if (GetGame().IsServer())
        {
            m_RPGroundContacts++;
            m_RPContacts.Capture(this, other, extra);
        }
    }

    override protected EffectSound CreateRPLoop(string name, Object parent)
    {
        name.Replace("RP_Preview", "RP_Approach");
        RPApproachSound sound = CreateRPApproachSound(name, parent.GetPosition(), true);
        return sound;
    }

    protected RPApproachSound CreateRPApproachSound(string name, vector position, bool loop)
    {
        RPApproachSound sound = new RPApproachSound;
        sound.SetSoundSet(name);
        sound.SetPosition(position);
        sound.SetSoundLoop(loop);
        sound.SetSoundAutodestroy(false);
        if (loop)
        {
            sound.SetSoundMaxVolume(0);
            sound.SetSoundVolume(0);
        }
        SEffectManager.EffectRegister(sound);
        RPApproachAudioBus.Track(sound);
        if (!sound.SoundPlay())
        {
            RPApproachAudioBus.Destroy(sound);
            return null;
        }
        sound.SetCurrentPosition(position);
        return sound;
    }

    protected void UpdateRPApproachClientAudio(float dt)
    {
        if (dt <= 0 || m_RPAudioFailed || !GetGame().GetPlayer()) return;
        RPApproachAudioBus.Touch(this);
        Object visual = m_ClientVisual;
        if (!visual) visual = this;
        vector position = visual.GetPosition();
        if (!m_RPIdle)
        {
            m_RPIdle = CreateRPLoop("RP_PreviewIdle_SoundSet", visual);
            m_RPSlow = CreateRPLoop("RP_PreviewSlow_SoundSet", visual);
            m_RPCruise = CreateRPLoop("RP_PreviewCruise_SoundSet", visual);
            if (!m_RPIdle || !m_RPSlow || !m_RPCruise)
            {
                StopRPAudio();
                m_RPAudioFailed = true;
                Print("[RailPilotApproachAudio] ERROR: motor playback failed");
                return;
            }
            Print("[RailPilotApproachAudio] START three owned loops, native wave follows the train");
        }
        m_RPEnvelope.Update(m_RPAudioSpeed, dt);
        m_RPStartup = Math.Min(1, m_RPStartup + dt * 2);
        float gain = 0.65 * m_RPStartup;
        ApplyRPVolume(m_RPIdle, gain * m_RPEnvelope.Idle(), position);
        ApplyRPVolume(m_RPSlow, gain * m_RPEnvelope.Slow(), position);
        ApplyRPVolume(m_RPCruise, gain * m_RPEnvelope.Cruise(), position);
        // Horn playback uses the global event exclusively. Ignore the body
        // sequence callback here to avoid doubling the horn for nearby players.
        m_RPHornPending = false;
        m_RPFollowDiagnostic += dt;
        if (m_RPFollowDiagnostic > 5)
        {
            m_RPFollowDiagnostic = 0;
            PrintRPWaveDiagnostic("idle", m_RPIdle, position);
            PrintRPWaveDiagnostic("slow", m_RPSlow, position);
            PrintRPWaveDiagnostic("cruise", m_RPCruise, position);
            RPApproachSound nativeSound = RPApproachSound.Cast(m_RPIdle);
            int totalSources = RPApproachAudioBus.Count() + RPApproachHorn.Count();
            string diagnostic = "[RailPilotApproachAudio] FOLLOW sources=" + totalSources.ToString();
            diagnostic += " waveMoves=" + nativeSound.PositionUpdates().ToString();
            diagnostic += " distance=" + vector.Distance(position, GetGame().GetPlayer().GetPosition()).ToString();
            Print(diagnostic);
        }
    }

    override protected void StopRPAudio()
    {
        RPApproachAudioBus.Release(this);
        m_RPIdle = null; m_RPSlow = null; m_RPCruise = null; m_RPHorn = null;
        m_RPStartup = 0;
    }

    override void EEDelete(EntityAI parent)
    {
        // Remove the sampler immediately; destructor timing during network
        // streaming must not restart sounds after the delete notification.
        if (s_MotionBodies) s_MotionBodies.RemoveItem(this);
        if (GetGame() && !GetGame().IsServer())
        {
            ReleaseRPHeadlights();
            StopRPAudio();
            if (m_ClientVisual) GetGame().ObjectDelete(m_ClientVisual);
            m_ClientVisual = null;
        }
        super.EEDelete(parent);
    }

    void ~RP_LocomotiveBodyProbe()
    {
        ReleaseRPHeadlights();
        if (GetGame() && !GetGame().IsServer()) StopRPAudio();
    }
};

// Separate body: it must not inherit the locomotive audio or its single CSV.
class RP_WagonBodyProbe : ItemBase
{
    protected static ref array<RP_WagonBodyProbe> s_Wagons = new array<RP_WagonBodyProbe>;
    protected Object m_Visual;
    protected ref RPVisualMotionBuffer m_Buffer;
    protected float m_Clock;
    protected float m_LastChange;
    protected float m_LastSnapshot;
    protected vector m_LastRaw;
    protected int m_Contacts;
    protected bool m_Failed;
    protected ref RPApproachContactDiagnostic m_RPContacts = new RPApproachContactDiagnostic;

    override void EEInit()
    {
        super.EEInit();
        // The client-only visual uses the same config but isn't a sampled body.
        if (GetType() == "RP_WagonBodyProbe")
            s_Wagons.Insert(this);
    }

    int RPGroundContacts() { return m_Contacts; }
    string RPBlockingContact() { return m_RPContacts.LastBlocking; }

    override void EOnContact(IEntity other, Contact extra)
    {
        super.EOnContact(other, extra);
        if (GetGame().IsServer())
        {
            m_Contacts++;
            m_RPContacts.Capture(this, other, extra);
        }
    }

    static void SampleAllWagons(float dt)
    {
        if (GetGame().IsServer())
            return;
        for (int i = 0; i < s_Wagons.Count(); i++)
        {
            RP_WagonBodyProbe wagon = s_Wagons.Get(i);
            if (wagon)
                wagon.UpdateVisual(dt);
        }
    }

    protected void UpdateVisual(float dt)
    {
        if (dt <= 0 || m_Failed)
            return;
        m_Clock += dt;
        vector raw = GetPosition();
        if (!m_Visual)
        {
            m_Visual = GetGame().CreateObjectEx("RP_WagonVisualProbe", raw, ECE_LOCAL | ECE_KEEPHEIGHT);
            if (!m_Visual)
            {
                m_Failed = true;
                return;
            }
            if (dBodyIsSet(m_Visual))
                dBodyDestroy(m_Visual);
            m_Visual.ClearFlags(EntityFlags.SOLID, false);
            m_Visual.SetAllowDamage(false);
            m_Buffer = new RPVisualMotionBuffer;
            m_Buffer.Push(m_Clock, raw);
            m_LastRaw = raw;
            m_LastChange = m_Clock;
            m_LastSnapshot = m_Clock;
        }
        if (vector.Distance(raw, m_LastRaw) > 0.0002)
        {
            m_Buffer.Push(m_Clock, raw);
            m_LastRaw = raw;
            m_LastChange = m_Clock;
            m_LastSnapshot = m_Clock;
        }
        else if (m_Clock - m_LastChange > 0.15 && m_Clock - m_LastSnapshot >= 0.05)
        {
            m_Buffer.Push(m_Clock, raw);
            m_LastSnapshot = m_Clock;
        }
        m_Visual.SetPosition(m_Buffer.Sample(m_Clock));
        m_Visual.SetOrientation(GetOrientation());
        ClearFlags(EntityFlags.VISIBLE, false);
    }

    override bool CanPutInCargo(EntityAI parent) { return false; }
    override bool CanPutIntoHands(EntityAI parent) { return false; }

    override void EEDelete(EntityAI parent)
    {
        if (s_Wagons) s_Wagons.RemoveItem(this);
        if (m_Visual && GetGame()) GetGame().ObjectDelete(m_Visual);
        m_Visual = null;
        super.EEDelete(parent);
    }

    void ~RP_WagonBodyProbe()
    {
        if (s_Wagons)
            s_Wagons.RemoveItem(this);
        if (m_Visual && GetGame())
            GetGame().ObjectDelete(m_Visual);
    }
};

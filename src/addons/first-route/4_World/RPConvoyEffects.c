// Only the active locomotive owns effects. Role change releases the old
// sources before the new leading locomotive is updated.
class RPConvoyEffects
{
    protected ref array<ref RPConvoySound> m_Motors = new array<ref RPConvoySound>;
    protected ref RPConvoyHeadlightRig m_Lights;
    protected float m_Ramp;

    int MotorCount() { return m_Motors.Count(); }
    int LampCount()
    {
        if (m_Lights) return m_Lights.LampCount();
        return 0;
    }
    bool MotorsDecoded()
    {
        if (m_Motors.Count() != 3) return false;
        for (int i = 0; i < m_Motors.Count(); i++)
            if (m_Motors[i].GetSoundWaveLength() <= 1 || !m_Motors[i].IsSoundPlaying()) return false;
        return true;
    }
    float SourceError(vector position)
    {
        float error;
        for (int i = 0; i < m_Motors.Count(); i++)
            error = Math.Max(error, vector.Distance(position, m_Motors[i].GetCurrentPosition()));
        return error;
    }

    void Update(Object loco, bool active, bool night, float speed, float cruise, float dt)
    {
        if (GetGame().IsDedicatedServer() || dt <= 0) return;
        if (!active) { Release(); return; }
        if (m_Motors.Count() == 0)
        {
            array<string> sets = {"RP_ConvoyIdle_SoundSet", "RP_ConvoySlow_SoundSet", "RP_ConvoyCruise_SoundSet"};
            for (int i = 0; i < sets.Count(); i++)
            {
                RPConvoySound sound = new RPConvoySound;
                sound.SetSoundSet(sets[i]); sound.SetPosition(loco.GetPosition());
                sound.SetSoundLoop(true); sound.SetSoundAutodestroy(false);
                sound.SetSoundMaxVolume(0); sound.SetSoundVolume(0);
                SEffectManager.EffectRegister(sound); sound.SoundPlay();
                m_Motors.Insert(sound);
            }
        }
        float normalized = Math.Clamp(speed / cruise, 0, 1);
        m_Ramp = Math.Min(1, m_Ramp + dt * 2);
        for (int j = 0; j < m_Motors.Count(); j++)
        {
            float gain = Math.Max(0, 1 - 2 * normalized);
            if (j == 1) gain = 1 - Math.AbsFloat(2 * normalized - 1);
            if (j == 2) gain = 0.5 * Math.Max(0, 2 * normalized - 1);
            m_Motors[j].SetCurrentPosition(loco.GetPosition());
            m_Motors[j].SetSoundMaxVolume(0.65 * m_Ramp * gain);
            m_Motors[j].SetSoundVolume(0.65 * m_Ramp * gain);
        }
        if (!m_Lights || m_Lights.IsReleased()) m_Lights = new RPConvoyHeadlightRig(loco);
        // Both locomotive models face outwards: always their own front lamps.
        m_Lights.Update(loco, night, 1, dt);
    }

    void Release()
    {
        for (int i = 0; i < m_Motors.Count(); i++)
        {
            m_Motors[i].StopImmediately();
            SEffectManager.DestroyEffect(m_Motors[i]);
        }
        m_Motors.Clear(); m_Ramp = 0;
        if (m_Lights) m_Lights.Release();
        m_Lights = null;
    }
};

class RPConvoyHorn
{
    static const int RPC_ID = 1724936135;
    protected static int s_LastSequence = -1;
    protected static ref RPConvoySound s_Sound;
    protected static int s_Slot;
    protected static vector s_Position;
    protected static float s_Elapsed;
    protected static int s_DecodedPlays;
    protected static int s_DecodedSequence;
    static int DecodedPlays() { return s_DecodedPlays; }

    static void StartReceiving()
    {
        s_LastSequence = -1;
        DayZGame game = DayZGame.Cast(GetGame());
        game.Event_OnRPC.Remove(Receive); game.Event_OnRPC.Insert(Receive);
    }

    static void Emit(RP_ConvoyUnit loco, int sequence)
    {
        if (!GetGame().IsServer() || !loco) return;
        RPTrainAINoise.Emit(RPTrainAINoise.SourcePosition(loco), RPTrainNoiseCue.Horn);
        if (!GetGame().IsMultiplayer()) { Play(sequence, loco.Slot(), loco.GetPosition()); return; }
        ScriptRPC rpc = new ScriptRPC;
        rpc.Write(1); rpc.Write(sequence); rpc.Write(loco.Slot()); rpc.Write(loco.GetPosition());
        array<Man> players = new array<Man>; GetGame().GetPlayers(players);
        for (int i = 0; i < players.Count(); i++)
        {
            Man player = players[i];
            if (player && player.GetIdentity() && vector.Distance(loco.GetPosition(), player.GetPosition()) <= 3000)
                rpc.Send(null, RPC_ID, true, player.GetIdentity());
        }
    }

    static void Receive(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type != RPC_ID || target || sender || GetGame().IsServer()) return;
        int schema; int sequence; int slot; vector position;
        if (!ctx.Read(schema) || schema != 1 || !ctx.Read(sequence) || !ctx.Read(slot) || !ctx.Read(position)) return;
        if (sequence < 0 || (slot != 0 && slot != 5)) return;
        Play(sequence, slot, position);
    }

    static void Play(int sequence, int slot, vector position)
    {
        if (sequence == s_LastSequence || GetGame().IsDedicatedServer()) return;
        s_LastSequence = sequence; Stop();
        s_Slot = slot; s_Position = position; s_Elapsed = 0;
        s_Sound = new RPConvoySound;
        s_Sound.SetSoundSet("RP_ConvoyHorn_SoundSet"); s_Sound.SetPosition(position);
        s_Sound.SetSoundLoop(false); s_Sound.SetSoundAutodestroy(false);
        SEffectManager.EffectRegister(s_Sound); s_Sound.SoundPlay();
    }

    static void Update(float dt)
    {
        if (!s_Sound) return;
        if (RP_ConvoyUnit.ActiveSlot() >= 0 && RP_ConvoyUnit.ActiveSlot() != s_Slot) { Stop(); return; }
        s_Elapsed += dt;
        if (s_LastSequence != s_DecodedSequence && s_Sound.GetSoundWaveLength() > 1 && s_Sound.IsSoundPlaying())
        { s_DecodedSequence = s_LastSequence; s_DecodedPlays++; }
        RP_ConvoyUnit unit = RP_ConvoyUnit.AtSlot(s_Slot);
        if (unit) s_Position = unit.GetPosition();
        s_Sound.SetCurrentPosition(s_Position);
        if (s_Elapsed > 15) Stop();
    }

    static void Stop()
    {
        if (!s_Sound) return;
        s_Sound.StopImmediately(); SEffectManager.DestroyEffect(s_Sound); s_Sound = null;
    }

    static void StopReceiving()
    {
        DayZGame game = DayZGame.Cast(GetGame()); game.Event_OnRPC.Remove(Receive); Stop();
    }
};

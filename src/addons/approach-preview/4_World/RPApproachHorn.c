// A transient world-space event: works even when the train is outside the
// listener's object replication bubble. No motor audio is sent this way.
class RPApproachHornGate
{
    protected bool m_Seen;
    protected ref RPAudioSequenceGate m_Sequence = new RPAudioSequenceGate;

    bool Accept(int sequence)
    {
        if (sequence < 0 || sequence >= 1000000) return false;
        if (!m_Seen)
        {
            m_Seen = true;
            m_Sequence.Accept(sequence);
            return true;
        }
        return m_Sequence.Accept(sequence);
    }
};

class RPApproachHorn
{
    static const int RPC_ID = 1724936129;
    static const int SCHEMA = 1;
    static const float RANGE = 3000;
    protected static ref RPApproachHornGate s_Gate;
    protected static ref RPApproachSound s_Sound;
    protected static vector s_Position;
    protected static float s_Elapsed;
    protected static bool s_Diagnosed;

    static bool InRange(vector source, vector listener)
    {
        return vector.Distance(source, listener) <= RANGE;
    }

    static bool ReadEvent(ParamsReadContext ctx, out int sequence, out vector position)
    {
        int schema;
        if (!ctx.Read(schema) || schema != SCHEMA) return false;
        if (!ctx.Read(sequence) || sequence < 0 || sequence >= 1000000) return false;
        if (!ctx.Read(position)) return false;
        return RPLimits.InRange(position[0], 0, 15360) && RPLimits.InRange(position[1], -100, 2000) && RPLimits.InRange(position[2], 0, 15360);
    }

    static void Emit(vector position, int sequence)
    {
        if (!GetGame().IsServer()) return;
        array<Man> players = new array<Man>;
        GetGame().GetPlayers(players);
        ScriptRPC rpc = new ScriptRPC;
        rpc.Write(SCHEMA);
        rpc.Write(sequence);
        rpc.Write(position);
        int recipients;
        for (int i = 0; i < players.Count(); i++)
        {
            Man player = players.Get(i);
            if (!player || !player.GetIdentity() || !InRange(position, player.GetPosition())) continue;
            rpc.Send(null, RPC_ID, true, player.GetIdentity());
            recipients++;
        }
        string message = "[RailPilotHorn] EMIT sequence=" + sequence.ToString();
        message += " recipients=" + recipients.ToString() + " range=" + RANGE.ToString();
        Print(message);
    }

    static void StartReceiving()
    {
        s_Gate = new RPApproachHornGate;
        DayZGame game = DayZGame.Cast(GetGame());
        game.Event_OnRPC.Remove(Receive);
        game.Event_OnRPC.Insert(Receive);
    }

    static void StopSound()
    {
        if (s_Sound)
        {
            s_Sound.StopImmediately();
            SEffectManager.DestroyEffect(s_Sound);
            s_Sound = null;
            Print("[RailPilotHorn] CLEANUP world event");
        }
    }

    static void StopReceiving()
    {
        DayZGame game = DayZGame.Cast(GetGame());
        game.Event_OnRPC.Remove(Receive);
        StopSound();
        s_Gate = null;
    }

    static void Receive(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
    {
        if (rpc_type != RPC_ID || target || GetGame().IsServer()) return;
        int sequence; vector position;
        if (!ReadEvent(ctx, sequence, position))
        {
            Print("[RailPilotHorn] DROP invalid event");
            return;
        }
        if (!s_Gate || !s_Gate.Accept(sequence)) return;
        Man listener = GetGame().GetPlayer();
        if (!listener || !InRange(position, listener.GetPosition())) return;
        StopSound();
        s_Sound = new RPApproachSound;
        s_Sound.SetSoundSet("RP_ApproachHorn_SoundSet");
        s_Sound.SetPosition(position);
        s_Sound.SetSoundLoop(false);
        s_Sound.SetSoundAutodestroy(false);
        SEffectManager.EffectRegister(s_Sound);
        s_Position = position;
        s_Elapsed = 0;
        s_Diagnosed = false;
        if (!s_Sound.SoundPlay())
        {
            StopSound();
            Print("[RailPilotHorn] ERROR playback request failed");
            return;
        }
        s_Sound.SetCurrentPosition(position);
        string message = "[RailPilotHorn] RECEIVE sequence=" + sequence.ToString();
        message += " listenerDistance=" + vector.Distance(position, listener.GetPosition()).ToString();
        Print(message);
    }

    static void Update(float dt)
    {
        if (!s_Sound || dt <= 0) return;
        s_Elapsed += dt;
        s_Sound.SetCurrentPosition(s_Position);
        if (!s_Diagnosed && s_Elapsed > 2)
        {
            s_Diagnosed = true;
            string message = "[RailPilotHorn] WAVE length=" + s_Sound.GetSoundWaveLength().ToString();
            message += " playing=" + s_Sound.IsSoundPlaying().ToString();
            message += " sourceError=" + vector.Distance(s_Position, s_Sound.GetCurrentPosition()).ToString();
            Print(message);
        }
        // Allow native loading/propagation delay, then release the event even
        // if no end callback arrives. Retain no references to the train body.
        if (s_Elapsed > 15) StopSound();
    }

    static int Count()
    {
        if (s_Sound) return 1;
        return 0;
    }
};

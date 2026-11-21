// Move the playing native wave as well as EffectSound's cached SoundObject.
// No changes to the previously approved short-audio package or its samples.
class RPApproachSound : EffectSound
{
    protected int m_PositionUpdates;

    override void SetCurrentPosition(vector pos, bool updateCached = true)
    {
        super.SetCurrentPosition(pos, updateCached);
        if (m_SoundWaveObject)
        {
            m_SoundWaveObject.SetPosition(pos);
            m_PositionUpdates++;
        }
    }

    void StopImmediately()
    {
        SetSoundAutodestroy(false);
        SetSoundFadeOut(0);
        SetSoundMaxVolume(0);
        SetSoundVolume(0);
        SetSoundLoop(false);
        if (m_SoundWaveObject)
        {
            m_SoundWaveObject.GetEvents().Event_OnSoundWaveHeaderLoaded.Remove(ValidateSoundWave);
            m_SoundWaveObject.Stop();
        }
        SoundStop();
    }

    int PositionUpdates() { return m_PositionUpdates; }
};

// One private train owns at most three motor loops. The separate horn world
// event owns one additional transient source. The motor heartbeat
// also releases sounds when network streaming misses an entity delete callback.
class RPApproachAudioBus
{
    protected static Object s_Owner;
    protected static int s_LastUpdate;
    protected static ref array<RPApproachSound> s_Sounds = new array<RPApproachSound>;

    static void Touch(Object owner)
    {
        if (s_Owner != owner)
        {
            Clear();
            s_Owner = owner;
        }
        s_LastUpdate = GetGame().GetTime();
    }

    static void Track(RPApproachSound sound)
    {
        s_Sounds.Insert(sound);
    }

    static void Destroy(RPApproachSound sound)
    {
        if (!sound) return;
        s_Sounds.RemoveItem(sound);
        sound.StopImmediately();
        SEffectManager.DestroyEffect(sound);
    }

    static void Clear()
    {
        int count = s_Sounds.Count();
        while (s_Sounds.Count() > 0)
        {
            RPApproachSound sound = s_Sounds.Get(0);
            s_Sounds.RemoveOrdered(0);
            if (sound)
            {
                sound.StopImmediately();
                SEffectManager.DestroyEffect(sound);
            }
        }
        s_Owner = null;
        if (count > 0)
            Print("[RailPilotApproachAudio] CLEANUP sources=" + count.ToString());
    }

    static void Release(Object owner)
    {
        if (s_Owner == owner) Clear();
    }

    static void Sweep()
    {
        if (s_Sounds.Count() > 0 && (!s_Owner || GetGame().GetTime() - s_LastUpdate > 1000))
            Clear();
    }

    static int Count() { return s_Sounds.Count(); }
};

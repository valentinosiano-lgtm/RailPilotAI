// Move the playing native wave as well as EffectSound's cached SoundObject.
// No changes to the previously approved short-audio package or its samples.
class RPConvoySound : EffectSound
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


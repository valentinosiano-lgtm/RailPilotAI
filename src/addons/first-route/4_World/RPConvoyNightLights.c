class RPConvoyNightLight : PointLightBase
{
    void Configure(bool red, bool interior)
    {
        SetVisibleDuringDaylight(false);
        SetFlickerAmplitude(0);
        SetCastShadow(interior);
        SetRadiusTo(1.5);
        SetBrightnessTo(2.8);
        SetDiffuseColor(1, 1, 1);
        SetAmbientColor(0.25, 0.25, 0.25);
        SetFlareVisible(true);
        SetFlareRelPosition(vector.Zero);
        SetFadeOutTime(0);
        if (red)
        {
            SetBrightnessTo(3.3);
            SetDiffuseColor(1, 0.035, 0.025);
            SetAmbientColor(0.28, 0.01, 0.007);
        }
        if (interior)
        {
            SetRadiusTo(5.2);
            SetBrightnessTo(1.45);
            SetFlareVisible(false);
        }
        FadeIn(0.7);
    }
};

modded class RP_ConvoyUnit
{
    protected ref array<vector> m_NightLightOffsets;
    protected ref array<RPConvoyNightLight> m_NightLights;
    protected int m_NightLightRetryAt;

    protected bool CreateNightLights(bool interior)
    {
        m_NightLightOffsets = new array<vector>;
        m_NightLights = new array<RPConvoyNightLight>;
        if (interior)
        {
            // The center of the inner roof lining is 2.0483m above the origin.
            m_NightLightOffsets.Insert("-0.00087 1.92 0.00272");
        }
        else
        {
            // Centers of the five front lenses, 4cm in front of the glass.
            // Both locomotives face outward in their respective local frames.
            m_NightLightOffsets.Insert("-0.11454 1.60117 6.0468");
            m_NightLightOffsets.Insert("-0.82616 -0.17108 6.04221");
            m_NightLightOffsets.Insert("0.595035 -0.17108 6.04221");
            m_NightLightOffsets.Insert("-0.82616 -0.42943 6.04221");
            m_NightLightOffsets.Insert("0.595035 -0.42943 6.04221");
        }
        for (int i = 0; i < m_NightLightOffsets.Count(); i++)
        {
            vector position = ModelToWorld(m_NightLightOffsets[i]);
            RPConvoyNightLight light = RPConvoyNightLight.Cast(ScriptedLightBase.CreateLight(RPConvoyNightLight, position));
            if (!light)
            {
                ReleaseNightLights();
                m_NightLightRetryAt = GetGame().GetTime() + 2000;
                return false;
            }
            light.Configure(!interior && (i == 1 || i == 2), interior);
            m_NightLights.Insert(light);
        }
        Print("[RailPilotLights] ACCENTS ON slot=" + m_Slot.ToString() + " count=" + m_NightLights.Count().ToString());
        return true;
    }

    protected void ReleaseNightLights()
    {
        if (!m_NightLights) return;
        foreach (RPConvoyNightLight source : m_NightLights)
        {
            if (source)
            {
                source.SetEnabled(false);
                source.Destroy();
            }
        }
        m_NightLights = null;
        m_NightLightOffsets = null;
    }

    override void Present(float clock, float dt)
    {
        super.Present(clock, dt);
        if (GetGame().IsDedicatedServer() || !m_HavePose) return;
        bool locomotive = m_Slot == 0 || m_Slot == 5;
        bool interior = m_Slot == 2 || m_Slot == 3;
        if (!locomotive && !interior) return;

        bool night = GetGame().GetWorld().IsNight();
        if (GetGame().IsMultiplayer()) night = night && m_Night == 1;
        if (!night || !m_ServiceEnabled || !GetGame().GetPlayer())
        {
            ReleaseNightLights();
            return;
        }
        if (!m_NightLights)
        {
            if (GetGame().GetTime() < m_NightLightRetryAt || !CreateNightLights(interior)) return;
        }
        else
        {
            for (int i = 0; i < m_NightLights.Count(); i++) m_NightLights[i].SetPosition(ModelToWorld(m_NightLightOffsets[i]));
        }
    }

    override void EEDelete(EntityAI parent)
    {
        ReleaseNightLights();
        super.EEDelete(parent);
    }
};

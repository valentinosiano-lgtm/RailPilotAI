// Preserved approved optics; only class names and offline-render guard adapted.
// Client-rendered lights; authoritative night/direction flags come from server.
class RPConvoyHeadlightRules
{
    static int Direction(float alongSpeed, int previous)
    {
        if (alongSpeed > 0.2) return 1;
        if (alongSpeed < -0.2) return -1;
        if (previous == -1) return -1;
        return 1;
    }

    static bool Enabled(int serverNight, bool clientNight)
    {
        return serverNight == 1 && clientNight;
    }

    static vector Offset(int side, int direction)
    {
        float x = 0.95;
        if (side < 0) x = -x;
        float z = 6.75;
        if (direction < 0) z = -z;
        return Vector(x, -0.45, z);
    }

    static vector Aim(Object visual, int direction)
    {
        vector forward = visual.GetTransformAxis(2);
        if (direction < 0) forward = -forward;
        forward[1] = forward[1] - 0.02;
        forward.Normalize();
        return forward;
    }
};

class RPConvoyHeadlight : SpotLightBase
{
    void RPConvoyHeadlight()
    {
        SetVisibleDuringDaylight(false);
        SetRadiusTo(250);
        SetSpotLightAngle(55);
        SetBrightnessTo(18);
        SetAmbientColor(0.10, 0.095, 0.085);
        SetDiffuseColor(1.0, 0.95, 0.85);
        SetCastShadow(true);
        SetFlareVisible(true);
        SetFlareRelPosition(vector.Zero);
        SetFadeOutTime(0);
    }
};

class RPConvoyHeadlightRig
{
    protected static ref array<ref RPConvoyHeadlightRig> s_Rigs = new array<ref RPConvoyHeadlightRig>;
    protected Object m_Owner;
    protected RPConvoyHeadlight m_Left;
    protected RPConvoyHeadlight m_Right;
    protected int m_LastUpdate;
    protected int m_LastDirection;
    protected bool m_Released;
    protected bool m_Failed;
    protected float m_DiagnosticTime;

    void RPConvoyHeadlightRig(Object owner)
    {
        m_Owner = owner;
        s_Rigs.Insert(this);
    }

    bool IsReleased() { return m_Released; }
    int LampCount()
    {
        int count;
        if (m_Left) count++;
        if (m_Right) count++;
        return count;
    }

    void Update(Object visual, bool enabled, int direction, float dt)
    {
        if (m_Released || GetGame().IsDedicatedServer()) return;
        m_LastUpdate = GetGame().GetTime();
        if (!enabled || !visual || !GetGame().GetPlayer())
        {
            Stop(); return;
        }
        if (m_Failed) return;
        if (!m_Left || !m_Right)
        {
            Stop();
            m_Left = RPConvoyHeadlight.Cast(ScriptedLightBase.CreateLight(RPConvoyHeadlight, visual.ModelToWorld(RPConvoyHeadlightRules.Offset(-1, direction))));
            m_Right = RPConvoyHeadlight.Cast(ScriptedLightBase.CreateLight(RPConvoyHeadlight, visual.ModelToWorld(RPConvoyHeadlightRules.Offset(1, direction))));
            if (!m_Left || !m_Right)
            {
                Stop(); m_Failed = true;
                Print("[RailPilotLights] ERROR client headlight creation failed");
                return;
            }
            Print("[RailPilotLights] ON two client spotlights, radius=250 brightness=18, flares enabled");
        }
        vector aim = RPConvoyHeadlightRules.Aim(visual, direction);
        vector leftPosition = visual.ModelToWorld(RPConvoyHeadlightRules.Offset(-1, direction));
        vector rightPosition = visual.ModelToWorld(RPConvoyHeadlightRules.Offset(1, direction));
        // Explicit world transforms follow the same interpolated visual as the
        // engine audio. No dependence on attachment physics or old positions.
        m_Left.SetPosition(leftPosition); m_Left.SetDirection(aim);
        m_Right.SetPosition(rightPosition); m_Right.SetDirection(aim);
        if (direction != m_LastDirection)
        {
            m_LastDirection = direction;
            Print("[RailPilotLights] DIRECTION " + direction.ToString());
        }
        m_DiagnosticTime += dt;
        if (m_DiagnosticTime > 5)
        {
            m_DiagnosticTime = 0;
            float sourceError = vector.Distance(m_Left.GetPosition(), leftPosition);
            sourceError = Math.Max(sourceError, vector.Distance(m_Right.GetPosition(), rightPosition));
            string message = "[RailPilotLights] FOLLOW count=2 sourceError=" + sourceError.ToString();
            message += " radius=" + m_Left.GetRadius().ToString();
            message += " brightness=" + m_Left.GetBrightness().ToString();
            message += " distance=" + vector.Distance(visual.GetPosition(), GetGame().GetPlayer().GetPosition()).ToString();
            Print(message);
        }
    }

    void Stop()
    {
        if (!GetGame()) { m_Left = null; m_Right = null; return; }
        bool hadLights = m_Left || m_Right;
        if (m_Left) { m_Left.SetEnabled(false); m_Left.Destroy(); m_Left = null; }
        if (m_Right) { m_Right.SetEnabled(false); m_Right.Destroy(); m_Right = null; }
        if (hadLights) Print("[RailPilotLights] OFF sources released");
    }

    void Release()
    {
        if (m_Released) return;
        Stop();
        m_Released = true;
        m_Owner = null;
        if (s_Rigs) s_Rigs.RemoveItem(this);
    }

    static void Sweep()
    {
        for (int i = s_Rigs.Count() - 1; i >= 0; i--)
        {
            RPConvoyHeadlightRig rig = s_Rigs.Get(i);
            if (!rig.m_Owner || rig.m_Owner.ToDelete() || GetGame().GetTime() - rig.m_LastUpdate > 1000)
                rig.Release();
        }
    }

    static void ClearAll()
    {
        while (s_Rigs.Count() > 0) s_Rigs.Get(0).Release();
    }

    static int RigCount() { return s_Rigs.Count(); }
};

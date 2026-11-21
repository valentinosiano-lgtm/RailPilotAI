enum RPTrainNoiseCue
{
    MotorIdle,
    MotorSlow,
    MotorCruise,
    Brake,
    BrakeAir,
    Horn
};

class RPTrainAINoise
{
    static const string VEHICLE_NOISE = "CfgVehicles OffroadHatchback NoiseCarHorn";
    static const float REFRESH_SECONDS = 1.25;
    protected static ref NoiseParams s_VehicleProfile;
    protected float m_Elapsed;
    protected int m_Motor = -1;
    protected bool m_Braking;
    protected bool m_Air;

    static vector SourcePosition(RP_ConvoyUnit loco)
    {
        vector bounds = loco.High();
        // Keep the native sound target clear of the locomotive collision model.
        return loco.ModelToWorld(Vector(0, bounds[1] + 0.3, 0));
    }

    static bool Emit(vector position, int cue)
    {
        if (!GetGame().IsServer()) return false;
        NoiseSystem hearing = GetGame().GetNoiseSystem();
        if (!hearing) return false;
        if (!s_VehicleProfile)
        {
            if (!GetGame().ConfigIsExisting(VEHICLE_NOISE)) return false;
            s_VehicleProfile = new NoiseParams;
            s_VehicleProfile.LoadFromPath(VEHICLE_NOISE);
        }
        // The engine applies its own hearing and environmental dampening.
        float lifetime = 2.0;
        float multiplier = 1.0;
        if (cue == RPTrainNoiseCue.Horn)
        {
            lifetime = 5.0;
            multiplier = 2.0;
        }
        hearing.AddNoiseTarget(position, lifetime, s_VehicleProfile, multiplier);
        return true;
    }

    void Update(vector position, float speed, float limit, bool braking, bool air, float dt)
    {
        if (dt <= 0 || limit <= 0) return;
        float fraction = speed / limit;
        int motor = RPTrainNoiseCue.MotorSlow;
        if (fraction <= 0.25) motor = RPTrainNoiseCue.MotorIdle;
        if (fraction >= 0.75) motor = RPTrainNoiseCue.MotorCruise;
        m_Elapsed += dt;
        bool refresh = m_Elapsed >= REFRESH_SECONDS;
        // One motor target also covers the overlap between the three loops.
        if (refresh || motor != m_Motor) Emit(position, motor);
        if (braking && (refresh || !m_Braking)) Emit(position, RPTrainNoiseCue.Brake);
        if (air && (refresh || !m_Air)) Emit(position, RPTrainNoiseCue.BrakeAir);
        if (refresh || motor != m_Motor) m_Elapsed = 0;
        m_Motor = motor;
        m_Braking = braking;
        m_Air = air;
    }
};

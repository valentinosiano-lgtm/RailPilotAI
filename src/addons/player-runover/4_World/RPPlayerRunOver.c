class RPPlayerRunOverTarget
{
    PlayerBase Player;
    vector Pose[4];
    vector Low;
    vector High;

    static bool ProtectedPassenger(PlayerBase player)
    {
        if (!player) return false;
        if (player.FirstRecoveryWaiting()) return true;
        return RPFirstPassengers.ParentWagon(player) != null;
    }

    bool Capture(PlayerBase player)
    {
        if (!player || !player.IsAlive() || ProtectedPassenger(player)) return false;
        vector bounds[2];
        if (!player.GetCollisionBox(bounds)) return false;
        Player = player; Low = bounds[0]; High = bounds[1];
        vector captured[4]; player.GetTransform(captured);
        for (int i = 0; i < 4; i++) Pose[i] = captured[i];
        return High[0] > Low[0] && High[1] > Low[1] && High[2] > Low[2];
    }
};

// The approved infected controller still runs unchanged. This additional
// server-only sweep shares its geometry, leading body and sampling rules.
modded class RPRunOverController
{
    protected vector m_PlayerPreviousPose[4];
    protected int m_PlayerDirection;
    protected float m_PlayerElapsed;
    protected ref array<ref RPPlayerRunOverTarget> m_PlayerPreviousTargets = new array<ref RPPlayerRunOverTarget>;
    int PlayerKills;
    int PlayerQueries;
    int PlayerSkippedDiscontinuities;

    override bool Bind(Object body, int direction)
    {
        if (!super.Bind(body, direction)) return false;
        vector pose[4]; body.GetTransform(pose);
        ResetPlayerSweep(pose, direction);
        return true;
    }

    void ResetPlayerSweep(vector pose[4], int direction)
    {
        for (int i = 0; i < 4; i++) m_PlayerPreviousPose[i] = pose[i];
        m_PlayerDirection = direction; m_PlayerElapsed = 0;
        m_PlayerPreviousTargets.Clear();
    }

    RPPlayerRunOverTarget PreviousPlayer(PlayerBase player)
    {
        for (int i = 0; i < m_PlayerPreviousTargets.Count(); i++)
            if (m_PlayerPreviousTargets[i].Player == player) return m_PlayerPreviousTargets[i];
        return null;
    }

    override void Tick(float timeslice, int direction, float serviceSpeed)
    {
        super.Tick(timeslice, direction, serviceSpeed);
        if (!GetGame().IsServer() || !m_Body || timeslice <= 0) return;
        m_PlayerElapsed += timeslice;
        if (m_PlayerElapsed < 0.05) return;
        vector pose[4]; m_Body.GetTransform(pose);
        float displacement = vector.Distance(pose[3], m_PlayerPreviousPose[3]);
        if (m_PlayerElapsed > 0.5 || displacement > 20)
        {
            PlayerSkippedDiscontinuities++; ResetPlayerSweep(pose, direction); return;
        }
        if (direction != m_PlayerDirection || serviceSpeed <= 0.05 || displacement < 0.00001)
        {
            ResetPlayerSweep(pose, direction); return;
        }
        float faceZ = m_High[2]; if (direction < 0) faceZ = m_Low[2];
        vector face = Vector((m_Low[0] + m_High[0]) * 0.5, (m_Low[1] + m_High[1]) * 0.5, faceZ);
        vector first = face.Multiply4(m_PlayerPreviousPose); vector last = face.Multiply4(pose);
        float radius = vector.Distance(first, last) * 0.5 + (m_High[0] - m_Low[0]) * 0.5 + 3;
        array<Object> objects = new array<Object>; array<CargoBase> cargo = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D((first + last) * 0.5, radius, objects, cargo); PlayerQueries++;
        array<ref RPPlayerRunOverTarget> currentTargets = new array<ref RPPlayerRunOverTarget>;
        for (int i = 0; i < objects.Count(); i++)
        {
            PlayerBase player = PlayerBase.Cast(objects[i]);
            RPPlayerRunOverTarget current = new RPPlayerRunOverTarget;
            if (!current.Capture(player)) continue;
            RPPlayerRunOverTarget before = PreviousPlayer(player);
            if (!before) before = current;
            // A player teleport is not evidence of traversing the entire gap.
            if (vector.Distance(before.Pose[3], current.Pose[3]) > 20) before = current;
            if (RPRunOverGeometry.Hits(m_PlayerPreviousPose, pose, m_Low, m_High, direction, before.Pose, current.Pose, current.Low, current.High))
            {
                // Observer characters in this isolated prototype are normally
                // invulnerable. Lift that flag ONLY for a proven frontal hit;
                // leave all survivors' original damage settings unchanged.
                bool allowed = player.GetAllowDamage();
                if (!allowed) player.SetAllowDamage(true);
                player.SetHealth("", "Health", 0);
                if (!player.IsAlive())
                {
                    PlayerKills++;
                    Print("[RailPilotPlayerRunOver] HIT player killed direction=" + direction.ToString() + " speed=" + serviceSpeed.ToString());
                }
                else
                {
                    if (!allowed) player.SetAllowDamage(false);
                    Print("[RailPilotPlayerRunOver] ERROR proven frontal hit did not kill player");
                }
            }
            currentTargets.Insert(current);
        }
        for (int j = 0; j < 4; j++) m_PlayerPreviousPose[j] = pose[j];
        m_PlayerPreviousTargets = currentTargets; m_PlayerElapsed = 0;
    }
};

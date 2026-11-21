class RPRunOverTarget
{
    ZombieBase Infected;
    vector Pose[4];
    vector Low;
    vector High;

    bool Capture(ZombieBase infected)
    {
        Infected = infected;
        vector bounds[2];
        if (!infected || !infected.IsAlive() || !infected.GetCollisionBox(bounds)) return false;
        Low = bounds[0]; High = bounds[1];
        vector capturedPose[4]; infected.GetTransform(capturedPose);
        for (int i = 0; i < 4; i++) Pose[i] = capturedPose[i];
        return High[1] > Low[1] && High[0] > Low[0] && High[2] > Low[2];
    }
};

class RPRunOverController
{
    protected Object m_Body;
    protected vector m_PreviousPose[4];
    protected vector m_Low;
    protected vector m_High;
    protected int m_Direction;
    protected float m_Elapsed;
    protected ref array<ref RPRunOverTarget> m_PreviousTargets = new array<ref RPRunOverTarget>;
    int Kills;
    int SkippedDiscontinuities;
    int Queries;

    bool Bind(Object body, int direction)
    {
        if (!GetGame().IsServer() || !body) return false;
        vector bounds[2]; if (!body.GetCollisionBox(bounds)) return false;
        m_Body = body; m_Low = bounds[0]; m_High = bounds[1];
        m_Direction = direction;
        vector initialPose[4]; body.GetTransform(initialPose);
        for (int i = 0; i < 4; i++) m_PreviousPose[i] = initialPose[i];
        m_PreviousTargets.Clear(); m_Elapsed = 0;
        return true;
    }

    void Reset(vector pose[4], int direction)
    {
        for (int i = 0; i < 4; i++) m_PreviousPose[i] = pose[i];
        m_Direction = direction; m_Elapsed = 0; m_PreviousTargets.Clear();
    }

    RPRunOverTarget Previous(ZombieBase infected)
    {
        for (int i = 0; i < m_PreviousTargets.Count(); i++)
            if (m_PreviousTargets.Get(i).Infected == infected) return m_PreviousTargets.Get(i);
        return null;
    }

    void Tick(float timeslice, int direction, float serviceSpeed)
    {
        if (!GetGame().IsServer() || !m_Body || timeslice <= 0) return;
        m_Elapsed += timeslice;
        if (m_Elapsed < 0.05) return;
        vector pose[4]; m_Body.GetTransform(pose);
        float displacement = vector.Distance(pose[3], m_PreviousPose[3]);
        if (m_Elapsed > 0.5 || displacement > 20)
        {
            SkippedDiscontinuities++; Reset(pose, direction); return;
        }
        if (direction != m_Direction || serviceSpeed <= 0.05 || displacement < 0.00001)
        {
            Reset(pose, direction); return;
        }
        float faceZ = m_High[2]; if (direction < 0) faceZ = m_Low[2];
        vector face = Vector((m_Low[0] + m_High[0]) * 0.5, (m_Low[1] + m_High[1]) * 0.5, faceZ);
        vector first = face.Multiply4(m_PreviousPose); vector last = face.Multiply4(pose);
        float radius = vector.Distance(first, last) * 0.5 + (m_High[0] - m_Low[0]) * 0.5 + 3;
        array<Object> objects = new array<Object>; array<CargoBase> cargo = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D((first + last) * 0.5, radius, objects, cargo); Queries++;
        ref array<ref RPRunOverTarget> currentTargets = new array<ref RPRunOverTarget>;
        for (int i = 0; i < objects.Count(); i++)
        {
            ZombieBase infected = ZombieBase.Cast(objects.Get(i));
            if (!infected || !infected.IsAlive()) continue;
            RPRunOverTarget current = new RPRunOverTarget;
            if (!current.Capture(infected)) continue;
            RPRunOverTarget before = Previous(infected); if (!before) before = current;
            if (RPRunOverGeometry.Hits(m_PreviousPose, pose, m_Low, m_High, direction, before.Pose, current.Pose, current.Low, current.High))
            {
                // Only a live ZombieBase reaches this call. No player damage,
                // physical impulses, body transforms or client RPCs are added.
                infected.SetHealth("", "Health", 0);
                if (!infected.IsAlive())
                {
                    Kills++;
                    string message = "[RailPilotRunOver] HIT infected=" + infected.GetType();
                    message += " direction=" + direction.ToString() + " speed=" + serviceSpeed.ToString();
                    Print(message);
                }
            }
            currentTargets.Insert(current);
        }
        for (int k = 0; k < 4; k++) m_PreviousPose[k] = pose[k];
        m_PreviousTargets = currentTargets; m_Elapsed = 0;
    }
};

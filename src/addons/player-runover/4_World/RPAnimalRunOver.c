class RPAnimalRunOverTarget
{
    AnimalBase Creature;
    vector Pose[4];
    vector Low;
    vector High;

    bool Capture(AnimalBase animal)
    {
        if (!animal || !animal.IsAlive()) return false;
        if (RP_ConvoyUnit.Cast(animal.GetParent())) return false;
        vector bounds[2];
        if (!animal.GetCollisionBox(bounds)) return false;
        Creature = animal; Low = bounds[0]; High = bounds[1];
        vector captured[4]; animal.GetTransform(captured);
        for (int i = 0; i < 4; i++) Pose[i] = captured[i];
        return High[0] > Low[0] && High[1] > Low[1] && High[2] > Low[2];
    }
};

// Independent animal sweep; the infected/player implementations are unchanged.
// All species use their actual native bounds and the same leading-face test.
modded class RPRunOverController
{
    protected vector m_AnimalPreviousPose[4];
    protected int m_AnimalDirection;
    protected float m_AnimalElapsed;
    protected ref array<ref RPAnimalRunOverTarget> m_AnimalPreviousTargets = new array<ref RPAnimalRunOverTarget>;
    int AnimalKills;
    int AnimalQueries;
    int AnimalSkippedDiscontinuities;

    override bool Bind(Object body, int direction)
    {
        if (!super.Bind(body, direction)) return false;
        vector pose[4]; body.GetTransform(pose);
        ResetAnimalSweep(pose, direction);
        return true;
    }
    void ResetAnimalSweep(vector pose[4], int direction)
    {
        for (int i = 0; i < 4; i++) m_AnimalPreviousPose[i] = pose[i];
        m_AnimalDirection = direction; m_AnimalElapsed = 0; m_AnimalPreviousTargets.Clear();
    }
    RPAnimalRunOverTarget PreviousAnimal(AnimalBase animal)
    {
        for (int i = 0; i < m_AnimalPreviousTargets.Count(); i++)
            if (m_AnimalPreviousTargets[i].Creature == animal) return m_AnimalPreviousTargets[i];
        return null;
    }
    override void Tick(float timeslice, int direction, float serviceSpeed)
    {
        super.Tick(timeslice, direction, serviceSpeed);
        if (!GetGame().IsServer() || !m_Body || timeslice <= 0) return;
        m_AnimalElapsed += timeslice;
        if (m_AnimalElapsed < 0.05) return;
        vector pose[4]; m_Body.GetTransform(pose);
        float displacement = vector.Distance(pose[3], m_AnimalPreviousPose[3]);
        if (m_AnimalElapsed > 0.5 || displacement > 20)
        {
            AnimalSkippedDiscontinuities++; ResetAnimalSweep(pose, direction); return;
        }
        if (direction != m_AnimalDirection || serviceSpeed <= 0.05 || displacement < 0.00001)
        {
            ResetAnimalSweep(pose, direction); return;
        }
        float faceZ = m_High[2]; if (direction < 0) faceZ = m_Low[2];
        vector face = Vector((m_Low[0] + m_High[0]) * 0.5, (m_Low[1] + m_High[1]) * 0.5, faceZ);
        vector first = face.Multiply4(m_AnimalPreviousPose); vector last = face.Multiply4(pose);
        // Small fauna can fit below the locomotive's visual lower bound on
        // the embankment. Include the ground at the leading face, capped at
        // one metre below that bound; never sweep down to a river under a bridge.
        vector ground = Vector(last[0], GetGame().SurfaceY(last[0], last[2]), last[2]);
        vector localGround = ground.InvMultiply4(pose);
        vector animalLow = m_Low;
        animalLow[1] = Math.Min(m_Low[1], Math.Max(m_Low[1] - 1, localGround[1]));
        float radius = vector.Distance(first, last) * 0.5 + (m_High[0] - m_Low[0]) * 0.5 + 3;
        array<Object> objects = new array<Object>; array<CargoBase> cargo = new array<CargoBase>;
        GetGame().GetObjectsAtPosition3D((first + last) * 0.5, radius, objects, cargo); AnimalQueries++;
        array<ref RPAnimalRunOverTarget> currentTargets = new array<ref RPAnimalRunOverTarget>;
        for (int i = 0; i < objects.Count(); i++)
        {
            AnimalBase animal = AnimalBase.Cast(objects[i]);
            RPAnimalRunOverTarget current = new RPAnimalRunOverTarget;
            if (!current.Capture(animal)) continue;
            RPAnimalRunOverTarget before = PreviousAnimal(animal);
            if (!before) before = current;
            if (vector.Distance(before.Pose[3], current.Pose[3]) > 20) before = current;
            if (RPRunOverGeometry.Hits(m_AnimalPreviousPose, pose, animalLow, m_High, direction, before.Pose, current.Pose, current.Low, current.High))
            {
                string species = animal.GetType();
                animal.SetHealth("", "Health", 0);
                // Some species replace their entity with a carcass at death.
                if (!animal || !animal.IsAlive())
                {
                    AnimalKills++;
                    Print("[RailPilotAnimalRunOver] HIT animal=" + species + " direction=" + direction.ToString() + " speed=" + serviceSpeed.ToString());
                }
                else Print("[RailPilotAnimalRunOver] ERROR proven frontal hit did not kill animal=" + species);
            }
            currentTargets.Insert(current);
        }
        for (int j = 0; j < 4; j++) m_AnimalPreviousPose[j] = pose[j];
        m_AnimalPreviousTargets = currentTargets; m_AnimalElapsed = 0;
    }
};

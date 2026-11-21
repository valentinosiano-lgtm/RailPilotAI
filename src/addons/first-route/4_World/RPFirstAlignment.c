// Rail contact measurements from the installed unmodified visual meshes.
// Evidence: .local/first-route/wheel-contact-analysis.json. Model origin is
// not the wheel centre. Never shift rails or use a station-specific offset.
class RPFirstAlignment
{
    static void Profile(int slot, out float centreX, out float centreZ, out float bottom, out float bogieHalf)
    {
        if (slot == 0 || slot == 5)
        {
            centreX = -0.115561; centreZ = -0.029685; bottom = -2.165714; bogieHalf = 3.155165;
        }
        else if (slot == 1)
        {
            centreX = 0.0188915; centreZ = -0.0302655; bottom = -1.762348; bogieHalf = 3.27455;
        }
        else if (slot == 2 || slot == 3)
        {
            centreX = -0.009268; centreZ = -0.031015; bottom = -1.587012; bogieHalf = 3.66433;
        }
        else
        {
            centreX = 0.0354285; centreZ = 0.003751; bottom = -0.383156; bogieHalf = 6.046553;
        }
    }
    static void Pose(RPConvoyRoute route, int slot, float distance, out vector matrix[4])
    {
        BuildPose(route, slot, distance, false, matrix);
    }
    static void PoseAtClock(RPConvoyRoute route, int slot, float clock, float offset, out vector matrix[4])
    {
        float distance; float speed; int direction; route.State(clock, distance, speed, direction);
        BuildSegmentPose(route, slot, distance - offset, route.SegmentAt(clock), matrix);
    }
    static void BuildPose(RPConvoyRoute route, int slot, float distance, bool reversing, out vector matrix[4])
    {
        int segment;
        if (reversing) segment = 1;
        BuildSegmentPose(route, slot, distance, segment, matrix);
    }
    static void BuildSegmentPose(RPConvoyRoute route, int slot, float distance, int segment, out vector matrix[4])
    {
        float centreX; float centreZ; float bottom; float half;
        Profile(slot, centreX, centreZ, bottom, half);
        float sign = 1; if (slot == 5) sign = -1;
        vector back = route.SegmentPoint(distance + sign * centreZ - half, segment);
        vector front = route.SegmentPoint(distance + sign * centreZ + half, segment);
        vector forward = front - back; forward.Normalize();
        Math3D.DirectionAndUpMatrix(forward, vector.Up, matrix);
        if (slot == 5) { matrix[0] = -matrix[0]; matrix[2] = -matrix[2]; }
        matrix[3] = (front + back) * 0.5 - matrix[0] * centreX - matrix[2] * centreZ - matrix[1] * bottom;
    }
};

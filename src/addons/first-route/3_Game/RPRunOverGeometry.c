// Intersect the swept leading face with the target's collision bounds.
// Radius queries are broad phase only; they never decide a kill.
class RPRunOverGeometry
{
    static void RelativeBounds(vector targetPose[4], vector low, vector high, vector trainPose[4], out vector resultLow, out vector resultHigh)
    {
        resultLow = "100000 100000 100000"; resultHigh = "-100000 -100000 -100000";
        for (int x = 0; x < 2; x++)
        {
            for (int y = 0; y < 2; y++)
            {
                for (int z = 0; z < 2; z++)
                {
                    vector corner = low;
                    if (x == 1) corner[0] = high[0];
                    if (y == 1) corner[1] = high[1];
                    if (z == 1) corner[2] = high[2];
                    vector world = corner.Multiply4(targetPose);
                    vector point = world.InvMultiply4(trainPose);
                    for (int axis = 0; axis < 3; axis++)
                    {
                        resultLow[axis] = Math.Min(resultLow[axis], point[axis]);
                        resultHigh[axis] = Math.Max(resultHigh[axis], point[axis]);
                    }
                }
            }
        }
    }

    // Clip an interval against a linear inequality value(t) <= limit.
    static bool Clip(float before, float after, float limit, inout float entry, inout float leave)
    {
        float delta = after - before;
        if (Math.AbsFloat(delta) < 0.000001) return before <= limit;
        float crossing = (limit - before) / delta;
        if (delta > 0) leave = Math.Min(leave, crossing);
        else entry = Math.Max(entry, crossing);
        return entry <= leave;
    }

    static bool Hits(vector previousTrain[4], vector currentTrain[4], vector trainLow, vector trainHigh, int direction, vector previousTarget[4], vector currentTarget[4], vector targetLow, vector targetHigh)
    {
        if (direction != 1 && direction != -1) return false;
        vector motion = currentTrain[3] - previousTrain[3];
        if (vector.Dot(motion, previousTrain[2]) * direction <= 0.00001) return false;
        vector lowBefore; vector highBefore; vector lowAfter; vector highAfter;
        RelativeBounds(previousTarget, targetLow, targetHigh, previousTrain, lowBefore, highBefore);
        RelativeBounds(currentTarget, targetLow, targetHigh, currentTrain, lowAfter, highAfter);
        float front = trainHigh[2];
        float nearBefore = lowBefore[2] - front; float farBefore = highBefore[2] - front;
        float nearAfter = lowAfter[2] - front; float farAfter = highAfter[2] - front;
        if (direction < 0)
        {
            front = trainLow[2];
            nearBefore = front - highBefore[2]; farBefore = front - lowBefore[2];
            nearAfter = front - highAfter[2]; farAfter = front - lowAfter[2];
        }
        // A target already behind the leading face is not run over by proximity.
        if (farBefore < 0) return false;
        float entry = 0; float leave = 1;
        if (!Clip(nearBefore, nearAfter, 0, entry, leave)) return false;
        if (!Clip(-farBefore, -farAfter, 0, entry, leave)) return false;
        if (!Clip(lowBefore[0], lowAfter[0], trainHigh[0], entry, leave)) return false;
        if (!Clip(-highBefore[0], -highAfter[0], -trainLow[0], entry, leave)) return false;
        if (!Clip(lowBefore[1], lowAfter[1], trainHigh[1], entry, leave)) return false;
        if (!Clip(-highBefore[1], -highAfter[1], -trainLow[1], entry, leave)) return false;
        return entry <= leave && leave >= 0 && entry <= 1;
    }
};

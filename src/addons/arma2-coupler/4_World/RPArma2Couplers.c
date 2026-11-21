class RP_Arma2ScrewCoupler : House {}
class RP_Arma2HookOriginal : House {}

// Visual links only. Five local, nonpersistent objects; no physics bodies,
// constraints, train transforms or changes to the approved wagon models.
class RPArma2Couplers
{
    protected static ref array<RP_Arma2ScrewCoupler> s_Links = new array<RP_Arma2ScrewCoupler>;
    static int Count() { return s_Links.Count(); }
    static RP_Arma2ScrewCoupler GetLink(int index)
    {
        if (index < 0 || index >= s_Links.Count()) return null;
        return s_Links[index];
    }
    // Preliminary contact points on the original Arma2 hook geometry, in native
    // Y-up coordinates. Native train transforms supply the current rail pose.
    static vector Arma2HookPoint(int slot, bool positive)
    {
        float side = -1; if (positive) side = 1;
        if (slot == 0 || slot == 5)
        {
            if (positive) return Vector(0.0036, 0.990, 6.5191);
            return Vector(0.0036, 0.990, -6.5046);
        }
        if (slot == 1)
        {
            if (positive) return Vector(0.0077, 0.830, 5.5287);
            return Vector(0.0070, 0.830, -5.5197);
        }
        if (slot == 4) return Vector(0.0066, 0.837, side * 8.5385);
        if (positive) return Vector(0.0082, 0.870, 5.8296);
        return Vector(0.0070, 0.870, -5.9906);
    }
    static vector LocalHookPoint(int slot, bool positive)
    {
        // Arma2 samples are based near wheel height. DayZ's existing bodies
        // have centered origins. Map between their measured wheel references;
        // never change a train pose/model to fit a decorative coupling.
        vector sourceWheel;
        if (slot == 0 || slot == 5) sourceWheel = Vector(0.000382513, 0.156184852, -0.022135019);
        else if (slot == 1) sourceWheel = Vector(0.018891573, -0.002933145, -0.028400421);
        else if (slot == 4) sourceWheel = Vector(0.001460552, 0.000947773, 0.003746033);
        else sourceWheel = Vector(-0.000097007, 0.037485003, -0.116095066);
        float centreX; float centreZ; float bottom; float half;
        RPFirstAlignment.Profile(slot, centreX, centreZ, bottom, half);
        vector hookPoint = Arma2HookPoint(slot, positive);
        // The loop bears inside the hook, behind its outward nose. This seat
        // also keeps the screw shaft extended instead of inverting its ends.
        if (positive) hookPoint[2] = hookPoint[2] - 0.12;
        else hookPoint[2] = hookPoint[2] + 0.12;
        return hookPoint - sourceWheel + Vector(centreX, bottom, centreZ);
    }
    static bool Ends(int index, out vector start, out vector end)
    {
        RP_ConvoyUnit a = RP_ConvoyUnit.AtSlot(index);
        RP_ConvoyUnit b = RP_ConvoyUnit.AtSlot(index + 1);
        if (!a || !b || !a.FirstPresentationReady() || !b.FirstPresentationReady()) return false;
        start = a.ModelToWorld(LocalHookPoint(index, false));
        end = b.ModelToWorld(LocalHookPoint(index + 1, index + 1 != 5));
        float length = vector.Distance(start, end);
        return length >= 0.54 && length <= 2;
    }
    static void Clear()
    {
        for (int i = 0; i < s_Links.Count(); i++) if (s_Links[i]) GetGame().ObjectDelete(s_Links[i]);
        s_Links.Clear();
    }
    static void UpdateAll(bool nativeAudit = false)
    {
        if (GetGame().IsDedicatedServer() && !nativeAudit) return;
        if (RP_ConvoyUnit.All().Count() != 6) { Clear(); return; }
        if (s_Links.Count() == 0)
        {
            for (int create = 0; create < 5; create++)
            {
                int flags = ECE_LOCAL | ECE_KEEPHEIGHT | ECE_NOLIFETIME | ECE_NOPERSISTENCY_WORLD;
                RP_Arma2ScrewCoupler link = RP_Arma2ScrewCoupler.Cast(GetGame().CreateObjectEx("RP_Arma2ScrewCoupler", vector.Zero, flags));
                s_Links.Insert(link);
            }
        }
        for (int i = 0; i < s_Links.Count(); i++)
        {
            RP_Arma2ScrewCoupler item = s_Links[i]; if (!item) continue;
            vector start; vector end;
            if (!Ends(i, start, end)) { item.SetInvisible(true); continue; }
            vector forward = end - start; float length = forward.Length(); forward.Normalize();
            vector pose[4]; Math3D.DirectionAndUpMatrix(forward, vector.Up, pose); pose[3] = start;
            item.SetAnimationPhaseNow("span", (length - 0.3) / 1.7);
            // Binarize can recenter a visual-only model. Use its compiled
            // start marker instead of assuming the authoring origin survives.
            vector localStart = item.GetSelectionPositionMS("link_start");
            vector correction = pose[0] * localStart[0];
            correction = correction + pose[1] * localStart[1];
            correction = correction + pose[2] * localStart[2];
            pose[3] = start - correction;
            item.SetTransform(pose);
            item.SetInvisible(false); item.Update();
        }
    }
};

// RailPilot's own JSON models. No dependencies on external train mods.
class RPSettings
{
    int SchemaVersion = 1;
    int Enabled = 0;
    float CruiseSpeedKph = 18.0;
    float AccelerationMps2 = 0.25;
    float ServiceBrakeMps2 = 0.40;
    float TickSeconds = 0.25;
    float SaveEverySeconds = 10.0;
    float DefaultDwellSeconds = 30.0;
    int DebugLogs = 0;

    // Experimental only: moving a vanilla wreck is NOT a rideable train.
    int EnableDebugVisual = 0;
    string DebugVisualClass = "Land_Train_742_Blue";
};

class RPWaypoint
{
    float X;
    float Y;
    float Z;
    string Name;
    int IsStation;
    float DwellSeconds;

    vector GetWorldPosition()
    {
        return Vector(X, Y, Z);
    }
};

class RPRoute
{
    int SchemaVersion = 2;
    string RouteId = "ChernarusPlus-rail-survey-v1";
    string Map = "chernarusplus";
    string ServiceMode = "Shuttle"; // Continuous return over the same surveyed track.
    ref array<ref RPWaypoint> Points;

    void RPRoute()
    {
        Points = new array<ref RPWaypoint>;
    }
};

class RPState
{
    int SchemaVersion = 3;
    int Generation = 0;
    string RouteId = "";
    int RouteFingerprint;
    int Direction = 1; // +1 outbound, -1 inbound; speed is always a magnitude.
    float DistanceMeters = 0.0;
    float SpeedMps = 0.0;
    float DwellRemainingSeconds = 0.0;
    int NextStopPointIndex = -1;
    int Phase = 0; // 0 approaching next stop, 1 dwelling at that stop.
};

// Closed intervals reject NaN as well as infinities without undocumented APIs.
class RPLimits
{
    static bool InRange(float value, float low, float high)
    {
        return value >= low && value <= high;
    }

    static bool Flag(int value)
    {
        return value == 0 || value == 1;
    }
};

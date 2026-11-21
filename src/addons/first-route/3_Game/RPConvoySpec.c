// One consist per service. Slots never change on reversal.
class RPConvoySpec
{
    static const int COUNT = 6;
    static const float COUPLING_GAP = 0.5;
    static const float FUEL_ML = 100000;

    static string TypeAt(int slot)
    {
        if (slot == 0 || slot == 5) return "RP_ConvoyBlue742";
        if (slot == 1) return "RP_ConvoyTanker";
        if (slot == 2 || slot == 3) return "RP_ConvoyBox";
        if (slot == 4) return "RP_ConvoyFlat";
        return "";
    }

    static int LeadingSlot(int direction)
    {
        if (direction < 0) return 5;
        return 0;
    }
};

class RPConvoyStartup
{
    int Enabled;
};

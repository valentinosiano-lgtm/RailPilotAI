// Own spatial policy. No world entities, physics walls or periodic scans.
class RPCCBox
{
    vector C; vector X; vector Y; vector Z; vector E;
    vector Low; vector High;
    void Bounds()
    {
        vector r;
        for (int j = 0; j < 3; j++) r[j] = Math.AbsFloat(X[j])*E[0] + Math.AbsFloat(Y[j])*E[1] + Math.AbsFloat(Z[j])*E[2];
        Low=C-r; High=C+r;
    }
    float Radius(vector axis)
    {
        return Math.AbsFloat(vector.Dot(axis,X))*E[0]+Math.AbsFloat(vector.Dot(axis,Y))*E[1]+Math.AbsFloat(vector.Dot(axis,Z))*E[2];
    }
    bool Separated(RPCCBox other, vector axis)
    {
        if (axis.LengthSq()<0.00000001) return false;
        return Math.AbsFloat(vector.Dot(other.C-C,axis)) > Radius(axis)+other.Radius(axis)+0.00001;
    }
    bool Intersects(RPCCBox other)
    {
        if (Low[0]>other.High[0] || High[0]<other.Low[0] || Low[1]>other.High[1] || High[1]<other.Low[1] || Low[2]>other.High[2] || High[2]<other.Low[2]) return false;
        if (Separated(other,X)||Separated(other,Y)||Separated(other,Z)||Separated(other,other.X)||Separated(other,other.Y)||Separated(other,other.Z)) return false;
        vector a[3]; a[0]=X;a[1]=Y;a[2]=Z;
        vector b[3]; b[0]=other.X;b[1]=other.Y;b[2]=other.Z;
        for (int i=0;i<3;i++) for (int j=0;j<3;j++)
        {
            vector v=a[i];vector w=b[j];
            vector cross=Vector(v[1]*w[2]-v[2]*w[1],v[2]*w[0]-v[0]*w[2],v[0]*w[1]-v[1]*w[0]);
            if (Separated(other,cross)) return false;
        }
        return true;
    }
};
class RPCCData
{
    int Schema; float Margin; float NumericAllowance; string RouteSha256;
    ref array<ref RPCCBox> Boxes;
};
class RPCCBucket
{
    ref array<RPCCBox> Boxes=new array<RPCCBox>;
};
class RPCorridor
{
    protected static ref RPCCData s_Data;
    protected static ref RPCCData s_WestData;
    protected static ref RPCCData s_NovayaData;
    protected static ref RPCCData s_SeverogradData;
    protected static ref RPCCData s_NovodmitrovskData;
    protected static ref RPCCData s_SvetloyarskData;
    protected static ref RPCCData s_BerezinoData;
    protected static ref RPCCData s_SouthCoastData;
    protected static ref map<int,ref RPCCBucket> s_Cells;
    protected static bool s_Attempted;
    static int Queries;
    static int Candidates;
    static bool Ready()
    {
        if (s_Attempted) return s_Data && s_Cells;
        s_Attempted=true;
        string error;
        if (!JsonFileLoader<RPCCData>.LoadFile("RailPilot\\RailCorridor\\corridor.json",s_Data,error))
        { Print("[RailPilotCorridor] ERROR geometry load "+error);s_Data=null;return false; }
        if (!s_Data || s_Data.Schema!=1 || s_Data.Margin!=1 || !s_Data.Boxes || s_Data.Boxes.Count()==0) { s_Data=null;return false; }
        if (!JsonFileLoader<RPCCData>.LoadFile("RailPilot\\RailCorridor\\west-corridor.json",s_WestData,error))
        { Print("[RailPilotCorridor] ERROR western geometry load "+error);s_Data=null;return false; }
        if (!s_WestData || s_WestData.Schema!=1 || s_WestData.Margin!=s_Data.Margin || !s_WestData.Boxes || s_WestData.Boxes.Count()==0)
        { s_Data=null;return false; }
        s_Cells=new map<int,ref RPCCBucket>;
        IndexBoxes(s_Data.Boxes);
        IndexBoxes(s_WestData.Boxes);
        int count = s_Data.Boxes.Count() + s_WestData.Boxes.Count();
        string routeHash = s_WestData.RouteSha256;
        if (!IndexExtension("novaya-corridor.json", s_NovayaData, count, routeHash)) { s_Data = null; return false; }
        if (!IndexExtension("severograd-corridor.json", s_SeverogradData, count, routeHash)) { s_Data = null; return false; }
        if (!IndexExtension("novodmitrovsk-corridor.json", s_NovodmitrovskData, count, routeHash)) { s_Data = null; return false; }
        if (!IndexExtension("svetloyarsk-corridor.json", s_SvetloyarskData, count, routeHash)) { s_Data = null; return false; }
        if (!IndexExtension("berezino-corridor.json", s_BerezinoData, count, routeHash)) { s_Data = null; return false; }
        if (!IndexExtension("south-coast-corridor.json", s_SouthCoastData, count, routeHash)) { s_Data = null; return false; }
        Print("[RailPilotCorridor] READY boxes=" + count.ToString() + " margin=1m, route=" + routeHash);
        return true;
    }
    protected static bool IndexExtension(string fileName, out RPCCData layer, inout int count, inout string routeHash)
    {
        string path = "RailPilot\\RailCorridor\\" + fileName;
        if (!FileExist(path)) return true;
        string error;
        if (!JsonFileLoader<RPCCData>.LoadFile(path, layer, error) || !layer)
        { Print("[RailPilotCorridor] ERROR geometry load " + fileName + " " + error); return false; }
        if (layer.Schema != 1 || layer.Margin != s_Data.Margin || !layer.Boxes || layer.Boxes.Count() == 0) return false;
        IndexBoxes(layer.Boxes);
        count += layer.Boxes.Count();
        routeHash = layer.RouteSha256;
        return true;
    }
    protected static void IndexBoxes(array<ref RPCCBox> boxes)
    {
        foreach (RPCCBox box:boxes)
        {
            box.Bounds();
            int x0=Math.Floor(box.Low[0]/16);int x1=Math.Floor(box.High[0]/16);
            int z0=Math.Floor(box.Low[2]/16);int z1=Math.Floor(box.High[2]/16);
            for (int x=x0;x<=x1;x++) for (int z=z0;z<=z1;z++)
            {
                int key=x*2048+z;
                RPCCBucket bucket;
                if (!s_Cells.Find(key,bucket)) { bucket=new RPCCBucket;s_Cells.Insert(key,bucket); }
                bucket.Boxes.Insert(box);
            }
        }
    }
    static RPCCBox MakeBox(vector low,vector high,vector pose[4])
    {
        RPCCBox box=new RPCCBox;
        vector centre=(low+high)*0.5;
        box.C=centre.Multiply4(pose);box.X=pose[0];box.Y=pose[1];box.Z=pose[2];box.E=(high-low)*0.5;box.Bounds();
        return box;
    }
    static bool Blocks(RPCCBox query)
    {
        Queries++;
        // A damaged/missing geometry package must not silently disable the rule.
        if (!Ready()) return true;
        query.Bounds();
        int x0=Math.Floor(query.Low[0]/16);int x1=Math.Floor(query.High[0]/16);
        int z0=Math.Floor(query.Low[2]/16);int z1=Math.Floor(query.High[2]/16);
        // Clamp a malformed huge footprint to the finite route's existing cells.
        if (x1-x0>2048 || z1-z0>2048) return true;
        for (int x=x0;x<=x1;x++) for (int z=z0;z<=z1;z++)
        {
            RPCCBucket bucket;
            if (!s_Cells.Find(x*2048+z,bucket)) continue;
            foreach (RPCCBox box:bucket.Boxes)
            {
                Candidates++;
                if (box.Intersects(query)) return true;
            }
        }
        return false;
    }
    static bool ObjectBlocked(Object object)
    {
        if (!object) return false;
        vector bounds[2];vector pose[4];object.GetTransform(pose);
        if (!object.GetCollisionBox(bounds)) return false;
        return Blocks(MakeBox(bounds[0],bounds[1],pose));
    }
    static bool PartBlocked(ConstructionBase construction,string partName)
    {
        if (!construction) return false;
        return construction.RPCorridorPartBlocked(partName);
    }
};

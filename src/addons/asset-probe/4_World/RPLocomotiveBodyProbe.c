// Private physics/visual entity. No inventory manipulation or passenger use.
class RP_LocomotiveBodyProbe : ItemBase
{
    protected static ref array<RP_LocomotiveBodyProbe> s_MotionBodies = new array<RP_LocomotiveBodyProbe>;
    protected float m_MotionElapsed;
    protected float m_MotionFlushElapsed;
    protected string m_MotionRows;
    protected string m_MotionFile;
    protected bool m_MotionDone;
    protected bool m_MotionHasOrigin;
    protected vector m_MotionOrigin;
    protected vector m_MotionRenderOrigin;
    protected Object m_ClientVisual;
    protected ref RPVisualMotionBuffer m_VisualBuffer;
    protected float m_VisualClock;
    protected float m_LastRawChange;
    protected float m_LastVisualSnapshot;
    protected vector m_LastRawPosition;
    protected bool m_VisualFailed;

    override void EEInit()
    {
        super.EEInit();
        m_MotionFile = "$profile:RailPilotMotion-client.csv";
        if (GetGame().IsServer())
            m_MotionFile = "$profile:RailPilotMotion-server.csv";
        FileHandle handle = OpenFile(m_MotionFile, FileMode.WRITE);
        if (handle)
        {
            FPrintln(handle, "elapsed,dt,physics_x,physics_y,physics_z,render_x,render_y,render_z,vx,vy,vz,presentation_x,presentation_y,presentation_z");
            CloseFile(handle);
        }
        s_MotionBodies.Insert(this);
        Print("[RailPilotMotion] Sampling position/render/frame time for 60 seconds: " + m_MotionFile);
    }

    static void SampleAllMotion(float timeSlice)
    {
        for (int i = 0; i < s_MotionBodies.Count(); i++)
        {
            RP_LocomotiveBodyProbe body = s_MotionBodies.Get(i);
            if (body)
            {
                body.UpdateClientVisual(timeSlice);
                body.SampleMotion(timeSlice);
            }
        }
    }

    protected void UpdateClientVisual(float timeSlice)
    {
        if (GetGame().IsServer() || m_VisualFailed || timeSlice <= 0)
            return;
        m_VisualClock += timeSlice;
        vector raw = GetPosition();
        if (!m_ClientVisual)
        {
            m_ClientVisual = GetGame().CreateObjectEx("RP_LocomotiveVisualProbe", raw, ECE_LOCAL | ECE_KEEPHEIGHT);
            if (!m_ClientVisual)
            {
                SetFlags(EntityFlags.VISIBLE, false);
                m_VisualFailed = true;
                Print("[RailPilotMotion] Local visual creation failed; authoritative body remains visible.");
                return;
            }
            // This is a NEW local visual entity, not a moving House. It has
            // no collision body and cannot carry/attach players or cargo.
            if (dBodyIsSet(m_ClientVisual))
                dBodyDestroy(m_ClientVisual);
            m_ClientVisual.ClearFlags(EntityFlags.SOLID, false);
            m_ClientVisual.SetAllowDamage(false);
            m_VisualBuffer = new RPVisualMotionBuffer;
            m_VisualBuffer.Push(m_VisualClock, raw);
            m_LastRawPosition = raw;
            m_LastRawChange = m_VisualClock;
            m_LastVisualSnapshot = m_VisualClock;
            Print("[RailPilotMotion] Client visual interpolation active, delay 120ms; server physics unchanged.");
        }
        bool changed = vector.Distance(raw, m_LastRawPosition) > 0.0002;
        if (changed)
        {
            m_VisualBuffer.Push(m_VisualClock, raw);
            m_LastRawPosition = raw;
            m_LastRawChange = m_VisualClock;
            m_LastVisualSnapshot = m_VisualClock;
        }
        else if (m_VisualClock - m_LastRawChange > 0.15 && m_VisualClock - m_LastVisualSnapshot >= 0.05)
        {
            // Stationary heartbeat prevents bridging a long dwell at restart.
            m_VisualBuffer.Push(m_VisualClock, raw);
            m_LastVisualSnapshot = m_VisualClock;
        }
        m_ClientVisual.SetPosition(m_VisualBuffer.Sample(m_VisualClock));
        m_ClientVisual.SetOrientation(GetOrientation());
        ClearFlags(EntityFlags.VISIBLE, false);
    }

    protected void SampleMotion(float timeSlice)
    {
        if (m_MotionDone || timeSlice <= 0)
            return;
        m_MotionElapsed += timeSlice;
        m_MotionFlushElapsed += timeSlice;
        vector position = GetPosition();
        vector matrix[4];
        GetRenderTransform(matrix);
        vector rendered = matrix[3];
        vector presentation = rendered;
        if (m_ClientVisual)
        {
            m_ClientVisual.GetRenderTransform(matrix);
            presentation = matrix[3];
        }
        // Relative coordinates preserve sub-centimetre detail: default float
        // formatting rounds large absolute world coordinates too aggressively.
        if (!m_MotionHasOrigin)
        {
            m_MotionOrigin = position;
            m_MotionRenderOrigin = rendered;
            m_MotionHasOrigin = true;
        }
        position = position - m_MotionOrigin;
        rendered = rendered - m_MotionRenderOrigin;
        presentation = presentation - m_MotionOrigin;
        vector velocity;
        if (dBodyIsSet(this))
            velocity = GetVelocity(this);
        string row = m_MotionElapsed.ToString() + "," + timeSlice.ToString();
        row += "," + RPMotionCsv3(position);
        row += "," + RPMotionCsv3(rendered);
        row += "," + RPMotionCsv3(velocity);
        row += "," + RPMotionCsv3(presentation) + "\n";
        m_MotionRows += row;
        if (m_MotionFlushElapsed >= 1)
        {
            FlushMotionRows();
            m_MotionFlushElapsed = 0;
        }
        if (m_MotionElapsed >= 60)
        {
            m_MotionDone = true;
            FlushMotionRows();
            Print("[RailPilotMotion] Sampling complete: " + m_MotionFile);
        }
    }

    protected string RPMotionCsv3(vector value)
    {
        return value[0].ToString() + "," + value[1].ToString() + "," + value[2].ToString();
    }

    protected void FlushMotionRows()
    {
        if (m_MotionRows == "" || m_MotionFile == "")
            return;
        FileHandle handle = OpenFile(m_MotionFile, FileMode.APPEND);
        if (!handle)
            return;
        FPrint(handle, m_MotionRows);
        CloseFile(handle);
        m_MotionRows = "";
    }

    void ~RP_LocomotiveBodyProbe()
    {
        if (s_MotionBodies)
            s_MotionBodies.RemoveItem(this);
        if (m_ClientVisual && GetGame())
            GetGame().ObjectDelete(m_ClientVisual);
        FlushMotionRows();
    }

    override bool CanPutInCargo(EntityAI parent)
    {
        return false;
    }

    override bool CanPutIntoHands(EntityAI parent)
    {
        return false;
    }
};

class RPCrossingWarningLight : PointLightBase
{
    void RPCrossingWarningLight()
    {
        SetVisibleDuringDaylight(true);
        SetRadiusTo(1.5);
        SetBrightnessTo(5);
        SetDiffuseColor(1,0,0);
        SetAmbientColor(0.35,0,0);
        SetCastShadow(false);
        SetFlareVisible(true);
        SetFlareRelPosition(vector.Zero);
        SetFadeOutTime(0);
    }
};

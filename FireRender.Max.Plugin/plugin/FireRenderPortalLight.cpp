/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "frWrap.h"
#include "Common.h"

#include "Resource.h"

#include "plugin/FireRenderPortalLight.h"

#include "utils/Utils.h"

#include <math.h>

#include "ParamBlock.h"
#include <iparamb2.h>
#include <LINSHAPE.H>
#include <3dsmaxport.h> //for DLGetWindowLongPtr
#include <hsv.h>

#include <decomp.h>
#include <iparamm2.h>
#include <memory>

FIRERENDER_NAMESPACE_BEGIN

TCHAR* FIRERENDER_PORTALLIGHT_CATEGORY = _T("Radeon ProRender");

TCHAR* FIRERENDER_PORTALLIGHT_OBJECT_NAME = _T("RPRPortalLight");
TCHAR* FIRERENDER_PORTALLIGHT_INTERNAL_NAME = _T("Portal Light");
TCHAR* FIRERENDER_PORTALLIGHT_CLASS_NAME = _T("Portal Light");

class FireRenderPortalLightClassDesc : public ClassDesc2
{
public:
    int             IsPublic() override { return 1; } // DEACTIVATED! SET TO 1 TO REACTIVATE.
    HINSTANCE       HInstance() override
    { 
        FASSERT(fireRenderHInstance != NULL);
        return fireRenderHInstance;
    }

    //used in MaxScript to create objects of this class
    const TCHAR*    InternalName() override { return FIRERENDER_PORTALLIGHT_INTERNAL_NAME; }

    //used as lable in creation UI and in MaxScript to create objects of this class
    const TCHAR*    ClassName() override {
        return FIRERENDER_PORTALLIGHT_CLASS_NAME;
    }
    Class_ID        ClassID() override { return FIRERENDER_PORTALLIGHT_CLASS_ID; }
    SClass_ID       SuperClassID() override { return LIGHT_CLASS_ID; }
    const TCHAR*    Category() override { return FIRERENDER_PORTALLIGHT_CATEGORY; }
    void*           Create(BOOL loading) override ;
};

static FireRenderPortalLightClassDesc desc;

static const int VERSION =  1;

static ParamBlockDesc2 paramBlock(0, _T("PortalLightPbDesc"), 0, &desc, P_AUTO_CONSTRUCT + P_VERSION + P_AUTO_UI,
    VERSION, // for P_VERSION
    0, //for P_AUTO_CONSTRUCT
    //P_AUTO_UI
    IDD_FIRERENDER_PORTALLIGHT, IDS_FR_ENV, 0, 0, NULL,

    PORTAL_PARAM_P0, _T("p0"), TYPE_POINT3, 0, 0,
    p_default, Point3(0.f, 0.f, 0.f), PB_END,

    PORTAL_PARAM_P1, _T("p1"), TYPE_POINT3, 0, 0,
    p_default, Point3(0.f, 0.f, 0.f), PB_END,
    
    p_end
);

ClassDesc2* GetFireRenderPortalLightDesc()
{
    return &desc; 
}

class FireRenderPortalLight : public GenLight, public IFireRenderPortalLight
{
    Point3 Vertices[4] = { {0.f, 0.f, 0.f},{ 0.f, 0.f, 0.f },{ 0.f, 0.f, 0.f },{ 0.f, 0.f, 0.f } };
    bool VerticesBuilt = false;

    class CreateCallBack : public CreateMouseCallBack
    {
    private:
        FireRenderPortalLight *ob = 0;
        IParamBlock2 *pblock = 0;
        Point3 p0, p1;

    public:
        int proc(ViewExp *vpt, int msg, int point, int flags, IPoint2 m, Matrix3& mat)
        {
            if (msg == MOUSE_POINT)
            {
                if (point == 0)
                {
                    p0 = p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
                    pblock->SetValue(PORTAL_PARAM_P0, 0, p0);
                    pblock->SetValue(PORTAL_PARAM_P1, 0, p0);
                    mat.SetTrans(p0);
                }
                else
                {
                    return 0;
                }
            }
            else if (msg == MOUSE_MOVE)
            {
                p1 = vpt->SnapPoint(m, m, NULL, SNAP_IN_PLANE);
                Point3 center = (p0 + p1) * 0.5f;
                mat.SetTrans(center);
                Point3 _p0, _p1;
                pblock->GetValue(PORTAL_PARAM_P0, 0, _p0, FOREVER);
                pblock->GetValue(PORTAL_PARAM_P1, 0, _p1, FOREVER);
                _p0 = center - p0;
                _p1 = center - p1;
                pblock->SetValue(PORTAL_PARAM_P0, 0, _p0);
                pblock->SetValue(PORTAL_PARAM_P1, 0, _p1);
            }
            else if (msg == MOUSE_ABORT)
            {
                return CREATE_ABORT;
            }
            else if (msg == MOUSE_FREEMOVE)
            {
                vpt->SnapPreview(m, m, NULL, SNAP_IN_PLANE);
            }
            return CREATE_CONTINUE;
        }

        void SetObj(FireRenderPortalLight* obj)
        {
            ob = obj;
            pblock = ob->GetParamBlock(0);
        }
    };

public:

    FireRenderPortalLight()
    {
        GetFireRenderPortalLightDesc()->MakeAutoParamBlocks(this);
    }

    ~FireRenderPortalLight() {
        DebugPrint(_T("FireRenderPortalLight::~FireRenderPortalLight\n"));
        DeleteAllRefs();
    }

    void GetCorners(Point3& p1, Point3& p2, Point3& p3, Point3& p4) override
    {
        BuildVertices();
        p1 = Vertices[0];
        p2 = Vertices[1];
        p3 = Vertices[2];
        p4 = Vertices[3];
    }
    
    CreateMouseCallBack *BaseObject::GetCreateMouseCallBack(void)
    {
        static CreateCallBack createCallback;
        createCallback.SetObj(this);
        return &createCallback;
    }

    GenLight *NewLight(int type)
    {
        return new FireRenderPortalLight();
    }

    // From Object
    ObjectState Eval(TimeValue time)
    {
        return ObjectState(this);
    }

    //makes part of the node name e.g. TheName001, if method returns L"TheName"
    void InitNodeName(TSTR& s) { s = FIRERENDER_PORTALLIGHT_OBJECT_NAME; }

    //displayed in Modifier list
    const MCHAR *GetObjectName() override { return FIRERENDER_PORTALLIGHT_OBJECT_NAME; }

    Interval ObjectValidity(TimeValue t) override
    {
        Interval valid;
        valid.SetInfinite();
        return valid;
    }

    BOOL UsesWireColor() { return 1; }      // 6/18/01 2:51pm --MQM-- now we can set object color
    int DoOwnSelectHilite() { return 1; }

    void NotifyChanged() { NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE); }

    // inherited virtual methods for Reference-management

    void BuildVertices(bool force = false)
    {
        if (VerticesBuilt && (!force))
            return;

        Point3 _p0, _p1, p0, p1;

        pblock2->GetValue(PORTAL_PARAM_P0, 0, p0, FOREVER);
        pblock2->GetValue(PORTAL_PARAM_P1, 0, p1, FOREVER);

        _p0.x = std::min(p0.x, p1.x);
        _p0.y = std::min(p0.y, p1.y);
        _p0.z = std::min(p0.z, p1.z);
        _p1.x = std::max(p0.x, p1.x);
        _p1.y = std::max(p0.y, p1.y);
        _p1.z = std::max(p0.z, p1.z);

        Vertices[0] = p0;
        Vertices[1] = Point3(p1.x, p0.y, p0.z);
        Vertices[2] = p1;
        Vertices[3] = Point3(p0.x, p1.y, p0.z);

        VerticesBuilt = true;
    }

    RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override
    {
        if ((REFMSG_CHANGE==msg)  && (hTarget == pblock2))
        {
            auto p = pblock2->LastNotifyParamID();
            if (p == PORTAL_PARAM_P0 || p == PORTAL_PARAM_P1)
                BuildVertices(true);
            //some params have changed - should redraw all
            if(GetCOREInterface()){
                GetCOREInterface()->RedrawViews(GetCOREInterface()->GetTime());
            }
        }
        return REF_SUCCEED;
    }

    IParamBlock2* pblock2 = NULL;

    void DeleteThis() override
    { 
        delete this; 
    }

    Class_ID ClassID() override { 
        return FIRERENDER_PORTALLIGHT_CLASS_ID;
    }

    //
    void GetClassName(TSTR& s) override { 
        FIRERENDER_PORTALLIGHT_CLASS_NAME;
    }

    RefTargetHandle Clone(RemapDir& remap) override
    {
        FireRenderPortalLight* newob = new FireRenderPortalLight();
        newob->VerticesBuilt = VerticesBuilt;
        newob->Vertices[0] = Vertices[0];
        newob->Vertices[1] = Vertices[1];
        newob->Vertices[2] = Vertices[2];
        newob->Vertices[3] = Vertices[3];
        newob->ReplaceReference(0, remap.CloneRef(pblock2));
        BaseClone(this, newob, remap);
        return(newob);
    }

    IParamBlock2* GetParamBlock(int i) override { 
        FASSERT(i == 0);
        return pblock2; 
    }

    IParamBlock2* GetParamBlockByID(BlockID id) override { 
        FASSERT(pblock2->ID() == id);
        return pblock2; 
    }

    int NumRefs() override {
        return 1;
    }

    void SetReference(int i, RefTargetHandle rtarg) override {
        FASSERT(0 == i);
        pblock2 = (IParamBlock2*)rtarg;
    }

    RefTargetHandle GetReference(int i) override {
        FASSERT(0 == i);
        return (RefTargetHandle)pblock2;
    }

    void DrawGeometry(ViewExp *vpt, IParamBlock2 *pblock, BOOL sel = FALSE, BOOL frozen = FALSE)
    {
        BuildVertices();

        if (sel)
        {
            vpt->getGW()->setColor(LINE_COLOR, Color(1.0f, 1.0f, 1.0f));
        }
        else
        {
            if (!frozen)
                vpt->getGW()->setColor(LINE_COLOR, Color(1.0f, 1.0f, 0.0f));
            else
                if (!frozen)
                    vpt->getGW()->setColor(LINE_COLOR, Color(.5f, .5f, .5f));
        }
        
        Point3 v[3];
        GraphicsWindow *gw = vpt->getGW();
        v[0] = Vertices[0];
        v[1] = Vertices[1];
        gw->polyline(2, v, NULL, NULL, FALSE, NULL);
        v[0] = Vertices[1];
        v[1] = Vertices[2];
        gw->polyline(2, v, NULL, NULL, FALSE, NULL);
        v[0] = Vertices[2];
        v[1] = Vertices[3];
        gw->polyline(2, v, NULL, NULL, FALSE, NULL);
        v[0] = Vertices[3];
        v[1] = Vertices[0];
        gw->polyline(2, v, NULL, NULL, FALSE, NULL);

        vpt->getGW()->marker(const_cast<Point3 *>(&Point3::Origin), X_MRKR);
    }

    Matrix3 GetTransformMatrix(TimeValue t, INode* inode, ViewExp* vpt)
    {
        Matrix3 tm = inode->GetObjectTM(t);
        return tm;
    }

    Color GetViewportMainColor(INode* pNode)
    {
        if (pNode->Dependent())
        {
            return Color(ColorMan()->GetColor(kViewportShowDependencies));
        }
        else if (pNode->Selected()) 
        {
            return Color(GetSelColor());
        }
        else if (pNode->IsFrozen())
        {
            return Color(GetFreezeColor());
        }
        return Color(1,1,0);
    }

    Color GetViewportColor(INode* pNode, Color selectedColor)
    {
        if (pNode->Dependent())
        {
            return Color(ColorMan()->GetColor(kViewportShowDependencies));
        }
        else if (pNode->Selected()) 
        {
            return selectedColor;
        }
        else if (pNode->IsFrozen())
        {
            return Color(GetFreezeColor());
        }
        return selectedColor * 0.5f;
    }

    int Display(TimeValue t, INode* inode, ViewExp *vpt, int flags) override
    {
        if (!vpt || !vpt->IsAlive())
        {
            // why are we here
            DbgAssert(!_T("Invalid viewport!"));
            return FALSE;
        }

        Matrix3 prevtm = vpt->getGW()->getTransform();
        Matrix3 tm = inode->GetObjectTM(t);
        vpt->getGW()->setTransform(tm);
        DrawGeometry(vpt, GetParamBlock(0), inode->Selected(), inode->IsFrozen());
        vpt->getGW()->setTransform(prevtm);

        return(0);
    }
    
    void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box)
    {
        if (!vpt || !vpt->IsAlive())
        {
            box.Init();
            return;
        }

        Matrix3 worldTM = inode->GetNodeTM(t);
        Point3 _Vertices[4];
        for (int i = 0; i < 4; i++)
            _Vertices[i] = Vertices[i] * worldTM;

        box.Init();
        for (int i = 0; i < 4; i++)
        {
            box.pmin.x = std::min(box.pmin.x, _Vertices[i].x);
            box.pmin.y = std::min(box.pmin.y, _Vertices[i].y);
            box.pmin.z = std::min(box.pmin.z, _Vertices[i].z);
            box.pmax.x = std::max(box.pmin.x, _Vertices[i].x);
            box.pmax.y = std::max(box.pmin.y, _Vertices[i].y);
            box.pmax.z = std::max(box.pmin.z, _Vertices[i].z);
        }
    }

    int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags, IPoint2 *p, ViewExp *vpt) override
    {
        if (!vpt || !vpt->IsAlive())
        {
            // why are we here
            DbgAssert(!_T("Invalid viewport!"));
            return FALSE;
        }

        HitRegion hitRegion;
        DWORD savedLimits;

        GraphicsWindow *gw = vpt->getGW();
        Matrix3 prevtm = gw->getTransform();

        gw->setTransform(idTM);
        MakeHitRegion(hitRegion, type, crossing, 8, p);
        savedLimits = gw->getRndLimits();

        gw->setRndLimits((savedLimits | GW_PICK) & ~GW_ILLUM & ~GW_Z_BUFFER);
        gw->setHitRegion(&hitRegion);
        gw->clearHitCode();

        Matrix3 tm = inode->GetObjectTM(t);
        gw->setTransform(tm);

        DrawGeometry(vpt, GetParamBlock(0));

        int res = gw->checkHitCode();

        gw->setRndLimits(savedLimits);
        gw->setTransform(prevtm);

        return res;
    };

    void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override
    {
        Matrix3 nodeTM = inode->GetNodeTM(t);
        Matrix3 parentTM = inode->GetParentNode()->GetNodeTM(t);
        Matrix3 localTM = nodeTM*Inverse(parentTM);

        Point3 _Vertices[4];
        for (int i = 0; i < 4; i++)
            _Vertices[i] = Vertices[i] * localTM;

        box.Init();
        for (int i = 0; i < 4; i++)
        {
            box.pmin.x = std::min(box.pmin.x, _Vertices[i].x);
            box.pmin.y = std::min(box.pmin.y, _Vertices[i].y);
            box.pmin.z = std::min(box.pmin.z, _Vertices[i].z);
            box.pmax.x = std::max(box.pmin.x, _Vertices[i].x);
            box.pmax.y = std::max(box.pmin.y, _Vertices[i].y);
            box.pmax.z = std::max(box.pmin.z, _Vertices[i].z);
        }
    }

    // LightObject methods

    Point3 GetRGBColor(TimeValue t, Interval &valid) override { return Point3(1.0f, 1.0f, 1.0f); }
    void SetRGBColor(TimeValue t, Point3 &color) override { /* empty */ }
    void SetUseLight(int onOff) override { /* empty */ }
    BOOL GetUseLight() override { return TRUE; }
    void SetHotspot(TimeValue time, float f) override { /* empty */ }
    float GetHotspot(TimeValue t, Interval& valid) override { return -1.0f; }
    void SetFallsize(TimeValue time, float f) override { /* empty */ }
    float GetFallsize(TimeValue t, Interval& valid) override { return -1.0f; }
    void SetAtten(TimeValue time, int which, float f) override { /* empty */ }
    float GetAtten(TimeValue t, int which, Interval& valid) override { return 0.0f; }
    void SetTDist(TimeValue time, float f) override { /* empty */ }
    float GetTDist(TimeValue t, Interval& valid) override { return 0.0f; }
    void SetConeDisplay(int s, int notify) override { /* empty */ }
    BOOL GetConeDisplay() override { return FALSE; }
    void SetIntensity(TimeValue time, float f) override { /* empty */ }
    float GetIntensity(TimeValue t, Interval& valid) override { return 1.0f; }
    void SetUseAtten(int s) override { /* empty */ }
    BOOL GetUseAtten() override { return FALSE; }
    void SetAspect(TimeValue t, float f) override { /* empty */ }
    float GetAspect(TimeValue t, Interval& valid) override { return -1.0f; }
    void SetOvershoot(int a) override { /* empty */ }
    int GetOvershoot() override { return 0; }
    void SetShadow(int a) override { /* empty */ }
    int GetShadow() override { return 0; }

    // From Light
    RefResult EvalLightState(TimeValue t, Interval& valid, LightState* cs)
    {
        cs->color = GetRGBColor(t, valid);
        cs->on = GetUseLight();
        cs->intens = GetIntensity(t, valid);
        cs->hotsize = GetHotspot(t, valid);
        cs->fallsize = GetFallsize(t, valid);
        cs->useNearAtten = FALSE;
        cs->useAtten = GetUseAtten();
        cs->nearAttenStart = GetAtten(t, ATTEN1_START, valid);
        cs->nearAttenEnd = GetAtten(t, ATTEN1_END, valid);
        cs->attenStart = GetAtten(t, ATTEN_START, valid);
        cs->attenEnd = GetAtten(t, ATTEN_END, valid);
        cs->aspect = GetAspect(t, valid);
        cs->overshoot = GetOvershoot();
        cs->shadow = GetShadow();
        cs->shape = CIRCLE_LIGHT;
        cs->ambientOnly = GetAmbientOnly();
        cs->affectDiffuse = GetAffectDiffuse();
        cs->affectSpecular = GetAffectSpecular();
        cs->type = OMNI_LGT;

        return REF_SUCCEED;
    }

    ObjLightDesc* CreateLightDesc(INode *inode, BOOL forceShadowBuf) override { return NULL; }

    int Type() override { return OMNI_LIGHT; }

    virtual void SetHSVColor(TimeValue, Point3 &)
    {
    }
    virtual Point3 GetHSVColor(TimeValue t, Interval &valid)
    {
        int h, s, v;
        Point3 rgbf = GetRGBColor(t, valid);
        DWORD rgb = RGB((int)(rgbf[0] * 255.0f), (int)(rgbf[1] * 255.0f), (int)(rgbf[2] * 255.0f));
        RGBtoHSV(rgb, &h, &s, &v);
        return Point3(h / 255.0f, s / 255.0f, v / 255.0f);
    }


    void SetAttenDisplay(int) override { /* empty */ }
    BOOL GetAttenDisplay() override { return FALSE; }
    void Enable(int) override {}
    void SetMapBias(TimeValue, float) override { /* empty */ }
    float GetMapBias(TimeValue, Interval &) override { return 0; }
    void SetMapRange(TimeValue, float) override { /* empty */ }
    float GetMapRange(TimeValue, Interval &) override { return 0; }
    void SetMapSize(TimeValue, int) override { /* empty */ }
    int  GetMapSize(TimeValue, Interval &) override { return 0; }
    void SetRayBias(TimeValue, float) override { /* empty */ }
    float GetRayBias(TimeValue, Interval &) override { return 0; }
    int GetUseGlobal() override { return FALSE; }
    void SetUseGlobal(int) override { /* empty */ }
    int GetShadowType() override { return -1; }
    void SetShadowType(int) override { /* empty */ }
    int GetAbsMapBias() override { return FALSE; }
    void SetAbsMapBias(int) override { /* empty */ }
    BOOL IsSpot() override { return FALSE; }
    BOOL IsDir() override { return FALSE; }
    void SetSpotShape(int) override { /* empty */ }
    int GetSpotShape() override { return CIRCLE_LIGHT; }
    void SetContrast(TimeValue, float) override { /* empty */ }
    float GetContrast(TimeValue, Interval &) override { return 0; }
    void SetUseAttenNear(int) override { /* empty */ }
    BOOL GetUseAttenNear() override { return FALSE; }
    void SetAttenNearDisplay(int) override { /* empty */ }
    BOOL GetAttenNearDisplay() override { return FALSE; }
    ExclList &GetExclusionList() override { return m_exclList; }
    void SetExclusionList(ExclList &) override { /* empty */ }

    BOOL SetHotSpotControl(Control *) override { return FALSE; }
    BOOL SetFalloffControl(Control *) override { return FALSE; }
    BOOL SetColorControl(Control *) override { return FALSE; }
    Control *GetHotSpotControl() override { return NULL; }
    Control *GetFalloffControl() override { return NULL; }
    Control *GetColorControl() override { return NULL; }

    void SetAffectDiffuse(BOOL onOff) override { /* empty */ }
    BOOL GetAffectDiffuse() override { return TRUE; }
    void SetAffectSpecular(BOOL onOff) override { /* empty */ }
    BOOL GetAffectSpecular() override { return TRUE; }
    void SetAmbientOnly(BOOL onOff) override { /* empty */ }
    BOOL GetAmbientOnly() override { return FALSE; }

    void SetDecayType(BOOL onOff) override {}
    BOOL GetDecayType() override { return 1; }
    void SetDecayRadius(TimeValue time, float f) override {}
    float GetDecayRadius(TimeValue t, Interval& valid) override { return 40.0f; } //! check different values
protected:
    ExclList m_exclList;
};

void*   FireRenderPortalLightClassDesc::Create(BOOL loading) {
    return new FireRenderPortalLight();
}

FIRERENDER_NAMESPACE_END


#include "FireRenderLight.h"

FIRERENDER_NAMESPACE_BEGIN

void FireRenderLight::SetAttenDisplay(int) { /* empty */ }
BOOL FireRenderLight::GetAttenDisplay() { return FALSE; }
void FireRenderLight::Enable(int) {}
void FireRenderLight::SetMapBias(TimeValue, float) { /* empty */ }
float FireRenderLight::GetMapBias(TimeValue, Interval &) { return 0; }
void FireRenderLight::SetMapRange(TimeValue, float) { /* empty */ }
float FireRenderLight::GetMapRange(TimeValue, Interval &) { return 0; }
void FireRenderLight::SetMapSize(TimeValue, int) { /* empty */ }
int  FireRenderLight::GetMapSize(TimeValue, Interval &) { return 0; }
void FireRenderLight::SetRayBias(TimeValue, float) { /* empty */ }
float FireRenderLight::GetRayBias(TimeValue, Interval &) { return 0; }
int FireRenderLight::GetUseGlobal() { return FALSE; }
void FireRenderLight::SetUseGlobal(int) { /* empty */ }
int FireRenderLight::GetShadowType() { return -1; }
void FireRenderLight::SetShadowType(int) { /* empty */ }
int FireRenderLight::GetAbsMapBias() { return FALSE; }
void FireRenderLight::SetAbsMapBias(int) { /* empty */ }

Point3 FireRenderLight::GetRGBColor(TimeValue t, Interval& valid) { return Color(1.f, 1.f, 1.f); }
void FireRenderLight::SetRGBColor(TimeValue t, Point3 &color) {}
void FireRenderLight::SetUseLight(int onOff) { /* empty */ }
BOOL FireRenderLight::GetUseLight() { return TRUE; }
void FireRenderLight::SetHotspot(TimeValue time, float f) { /* empty */ }
float FireRenderLight::GetHotspot(TimeValue t, Interval& valid) { return -1.0f; }
void FireRenderLight::SetFallsize(TimeValue time, float f) { /* empty */ }
float FireRenderLight::GetFallsize(TimeValue t, Interval& valid) { return -1.0f; }
void FireRenderLight::SetAtten(TimeValue time, int which, float f) { /* empty */ }
float FireRenderLight::GetAtten(TimeValue t, int which, Interval& valid) { return 0.0f; }
void FireRenderLight::SetConeDisplay(int s, int notify) { /* empty */ }
BOOL FireRenderLight::GetConeDisplay() { return FALSE; }
void FireRenderLight::SetIntensity(TimeValue time, float f) {}
float FireRenderLight::GetIntensity(TimeValue t, Interval& valid) { return 1.f; }
void FireRenderLight::SetUseAtten(int s) { /* empty */ }
BOOL FireRenderLight::GetUseAtten() { return FALSE; }
void FireRenderLight::SetAspect(TimeValue t, float f) { /* empty */ }
float FireRenderLight::GetAspect(TimeValue t, Interval& valid) { return -1.0f; }
void FireRenderLight::SetOvershoot(int a) { /* empty */ }
int FireRenderLight::GetOvershoot() { return 0; }
void FireRenderLight::SetShadow(int a) { /* empty */ }
int FireRenderLight::GetShadow() { return 0; }
ObjLightDesc* FireRenderLight::CreateLightDesc(INode *inode, BOOL forceShadowBuf) { return nullptr; }

void FireRenderLight::UpdateTargDistance(TimeValue t, INode* inode) {}
void FireRenderLight::SetTDist(TimeValue time, float f) {}
float FireRenderLight::GetTDist(TimeValue t, Interval& valid) { return 1.f; }

FIRERENDER_NAMESPACE_END
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

#pragma once
#include <max.h>
#include "Common.h"
FIRERENDER_NAMESPACE_BEGIN;

/// Implementation of 3ds Max ShadeContext class that allows reasonable sampling of UVW-mapped texmaps and some VERY basic 
/// sampling of local/world-space texmaps - it samples them as if they were placed on top of a plane with size UvwContext::SIZE.
class UvwContext : public ::ShadeContext
{
    void operator=(const UvwContext&) = delete;
    UvwContext(const UvwContext&) = delete;
protected:

    /// currently sampled UVW coordinates. This member is changed every time a new sample is taken to cover the entire UVW space
    Point3 uvw;

    /// The target resolution used for sampling. This is used for texture filtering
    int resolution_x;
	int resolution_y;

    /// Scene time in which the sampling takes place
    TimeValue time;

    /// Size of the virtual plane centered in (0 0 0) which is used for sampling position-mapped textures
    static const float SIZE;

public:

    /// Sets reasonable defaults to some 3ds Max-inherited variables, but setup() has to be called before each sampling
    UvwContext()
	{
        doMaps = TRUE;
        backFace = FALSE;
        filterMaps = TRUE;
		shadow = FALSE;
        mtlNum = 0;
        rayLevel = 0;
    }

    /// Prepares the context to be sampled in give UVW point, at given time and resolution
    void setup(const Point3 &uvw, const TimeValue &t, const int &resolution_x, const int &resolution_y)
	{
        this->uvw = uvw;
        this->time = t;
        this->resolution_x = resolution_x;
		this->resolution_y = resolution_y;
    }

    // We are required to provide implementation of a LOT of methods. Most of them are unused by most plugins/cannot be supported
    // because this interface was created for scanline-like renderers.

    int Antialias() override
	{
        return TRUE;
    }

    int ProjType() override
	{
        return 0;
    }

    LightDesc* Light(int n) override
	{
        return NULL;
    }

    int FaceNumber() override
	{
        return 0;
    }

    float Curve() override
	{
        return 0.f;
    }

    void SetView(Point3 p) override
	{
	}

    Point3 DP() override
	{
        return Point3(0, 0, 0);
    }

    Point3 DPObj() override
	{
        return DP();
    }

    Box3 ObjectBox() override
	{
        return Box3(Point3(-SIZE, -SIZE, -SIZE), Point3(SIZE, SIZE, SIZE));
    }

    Point3 PObjRelBox() override
	{
        return P();
    }

    Point3 DPObjRelBox() override
	{
        return DP();
    }

    void GetBGColor(Color &bgcol, Color& transp, BOOL fogBG = TRUE) override
	{
	}

    Point3 PointTo(const Point3& p, RefFrame ito) override
	{
        return p;
    }

    Point3 PointFrom(const Point3& p, RefFrame ifrom) override 
	{
        return p;
    }

    Point3 VectorTo(const Point3& p, RefFrame ito) override 
	{
        return p;
    }

    Point3 VectorFrom(const Point3& p, RefFrame ifrom) override 
	{
        return p;
    }

    TimeValue CurTime() override 
	{
        return this->time;
    }

    int InMtlEditor() override 
	{
        return FALSE;
    }

    Point3 ReflectVector() override 
	{
        return -V();
    }

    Point3 RefractVector(float ior) override 
	{
        return V();
    }

    Point3 GNormal() override 
	{
        return Normal();
    }
    
	Point3 DUVW(int channel = 0) override 
	{
        const float dx = 1.f / this->resolution_x;
		const float dy = 1.f / this->resolution_y;
        // Compute UVW space derivatives from the resolution we are sampling with,
		// therefore dUVW is roughly the distance between samples
        return Point3(dx, dy, 0.f);
    }

    Point3 PObj() override
	{
        return P();
    }

    void DPdUVW(Point3 dP[3], int channel = 0) override 
	{
        dP[0] = dP[1] = dP[2] = Point3(0.f, 0.f, 0.f);
    }

    // Used by enviroment map sampling
    Point3 V() override 
	{
        const float inclination = PI * (1.f - uvw.y);
        const float azimuth = 2 * PI * (uvw.x) + PI * 0.5f;
        const float sinInclination = std::sin(inclination);
        Point3 res(std::cos(azimuth)*sinInclination, std::sin(azimuth)*sinInclination, std::cos(inclination));
#ifdef SWITCH_AXES
        std::swap(res.y, res.z);
#endif
        return res;
    }

    Point3 CamPos() override 
	{
        return Point3(0.f, 0.f, SIZE * 5);
    }

    // Transforms the UVW input onto imaginary zero-centered plane of size "SIZE"
    Point3 P() override 
	{
        return this->uvw * SIZE * 2.f - Point3(SIZE, SIZE, 0.f);
    }

    Point3 BarycentricCoords() override 
	{
        return UVW();
    }

    Point3 Normal() override 
	{
        return Point3(0.f, 0.f, -SIZE);
    }

    Point3 UVW(int channel = 0) override 
	{
        return this->uvw;
    }

    void ScreenUV(Point2& uv, Point2 &duv) override 
	{
        uv.x = uvw.x;
        uv.y = uvw.y;
        const Point3 deriv = DUVW();
        duv.x = deriv.x;
        duv.y = deriv.y;
    }

    IPoint2 ScreenCoord() override 
	{
        return IPoint2(int(uvw.x * resolution_x), int(uvw.y * resolution_y));
    }
};

__declspec(selectany) const float UvwContext::SIZE = 50;

FIRERENDER_NAMESPACE_END;

#pragma once

#include "Common.h"

FIRERENDER_NAMESPACE_BEGIN;

//Temperature must fall between 1000 and 40000 degrees
constexpr float MinKelvin = 1000.f;
constexpr float MaxKelvin = 40000.f;
constexpr float DefaultKelvin = 2700.f;

Color KelvinToColor(float kelvin);

FIRERENDER_NAMESPACE_END;
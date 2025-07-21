#pragma once

#include "SensorModel.h"
#include <optional>

std::optional<Point> trilaterate(const Point& s1, double d1,
                                const Point& s2, double d2,
                                const Point& s3, double d3);
#include "Trilateration.h"
#include <cmath>

std::optional<Point> trilaterate(const Point& s1, double d1,
                                const Point& s2, double d2,
                                const Point& s3, double d3)
{
    double A = 2* (s2.x - s1.x);
    double B = 2 * (s2.y - s1.y);
    double C = pow(d1, 2) - pow (d2, 2) + pow(s2.x, 2) - pow(s1.x, 2)
                + pow(s2.y, 2) - pow(s1.y, 2);
    double D = 2 * (s3.x - s1.x);
    double E = 2 * (s3.y - s1.y);
    double F = pow(d1, 2) - pow(d3, 2) + pow(s3.x, 2) - pow(s1.x, 2)
                + pow(s3.y, 2) - pow(s1.y, 2);

    double determinant = (A * E) - (B * D);

    if (std::abs(determinant) < 1e-9) {
        return std::nullopt;
    }

    Point result;
    result.x = (C * E - F * B) / determinant;
    result.y = (A * F - D * C) / determinant;

    return result;
}
// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "Triangle.h"


Triangle::Triangle(Vector3d a, Vector3d b, Vector3d c)
{
	this->a = a;
	this->b = b;
	this->c = c;
};

Vector3d Triangle::GetNormal()
{
	Vector3d ab = b - a;
	Vector3d ac = c - a;
	ab.Cross(ac).Normalize();
	return ab;
}

Vector3d Triangle::GetClosestPoint(Vector3d p)
{
	Vector3d ab = b - a;
	Vector3d ac = c - a;

	Vector3d ap = p - a;
	double d1 = ab * ap;
	double d2 = ac * ap;

	if( d1 <= 0.0 && d2 <= 0.0 )
	{
		return a;
	}

	Vector3d bp = p - b;
	double d3 = ab * bp;
	double d4 = ac * bp;
	if( d3 >= 0.0 && d4 <= d3 )
	{
		return b;
	}

	double vc = d1*d4 - d3*d2;
	if( vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0 )
	{
		double v = d1 / (d1 - d3);
		return a + ab * v;
	}

	Vector3d cp = p - c;
	double d5 = ab * cp;
	double d6 = ac * cp;
	if( d6 > 0.0 && d5 <= d6 )
	{
		return c;
	}

	double vb = d5*d2 - d1*d6;
	if( vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0 )
	{
		double w = d2 / (d2 - d6);
		return a + ac * w;
	}

	double va = d3*d6 - d5*d4;
	if( va <= 0.0 && (d4-d3) >= 0.0 && d5-d6 >= 0.0 )
	{
		double w = (d4-d3) / ((d4-d3)+(d5-d6));
		return b + (c-b) * w;
	}

	double denom = 1.0 / (va+vb+vc);
	double v = vb * denom;
	double w = vc * denom;

	return a + ab * v + ac * w;
}

bool Triangle::ContainsPoint(Vector3d p)
{
		Vector3d v0 = c - a;
		Vector3d v1 = b - a;
		Vector3d v2 = p - a;

		double dot00 = v0 * v0;
		double dot01 = v0 * v1;
		double dot02 = v0 * v2;
		double dot11 = v1 * v1;
		double dot12 = v1 * v2;

		// Compute barycentric coordinates
		double invDenom = 1.0 / (dot00 * dot11 - dot01 * dot01);
		double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		// Check if point is in triangle
		return (u >= 0) && (v >= 0) && (u + v < 1);
}
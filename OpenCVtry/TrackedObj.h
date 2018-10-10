#pragma once
#include<vector>

class TrackedObj {
public:
	int cx;
	int cy;
	double cz;
	int sides;
	double angle;
	double area;

	TrackedObj();

	TrackedObj(int cx, int cy);

	TrackedObj(int cx, int cy, double cz);

	~TrackedObj();
};
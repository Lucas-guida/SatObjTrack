#include "pch.h"
#include "TrackedObj.h"

TrackedObj::TrackedObj()
{
}

TrackedObj::TrackedObj(int cx, int cy)
{
	this->cx = cx;
	this->cy = cy;
}

TrackedObj::TrackedObj(int cx, int cy, double cz)
{
	this->cx = cx;
	this->cy = cy;
	this->cz = cz;
}

TrackedObj::~TrackedObj()
{
}

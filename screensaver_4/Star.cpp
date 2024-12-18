#include "Star.h"

Star::Star(double x, double y, double z, double vx, double vy, double vz, double M)
{
	this->G = 2500;
	this->x = x;
	this->y = y;
	this->z = z;
	this->vx = vx;
	this->vy = vy;
	this->vz = vz;
	this->M = M;
}

Star::~Star()
{
}

void Star::update(std::vector<Star> &stars, double dt)
{
	double ax = 0;
	double ay = 0;
	double az = 0;

	for (auto &star : stars)
	{
		if (&star == this)
			continue;

		if (star.M == 0.0)
			continue;

		double dx = star.x - x;
		double dy = star.y - y;
		double dz = star.z - z;

		double d = sqrt(dx * dx + dy * dy + dz * dz);
		double F = G * M * star.M / (d * d);

		ax += F * dx / d;
		ay += F * dy / d;
		az += F * dz / d;
	}

	vx += ax;
	vy += ay;
	vz += az;

	x += vx * dt;
	y += vy * dt;
	z += vz * dt;
}

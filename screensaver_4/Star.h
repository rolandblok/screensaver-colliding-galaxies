#pragma once

#include <vector>



class Star
{
	static double G;

	public:
	Star( double x, double y, double z, double vx, double vy, double vz, double M);
	~Star();
	void update(std::vector<Star> &stars, double dt);

	double x; // x position
	double y; // y position
	double z; // z position
	double vx; // x velocity
	double vy; // y velocity
	double vz; // z velocity
	double M; // mass of the star
};


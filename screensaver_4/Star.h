#pragma once

#include <vector>
#include <wtypes.h>

#include"vec3.h"

// constant define for the gravitational constant

constexpr double G = 25000;
constexpr double dt = 0.00001;
constexpr double inner_R = 1;  //pixels

constexpr double outer_R = 100; // pixels
constexpr double init_dist = 500; // pixels
constexpr double init_gv = 800; // pixels per second
constexpr double star_M = 15.0; // mass of the star

class Star
{
public:
	Star( vec3<double> p, vec3<double> v , double M);
	~Star();
	double update(std::vector<Star> &stars, double dt);
	void draw(DWORD* pixels, int width, int height);


	vec3<double> p; // position
	vec3<double> v; // velocity
	double M; // mass of the star
};

class Universe
{
public:
	Universe(int no_galaxies, int no_stars_per_galaxy, int w, int h);
	~Universe();
	void update();
	void draw(DWORD* pixels, int width, int height);

private:
	std::vector<Star> stars;
	std::vector<Star> createGalaxy(int no_stars_per_galaxy, vec3<double> gp, vec3<double> gv);
	int no_galaxies;
	int no_stars_per_galaxy;
	int w, h;
	int ups;
	LARGE_INTEGER last_ups, frequency;



};
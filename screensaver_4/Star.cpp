#include "Star.h"

#include <cmath>
#include <string>

#define TWO_PI 6.283185307179586476925286766559
#define PI 3.1415926535897932384626433832795

Star::Star(vec3<double> p, vec3<double> v, double M)
{
	this->p = p;
	this->v = v;
	this->M = M;
}

Star::~Star()
{
}

void Star::update(std::vector<Star> &stars, double dt_s)
{
	vec3<double> a = { 0, 0, 0 };

	for (auto &star : stars)
	{
		if (&star == this)
			continue;

		if (star.M == 0.0)
			continue;

		vec3<double> dp = star.p - p;
		double r2 = dp.length2();
		double r = sqrt(r2);
		double F = G * star.M / (r2*r);
		a += dp * F ;
	}

	v += a * dt_s;
	p += v * dt_s;
}


void Star::draw(DWORD * pixels, int width, int height)
{
	// Draw the star
	vec3<int> pi = p.toInt();
	if (pi.x < 0 || pi.x >= width || pi.y < 0 || pi.y >= height)
		return;
	pixels[pi.x + pi.y * width] = RGB(255, 255, 255);
}

Universe::Universe(int no_galaxies, int no_stars_per_galaxy, int w, int h)
{
	this->no_galaxies = no_galaxies;
	this->no_stars_per_galaxy = no_stars_per_galaxy;
	this->w = w;
	this->h = h;

	stars = std::vector<Star>();

	// seed randomnumber generator
	srand((unsigned int) GetTickCount64());


	for (int i = 0; i < no_galaxies; i++) {
		double angle_z = TWO_PI * (rand() / (RAND_MAX + 1.0));
		vec3<double> gp = { 0, init_dist * (rand()/(RAND_MAX + 1.0)), 0};
		gp = gp.rotateZ(angle_z);
		gp += vec3<double> (w / 2, h / 2, 0);
		vec3<double> gv = { - init_gv + 2.0 * init_gv * (rand() / (RAND_MAX + 1.0)),
							- init_gv + 2.0 * init_gv * (rand() / (RAND_MAX + 1.0)), 0.0};


		createGalaxy(no_stars_per_galaxy, gp, gv);
	}

	ups = 0;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&last_ups);


}

Universe::~Universe()
{
}

void Universe::update()
{
	LARGE_INTEGER start;
	QueryPerformanceCounter(&start);


	for (auto &star : stars)
		star.update(stars, dt);

	ups++;
	LARGE_INTEGER current_time;
	QueryPerformanceCounter(&current_time);
	
	double elapsed_time_ms = 1000.0 * (current_time.QuadPart - last_ups.QuadPart) / frequency.QuadPart;
	if (elapsed_time_ms > 1000)
	{
		QueryPerformanceCounter(&last_ups);
		OutputDebugStringA(("ups: " + std::to_string(ups) + "\n").c_str());
		ups = 0;
	}
}

void Universe::createGalaxy(int no_stars_per_galaxy, vec3<double> gp, vec3<double> gv)
{
	// add the black hole
	double black_hole_M = star_M * no_stars_per_galaxy;
	Star black_hole(gp, gv, black_hole_M);
	stars.push_back(black_hole);

	double angle_x = PI * (rand() / (RAND_MAX + 1.0));
	double angle_y = PI * (rand() / (RAND_MAX + 1.0));

	for (int i = 0; i < no_stars_per_galaxy; i++)
	{
		double r = inner_R + outer_R * (rand() / (RAND_MAX + 1.0));
		double V = std::sqrt(G * black_hole_M / r);
		double theta = TWO_PI * (rand() / (RAND_MAX + 1.0));
		vec3<double> p = { r * cos(theta), -r * sin(theta), 0 };
		vec3<double> v = { V * sin(theta), V * cos(theta), 0 };

		p = p.rotateY(angle_y);
		v = v.rotateY(angle_y);

		p = p.rotateX(angle_x);
		v = v.rotateX(angle_x);

		p += gp;
		v += gv;

		stars.push_back(Star(p, v, 0.0));
	}
}

void Universe::draw(DWORD * pixels, int width, int height)
{
	for (auto &star : stars)
	{
		star.draw(pixels, width, height);
	}
}

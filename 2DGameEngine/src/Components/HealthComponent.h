#pragma once

/*
 * HealthComponent
 *
 * Component that holds information about health.
 */
struct HealthComponent
{
public:
	explicit HealthComponent(double health = 100.0);

public:
	double Health;
};


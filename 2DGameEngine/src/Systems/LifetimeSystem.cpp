#include "LifetimeSystem.h"
#include "../Components/LifetimeComponent.h"

LifetimeSystem::LifetimeSystem(Registry* registry)
	:RegistryPtr(registry)
{
	RequireComponent<LifetimeComponent>();
}

void LifetimeSystem::Update(const double deltaTime) const
{
	const std::vector<Entity> entities = GetSystemEntities();

	std::vector<Entity> toBeKilledEntities;

	// First, lets find all entities that we want to kill.
	for (Entity systemEntity : entities)
	{
		auto& lifetimeComponent = systemEntity.GetComponent<LifetimeComponent>();

		lifetimeComponent.CurrentTimerMiliSeconds += static_cast<int>(deltaTime * 1000.0);

		if (lifetimeComponent.CurrentTimerMiliSeconds >= lifetimeComponent.LifetimeMiliSeconds)
		{
			toBeKilledEntities.push_back(systemEntity);
		}
	}

	// Kill entities in toBeKilledEntities vector.
	for (const Entity toBeKilledEntity : toBeKilledEntities)
	{
		RegistryPtr->KillEntity(toBeKilledEntity);
	}
}

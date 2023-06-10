#include "CollisionSystem.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/TransformComponent.h"

CollisionSystem::CollisionSystem()
{
	RequireComponent<BoxColliderComponent>();
	RequireComponent<TransformComponent>();
}

void CollisionSystem::Update()
{
	// First, lets save our system entities so we don't have to call it again.
	std::vector<Entity> entities = GetSystemEntities();
	// Iterate through entities vector with for which i is returning us a reference to the entity.
	for (auto i = entities.begin(); i != entities.end(); ++i)
	{
		// Start with i + 1 because we don't want to check same entity against itself.
		for (auto j = i + 1; j != entities.end(); ++j)
		{
			CheckAABBCollision(*i, *j);
		}
	}
}

void CollisionSystem::CheckAABBCollision(Entity sourceEntity, Entity targetEntity) const
{
	const auto sourceTransformComponent = sourceEntity.GetComponent<TransformComponent>();
	const auto sourceBoxColliderComponent = sourceEntity.GetComponent<BoxColliderComponent>();

	const glm::vec2 sourceOffsetLocation = { sourceTransformComponent.Position + sourceBoxColliderComponent.Offset };
	const glm::vec2 sourceBoxColliderSize = {
		static_cast<float>(sourceBoxColliderComponent.Width) * sourceTransformComponent.Scale.x,
		static_cast<float>(sourceBoxColliderComponent.Height) * sourceTransformComponent.Scale.y };

	const auto targetTransformComponent = targetEntity.GetComponent<TransformComponent>();
	const auto targetBoxColliderComponent = targetEntity.GetComponent<BoxColliderComponent>();

	const glm::vec2 targetOffsetLocation = { targetTransformComponent.Position + targetBoxColliderComponent.Offset };
	const glm::vec2 targetBoxColliderSize = {
		static_cast<float>(targetBoxColliderComponent.Width) * targetTransformComponent.Scale.x,
		static_cast<float>(targetBoxColliderComponent.Height) * targetTransformComponent.Scale.y };

	const bool collision =
		sourceOffsetLocation.x < targetOffsetLocation.x + targetBoxColliderSize.x
		&& sourceOffsetLocation.x + sourceBoxColliderSize.x > targetOffsetLocation.x
		&& sourceOffsetLocation.y < targetOffsetLocation.y + targetBoxColliderSize.y
		&& sourceOffsetLocation.y + sourceBoxColliderSize.y > targetOffsetLocation.y;

	if (collision)
	{
		Logger::Log("Collision between entity : " + std::to_string(sourceEntity.GetId()) + " and entity with id :" + std::to_string(targetEntity.GetId()));
	}
}

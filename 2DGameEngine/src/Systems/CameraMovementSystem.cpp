#include "CameraMovementSystem.h"
#include <algorithm>
#include <SDL_rect.h>
#include "../Components/CameraFollowComponent.h"
#include "../Components/TransformComponent.h"
#include "../Game/Game.h"

CameraMovementSystem::CameraMovementSystem()
{
	RequireComponent<TransformComponent>();
	RequireComponent<CameraFollowComponent>();
}

void CameraMovementSystem::Update(SDL_Rect& camera) const
{
	for (Entity systemEntity : GetSystemEntities())
	{
		const auto transformComponent = systemEntity.GetComponent<TransformComponent>();

		const int wantedCamXPos = transformComponent.Location.x - Game::WindowWidth / 2;
		const int wantedCamYPos = transformComponent.Location.y - Game::WindowHeight / 2;

		camera.x = std::clamp(wantedCamXPos, 0, Game::MapWidth - Game::WindowWidth);
		camera.y = std::clamp(wantedCamYPos, 0, Game::MapHeight - Game::WindowHeight);
	}
}

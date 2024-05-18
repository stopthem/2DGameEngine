#pragma once

#include "../ECS/ECS.h"

class EventBus;
class KeyUpEvent;
/*
 * KeyboardControlSystem
 *
 * Currently handles only key pressed events.
 */
class KeyboardControlSystem : public System
{
public:
	explicit KeyboardControlSystem(EventBus* eventBus);

private:
	EventBus* EventBusPtr;

private:
	void OnKeyboardKeyPressed(KeyUpEvent& keyPressedEvent);

public:
	// Unsubscribe from key pressed event.
	void OnSystemRemoved() override;

};


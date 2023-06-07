#pragma once
#include <vector>
#include <bitset>
#include <unordered_map>
#include <typeindex>
#include <set>

constexpr unsigned int MAX_COMPONENTS = 32;

// We use a bitset (1-0)s to keep track of which components an entity has,
// and also helps keep track of which entities a system is interested in.
typedef std::bitset<MAX_COMPONENTS> Signature;

struct IComponent
{
protected:
	static int NextId;
};

template<typename T>
class Component : public IComponent
{
	// Returns the unique id of Component<TComponent>.
	static int GetId() {
		static int id = NextId++;
		return id;
	}
};

class Entity {

public:
	explicit Entity(int id);

	Entity(const Entity& otherEntity) = default;

public:
	[[nodiscard]] int GetId() const { return Id; }
private:
	int Id;

public:
	Entity& operator=(const Entity& other) = default;

	bool operator==(const Entity& other) const {
		return GetId() == other.GetId();
	}
	bool operator!=(const Entity& other) const {
		return GetId() != other.GetId();
	}
	bool operator<(const Entity& other) const
	{
		return GetId() < other.GetId();
	}
};

/*
* System
*
* The system process entities that contain specific signature.
*/
class System {

public:
	System() = default;
	~System() = default;

public:
	// Define the component type TComponent that entities must have to be 
	// considered by the system.
	template<typename TComponent>
	void RequireComponent();
public:
	void AddEntityToSystem(Entity entity);

	void RemoveEntityFromSystem(Entity entity);

	[[nodiscard]] std::vector<Entity> GetSystemEntities() const { return Entities; }

private:
	std::vector<Entity> Entities;

public:
	[[nodiscard]] Signature GetSignature() const { return ComponentSignature; }
private:
	Signature ComponentSignature;

};

template<typename TComponent>
void System::RequireComponent() {

	const int componentId = Component<TComponent>().GetId();

	ComponentSignature.set(componentId);
}

class IPool {
public:
	virtual ~IPool() = default;
};

/*
* Pool
*
* A pool is just a vector(contiguous data) of objects of type TComponent.
*/
template<typename TPool>
class Pool final :public IPool {
public:
	explicit Pool(int size = 100) { Data.reserve(size); }
	~Pool() override = default;

private:
	std::vector<TPool> Data;

public:
	bool IsEmpty() { return Data.empty(); }

	int GetSize() { return Data.size(); }

	void Resize(int size) { Data.resize(size); }

	void Clear() { Data.clear(); }

	void Add(TPool object) { Data.push_back(object); }

	void Set(int index, TPool object) { Data[index] = object; }

	TPool& Get(int index) { return static_cast<TPool&>(Data[index]); }

	TPool& operator[](unsigned int index) { return Data[index]; }
};

/*
* Registry
*
* The registry manages the creation and destruction of entities, as well as
* adding system and adding components to entities.
*/
class Registry {
private:
	// Keep track of how many entities were added to the scene.
	int NumEntities = 0;

public:
	Registry() = default;

private:
	std::set<Entity> EntitiesToBeAdded;
	std::set<Entity> EntitiesToBeKilled;

public:
	Entity CreateEntity();

#pragma region Component

private:
	// Vector of component pools, each pool contains all the data for a certain component t.
	// Vector index = component id.
	// Pool index = entity id.
	std::vector<IPool*> ComponentPools;

	// Vector of component signatures per entity, saying which component is turned "on" for the entity.
	// Vector index = entity id.
	std::vector<Signature> EntityComponentSignatures;

public:
	template<typename TComponent, typename ...TComponentArgs>   
	void AddComponent(Entity entity, TComponentArgs ...componentArgs);

	template<typename TComponent>
	void RemoveComponent(Entity entity);

	template<typename T>
	[[nodiscard]] bool HasComponent(Entity entity) const;
#pragma endregion

#pragma region System

private:
	// Map index = system id.
	std::unordered_map<std::type_index, System> Systems;

public:
	template<typename TSystem, typename ...TSystemArgs>
	void AddSystem(TSystemArgs&& ...systemArgs);

	template<typename TSystem>
	void RemoveSystem();

	template<typename TSystem>
	[[nodiscard]] bool HasSystem() const;

	template<typename TSystem>
	[[nodiscard]] TSystem& GetSystem() const;

	// Checks the component signature of an entity and add the entity to the systems
	// that are interested in it.
	void AddEntityToSystems(Entity entity);
#pragma endregion
public:
	void Update();
};

#pragma region Registry Component Template Functions

template<typename TComponent, typename ...TComponentArgs>
inline void Registry::AddComponent(const Entity entity, TComponentArgs ...componentArgs)
{
	const int componentId = Component<TComponent>::GetId();
	const int entityId = entity.GetId();

	// If component id is greater than componentPools.size, resize it and null the new object.
	if (componentId >= ComponentPools.size())
	{
		ComponentPools.resize(componentId + 1, nullptr);
	}

	// If we still don't have a pool for that component type create one.
	if (!ComponentPools[componentId])
	{
		Pool<TComponent>* newComponentPool = new Pool<TComponent>();
		ComponentPools[componentId] = newComponentPool;
	}

	// Fetch the corresponding type of pool from component pools.
	Pool<TComponent>* componentPool = Pool<TComponent>(ComponentPools[componentId]);

	// If entity id is greater than current componentPool size, resize it.
	if (entityId >= componentPool->GetSize())
	{
		componentPool->Resize(NumEntities);
	}

	// Create the new component with the given multiple componentArgs.
	TComponent newComponent(std::forward<TComponentArgs>(componentArgs)...);

	// Add the new component ot the component pool list, using the entity id as index.
	componentPool->Set(entityId, newComponent);

	// Change the signature of the entity to say that, entity has that component.
	EntityComponentSignatures[entityId].set(componentId);
}

template<typename TComponent>
inline void Registry::RemoveComponent(const Entity entity)
{
	const int componentId = Component<TComponent>::GetId();
	const int entityId = entity.GetId();

	EntityComponentSignatures[entityId].set(componentId);
}

template<typename TComponent>
inline bool Registry::HasComponent(const Entity entity) const
{
	const int componentId = Component<TComponent>::GetId();
	const int entityId = entity.GetId();

	return EntityComponentSignatures[entity.GetId()].test(componentId);
}
#pragma endregion

#pragma region Registry System Template Functions

template <typename TSystem, typename ...TSystemArgs>
void Registry::AddSystem(TSystemArgs&&... systemArgs)
{
	const TSystem* newSystem(new TSystem(std::forward<TSystemArgs>(systemArgs)));

	Systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));
}

template <typename TSystem>
void Registry::RemoveSystem()
{
	Systems.erase(std::type_index(typeid(TSystem)));
}

template <typename TSystem>
bool Registry::HasSystem() const
{
	return Systems.find(std::type_index(typeid(TSystem))) != Systems.end();
}

template <typename TSystem>
TSystem& Registry::GetSystem() const
{
	auto foundSystem = Systems.find(std::type_index(typeid(TSystem))) != Systems.end();

	return Systems.at(std::type_index(typeid(TSystem)));
}
#pragma endregion
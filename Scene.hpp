#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Math.hpp"

namespace orbit {

class Renderer;
using EntityId = std::uint32_t;

struct Transform {
    Vec2 position{};
    Vec2 scale{1.0F, 1.0F};
};

struct Sprite {
    Vec2 size{16.0F, 16.0F};
    Color color = colors::white;
    int layer = 0;
    bool visible = true;
};

struct Collider {
    Vec2 halfExtents{8.0F, 8.0F};
    bool isTrigger = false;
};

struct RigidBody {
    Vec2 velocity{};
    float bounciness = 0.0F;
    bool dynamic = true;
};

struct Entity {
    EntityId id = 0;
    std::string name;
    Transform transform{};
    std::optional<Sprite> sprite;
    std::optional<Collider> collider;
    std::optional<RigidBody> body;
    bool enabled = true;
};

struct Camera2D {
    Vec2 center{};
    float zoom = 1.0F;
};

struct CollisionEvent {
    EntityId first = 0;
    EntityId second = 0;
    Vec2 normal{}; // Points from first toward second.
    bool trigger = false;
};

class Scene {
public:
    Entity& createEntity(std::string name);
    Entity& entity(EntityId id);
    const Entity& entity(EntityId id) const;

    void step(float deltaSeconds);
    void render(Renderer& renderer, const Camera2D& camera) const;

    [[nodiscard]] const std::vector<CollisionEvent>& collisions() const { return collisions_; }
    Vec2 gravity{};

private:
    static bool collides(const Entity& first, const Entity& second, Vec2& normal, float& overlap);
    void resolveCollision(Entity& first, Entity& second, const Vec2& normal, float overlap);

    EntityId nextId_ = 1;
    std::vector<Entity> entities_;
    std::vector<CollisionEvent> collisions_;
};

} // namespace orbit

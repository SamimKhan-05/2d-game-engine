#include "Scene.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "Renderer.hpp"

namespace orbit {

Entity& Scene::createEntity(std::string name) {
    Entity item;
    item.id = nextId_++;
    item.name = std::move(name);
    entities_.push_back(std::move(item));
    return entities_.back();
}

Entity& Scene::entity(EntityId id) {
    const auto found = std::find_if(entities_.begin(), entities_.end(), [id](const Entity& item) {
        return item.id == id;
    });
    if (found == entities_.end()) {
        throw std::out_of_range("No entity with the requested ID exists.");
    }
    return *found;
}

const Entity& Scene::entity(EntityId id) const {
    const auto found = std::find_if(entities_.begin(), entities_.end(), [id](const Entity& item) {
        return item.id == id;
    });
    if (found == entities_.end()) {
        throw std::out_of_range("No entity with the requested ID exists.");
    }
    return *found;
}

void Scene::step(float deltaSeconds) {
    collisions_.clear();
    const float timeStep = clamp(deltaSeconds, 0.0F, 1.0F / 20.0F);

    for (Entity& item : entities_) {
        if (!item.enabled || !item.body.has_value() || !item.body->dynamic) {
            continue;
        }
        item.body->velocity += gravity * timeStep;
        item.transform.position += item.body->velocity * timeStep;
    }

    for (std::size_t firstIndex = 0; firstIndex < entities_.size(); ++firstIndex) {
        for (std::size_t secondIndex = firstIndex + 1; secondIndex < entities_.size(); ++secondIndex) {
            Entity& first = entities_[firstIndex];
            Entity& second = entities_[secondIndex];
            Vec2 normal{};
            float overlap = 0.0F;

            if (!collides(first, second, normal, overlap)) {
                continue;
            }

            const bool trigger = first.collider->isTrigger || second.collider->isTrigger;
            collisions_.push_back({first.id, second.id, normal, trigger});
            if (!trigger) {
                resolveCollision(first, second, normal, overlap);
            }
        }
    }
}

bool Scene::collides(const Entity& first, const Entity& second, Vec2& normal, float& overlap) {
    if (!first.enabled || !second.enabled || !first.collider.has_value() || !second.collider.has_value()) {
        return false;
    }

    const Vec2 delta = second.transform.position - first.transform.position;
    const float overlapX = first.collider->halfExtents.x + second.collider->halfExtents.x - std::abs(delta.x);
    const float overlapY = first.collider->halfExtents.y + second.collider->halfExtents.y - std::abs(delta.y);
    if (overlapX <= 0.0F || overlapY <= 0.0F) {
        return false;
    }

    if (overlapX < overlapY) {
        normal = {delta.x < 0.0F ? -1.0F : 1.0F, 0.0F};
        overlap = overlapX;
    } else {
        normal = {0.0F, delta.y < 0.0F ? -1.0F : 1.0F};
        overlap = overlapY;
    }
    return true;
}

void Scene::resolveCollision(Entity& first, Entity& second, const Vec2& normal, float overlap) {
    const float firstInverseMass = first.body.has_value() && first.body->dynamic ? 1.0F : 0.0F;
    const float secondInverseMass = second.body.has_value() && second.body->dynamic ? 1.0F : 0.0F;
    const float totalInverseMass = firstInverseMass + secondInverseMass;
    if (totalInverseMass <= 0.0F) {
        return;
    }

    const Vec2 correction = normal * (overlap / totalInverseMass);
    first.transform.position -= correction * firstInverseMass;
    second.transform.position += correction * secondInverseMass;

    const Vec2 firstVelocity = first.body.has_value() ? first.body->velocity : Vec2{};
    const Vec2 secondVelocity = second.body.has_value() ? second.body->velocity : Vec2{};
    const float separatingVelocity = dot(secondVelocity - firstVelocity, normal);
    if (separatingVelocity >= 0.0F) {
        return;
    }

    const float firstBounciness = first.body.has_value() ? first.body->bounciness : 0.0F;
    const float secondBounciness = second.body.has_value() ? second.body->bounciness : 0.0F;
    const float restitution = std::min(firstBounciness, secondBounciness);
    const float impulseSize = -(1.0F + restitution) * separatingVelocity / totalInverseMass;
    const Vec2 impulse = normal * impulseSize;

    if (first.body.has_value() && first.body->dynamic) {
        first.body->velocity -= impulse * firstInverseMass;
    }
    if (second.body.has_value() && second.body->dynamic) {
        second.body->velocity += impulse * secondInverseMass;
    }
}

void Scene::render(Renderer& renderer, const Camera2D& camera) const {
    std::vector<const Entity*> drawList;
    drawList.reserve(entities_.size());
    for (const Entity& item : entities_) {
        if (item.enabled && item.sprite.has_value() && item.sprite->visible) {
            drawList.push_back(&item);
        }
    }
    std::sort(drawList.begin(), drawList.end(), [](const Entity* first, const Entity* second) {
        return first->sprite->layer < second->sprite->layer;
    });

    const float zoom = std::max(camera.zoom, 0.001F);
    const Vec2 viewportCenter{static_cast<float>(renderer.width()) * 0.5F, static_cast<float>(renderer.height()) * 0.5F};
    for (const Entity* item : drawList) {
        const Vec2 size{
            item->sprite->size.x * item->transform.scale.x * zoom,
            item->sprite->size.y * item->transform.scale.y * zoom};
        const Vec2 center = (item->transform.position - camera.center) * zoom + viewportCenter;
        renderer.fillRect({center - size * 0.5F, size}, item->sprite->color);
    }
}

} // namespace orbit

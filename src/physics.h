#ifndef PHYSICS_H
#define PHYSICS_H

#include <limits>
#include <entt/entt.hpp> // https://github.com/skypjack/entt
#include <glm/glm.hpp>
#include "components.h"

constexpr float MIN_TICK_TIME = std::numeric_limits<float>::epsilon();

static double calcGravity(float m1, float m2, float distance) {
    static constexpr double G = 6.6743e-11;
    return G * m1 * m2 / std::pow(distance, 2.0);
}

// Check for sphere collision
bool isColliding(const component::trans& s1, const component::trans& s2) {
    static constexpr auto distSquared = [](const glm::vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; };
    return distSquared(s1.pos - s2.pos) < std::powf(s1.scale.x + s2.scale.x, 2.f);
}

/**
 * m_1 * v_1 + m_2 * v_2 = M_1 * V_1 + M_2 * V_2
 * m_1 * v_1 + m_2 * v_2 - M_2 * V_2 = M_1 * V_1
 * m_1 * v_1 + m_2 * (v_2 - V_2) = m_1 * V_1
 * 
 * V_1 = (m_1 - m_2) * v_1 / (m_1 + m_2) + 2 * m_2 * v_2 / (m_1 + m_2)
 */
std::pair<glm::dvec3, glm::dvec3> getImpactVel(const component::phys& p1, const component::phys& p2) {
    // return static_cast<double>((affected.mass - other.mass) * glm::length(affected.vel) / (affected.mass + other.mass))
    //     + static_cast<double>(2.f * other.mass * glm::length(other.vel) / (affected.mass + other.mass));
    auto v_1{glm::length(p1.vel)}, v_2{glm::length(p2.vel)};
    auto f_1{v_1 * p1.mass}, f_2{v_2 * p2.mass};
    auto fTotal = f_1 + f_2;

    // If colliding with an immovable object, just bounce right back.
    if (p1.bStatic)
        return {{}, -2.0 * p2.vel};
    else if (p2.bStatic)
        return {-2.0 * p1.vel, {}};

    return {
        (fTotal * 0.5 / p1.mass) * -glm::normalize(p1.vel),
        (fTotal * 0.5 / p2.mass) * -glm::normalize(p2.vel),
    };
}

void enforcePosition(component::trans& t1, component::trans& t2, bool p1Static, bool p2Static) {
    if (p1Static && p2Static)
        return;
    
    auto r1{t1.scale.x}, r2{t2.scale.x};
    auto dist = t2.pos - t1.pos;
    auto dir = glm::normalize(dist);
    auto disp = dir * (r1 + r2);
    auto centre = t1.pos + (r1 / r2) * dist;
    
    if (p2Static) {
        t1.pos = centre - dir * r1;
    } else if (p1Static) {
        t2.pos = centre + dir * r2;
    } else {
        t1.pos = centre - dir * r1 * 0.5f;
        t2.pos = centre + dir * r2 * 0.5f;
    }
}

/**
 * Note: For ekstra precision during physics calculations
 * we promote variables to doubles.
 */
/// When you don't know the param syntax, just make it a template. :D
template <typename T>
void calcPhysics(T&& entities, float deltaTime = 0.f)
{
    if (deltaTime <= MIN_TICK_TIME)
        return;

    const auto time = static_cast<double>(deltaTime);
    std::vector<std::pair<entt::entity, entt::entity>> collidedObjects{};
    collidedObjects.reserve(entities.size());

    unsigned int i{0};
    for (auto it{entities.begin()}; it != entities.end(); ++it, ++i)
    {
        auto t = entities.get<component::trans>(*it);
        auto &p = entities.get<component::phys>(*it);

        if (p.bStatic)
            continue;

        glm::dvec3 f{0.f, 0.f, 0.f};

        // Since t and t2 are copies, we can modify them all we want before testing for collision.
        // t.pos += static_cast<glm::vec3>(p.vel * time);

        for (auto other{entities.begin()}; other != entities.end(); ++other)
        {
            if (it == other)
                continue;

            auto [t2, p2] = entities.get<component::trans, component::phys>(*other);
            glm::dvec3 dist = t2.pos - t.pos;

            const auto distNorm = glm::normalize(dist);

            f += distNorm * calcGravity(p.mass, p2.mass, dist.length());

            // t2.pos += static_cast<glm::vec3>(p2.vel * time);
            if (isColliding(t, t2))
                collidedObjects.push_back({*it, *other});
        }

        // std::cout << "a: " << glm::length(a) << ", deltaTime: " << deltaTime << ", a * deltaTime: " << glm::length(a *deltaTime) << std::endl;
        const auto a = f / static_cast<double>(p.mass);
        if (glm::any(glm::isnan(a)))
            continue;
        p.vel += a * time;
        // // Demote double to float for final calculation. (No need to keep variable if it cannot be stored)
        // t.pos += static_cast<glm::vec3>(p.vel * time);
    }
    

    // Handle collisions
    for (auto it = collidedObjects.begin(); it != collidedObjects.end(); ++it) {
        auto& [e1, e2] = *it;
        // Check if collision has already been handled before.
        bool bPreviouslyTested = false;
        for (auto prev{collidedObjects.begin()}; prev != it; ++prev) {
            if (e1 == prev->second && e2 == prev->first) {
                bPreviouslyTested = true;
                break;
            }
        }
        if (bPreviouslyTested)
            continue;
        
        // if (!(EM.has<component::trans, component::phys>(e1) && EM.has<component::trans, component::phys>(e2))) {
        //     std::cout << "Skipped collision because entity was invalid" << std::endl;
        //     continue;
        // }

        auto& [t1, p1] = entities.get<component::trans, component::phys>(e1);
        auto& [t2, p2] = entities.get<component::trans, component::phys>(e2);

        enforcePosition(t1, t2, p1.bStatic, p2.bStatic);

        auto& [v1, v2] = getImpactVel(p1, p2);
        p1.vel += v1;
        p2.vel += v2;
    }


    // Apply velocities:
    for (auto it{entities.begin()}; it != entities.end(); ++it) {
        auto &[t, p] = entities.get<component::trans, component::phys>(*it);
        // Demote double to float for final calculation. (No need to keep variable if it cannot be stored)
        t.pos += static_cast<glm::vec3>(p.vel * time);
    }
}

#endif // PHYSICS_H
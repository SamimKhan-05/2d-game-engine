#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>

#include "engine/Application.hpp"

namespace {

using namespace orbit;

constexpr float kWidth = 960.0F;
constexpr float kHeight = 540.0F;
constexpr float kGroundY = 448.0F;
constexpr float kGravity = 1'850.0F;
constexpr float kRunSpeed = 260.0F;
constexpr float kDashSpeed = 680.0F;
constexpr float kJumpSpeed = 690.0F;
constexpr float kFighterWidth = 36.0F;
constexpr float kPi = 3.14159265359F;

enum class MeleeWeapon {
    sword,
    axe,
    spear
};

enum class Offhand {
    grenade,
    shield
};

enum class MatchState {
    fighting,
    victory
};

struct WeaponStats {
    float damage = 0.0F;
    float reach = 0.0F;
    float cooldown = 0.0F;
    float activeTime = 0.0F;
    float knockback = 0.0F;
};

struct Fighter {
    Vec2 position{};
    Vec2 velocity{};
    float health = 100.0F;
    float displayHealth = 100.0F;
    float attackTimer = 0.0F;
    float attackDuration = 0.0F;
    float attackCooldown = 0.0F;
    float dashTimer = 0.0F;
    float dashCooldown = 0.0F;
    float shieldTimer = 0.0F;
    float utilityCooldown = 0.0F;
    float hitFlashTimer = 0.0F;
    float loadoutFlashTimer = 0.0F;
    bool grounded = true;
    bool facingRight = true;
    bool attackConnected = false;
    MeleeWeapon weapon = MeleeWeapon::sword;
    Offhand offhand = Offhand::shield;
};

struct Grenade {
    bool active = false;
    int owner = 0;
    Vec2 position{};
    Vec2 velocity{};
    float fuse = 0.0F;
    float spin = 0.0F;
};

struct Explosion {
    bool active = false;
    Vec2 position{};
    float timer = 0.0F;
};

class ArenaClash final : public IGame {
public:
    void onStart() override {
        resetMatch();
    }

    void onUpdate(float deltaSeconds, const Input& input) override {
        animationTime_ += deltaSeconds;
        for (Fighter& fighter : fighters_) {
            fighter.displayHealth += (fighter.health - fighter.displayHealth)
                * std::min(1.0F, deltaSeconds * 10.0F);
        }

        if (input.pressed(Key::r)) {
            resetMatch();
            return;
        }
        if (state_ == MatchState::victory) {
            return;
        }

        for (Fighter& fighter : fighters_) {
            tickTimers(fighter, deltaSeconds);
        }

        updateLoadout(input);
        moveFighter(fighters_[0], input.down(Key::a), input.down(Key::d), input.pressed(Key::w), input.pressed(Key::s), deltaSeconds);
        moveFighter(fighters_[1], input.down(Key::j), input.down(Key::l), input.pressed(Key::i), input.pressed(Key::k), deltaSeconds);

        if (input.pressed(Key::q)) {
            startAttack(0);
        }
        if (input.pressed(Key::u)) {
            startAttack(1);
        }
        if (input.pressed(Key::e)) {
            useOffhand(0);
        }
        if (input.pressed(Key::o)) {
            useOffhand(1);
        }

        integrate(fighters_[0], deltaSeconds);
        integrate(fighters_[1], deltaSeconds);
        separateFighters();
        resolveAttack(0, 1);
        resolveAttack(1, 0);
        updateGrenades(deltaSeconds);

        if (fighters_[0].health <= 0.0F || fighters_[1].health <= 0.0F) {
            state_ = MatchState::victory;
            winner_ = fighters_[0].health <= 0.0F ? 1 : 0;
        }
    }

    void onRender(Renderer& renderer) override {
        drawStage(renderer);
        drawGrenades(renderer);
        drawFighter(renderer, fighters_[0], colors::blue, colors::cyan);
        drawFighter(renderer, fighters_[1], colors::red, colors::coral);
        drawHud(renderer);

        if (state_ == MatchState::victory) {
            const bool blueWon = winner_ == 0;
            const Color victoryColor = blueWon ? colors::blue : colors::red;
            const float pulse = (std::sin(animationTime_ * 5.0F) + 1.0F) * 0.5F;
            const float panelInset = 8.0F + pulse * 8.0F;
            renderer.fillRect({{214.0F - panelInset, 155.0F - panelInset}, {532.0F + panelInset * 2.0F, 206.0F + panelInset * 2.0F}}, colors::ink);
            renderer.drawRect({{214.0F - panelInset, 155.0F - panelInset}, {532.0F + panelInset * 2.0F, 206.0F + panelInset * 2.0F}}, victoryColor, 3);
            renderer.drawCircle({kWidth * 0.5F, 250.0F}, 96.0F + pulse * 18.0F, victoryColor, 2);
            drawCentered(renderer, 192.0F + pulse * 4.0F, blueWon ? "BLUE VICTORY" : "RED VICTORY", victoryColor, 4);
            drawCentered(renderer, 274.0F, "PRESS R TO FIGHT AGAIN", colors::gold, 2);
        }
    }

private:
    static WeaponStats statsFor(MeleeWeapon weapon) {
        switch (weapon) {
        case MeleeWeapon::sword: return {13.0F, 78.0F, 0.42F, 0.16F, 300.0F};
        case MeleeWeapon::axe: return {23.0F, 60.0F, 0.72F, 0.23F, 430.0F};
        case MeleeWeapon::spear: return {16.0F, 132.0F, 0.55F, 0.18F, 350.0F};
        }
        return {};
    }

    void resetMatch() {
        state_ = MatchState::fighting;
        winner_ = -1;
        fighters_[0] = {};
        fighters_[0].position = {210.0F, kGroundY};
        fighters_[0].facingRight = true;
        fighters_[0].weapon = MeleeWeapon::sword;
        fighters_[0].offhand = Offhand::shield;

        fighters_[1] = {};
        fighters_[1].position = {750.0F, kGroundY};
        fighters_[1].facingRight = false;
        fighters_[1].weapon = MeleeWeapon::axe;
        fighters_[1].offhand = Offhand::grenade;

        grenades_ = {};
        explosion_ = {};
    }

    void tickTimers(Fighter& fighter, float deltaSeconds) {
        fighter.attackTimer = std::max(0.0F, fighter.attackTimer - deltaSeconds);
        fighter.attackCooldown = std::max(0.0F, fighter.attackCooldown - deltaSeconds);
        fighter.dashTimer = std::max(0.0F, fighter.dashTimer - deltaSeconds);
        fighter.dashCooldown = std::max(0.0F, fighter.dashCooldown - deltaSeconds);
        fighter.shieldTimer = std::max(0.0F, fighter.shieldTimer - deltaSeconds);
        fighter.utilityCooldown = std::max(0.0F, fighter.utilityCooldown - deltaSeconds);
        fighter.hitFlashTimer = std::max(0.0F, fighter.hitFlashTimer - deltaSeconds);
        fighter.loadoutFlashTimer = std::max(0.0F, fighter.loadoutFlashTimer - deltaSeconds);
    }

    void updateLoadout(const Input& input) {
        if (input.pressed(Key::one)) selectWeapon(0, MeleeWeapon::sword);
        if (input.pressed(Key::two)) selectWeapon(0, MeleeWeapon::axe);
        if (input.pressed(Key::three)) selectWeapon(0, MeleeWeapon::spear);
        if (input.pressed(Key::four)) selectOffhand(0, Offhand::grenade);
        if (input.pressed(Key::five)) selectOffhand(0, Offhand::shield);

        if (input.pressed(Key::seven)) selectWeapon(1, MeleeWeapon::sword);
        if (input.pressed(Key::eight)) selectWeapon(1, MeleeWeapon::axe);
        if (input.pressed(Key::nine)) selectWeapon(1, MeleeWeapon::spear);
        if (input.pressed(Key::n)) selectOffhand(1, Offhand::grenade);
        if (input.pressed(Key::m)) selectOffhand(1, Offhand::shield);
    }

    void selectWeapon(int fighterIndex, MeleeWeapon weapon) {
        Fighter& fighter = fighters_[static_cast<std::size_t>(fighterIndex)];
        fighter.weapon = weapon;
        fighter.loadoutFlashTimer = 0.28F;
    }

    void selectOffhand(int fighterIndex, Offhand offhand) {
        Fighter& fighter = fighters_[static_cast<std::size_t>(fighterIndex)];
        fighter.offhand = offhand;
        fighter.loadoutFlashTimer = 0.28F;
    }

    void moveFighter(Fighter& fighter, bool left, bool right, bool jumpPressed, bool dashPressed, float deltaSeconds) {
        const float direction = static_cast<float>(right) - static_cast<float>(left);
        if (direction != 0.0F) {
            fighter.facingRight = direction > 0.0F;
        }

        if (dashPressed && fighter.dashCooldown <= 0.0F) {
            fighter.dashTimer = 0.16F;
            fighter.dashCooldown = 0.78F;
        }

        if (fighter.dashTimer > 0.0F) {
            fighter.velocity.x = fighter.facingRight ? kDashSpeed : -kDashSpeed;
        } else if (direction != 0.0F) {
            fighter.velocity.x = direction * kRunSpeed;
        } else {
            fighter.velocity.x *= std::max(0.0F, 1.0F - deltaSeconds * 13.0F);
        }

        if (jumpPressed && fighter.grounded) {
            fighter.velocity.y = -kJumpSpeed;
            fighter.grounded = false;
        }
    }

    void startAttack(int attackerIndex) {
        Fighter& attacker = fighters_[static_cast<std::size_t>(attackerIndex)];
        if (attacker.attackCooldown > 0.0F) {
            return;
        }

        const WeaponStats stats = statsFor(attacker.weapon);
        attacker.attackTimer = stats.activeTime;
        attacker.attackDuration = stats.activeTime;
        attacker.attackCooldown = stats.cooldown;
        attacker.attackConnected = false;
    }

    void useOffhand(int owner) {
        Fighter& fighter = fighters_[static_cast<std::size_t>(owner)];
        if (fighter.utilityCooldown > 0.0F) {
            return;
        }

        if (fighter.offhand == Offhand::shield) {
            fighter.shieldTimer = 0.62F;
            fighter.utilityCooldown = 0.9F;
            return;
        }

        Grenade& grenade = grenades_[static_cast<std::size_t>(owner)];
        if (grenade.active) {
            return;
        }

        const float direction = fighter.facingRight ? 1.0F : -1.0F;
        grenade.active = true;
        grenade.owner = owner;
        grenade.position = fighter.position + Vec2{direction * 28.0F, -54.0F};
        grenade.velocity = {direction * 440.0F, -370.0F};
        grenade.fuse = 1.25F;
        grenade.spin = 0.0F;
        fighter.utilityCooldown = 0.85F;
    }

    void integrate(Fighter& fighter, float deltaSeconds) {
        if (!fighter.grounded) {
            fighter.velocity.y += kGravity * deltaSeconds;
        }
        fighter.position += fighter.velocity * deltaSeconds;

        if (fighter.position.y >= kGroundY) {
            fighter.position.y = kGroundY;
            fighter.velocity.y = 0.0F;
            fighter.grounded = true;
        }
        fighter.position.x = clamp(fighter.position.x, 42.0F, kWidth - 42.0F);
    }

    void separateFighters() {
        Fighter& blue = fighters_[0];
        Fighter& red = fighters_[1];
        const float distance = red.position.x - blue.position.x;
        const float overlap = kFighterWidth - std::abs(distance);
        if (overlap <= 0.0F || std::abs(red.position.y - blue.position.y) > 78.0F) {
            return;
        }

        const float direction = distance >= 0.0F ? 1.0F : -1.0F;
        blue.position.x = clamp(blue.position.x - direction * overlap * 0.5F, 42.0F, kWidth - 42.0F);
        red.position.x = clamp(red.position.x + direction * overlap * 0.5F, 42.0F, kWidth - 42.0F);
    }

    void resolveAttack(int attackerIndex, int defenderIndex) {
        Fighter& attacker = fighters_[static_cast<std::size_t>(attackerIndex)];
        Fighter& defender = fighters_[static_cast<std::size_t>(defenderIndex)];
        if (attacker.attackTimer <= 0.0F || attacker.attackConnected) {
            return;
        }

        const WeaponStats stats = statsFor(attacker.weapon);
        const float facing = attacker.facingRight ? 1.0F : -1.0F;
        const float forwardDistance = (defender.position.x - attacker.position.x) * facing;
        const float heightDifference = std::abs(defender.position.y - attacker.position.y);
        if (forwardDistance < 0.0F || forwardDistance > stats.reach || heightDifference > 84.0F) {
            return;
        }

        attacker.attackConnected = true;
        damage(defender, stats.damage, facing * stats.knockback);
    }

    void updateGrenades(float deltaSeconds) {
        explosion_.timer = std::max(0.0F, explosion_.timer - deltaSeconds);
        if (explosion_.timer <= 0.0F) {
            explosion_.active = false;
        }

        for (Grenade& grenade : grenades_) {
            if (!grenade.active) {
                continue;
            }
            grenade.velocity.y += kGravity * deltaSeconds;
            grenade.position += grenade.velocity * deltaSeconds;
            grenade.fuse -= deltaSeconds;
            grenade.spin += grenade.velocity.x * deltaSeconds * 0.025F;

            if (grenade.position.y >= kGroundY - 6.0F && grenade.velocity.y > 0.0F) {
                grenade.position.y = kGroundY - 6.0F;
                grenade.velocity.y *= -0.32F;
                grenade.velocity.x *= 0.78F;
                if (std::abs(grenade.velocity.y) < 90.0F) {
                    grenade.fuse = std::min(grenade.fuse, 0.15F);
                }
            }
            if (grenade.position.x < 20.0F || grenade.position.x > kWidth - 20.0F) {
                grenade.velocity.x *= -0.7F;
                grenade.position.x = clamp(grenade.position.x, 20.0F, kWidth - 20.0F);
            }
            if (grenade.fuse <= 0.0F) {
                detonate(grenade);
            }
        }
    }

    void detonate(Grenade& grenade) {
        constexpr float blastRadius = 118.0F;
        const int targetIndex = grenade.owner == 0 ? 1 : 0;
        Fighter& target = fighters_[static_cast<std::size_t>(targetIndex)];
        const float distance = length(target.position - grenade.position);
        if (distance < blastRadius) {
            const float damageAmount = 30.0F * (1.0F - distance / blastRadius);
            const float direction = target.position.x >= grenade.position.x ? 1.0F : -1.0F;
            damage(target, damageAmount, direction * 390.0F);
        }

        explosion_.active = true;
        explosion_.position = grenade.position;
        explosion_.timer = 0.24F;
        grenade.active = false;
    }

    static void damage(Fighter& target, float amount, float horizontalForce) {
        target.hitFlashTimer = 0.12F;
        if (target.shieldTimer > 0.0F) {
            target.velocity.x += horizontalForce * 0.35F;
            return;
        }

        target.health = std::max(0.0F, target.health - amount);
        target.velocity.x += horizontalForce;
        if (target.grounded) {
            target.velocity.y = -190.0F;
            target.grounded = false;
        }
    }

    static Vec2 rotate(Vec2 vector, float radians) {
        const float cosine = std::cos(radians);
        const float sine = std::sin(radians);
        return {vector.x * cosine - vector.y * sine, vector.x * sine + vector.y * cosine};
    }

    static Color blend(Color from, Color to, float amount) {
        const float blendAmount = clamp(amount, 0.0F, 1.0F);
        const auto channel = [blendAmount](std::uint8_t first, std::uint8_t second) {
            return static_cast<std::uint8_t>(std::lround(
                static_cast<float>(first) + (static_cast<float>(second) - static_cast<float>(first)) * blendAmount));
        };
        return {channel(from.r, to.r), channel(from.g, to.g), channel(from.b, to.b)};
    }

    void drawStage(Renderer& renderer) const {
        const float skyPulse = (std::sin(animationTime_ * 0.35F) + 1.0F) * 0.5F;
        renderer.clear(blend(colors::navy, Color{15, 31, 58}, skyPulse));
        renderer.fillRect({{0.0F, 250.0F}, {kWidth, 200.0F}}, blend(Color{11, 38, 54}, Color{16, 48, 65}, skyPulse));

        const std::array<Vec2, 9> stars{{{72.0F, 92.0F}, {166.0F, 156.0F}, {264.0F, 66.0F}, {380.0F, 130.0F}, {492.0F, 84.0F}, {608.0F, 166.0F}, {702.0F, 50.0F}, {844.0F, 180.0F}, {920.0F, 82.0F}}};
        for (std::size_t index = 0; index < stars.size(); ++index) {
            const float twinkle = (std::sin(animationTime_ * (2.0F + static_cast<float>(index) * 0.17F) + static_cast<float>(index)) + 1.0F) * 0.5F;
            renderer.fillCircle(stars[index], 1.0F + twinkle * 2.0F, blend(colors::slate, colors::white, twinkle));
        }

        const float cloudDrift = std::fmod(animationTime_ * 17.0F, kWidth + 220.0F) - 110.0F;
        for (int cloud = 0; cloud < 3; ++cloud) {
            const float cloudX = std::fmod(cloudDrift + static_cast<float>(cloud) * 360.0F, kWidth + 180.0F) - 80.0F;
            const float cloudY = 115.0F + static_cast<float>(cloud) * 38.0F;
            const Color cloudColor = blend(Color{18, 43, 65}, Color{28, 57, 79}, 0.3F + skyPulse * 0.35F);
            renderer.fillCircle({cloudX, cloudY}, 22.0F, cloudColor);
            renderer.fillCircle({cloudX + 28.0F, cloudY - 9.0F}, 29.0F, cloudColor);
            renderer.fillCircle({cloudX + 61.0F, cloudY}, 21.0F, cloudColor);
            renderer.fillRect({{cloudX - 18.0F, cloudY}, {98.0F, 21.0F}}, cloudColor);
        }

        const float moonPulse = (std::sin(animationTime_ * 1.8F) + 1.0F) * 0.5F;
        renderer.drawCircle({760.0F, 112.0F}, 67.0F + moonPulse * 7.0F, Color{134, 159, 193}, 1);
        renderer.fillCircle({760.0F, 112.0F}, 58.0F, Color{255, 222, 132});
        renderer.fillCircle({760.0F, 112.0F}, 45.0F, Color{255, 235, 165});

        const std::array<int, 11> buildings{72, 120, 86, 162, 98, 134, 184, 108, 148, 82, 126};
        for (std::size_t index = 0; index < buildings.size(); ++index) {
            const float width = 86.0F;
            const float height = static_cast<float>(buildings[index]);
            const float x = static_cast<float>(index) * width;
            renderer.fillRect({{x, kGroundY - height}, {width - 4.0F, height}}, Color{12, 29, 47});
            for (float windowY = kGroundY - height + 18.0F; windowY < kGroundY - 12.0F; windowY += 26.0F) {
                const float light = (std::sin(animationTime_ * 3.0F + static_cast<float>(index) * 1.7F + windowY * 0.04F) + 1.0F) * 0.5F;
                const Color windowColor = blend(Color{112, 92, 61}, Color{244, 202, 106}, light);
                renderer.fillRect({{x + 16.0F, windowY}, {9.0F, 8.0F}}, windowColor);
                renderer.fillRect({{x + 52.0F, windowY}, {9.0F, 8.0F}}, windowColor);
            }
        }

        renderer.fillRect({{0.0F, kGroundY}, {kWidth, kHeight - kGroundY}}, Color{21, 44, 50});
        renderer.drawLine({0.0F, kGroundY}, {kWidth, kGroundY}, colors::gold, 2);
        const float floorScroll = std::fmod(animationTime_ * 62.0F, 42.0F);
        for (float x = -floorScroll; x < kWidth; x += 42.0F) {
            renderer.drawLine({x, kGroundY + 18.0F}, {x + 20.0F, kGroundY + 18.0F}, colors::slate, 2);
        }
    }

    void drawFighter(Renderer& renderer, const Fighter& fighter, Color bodyColor, Color accentColor) const {
        const float facing = fighter.facingRight ? 1.0F : -1.0F;
        const float movement = clamp(std::abs(fighter.velocity.x) / kRunSpeed, 0.0F, 1.0F);
        const float animationRate = fighter.grounded ? 2.0F + movement * 16.0F : 0.0F;
        const float phase = animationTime_ * animationRate + fighter.position.x * 0.025F;
        const float stride = fighter.grounded ? std::sin(phase) * (3.0F + movement * 9.0F) : 0.0F;
        const float bob = fighter.grounded ? std::abs(std::sin(phase)) * (1.0F + movement * 2.0F) : 0.0F;
        const Vec2 base{fighter.position.x, fighter.position.y - bob};
        const bool flashing = fighter.hitFlashTimer > 0.0F && std::sin(animationTime_ * 52.0F) > 0.0F;
        const Color currentColor = flashing ? colors::white : bodyColor;
        const float attackProgress = fighter.attackTimer > 0.0F && fighter.attackDuration > 0.0F
            ? 1.0F - clamp(fighter.attackTimer / fighter.attackDuration, 0.0F, 1.0F)
            : 0.0F;
        const float attackArc = fighter.attackTimer > 0.0F ? std::sin(attackProgress * kPi) : 0.0F;
        const float dashLean = fighter.dashTimer > 0.0F ? facing * 8.0F : 0.0F;

        if (fighter.dashTimer > 0.0F) {
            for (int streak = 1; streak <= 5; ++streak) {
                const float offset = static_cast<float>(streak) * 17.0F + std::sin(animationTime_ * 24.0F + static_cast<float>(streak)) * 4.0F;
                renderer.drawLine({base.x - facing * offset, base.y - 28.0F - static_cast<float>(streak) * 4.0F},
                    {base.x - facing * (offset + 22.0F), base.y - 28.0F - static_cast<float>(streak) * 4.0F}, accentColor, 2);
            }
        }

        const Vec2 leftHip{base.x - 7.0F + dashLean, base.y - 26.0F};
        const Vec2 rightHip{base.x + 7.0F + dashLean, base.y - 26.0F};
        const Vec2 leftFoot{base.x - 8.0F - stride, base.y};
        const Vec2 rightFoot{base.x + 8.0F + stride, base.y};
        const Vec2 jumpLeftFoot{base.x - facing * 15.0F, base.y - 9.0F};
        const Vec2 jumpRightFoot{base.x + facing * 5.0F, base.y - 14.0F};
        renderer.drawLine(leftHip, fighter.grounded ? leftFoot : jumpLeftFoot, currentColor, 7);
        renderer.drawLine(rightHip, fighter.grounded ? rightFoot : jumpRightFoot, currentColor, 7);
        renderer.fillRect({{base.x - 13.0F + dashLean, base.y - 59.0F}, {26.0F, 34.0F}}, currentColor);
        renderer.fillCircle({base.x + dashLean * 0.45F, base.y - 73.0F - attackArc * 2.0F}, 15.0F, currentColor);

        const Vec2 weaponHand{base.x + facing * (21.0F + attackArc * 8.0F) + dashLean, base.y - 42.0F - attackArc * 10.0F};
        const Vec2 armStart{base.x + dashLean, base.y - 50.0F};
        renderer.drawLine(armStart, weaponHand, currentColor, 5);

        const Color weaponColor = fighter.attackTimer > 0.0F ? colors::gold : colors::white;
        const float swing = fighter.attackTimer > 0.0F
            ? facing * (-0.8F + attackProgress * 1.6F)
            : facing * -0.12F;
        switch (fighter.weapon) {
        case MeleeWeapon::sword: {
            const Vec2 blade = weaponHand + rotate({facing * 36.0F, -9.0F}, swing);
            renderer.drawLine(weaponHand, blade, weaponColor, 3);
            const Vec2 guard = weaponHand + rotate({facing * 4.0F, 5.0F}, swing);
            renderer.drawLine(guard + rotate({0.0F, 7.0F}, swing), guard + rotate({0.0F, -8.0F}, swing), accentColor, 3);
            break;
        }
        case MeleeWeapon::axe: {
            const Vec2 handleEnd = weaponHand + rotate({facing * 24.0F, -24.0F}, swing * 1.25F);
            renderer.drawLine(weaponHand, handleEnd, colors::gold, 3);
            renderer.fillCircle(handleEnd + rotate({facing * 4.0F, -1.0F}, swing * 1.25F), 8.0F + attackArc * 3.0F, weaponColor);
            break;
        }
        case MeleeWeapon::spear: {
            const float thrust = 1.0F + attackArc * 0.52F;
            const Vec2 spearTip = weaponHand + rotate({facing * 66.0F * thrust, -4.0F}, swing * 0.35F);
            renderer.drawLine(weaponHand, spearTip, weaponColor, 2);
            renderer.fillCircle(spearTip, 4.0F + attackArc * 2.0F, colors::gold);
            break;
        }
        }

        if (fighter.attackTimer > 0.0F) {
            renderer.drawCircle(weaponHand, 20.0F + attackArc * 18.0F, weaponColor, 1);
        }

        const Vec2 offhand{base.x - facing * 20.0F + dashLean, base.y - 40.0F + std::sin(animationTime_ * 7.0F) * 2.0F};
        if (fighter.offhand == Offhand::grenade) {
            const float grenadeBob = std::sin(animationTime_ * 8.0F + base.x * 0.02F) * 1.5F;
            renderer.fillCircle(offhand + Vec2{0.0F, grenadeBob}, 7.0F, colors::gold);
            renderer.drawLine(offhand + Vec2{0.0F, -8.0F + grenadeBob}, offhand + Vec2{facing * 4.0F, -13.0F + grenadeBob}, colors::white, 1);
        } else {
            const float shieldPulse = (std::sin(animationTime_ * 22.0F) + 1.0F) * 0.5F;
            const float shieldWidth = 14.0F + (fighter.shieldTimer > 0.0F ? shieldPulse * 7.0F : 0.0F);
            const float shieldHeight = 28.0F + (fighter.shieldTimer > 0.0F ? shieldPulse * 9.0F : 0.0F);
            const Color shieldColor = fighter.shieldTimer > 0.0F ? blend(colors::gold, colors::white, shieldPulse) : accentColor;
            if (fighter.shieldTimer > 0.0F) {
                renderer.drawCircle(offhand, 22.0F + shieldPulse * 10.0F, shieldColor, 2);
            }
            renderer.fillRect({{offhand.x - shieldWidth * 0.5F, offhand.y - shieldHeight * 0.5F}, {shieldWidth, shieldHeight}}, shieldColor);
            renderer.drawRect({{offhand.x - shieldWidth * 0.5F, offhand.y - shieldHeight * 0.5F}, {shieldWidth, shieldHeight}}, colors::white, 1);
        }
    }

    void drawGrenades(Renderer& renderer) const {
        for (const Grenade& grenade : grenades_) {
            if (grenade.active) {
                const float fusePulse = (std::sin(animationTime_ * 18.0F) + 1.0F) * 0.5F;
                const Color grenadeColor = grenade.fuse < 0.45F ? blend(colors::gold, colors::red, fusePulse) : colors::gold;
                renderer.drawCircle(grenade.position, 10.0F + fusePulse * 2.0F, grenadeColor, 1);
                renderer.fillCircle(grenade.position, 7.0F, grenadeColor);
                const Vec2 spinArm = rotate({0.0F, -8.0F}, grenade.spin);
                renderer.drawLine(grenade.position - spinArm, grenade.position + spinArm, colors::white, 1);
                renderer.drawLine(grenade.position - rotate(spinArm, kPi * 0.5F), grenade.position + rotate(spinArm, kPi * 0.5F), colors::white, 1);
            }
        }
        if (explosion_.active) {
            const float progress = 1.0F - explosion_.timer / 0.24F;
            renderer.drawCircle(explosion_.position, 18.0F + progress * 98.0F, colors::gold, 3);
            renderer.drawCircle(explosion_.position, 8.0F + progress * 67.0F, colors::red, 2);
            renderer.fillCircle(explosion_.position, 13.0F + progress * 38.0F, blend(colors::white, colors::gold, progress));
        }
    }

    void drawHud(Renderer& renderer) const {
        renderer.fillRect({{0.0F, 0.0F}, {kWidth, 82.0F}}, Color{7, 18, 31});
        const float titleBob = std::sin(animationTime_ * 2.2F) * 2.0F;
        renderer.drawText({390.0F, 14.0F + titleBob}, "ARENA CLASH", colors::gold, 2);
        drawHealthBar(renderer, 28.0F, 46.0F, fighters_[0], colors::blue, false);
        drawHealthBar(renderer, kWidth - 288.0F, 46.0F, fighters_[1], colors::red, true);
        renderer.drawText({28.0F, 28.0F}, "BLUE", colors::blue, 2);
        renderer.drawText({kWidth - 82.0F, 28.0F}, "RED", colors::red, 2);
        const Color blueLoadoutColor = fighters_[0].loadoutFlashTimer > 0.0F ? colors::gold : colors::white;
        const Color redLoadoutColor = fighters_[1].loadoutFlashTimer > 0.0F ? colors::gold : colors::white;
        renderer.drawText({28.0F, 68.0F}, std::string(weaponName(fighters_[0].weapon)) + " " + offhandName(fighters_[0].offhand), blueLoadoutColor, 1);
        const std::string redLoadout = std::string(weaponName(fighters_[1].weapon)) + " " + offhandName(fighters_[1].offhand);
        renderer.drawText({kWidth - static_cast<float>(redLoadout.size() * 6) - 28.0F, 68.0F}, redLoadout, redLoadoutColor, 1);

        drawCooldownMeter(renderer, 86.0F, 34.0F, fighters_[0].dashCooldown / 0.78F, colors::blue, false);
        drawCooldownMeter(renderer, 110.0F, 34.0F, fighters_[0].utilityCooldown / 0.9F, colors::gold, false);
        drawCooldownMeter(renderer, kWidth - 112.0F, 34.0F, fighters_[1].dashCooldown / 0.78F, colors::red, true);
        drawCooldownMeter(renderer, kWidth - 136.0F, 34.0F, fighters_[1].utilityCooldown / 0.9F, colors::gold, true);

        renderer.fillRect({{0.0F, 474.0F}, {kWidth, 66.0F}}, Color{7, 18, 31});
        const float panelSweep = std::fmod(animationTime_ * 90.0F, kWidth);
        renderer.drawLine({panelSweep - 90.0F, 474.0F}, {panelSweep, 474.0F}, colors::gold, 1);
        renderer.drawText({22.0F, 482.0F}, "BLUE A D MOVE W JUMP S DASH Q ATTACK E USE", colors::blue, 1);
        renderer.drawText({22.0F, 494.0F}, "1 SWORD 2 AXE 3 SPEAR 4 GRENADE 5 SHIELD", colors::white, 1);
        renderer.drawText({22.0F, 510.0F}, "RED J L MOVE I JUMP K DASH U ATTACK O USE", colors::red, 1);
        renderer.drawText({22.0F, 522.0F}, "7 SWORD 8 AXE 9 SPEAR N GRENADE M SHIELD", colors::white, 1);
    }

    static void drawHealthBar(Renderer& renderer, float x, float y, const Fighter& fighter, Color color, bool growsLeft) {
        constexpr float width = 260.0F;
        renderer.fillRect({{x, y}, {width, 14.0F}}, colors::ink);
        const float healthWidth = width * clamp(fighter.displayHealth, 0.0F, 100.0F) / 100.0F;
        const float fillX = growsLeft ? x + width - healthWidth : x;
        renderer.fillRect({{fillX, y}, {healthWidth, 14.0F}}, color);
        const Color borderColor = fighter.hitFlashTimer > 0.0F ? colors::gold : colors::white;
        renderer.drawRect({{x, y}, {width, 14.0F}}, borderColor, 1);
    }

    static void drawCooldownMeter(Renderer& renderer, float x, float y, float cooldownFraction, Color color, bool fillsLeft) {
        constexpr float width = 18.0F;
        constexpr float height = 5.0F;
        const float available = 1.0F - clamp(cooldownFraction, 0.0F, 1.0F);
        renderer.fillRect({{x, y}, {width, height}}, colors::ink);
        const float filledWidth = width * available;
        renderer.fillRect({{fillsLeft ? x + width - filledWidth : x, y}, {filledWidth, height}}, color);
        renderer.drawRect({{x, y}, {width, height}}, colors::white, 1);
    }

    static const char* weaponName(MeleeWeapon weapon) {
        switch (weapon) {
        case MeleeWeapon::sword: return "SWORD";
        case MeleeWeapon::axe: return "AXE";
        case MeleeWeapon::spear: return "SPEAR";
        }
        return "SWORD";
    }

    static const char* offhandName(Offhand offhand) {
        return offhand == Offhand::grenade ? "GRENADE" : "SHIELD";
    }

    static void drawCentered(Renderer& renderer, float y, const std::string& text, Color color, int scale) {
        const float width = static_cast<float>(text.size() * 6 * std::max(scale, 1));
        renderer.drawText({kWidth * 0.5F - width * 0.5F, y}, text, color, scale);
    }

    std::array<Fighter, 2> fighters_{};
    std::array<Grenade, 2> grenades_{};
    Explosion explosion_{};
    MatchState state_ = MatchState::fighting;
    int winner_ = -1;
    float animationTime_ = 0.0F;
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Application application;
    ArenaClash game;
    return application.run({"Orbit2D | Arena Clash", static_cast<int>(kWidth), static_cast<int>(kHeight), 120}, game);
}

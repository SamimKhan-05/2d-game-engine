#include <algorithm>
#include <cmath>
#include <string>

#include "engine/Application.hpp"
#include "engine/Scene.hpp"

namespace {

using namespace orbit;

constexpr float kArenaWidth = 960.0F;
constexpr float kArenaHeight = 540.0F;
constexpr int kWinningScore = 10;
constexpr float kBaseBallSpeed = 410.0F;
constexpr float kBallSpeedIncrease = 24.0F;
constexpr float kMaximumBallSpeed = 680.0F;

class PongGame final : public IGame {
public:
    void onStart() override {
        camera_.center = {kArenaWidth * 0.5F, kArenaHeight * 0.5F};
        scene_.gravity = {0.0F, 0.0F};

        ball_ = createBall();
        leftPaddle_ = createPaddle("left paddle", {72.0F, kArenaHeight * 0.5F}, colors::blue);
        rightPaddle_ = createPaddle("right paddle", {kArenaWidth - 72.0F, kArenaHeight * 0.5F}, colors::red);

        createWall("top wall", {kArenaWidth * 0.5F, 12.0F}, {kArenaWidth * 0.5F, 12.0F});
        createWall("bottom wall", {kArenaWidth * 0.5F, kArenaHeight - 12.0F}, {kArenaWidth * 0.5F, 12.0F});
        leftGoal_ = createGoal("left goal", {-20.0F, kArenaHeight * 0.5F});
        rightGoal_ = createGoal("right goal", {kArenaWidth + 20.0F, kArenaHeight * 0.5F});

        beginCountdown(1.0F);
    }

    void onUpdate(float deltaSeconds, const Input& input) override {
        if (input.pressed(Key::r)) {
            resetMatch();
        }

        if (state_ != RoundState::victory) {
            movePaddle(leftPaddle_, input.down(Key::w), input.down(Key::s), deltaSeconds);
            movePaddle(rightPaddle_, input.down(Key::up), input.down(Key::down), deltaSeconds);
        }

        if (state_ == RoundState::countdown) {
            countdownSeconds_ -= deltaSeconds;
            if (countdownSeconds_ <= 0.0F) {
                launchRound();
            }
            return;
        }
        if (state_ == RoundState::victory) {
            return;
        }

        scene_.step(deltaSeconds);

        for (const CollisionEvent& event : scene_.collisions()) {
            if (!event.trigger && involves(event, ball_, leftPaddle_)) {
                bounceFromPaddle(leftPaddle_);
            }
            if (!event.trigger && involves(event, ball_, rightPaddle_)) {
                bounceFromPaddle(rightPaddle_);
            }

            if (!event.trigger) {
                continue;
            }
            if (involves(event, ball_, leftGoal_)) {
                awardPoint(false);
                break;
            }
            if (involves(event, ball_, rightGoal_)) {
                awardPoint(true);
                break;
            }
        }
    }

    void onRender(Renderer& renderer) override {
        renderer.clear(colors::navy);

        for (int y = 38; y < static_cast<int>(kArenaHeight - 36.0F); y += 26) {
            renderer.fillRect({{kArenaWidth * 0.5F - 2.0F, static_cast<float>(y)}, {4.0F, 14.0F}}, colors::slate);
        }
        renderer.drawRect({{12.0F, 12.0F}, {kArenaWidth - 24.0F, kArenaHeight - 24.0F}}, colors::slate, 2);
        scene_.render(renderer, camera_);

        renderer.drawText({44.0F, 32.0F}, "BLUE", colors::blue, 2);
        renderer.drawText({kArenaWidth - 105.0F, 32.0F}, "RED", colors::red, 2);
        renderer.drawText({kArenaWidth * 0.5F - 58.0F, 26.0F}, std::to_string(leftScore_) + "-" + std::to_string(rightScore_), colors::gold, 3);
        drawCenteredText(renderer, 64.0F, "ROUND " + std::to_string(roundNumber_), colors::white, 1);
        renderer.drawText({302.0F, kArenaHeight - 31.0F}, "W S / UP DOWN    R RESET", colors::white, 1);

        if (state_ == RoundState::countdown) {
            renderer.fillRect({{322.0F, 178.0F}, {316.0F, 184.0F}}, colors::ink);
            renderer.drawRect({{322.0F, 178.0F}, {316.0F, 184.0F}}, colors::gold, 2);
            drawCenteredText(renderer, 208.0F, "ROUND " + std::to_string(roundNumber_), colors::white, 3);
            const int count = std::max(1, static_cast<int>(std::ceil(countdownSeconds_)));
            drawCenteredText(renderer, 266.0F, std::to_string(count), colors::gold, 8);
            drawCenteredText(renderer, 340.0F, "GET READY", colors::white, 2);
        } else if (state_ == RoundState::victory) {
            const bool blueWon = winner_ == 1;
            const Color winnerColor = blueWon ? colors::blue : colors::red;
            renderer.fillRect({{230.0F, 172.0F}, {500.0F, 198.0F}}, colors::ink);
            renderer.drawRect({{230.0F, 172.0F}, {500.0F, 198.0F}}, winnerColor, 3);
            drawCenteredText(renderer, 214.0F, blueWon ? "BLUE VICTORY" : "RED VICTORY", winnerColor, 4);
            drawCenteredText(renderer, 286.0F, "FIRST TO 10", colors::gold, 2);
            drawCenteredText(renderer, 330.0F, "PRESS R TO RESET", colors::white, 1);
        }
    }

private:
    enum class RoundState {
        countdown,
        playing,
        victory
    };

    EntityId createBall() {
        Entity& ball = scene_.createEntity("ball");
        ball.transform.position = {kArenaWidth * 0.5F, kArenaHeight * 0.5F};
        ball.sprite = Sprite{{16.0F, 16.0F}, colors::gold};
        ball.collider = Collider{{8.0F, 8.0F}};
        ball.body = RigidBody{{}, 1.0F, true};
        return ball.id;
    }

    EntityId createPaddle(const std::string& name, Vec2 position, Color color) {
        Entity& paddle = scene_.createEntity(name);
        paddle.transform.position = position;
        paddle.sprite = Sprite{{18.0F, 96.0F}, color};
        paddle.collider = Collider{{9.0F, 48.0F}};
        // Paddles are kinematic: player code moves them, but ball impacts cannot push them.
        paddle.body = RigidBody{{}, 1.0F, false};
        return paddle.id;
    }

    void createWall(const std::string& name, Vec2 position, Vec2 halfExtents) {
        Entity& wall = scene_.createEntity(name);
        wall.transform.position = position;
        wall.collider = Collider{halfExtents};
        wall.body = RigidBody{{}, 1.0F, false};
    }

    EntityId createGoal(const std::string& name, Vec2 position) {
        Entity& goal = scene_.createEntity(name);
        goal.transform.position = position;
        goal.collider = Collider{{28.0F, kArenaHeight * 0.5F - 12.0F}, true};
        return goal.id;
    }

    void movePaddle(EntityId paddleId, bool movingUp, bool movingDown, float deltaSeconds) {
        Entity& paddle = scene_.entity(paddleId);
        const float direction = static_cast<float>(movingDown) - static_cast<float>(movingUp);
        paddle.transform.position.y = clamp(
            paddle.transform.position.y + direction * 390.0F * deltaSeconds,
            60.0F,
            kArenaHeight - 60.0F);
    }

    void beginCountdown(float horizontalDirection) {
        Entity& ball = scene_.entity(ball_);
        state_ = RoundState::countdown;
        nextServeDirection_ = horizontalDirection;
        countdownSeconds_ = 3.0F;
        ballSpeed_ = kBaseBallSpeed;
        ball.body->velocity = {};
        ball.enabled = false;
    }

    void launchRound() {
        Entity& ball = scene_.entity(ball_);
        state_ = RoundState::playing;
        ball.enabled = true;
        ball.transform.position = {kArenaWidth * 0.5F, kArenaHeight * 0.5F};
        ballSpeed_ = kBaseBallSpeed;
        ball.body->velocity = normalized({nextServeDirection_, serveOffset_ ? 0.42F : -0.42F}) * ballSpeed_;
        serveOffset_ = !serveOffset_;
    }

    void awardPoint(bool blueScored) {
        if (blueScored) {
            ++leftScore_;
        } else {
            ++rightScore_;
        }

        if (leftScore_ >= kWinningScore || rightScore_ >= kWinningScore) {
            state_ = RoundState::victory;
            winner_ = blueScored ? 1 : 2;
            Entity& ball = scene_.entity(ball_);
            ball.body->velocity = {};
            ball.enabled = false;
            return;
        }

        ++roundNumber_;
        beginCountdown(blueScored ? -1.0F : 1.0F);
    }

    void resetMatch() {
        leftScore_ = 0;
        rightScore_ = 0;
        roundNumber_ = 1;
        winner_ = 0;
        beginCountdown(1.0F);
    }

    void bounceFromPaddle(EntityId paddleId) {
        Entity& ball = scene_.entity(ball_);
        Entity& paddle = scene_.entity(paddleId);
        const float impact = clamp((ball.transform.position.y - paddle.transform.position.y) / 48.0F, -0.9F, 0.9F);
        const float horizontalDirection = ball.transform.position.x < paddle.transform.position.x ? -1.0F : 1.0F;
        ballSpeed_ = std::min(ballSpeed_ + kBallSpeedIncrease, kMaximumBallSpeed);
        ball.body->velocity = normalized({horizontalDirection, impact * 0.85F}) * ballSpeed_;
    }

    static void drawCenteredText(Renderer& renderer, float y, const std::string& text, Color color, int scale) {
        const float width = static_cast<float>(text.size() * 6 * std::max(scale, 1));
        renderer.drawText({kArenaWidth * 0.5F - width * 0.5F, y}, text, color, scale);
    }

    static bool involves(const CollisionEvent& event, EntityId first, EntityId second) {
        return (event.first == first && event.second == second)
            || (event.first == second && event.second == first);
    }

    Scene scene_;
    Camera2D camera_;
    EntityId ball_ = 0;
    EntityId leftPaddle_ = 0;
    EntityId rightPaddle_ = 0;
    EntityId leftGoal_ = 0;
    EntityId rightGoal_ = 0;
    int leftScore_ = 0;
    int rightScore_ = 0;
    int roundNumber_ = 1;
    int winner_ = 0;
    float ballSpeed_ = kBaseBallSpeed;
    float nextServeDirection_ = 1.0F;
    float countdownSeconds_ = 0.0F;
    bool serveOffset_ = false;
    RoundState state_ = RoundState::countdown;
};

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    Application application;
    PongGame game;
    return application.run({"Orbit2D | Pong Sample", static_cast<int>(kArenaWidth), static_cast<int>(kArenaHeight), 120}, game);
}

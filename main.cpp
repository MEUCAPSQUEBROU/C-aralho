#include "raylib.h"
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <algorithm>

constexpr int SCREEN_W = 480;
constexpr int SCREEN_H = 720;

constexpr float GRAVITY    = 1400.0f;
constexpr float FLAP_SPEED = -460.0f;
constexpr float SCROLL_SPD = 180.0f;

constexpr float GAP_HEIGHT     = 180.0f;
constexpr float BUILDING_WIDTH = 80.0f;
constexpr float BUILDING_SPACE = 240.0f;
constexpr float MIN_TOP        = 80.0f;
constexpr float MIN_BOTTOM     = 120.0f;

struct Plane {
    float x, y;
    float vy;
    float rot;
};

struct Building {
    float x;
    float gapY;
    bool scored;
};

enum class GameState { Menu, Playing, GameOver };

static float randRange(float a, float b) {
    return a + (b - a) * (float)rand() / (float)RAND_MAX;
}

static void drawPlane(const Plane& p) {
    Vector2 center = { p.x, p.y };
    float c = cosf(p.rot * DEG2RAD);
    float s = sinf(p.rot * DEG2RAD);
    auto rot = [&](float lx, float ly) {
        return Vector2{ center.x + lx * c - ly * s, center.y + lx * s + ly * c };
    };

    Vector2 nose  = rot( 26,  0);
    Vector2 tailT = rot(-22, -10);
    Vector2 tailB = rot(-22,  10);
    DrawTriangle(nose, tailB, tailT, Color{ 220, 230, 240, 255 });

    Vector2 wTL = rot(-4, -22);
    Vector2 wTR = rot( 8, -10);
    Vector2 wBR = rot( 8,  10);
    Vector2 wBL = rot(-4,  22);
    DrawTriangle(wTL, wBL, wBR, Color{ 180, 190, 205, 255 });
    DrawTriangle(wTL, wBR, wTR, Color{ 180, 190, 205, 255 });

    Vector2 fTL = rot(-22, -4);
    Vector2 fTR = rot(-14, -10);
    Vector2 fBR = rot(-14,  10);
    Vector2 fBL = rot(-22,  4);
    DrawTriangle(fTL, fBL, fBR, Color{ 200, 60, 60, 255 });
    DrawTriangle(fTL, fBR, fTR, Color{ 200, 60, 60, 255 });

    DrawCircleV(rot(10, 0), 3.0f, Color{ 120, 200, 255, 255 });
}

static void drawBuilding(const Building& b, float groundY) {
    Color body = { 25, 30, 50, 255 };
    Color edge = { 45, 55, 80, 255 };
    Color win  = { 255, 215, 120, 255 };
    Color winOff= { 50, 55, 75, 255 };

    float topH = b.gapY - GAP_HEIGHT * 0.5f;
    float botY = b.gapY + GAP_HEIGHT * 0.5f;

    DrawRectangle((int)b.x, 0, (int)BUILDING_WIDTH, (int)topH, body);
    DrawRectangleLines((int)b.x, 0, (int)BUILDING_WIDTH, (int)topH, edge);
    DrawRectangle((int)b.x, (int)botY, (int)BUILDING_WIDTH, (int)(groundY - botY), body);
    DrawRectangleLines((int)b.x, (int)botY, (int)BUILDING_WIDTH, (int)(groundY - botY), edge);

    int hashSeed = (int)(b.x * 13.37f) ^ (int)(b.gapY * 7.91f);
    auto pseudo = [&](int i) { return ((hashSeed * 1103515245 + 12345 + i * 2654435761) >> 8) & 0xFFFF; };

    const int winW = 8, winH = 10, padX = 6, padY = 8;
    int cols = (int)((BUILDING_WIDTH - padX) / (winW + padX));
    int idx = 0;

    for (int row = 0; row * (winH + padY) + padY + winH < (int)topH - 8; ++row) {
        for (int col = 0; col < cols; ++col) {
            int wx = (int)b.x + padX + col * (winW + padX);
            int wy = padY + row * (winH + padY);
            bool lit = (pseudo(idx++) % 100) < 55;
            DrawRectangle(wx, wy, winW, winH, lit ? win : winOff);
        }
    }

    int bottomH = (int)(groundY - botY);
    for (int row = 0; row * (winH + padY) + padY + winH < bottomH - 8; ++row) {
        for (int col = 0; col < cols; ++col) {
            int wx = (int)b.x + padX + col * (winW + padX);
            int wy = (int)botY + padY + row * (winH + padY);
            bool lit = (pseudo(idx++) % 100) < 55;
            DrawRectangle(wx, wy, winW, winH, lit ? win : winOff);
        }
    }

    DrawRectangle((int)b.x - 2, (int)topH - 4, (int)BUILDING_WIDTH + 4, 4, edge);
    DrawRectangle((int)b.x - 2, (int)botY,     (int)BUILDING_WIDTH + 4, 4, edge);
}

struct Star { float x, y, r; unsigned char a; };

int main() {
    srand((unsigned)time(nullptr));
    InitWindow(SCREEN_W, SCREEN_H, "Plane City - Night Flight");
    SetTargetFPS(60);

    const float groundY = SCREEN_H - 60.0f;

    std::vector<Star> stars;
    stars.reserve(80);
    for (int i = 0; i < 80; ++i) {
        stars.push_back({ randRange(0, SCREEN_W), randRange(0, groundY - 100), randRange(0.6f, 1.8f),
                          (unsigned char)(rand() % 155 + 100) });
    }

    Plane plane;
    std::vector<Building> buildings;
    int score = 0, best = 0;
    GameState state = GameState::Menu;

    auto resetGame = [&]() {
        plane = { SCREEN_W * 0.3f, SCREEN_H * 0.45f, 0.0f, 0.0f };
        buildings.clear();
        for (int i = 0; i < 4; ++i) {
            Building b;
            b.x = SCREEN_W + 100.0f + i * BUILDING_SPACE;
            b.gapY = randRange(MIN_TOP + GAP_HEIGHT * 0.5f, groundY - MIN_BOTTOM - GAP_HEIGHT * 0.5f);
            b.scored = false;
            buildings.push_back(b);
        }
        score = 0;
    };
    resetGame();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        bool flap = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) ||
                    IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

        if (state == GameState::Menu) {
            if (flap) { state = GameState::Playing; plane.vy = FLAP_SPEED; }
        } else if (state == GameState::Playing) {
            if (flap) plane.vy = FLAP_SPEED;
            plane.vy += GRAVITY * dt;
            plane.y  += plane.vy * dt;
            plane.rot = std::clamp(plane.vy * 0.08f, -25.0f, 70.0f);

            for (auto& b : buildings) {
                b.x -= SCROLL_SPD * dt;
                if (!b.scored && b.x + BUILDING_WIDTH < plane.x) {
                    b.scored = true;
                    score++;
                }
            }

            if (!buildings.empty() && buildings.front().x + BUILDING_WIDTH < -10) {
                buildings.erase(buildings.begin());
                Building nb;
                nb.x = buildings.back().x + BUILDING_SPACE;
                nb.gapY = randRange(MIN_TOP + GAP_HEIGHT * 0.5f, groundY - MIN_BOTTOM - GAP_HEIGHT * 0.5f);
                nb.scored = false;
                buildings.push_back(nb);
            }

            Rectangle pr = { plane.x - 22, plane.y - 12, 48, 24 };
            for (auto& b : buildings) {
                Rectangle top = { b.x, 0, BUILDING_WIDTH, b.gapY - GAP_HEIGHT * 0.5f };
                Rectangle bot = { b.x, b.gapY + GAP_HEIGHT * 0.5f, BUILDING_WIDTH,
                                  groundY - (b.gapY + GAP_HEIGHT * 0.5f) };
                if (CheckCollisionRecs(pr, top) || CheckCollisionRecs(pr, bot)) {
                    state = GameState::GameOver;
                }
            }
            if (plane.y + 12 >= groundY || plane.y - 12 <= 0) state = GameState::GameOver;

            if (state == GameState::GameOver && score > best) best = score;
        } else {
            if (flap) { resetGame(); state = GameState::Playing; plane.vy = FLAP_SPEED; }
        }

        BeginDrawing();
        ClearBackground(Color{ 10, 12, 30, 255 });
        DrawRectangleGradientV(0, 0, SCREEN_W, (int)groundY, Color{8,10,28,255}, Color{40,30,70,255});

        for (auto& s : stars) DrawCircle((int)s.x, (int)s.y, s.r, Color{255,255,255,s.a});
        DrawCircle(SCREEN_W - 70, 90, 32, Color{ 245, 240, 210, 255 });
        DrawCircle(SCREEN_W - 60, 82, 30, Color{ 10, 12, 30, 255 });

        for (auto& b : buildings) drawBuilding(b, groundY);

        DrawRectangle(0, (int)groundY, SCREEN_W, SCREEN_H - (int)groundY, Color{ 18, 20, 35, 255 });
        DrawRectangle(0, (int)groundY, SCREEN_W, 2, Color{ 80, 90, 120, 255 });

        drawPlane(plane);

        char buf[64];
        snprintf(buf, sizeof(buf), "%d", score);
        int fs = 60;
        int tw = MeasureText(buf, fs);
        DrawText(buf, SCREEN_W / 2 - tw / 2 + 2, 42, fs, Color{0,0,0,160});
        DrawText(buf, SCREEN_W / 2 - tw / 2,     40, fs, WHITE);

        if (state == GameState::Menu) {
            const char* t1 = "PLANE CITY";
            const char* t2 = "Aperte ESPACO para voar";
            int w1 = MeasureText(t1, 40), w2 = MeasureText(t2, 20);
            DrawText(t1, SCREEN_W/2 - w1/2, SCREEN_H/2 - 60, 40, WHITE);
            DrawText(t2, SCREEN_W/2 - w2/2, SCREEN_H/2 + 0,  20, Color{200,200,220,255});
        } else if (state == GameState::GameOver) {
            const char* t1 = "GAME OVER";
            char t2[64]; snprintf(t2, sizeof(t2), "Score: %d   Best: %d", score, best);
            const char* t3 = "Aperte ESPACO para reiniciar";
            int w1 = MeasureText(t1, 50), w2 = MeasureText(t2, 22), w3 = MeasureText(t3, 18);
            DrawRectangle(0, SCREEN_H/2 - 90, SCREEN_W, 200, Color{0,0,0,160});
            DrawText(t1, SCREEN_W/2 - w1/2, SCREEN_H/2 - 70, 50, Color{255,90,90,255});
            DrawText(t2, SCREEN_W/2 - w2/2, SCREEN_H/2 - 5,  22, WHITE);
            DrawText(t3, SCREEN_W/2 - w3/2, SCREEN_H/2 + 40, 18, Color{200,200,220,255});
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

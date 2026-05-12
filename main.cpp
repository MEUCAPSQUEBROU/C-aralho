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

static void drawPlane(const Plane& p, const Texture2D& tex) {
    float targetW = 80.0f;
    float scale = targetW / (float)tex.width;
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    Rectangle dst = { p.x, p.y, tex.width * scale, tex.height * scale };
    Vector2 origin = { dst.width * 0.5f, dst.height * 0.5f };
    DrawTexturePro(tex, src, dst, origin, p.rot, WHITE);
}

static void drawBuilding(const Building& b, float groundY) {
    Color body = { 215, 210, 200, 255 };
    Color edge = { 130, 125, 120, 255 };
    Color win  = { 140, 200, 240, 255 };
    Color winOff= { 90, 140, 190, 255 };

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

struct Cloud { float x, y, r; float speed; };

static void drawCloud(const Cloud& c) {
    Color base = { 255, 255, 255, 230 };
    Color shade = { 230, 235, 245, 230 };
    DrawCircle((int)(c.x - c.r * 0.7f), (int)(c.y + c.r * 0.2f), c.r * 0.7f, shade);
    DrawCircle((int)(c.x + c.r * 0.6f), (int)(c.y + c.r * 0.2f), c.r * 0.65f, shade);
    DrawCircle((int)c.x, (int)c.y, c.r, base);
    DrawCircle((int)(c.x - c.r * 0.5f), (int)(c.y - c.r * 0.1f), c.r * 0.75f, base);
    DrawCircle((int)(c.x + c.r * 0.5f), (int)(c.y - c.r * 0.1f), c.r * 0.7f, base);
}

struct Hill { float x, r; };
struct DistantBuilding { float x, w, h; };
struct GrassTuft { float x; unsigned char shade; };

struct Particle {
    float x, y;
    float vx, vy;
    float life;
    float maxLife;
    float size;
    Color color;
};

static void spawnExplosion(std::vector<Particle>& particles, float cx, float cy) {
    const int count = 60;
    Color palette[] = {
        { 255, 220, 100, 255 },
        { 255, 140,  40, 255 },
        { 255,  70,  30, 255 },
        { 180,  40,  20, 255 },
        { 255, 255, 200, 255 },
    };
    for (int i = 0; i < count; ++i) {
        float angle = randRange(0.0f, 2.0f * PI);
        float speed = randRange(80.0f, 360.0f);
        Particle p;
        p.x = cx;
        p.y = cy;
        p.vx = cosf(angle) * speed;
        p.vy = sinf(angle) * speed;
        p.maxLife = randRange(0.45f, 0.95f);
        p.life = p.maxLife;
        p.size = randRange(2.0f, 5.0f);
        p.color = palette[rand() % (int)(sizeof(palette) / sizeof(palette[0]))];
        particles.push_back(p);
    }
}

int main() {
    srand((unsigned)time(nullptr));
    InitWindow(SCREEN_W, SCREEN_H, "Plane City - Night Flight");
    InitAudioDevice();
    SetTargetFPS(60);

    Sound explosionSfx = LoadSound("assets/explosion.mp3");
    SetSoundVolume(explosionSfx, 0.9f);

    Texture2D planeTex = LoadTexture("assets/plane.png");
    SetTextureFilter(planeTex, TEXTURE_FILTER_BILINEAR);

    const float groundY = SCREEN_H - 60.0f;

    std::vector<Cloud> clouds;
    clouds.reserve(10);
    for (int i = 0; i < 10; ++i) {
        clouds.push_back({ randRange(0, SCREEN_W), randRange(40, groundY - 200),
                           randRange(18.0f, 34.0f), randRange(12.0f, 28.0f) });
    }

    std::vector<Hill> hills;
    hills.reserve(8);
    for (int i = 0; i < 8; ++i) {
        hills.push_back({ randRange(-100, SCREEN_W + 100), randRange(70.0f, 130.0f) });
    }

    std::vector<DistantBuilding> distantBuildings;
    distantBuildings.reserve(20);
    for (int i = 0; i < 20; ++i) {
        distantBuildings.push_back({ randRange(-50, SCREEN_W + 50),
                                     randRange(22.0f, 44.0f),
                                     randRange(40.0f, 130.0f) });
    }

    std::vector<GrassTuft> tufts;
    tufts.reserve(30);
    for (int i = 0; i < 30; ++i) {
        tufts.push_back({ randRange(0, SCREEN_W),
                          (unsigned char)(rand() % 40 + 90) });
    }

    const float HILL_SPD    = SCROLL_SPD * 0.15f;
    const float SKYLINE_SPD = SCROLL_SPD * 0.35f;
    const float TUFT_SPD    = SCROLL_SPD * 1.35f;

    Plane plane;
    std::vector<Building> buildings;
    std::vector<Particle> particles;
    bool planeExploded = false;
    float flashTimer = 0.0f;
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
        particles.clear();
        planeExploded = false;
        flashTimer = 0.0f;
        score = 0;
    };
    resetGame();

    auto triggerExplosion = [&]() {
        if (planeExploded) return;
        planeExploded = true;
        flashTimer = 0.25f;
        spawnExplosion(particles, plane.x, plane.y);
        PlaySound(explosionSfx);
    };

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
                    triggerExplosion();
                    state = GameState::GameOver;
                }
            }
            if (plane.y + 12 >= groundY || plane.y - 12 <= 0) {
                triggerExplosion();
                state = GameState::GameOver;
            }

            if (state == GameState::GameOver && score > best) best = score;
        } else {
            if (flap) { resetGame(); state = GameState::Playing; plane.vy = FLAP_SPEED; }
        }

        if (flashTimer > 0.0f) flashTimer -= dt;
        for (auto& pt : particles) {
            pt.life -= dt;
            pt.vy += 320.0f * dt;
            pt.vx *= (1.0f - 0.8f * dt);
            pt.x  += pt.vx * dt;
            pt.y  += pt.vy * dt;
        }
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                           [](const Particle& p){ return p.life <= 0.0f; }),
            particles.end());

        for (auto& c : clouds) {
            c.x -= c.speed * dt;
            if (c.x + c.r * 2.0f < 0) {
                c.x = SCREEN_W + c.r * 2.0f;
                c.y = randRange(40, groundY - 200);
                c.r = randRange(18.0f, 34.0f);
                c.speed = randRange(12.0f, 28.0f);
            }
        }
        for (auto& h : hills) {
            h.x -= HILL_SPD * dt;
            if (h.x + h.r < 0) {
                h.x = SCREEN_W + h.r;
                h.r = randRange(70.0f, 130.0f);
            }
        }
        for (auto& db : distantBuildings) {
            db.x -= SKYLINE_SPD * dt;
            if (db.x + db.w < 0) {
                db.x = SCREEN_W + db.w;
                db.w = randRange(22.0f, 44.0f);
                db.h = randRange(40.0f, 130.0f);
            }
        }
        for (auto& t : tufts) {
            t.x -= TUFT_SPD * dt;
            if (t.x < -10) {
                t.x = SCREEN_W + randRange(0, 30);
                t.shade = (unsigned char)(rand() % 40 + 90);
            }
        }

        BeginDrawing();
        ClearBackground(Color{ 135, 206, 235, 255 });
        DrawRectangleGradientV(0, 0, SCREEN_W, (int)groundY,
                               Color{ 110, 180, 230, 255 }, Color{ 200, 230, 245, 255 });

        DrawCircle(SCREEN_W - 70, 90, 38, Color{ 255, 240, 180, 255 });
        DrawCircle(SCREEN_W - 70, 90, 30, Color{ 255, 230, 130, 255 });

        for (auto& h : hills) {
            DrawCircle((int)h.x, (int)(groundY + h.r * 0.15f), h.r,
                       Color{ 150, 175, 200, 255 });
        }

        for (auto& db : distantBuildings) {
            DrawRectangle((int)db.x, (int)(groundY - db.h),
                          (int)db.w, (int)db.h,
                          Color{ 110, 130, 165, 255 });
            DrawRectangle((int)db.x, (int)(groundY - db.h),
                          (int)db.w, 3,
                          Color{ 90, 110, 145, 255 });
        }

        for (auto& c : clouds) drawCloud(c);

        for (auto& b : buildings) drawBuilding(b, groundY);

        DrawRectangle(0, (int)groundY, SCREEN_W, SCREEN_H - (int)groundY, Color{ 110, 160, 80, 255 });
        DrawRectangle(0, (int)groundY, SCREEN_W, 2, Color{ 70, 110, 50, 255 });

        for (auto& t : tufts) {
            Color gc = { 70, (unsigned char)(120 + (t.shade - 90)), 50, 255 };
            int gx = (int)t.x;
            int gy = (int)groundY + 4;
            DrawTriangle(Vector2{ (float)gx - 3, (float)gy + 8 },
                         Vector2{ (float)gx + 3, (float)gy + 8 },
                         Vector2{ (float)gx,     (float)gy },
                         gc);
        }

        if (!planeExploded) drawPlane(plane, planeTex);

        for (auto& pt : particles) {
            float t = pt.life / pt.maxLife;
            unsigned char a = (unsigned char)(std::clamp(t, 0.0f, 1.0f) * 255.0f);
            Color c = pt.color;
            c.a = a;
            DrawCircleV(Vector2{ pt.x, pt.y }, pt.size * (0.6f + 0.4f * t), c);
        }

        if (flashTimer > 0.0f) {
            float f = std::clamp(flashTimer / 0.25f, 0.0f, 1.0f);
            float radius = 90.0f * (1.0f - f) + 30.0f;
            DrawCircleV(Vector2{ plane.x, plane.y }, radius,
                        Color{ 255, 230, 160, (unsigned char)(180 * f) });
            DrawCircleV(Vector2{ plane.x, plane.y }, radius * 0.5f,
                        Color{ 255, 255, 220, (unsigned char)(220 * f) });
        }

        char buf[64];
        snprintf(buf, sizeof(buf), "%d", score);
        int fs = 60;
        int tw = MeasureText(buf, fs);
        DrawText(buf, SCREEN_W / 2 - tw / 2 + 3, 43, fs, Color{ 30, 50, 70, 200 });
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

    UnloadTexture(planeTex);
    UnloadSound(explosionSfx);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}

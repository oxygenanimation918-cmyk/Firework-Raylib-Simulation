#include "raylib.h"
#include <vector>
#include <cmath>

#define MAX_PARTICLES 200000 
#define PI_VAL 3.1415926535f

struct Particle {
    Vector2 pos, vel;
    Color color;
    float life, maxLife;
    bool active, isSpark;
};
Particle* pool = nullptr;
int poolIdx = 0;
bool showSparkles = true, showShake = true, showFlash = true, showWind = true;
bool autoMode = false;
float shakeIntensity = 0.0f;
float windForce = 0.4f;
float autoTimer = 0.0f;
Color skyFlashColor = BLACK;
Sound explosionSound;

Sound GenerateExplosionSound() {
    int sampleCount = 44100 / 2;
    short* samples = (short*)MemAlloc(sampleCount * sizeof(short));
    for (int i = 0; i < sampleCount; i++) {
        float noise = (float)GetRandomValue(-32000, 32000);
        float envelope = powf(1.0f - ((float)i / (float)sampleCount), 3.0f);
        samples[i] = (short)(noise * envelope);
    }
    Wave wave = { (unsigned int)sampleCount, 44100, 16, 1, samples };
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

void AddToPool(Vector2 pos, Vector2 vel, Color col, float life, bool spark) {
    poolIdx = (poolIdx + 1) % MAX_PARTICLES;
    pool[poolIdx].pos = pos; pool[poolIdx].vel = vel;
    pool[poolIdx].color = col; pool[poolIdx].maxLife = life;
    pool[poolIdx].life = life; pool[poolIdx].active = true;
    pool[poolIdx].isSpark = spark;
}

void LaunchFirework(Vector2 origin, int type) {
    float baseHue = (float)GetRandomValue(0, 360);
    if (showFlash) skyFlashColor = Fade(ColorFromHSV(baseHue, 0.6f, 1.0f), 0.3f);
    if (showShake) shakeIntensity = 15.0f;

    SetSoundPitch(explosionSound, 0.6f + (float)(type % 100) / 150.0f);
    PlaySound(explosionSound);

    int count = 1200;
    for (int i = 0; i < count; i++) {
        float angle = (float)i / (float)count * 2.0f * PI_VAL;
        float speed = (float)GetRandomValue(200, 750) / 100.0f;
        float vx = cosf(angle), vy = sinf(angle);

        if (type > 0) {
            float petals = (float)(type % 12 + 2);
            float modifier = 1.0f + 0.5f * sinf(angle * petals);
            vx *= modifier; vy *= modifier;
            if (type % 5 == 0) { vx *= 1.5f; vy *= 0.5f; }
            if (type % 3 == 0) speed *= (i % 2 == 0 ? 1.3f : 0.7f);
        }
        AddToPool(origin, { vx * speed, vy * speed }, ColorFromHSV(baseHue + GetRandomValue(-20, 20), 0.8f, 1.0f), (float)GetRandomValue(80, 160) / 100.0f, false);
    }
}

int main() {
    InitWindow(GetScreenWidth(), GetScreenHeight(), "Ultimate Fireworks Simulation");
    InitAudioDevice();
    SetTargetFPS(60);

    explosionSound = GenerateExplosionSound();
    pool = new Particle[MAX_PARTICLES]();
    for (int i = 0; i < MAX_PARTICLES; i++) pool[i].active = false;

    int currentMode = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (GetMouseWheelMove() != 0) currentMode = (currentMode + (int)GetMouseWheelMove() + 10000) % 10000;
        if (IsKeyPressed(KEY_SPACE)) showSparkles = !showSparkles;
        if (IsKeyPressed(KEY_S)) showShake = !showShake;
        if (IsKeyPressed(KEY_F)) showFlash = !showFlash;
        if (IsKeyPressed(KEY_W)) showWind = !showWind;
        if (IsKeyPressed(KEY_A)) autoMode = !autoMode;

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) LaunchFirework(GetMousePosition(), currentMode);

        if (autoMode) {
            autoTimer += dt;
            if (autoTimer > 0.4f) {
                Vector2 randPos = { (float)GetRandomValue(200, GetScreenWidth() - 200), (float)GetRandomValue(100, 600) };
                LaunchFirework(randPos, GetRandomValue(0, 9999));
                autoTimer = 0;
            }
        }

        if (shakeIntensity > 0) shakeIntensity -= dt * 40.0f;
        if (skyFlashColor.a > 0) {
            float fadeOut = (float)skyFlashColor.a - (dt * 600.0f);
            skyFlashColor.a = (fadeOut < 0) ? 0 : (unsigned char)fadeOut;
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!pool[i].active) continue;
            float currentWind = showWind ? windForce : 0.0f;
            pool[i].pos.x += pool[i].vel.x + (currentWind * dt * 60.0f);
            pool[i].pos.y += pool[i].vel.y;

            if (pool[i].isSpark) {
                pool[i].life -= dt * 2.0f;
                pool[i].vel.y += 0.02f;
            }
            else {
                pool[i].vel.y += 0.05f; pool[i].life -= dt * 0.5f;
                if (showSparkles && GetRandomValue(0, 100) < 12) {
                    AddToPool(pool[i].pos, { (float)GetRandomValue(-5, 5) / 20.0f, (float)GetRandomValue(5, 15) / 10.0f }, pool[i].color, 0.4f, true);
                }
            }
            if (pool[i].life <= 0) pool[i].active = false;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (showFlash || skyFlashColor.a > 0) DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), skyFlashColor);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f));
        DrawRectangleGradientV(0, GetScreenHeight() - 150, GetScreenWidth(), 150, Fade(BLACK, 0), Fade(skyFlashColor, 0.4f));

        Vector2 sOffset = { 0, 0 };
        if (showShake && shakeIntensity > 0) {
            sOffset.x = (float)GetRandomValue(-shakeIntensity, shakeIntensity);
            sOffset.y = (float)GetRandomValue(-shakeIntensity, shakeIntensity);
        }

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!pool[i].active) continue;
            if (pool[i].life < (pool[i].maxLife * 0.4f) && GetRandomValue(0, 10) > 8) continue;
            Color c = pool[i].color; c.a = (unsigned char)((pool[i].life / pool[i].maxLife) * 255);
            DrawPixelV({ pool[i].pos.x + sOffset.x, pool[i].pos.y + sOffset.y }, c);
        }
        DrawRectangle(10, 10, 480, 210, Fade(BLACK, 0.6f));
        DrawText(TextFormat("MODE ID: %d / 10,000", currentMode), 20, 20, 25, GOLD);
        DrawText(TextFormat("AUTO-SHOW: %s (A)", autoMode ? "ON" : "OFF"), 20, 55, 20, autoMode ? GREEN : RED);
        DrawText(TextFormat("WIND: %s (W) | FLASH: %s (F)", showWind ? "ON" : "OFF", showFlash ? "ON" : "OFF"), 20, 85, 18, RAYWHITE);
        DrawText(TextFormat("SPARKLES: %s (SPACE) | SHAKE: %s (S)", showSparkles ? "ON" : "OFF", showShake ? "ON" : "OFF"), 20, 115, 18, RAYWHITE);
        DrawText("Wheel: Change Type | Click: Fire", 20, 145, 16, SKYBLUE);
        DrawFPS(GetScreenWidth() - 100, 20);
        EndDrawing();
    }
    UnloadSound(explosionSound); CloseAudioDevice(); delete[] pool; CloseWindow();
    return 0;
}

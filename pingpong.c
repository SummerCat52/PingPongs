#include <GL/gl.h>
#include <GL/glu.h>
#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:WinMainCRTStartup")

//              Game constants
#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 800
#define PADDLE_HEIGHT 10
#define PADDLE_WIDTH  160
#define BALL_RADIUS   15
#define MAX_POWERUPS  5
#define MAX_PARTICLES 100
#define MAX_TRAIL     20
#define PI 3.14159265358979323846f

// Different game screens / modes
typedef enum {
    MODE_MENU,
    MODE_PVP,
    MODE_PVC,
    MODE_DIFFICULTY_SELECT,
    MODE_SPEED_SELECT
} GameMode;

// Difficulty presets
typedef enum {
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD
} DifficultyLevel;

// Ways to control paddles
typedef enum {
    CONTROL_KEYBOARD_1,     // A/D for bottom + arrows for top
    CONTROL_KEYBOARD_2,     // Arrows only
    CONTROL_MOUSE,          // Mouse movement
    CONTROL_AUTO            // Computer / AI control
} ControlMode;

// Ball types / visual styles
typedef enum {
    BALL_NORMAL,
    BALL_FIRE,
    BALL_ICE,
    BALL_MAGNETIC,
    BALL_SPLIT
} BallType;

// Power-up types that can appear
typedef enum {
    POWERUP_NONE,
    POWERUP_BIG_PADDLE,
    POWERUP_SLOW_BALL,
    POWERUP_EXTRA_POINTS,
    POWERUP_SLOW_TIME,
    POWERUP_FAST_PADDLE,
    POWERUP_INVISIBLE_BALL,
    POWERUP_SPLIT_BALL
} PowerUpType;

// Achievement entry
typedef struct {
    char name[50];
    char description[100];
    int unlocked;
    int condition;
    int progress;
} Achievement;

// Single particle (spark, explosion bit)
typedef struct {
    float x, y;
    float vx, vy;
    float life;
    float size;
    float r, g, b;
} Particle;

// One point in ball's trail effect
typedef struct {
    float x, y;
    float life;
    float size;
} TrailPoint;

// Main ball structure
typedef struct {
    float x, y;
    float vx, vy;
    float radius;
    BallType type;
    int active;
    float effectTimer;
    TrailPoint trail[MAX_TRAIL];
    int trailIndex;
} Ball;

// Floating power-up cube
typedef struct {
    float x, y;
    PowerUpType type;
    int active;
    float size;
    float rotation;
} PowerUp;

//              Global game state

static int windowWidth = WINDOW_WIDTH;
static int windowHeight = WINDOW_HEIGHT;

static float orthoLeft   = -600;
static float orthoRight  =  600;
static float orthoBottom = -400;
static float orthoTop    =  400;

static int player1_score = 0;
static int player2_score = 0;

static int paddle_height = PADDLE_HEIGHT;
static int paddle_width  = PADDLE_WIDTH;
static float paddle_velocity = 15.0f;
static float paddle_edge_offset = 20.0f;

static float player1_paddle_x = 0;      // bottom paddle
static float player2_paddle_x = 0;      // top paddle

static float player1_paddle_speed = 1.0f;
static float player2_paddle_speed = 1.0f;

static Ball balls[3];
static int activeBalls = 1;

static float ball_speed = 15.0f;
static int game_running = 0;

static GameMode currentMode = MODE_MENU;
static DifficultyLevel currentDifficulty = DIFFICULTY_MEDIUM;

static float pvp_ball_speed = 15.0f;

static int combo_multiplier = 1;
static int consecutive_hits = 0;
static float combo_timer = 0.0f;

static PowerUp powerups[MAX_POWERUPS];
static float powerup_timer = 0.0f;

static int player1_big_paddle = 0;
static int player2_big_paddle = 0;
static float powerup_duration = 0.0f;

static int needsRedraw = 1;
static float animation_time = 0.0f;

static float slow_time_factor = 1.0f;
static float slow_time_timer = 0.0f;

static int fullscreen = 0;
static RECT windowRect;
static DWORD windowStyle;
static DWORD windowExStyle;

static Particle particles[MAX_PARTICLES];

// List of unlockable achievements
static Achievement achievements[7] = {
    {"First Blood",      "Score your first point",              0, 1,  0},
    {"Combo Master",     "Get 5 hits in a row",                 0, 5,  0},
    {"Speed Demon",      "Reach ball speed 20",                 0, 20, 0},
    {"Power Collector",  "Collect 10 powerups",                 0, 10, 0},
    {"Hard Win",         "Win on Hard difficulty",              0, 1,  0},
    {"Perfect Game",     "Win without missing a ball",          0, 1,  0},
    {"Long Rally",       "Rally of 20 hits",                    0, 20, 0}
};

static int achievements_unlocked = 0;
static int powerups_collected = 0;
static float max_ball_speed = 0;
static int total_hits = 0;

static ControlMode player1_control = CONTROL_KEYBOARD_1;
static ControlMode player2_control = CONTROL_KEYBOARD_2;

// Keyboard press tracking
static int key_d_pressed = 0;
static int key_a_pressed = 0;
static int key_o_pressed = 0;
static int key_l_pressed = 0;
static int key_up_pressed    = 0;
static int key_down_pressed  = 0;
static int key_w_pressed = 0;
static int key_s_pressed = 0;
static int key_left_pressed  = 0;
static int key_right_pressed = 0;

// Smooth paddle movement targets
static float player1_target_x = 0;
static float player2_target_x = 0;
static float paddle_acceleration = 0.2f;

HWND hwnd;
HDC hdc;
HGLRC hrc;
HFONT gameFont;
HFONT largeFont;

//              Function prototypes

void initOpenGL();
void initGame();
void initBalls();
void drawText(const char* text, float x, float y, int useLargeFont);
void drawCenterLine();
void drawPaddle(float x, float y, int isBig, int player);
void drawBall(Ball* ball);
void drawPowerUp(PowerUp p);
void resetBall(Ball* ball);
void spawnPowerUp();
void updateAI(int player);
void playSound(int frequency, int duration);
void drawMenu();
void drawDifficultyMenu();
void drawSpeedMenu();
void updatePowerUps();
void checkPowerUpCollision(Ball* ball);
void updateCombo();
void redraw();
void drawCircle(float cx, float cy, float r, int segments);
void display();
void update();
void updateParticles();
void drawParticles();
void updateTrail(Ball* ball);
void drawTrail(Ball* ball);
void drawAchievements();
void checkAchievements();
void addParticle(float x, float y, float r, float g, float b);
void updateControls(float deltaTime);
void updateOrthoBounds();
void correctPaddlePositions();

//              Implementation

void updateOrthoBounds() {
    // Adapt orthographic projection to current window aspect ratio
    // We try to keep roughly 3:2 proportions, but stretch when needed
    float aspect = (float)windowWidth / (float)windowHeight;

    if (aspect > 1.5f) {
        // Window is wider than target ratio → expand horizontally
        float newWidth = 800.0f * aspect;
        orthoLeft  = -newWidth / 2.0f;
        orthoRight =  newWidth / 2.0f;
        orthoBottom = -400.0f;
        orthoTop    =  400.0f;
    } else {
        // Window is taller or closer to square → expand vertically
        float newHeight = 1200.0f / aspect;
        orthoLeft  = -600.0f;
        orthoRight =  600.0f;
        orthoBottom = -newHeight / 2.0f;
        orthoTop    =  newHeight / 2.0f;
    }
}

void correctPaddlePositions() {
    // Make sure paddles never go outside visible area after resize/fullscreen
    float leftLimit  = orthoLeft  + paddle_width / 2;
    float rightLimit = orthoRight - paddle_width / 2;

    player1_paddle_x = fmaxf(leftLimit, fminf(rightLimit, player1_paddle_x));
    player2_paddle_x = fmaxf(leftLimit, fminf(rightLimit, player2_paddle_x));

    player1_target_x = fmaxf(leftLimit, fminf(rightLimit, player1_target_x));
    player2_target_x = fmaxf(leftLimit, fminf(rightLimit, player2_target_x));
}

// Basic Windows beep sound with sanity checks
void playSound(int frequency, int duration) {
    if (frequency < 37  || frequency > 32767) frequency = 1000;
    if (duration   < 1   || duration   > 5000)  duration   = 100;
    Beep(frequency, duration);
}

// Initialize OpenGL context, fonts, blending, initial state
void initOpenGL() {
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 32,
        0,0,0,0,0,0,0,0, 0,0,0,0,0, 24, 0, 0,
        PFD_MAIN_PLANE, 0, 0,0,0
    };

    hdc = GetDC(hwnd);
    int fmt = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, fmt, &pfd);
    hrc = wglCreateContext(hdc);
    wglMakeCurrent(hdc, hrc);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glShadeModel(GL_SMOOTH);
    srand((unsigned)time(NULL));

    // Create two font sizes for UI
    gameFont = CreateFontA(24,0,0,0, FW_NORMAL, FALSE,FALSE,FALSE,
                           DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                           CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                           DEFAULT_PITCH|FF_DONTCARE, "Arial");

    largeFont = CreateFontA(32,0,0,0, FW_BOLD, FALSE,FALSE,FALSE,
                            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                            CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                            DEFAULT_PITCH|FF_DONTCARE, "Arial");

    // Clear powerups & particles
    for (int i = 0; i < MAX_POWERUPS;  i++) powerups[i].active = 0;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].life = 0.0f;

    initBalls();

    needsRedraw = 1;

    if (hwnd) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    updateOrthoBounds();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1, 1);
    glMatrixMode(GL_MODELVIEW);

    correctPaddlePositions();
}

// Reset ball array — only first one active initially
void initBalls() {
    for (int i = 0; i < 3; i++) {
        balls[i].active     = (i == 0);
        balls[i].type       = BALL_NORMAL;
        balls[i].trailIndex = 0;
        for (int j = 0; j < MAX_TRAIL; j++)
            balls[i].trail[j].life = 0.0f;
    }
    activeBalls = 1;
}

// Reset scores, paddles, timers, spawn initial ball
void initGame() {
    player1_score = player2_score = 0;
    player1_paddle_x = player2_paddle_x = 0;
    player1_target_x = player2_target_x = 0;
    player1_paddle_speed = player2_paddle_speed = 1.0f;

    ball_speed = (currentMode == MODE_PVP)
        ? pvp_ball_speed
        : (currentDifficulty == DIFFICULTY_MEDIUM ? 16.0f : 18.0f);

    paddle_height = PADDLE_HEIGHT;
    paddle_width  = PADDLE_WIDTH;

    combo_multiplier = 1;
    consecutive_hits = 0;
    combo_timer = 0.0f;
    powerup_duration = 0.0f;
    player1_big_paddle = player2_big_paddle = 0;
    slow_time_factor = 1.0f;
    slow_time_timer = 0.0f;
    total_hits = 0;

    for (int i = 0; i < MAX_POWERUPS; i++)
        powerups[i].active = 0;

    initBalls();

    for (int i = 0; i < 3; i++)
        if (balls[i].active)
            resetBall(&balls[i]);
}

// Draw filled circle (used for balls, glows, effects)
void drawCircle(float cx, float cy, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float a = 2.0f * PI * i / segments;
        glVertex2f(cx + cosf(a) * r, cy + sinf(a) * r);
    }
    glEnd();
}

// Draw 2D text using Windows GDI (switches to pixel coordinates)
void drawText(const char* text, float x, float y, int useLarge) {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    // Convert game coords to screen pixels
    int sx = (int)((x - orthoLeft)   * windowWidth  / (orthoRight - orthoLeft));
    int sy = windowHeight - (int)((y - orthoBottom) * windowHeight / (orthoTop - orthoBottom)) - 24;

    wglMakeCurrent(NULL, NULL);

    HFONT font = useLarge ? largeFont : gameFont;
    HGDIOBJ old = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,255));

    TextOutA(hdc, sx, sy, text, (int)strlen(text));

    SelectObject(hdc, old);
    wglMakeCurrent(hdc, hrc);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Spawn one particle with random direction
void addParticle(float x, float y, float r, float g, float b) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life <= 0.0f) {
            particles[i].x = x; particles[i].y = y;
            particles[i].vx = (rand() % 100 - 50) / 50.0f;
            particles[i].vy = (rand() % 100 - 50) / 50.0f;
            particles[i].life = 1.0f;
            particles[i].size = (rand() % 5 + 2);
            particles[i].r = r; particles[i].g = g; particles[i].b = b;
            break;
        }
    }
}

void updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0.0f) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy -= 0.05f;           // light gravity
            particles[i].life -= 0.02f;
            particles[i].size *= 0.98f;
        }
    }
}

void drawParticles() {
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0.0f) {
            float a = particles[i].life;
            glColor4f(particles[i].r, particles[i].g, particles[i].b, a);
            glVertex2f(particles[i].x, particles[i].y);
        }
    }
    glEnd();
}

// Add current position to ball's trail (circular buffer)
void updateTrail(Ball* ball) {
    ball->trailIndex = (ball->trailIndex + 1) % MAX_TRAIL;
    TrailPoint* p = &ball->trail[ball->trailIndex];
    p->x = ball->x;
    p->y = ball->y;
    p->life = 1.0f;
    p->size = ball->radius;

    for (int i = 0; i < MAX_TRAIL; i++)
        if (ball->trail[i].life > 0.0f) {
            ball->trail[i].life -= 0.1f;
            ball->trail[i].size *= 0.95f;
        }
}

// Draw trail — currently only for fire ball
void drawTrail(Ball* ball) {
    if (ball->type != BALL_FIRE) return;

    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < MAX_TRAIL; i++) {
        int idx = (ball->trailIndex + i) % MAX_TRAIL;
        if (ball->trail[idx].life > 0.0f) {
            float a = ball->trail[idx].life * 0.5f;
            float s = ball->trail[idx].size;
            glColor4f(1.0f, 0.5f, 0.0f, a);
            glVertex2f(ball->trail[idx].x - s, ball->trail[idx].y);
            glColor4f(1.0f, 0.0f, 0.0f, a * 0.5f);
            glVertex2f(ball->trail[idx].x + s, ball->trail[idx].y);
        }
    }
    glEnd();
}

// Pulsing dotted vertical center line
void drawCenterLine() {
    glColor3f(0.5f, 0.5f, 0.5f);
    glLineWidth(2.0f);
    glBegin(GL_LINES);

    for (int x = -580; x <= 580; x += 40) {
        float alpha = (sinf(animation_time + x * 0.1f) + 1.0f) * 0.5f;
        glColor3f(alpha, alpha, alpha);
        glVertex2f(x, -5);
        glVertex2f(x + 20, 5);
    }
    glEnd();
    glLineWidth(1.0f);
}

// Draw horizontal paddle stuck to top or bottom edge
void drawPaddle(float x, float y_unused, int isBig, int player) {
    int w = isBig ? (int)(paddle_width * 1.5f) : paddle_width;
    int h = paddle_height;

    // Clamp position to visible area
    float minX = orthoLeft  + w/2;
    float maxX = orthoRight - w/2;
    if (x < minX) x = minX;
    if (x > maxX) x = maxX;

    float py = (player == 1) ? orthoBottom + h : orthoTop - h;

    if (player == 1) { // bottom — blue theme
        glBegin(GL_QUADS);
        glColor3f(0.2f, 0.4f, 1.0f);
        glVertex2f(x - w/2, py - h);
        glVertex2f(x + w/2, py - h);
        glColor3f(0.1f, 0.2f, 0.8f);
        glVertex2f(x + w/2, py + h);
        glVertex2f(x - w/2, py + h);
        glEnd();
    } else { // top — red theme
        glBegin(GL_QUADS);
        glColor3f(1.0f, 0.4f, 0.2f);
        glVertex2f(x - w/2, py - h);
        glVertex2f(x + w/2, py - h);
        glColor3f(0.8f, 0.2f, 0.1f);
        glVertex2f(x + w/2, py + h);
        glVertex2f(x - w/2, py + h);
        glEnd();
    }

    // Glow when enlarged
    if (isBig) {
        glEnable(GL_BLEND);
        glColor4f(player==1 ? 0.2f:1.0f, player==1 ? 0.4f:0.2f, player==1 ? 1.0f:0.1f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(x - w/2 - 3, py - h - 3);
        glVertex2f(x + w/2 + 3, py - h - 3);
        glVertex2f(x + w/2 + 3, py + h + 3);
        glVertex2f(x - w/2 - 3, py + h + 3);
        glEnd();
        glDisable(GL_BLEND);
    }
}

// Draw animated ball depending on its current type
void drawBall(Ball* ball) {
    if (!ball->active) return;

    glPushMatrix();
    glTranslatef(ball->x, ball->y, 0);
    glRotatef(animation_time * 50.0f, 0,0,1);

    switch (ball->type) {
        case BALL_NORMAL:
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(1.0f, 0.5f, 0.0f);
            glVertex2f(0,0);
            for (int i = 0; i <= 360; i += 15) {
                float a = i * PI / 180.0f;
                float r = ball->radius * (0.9f + sinf(animation_time*3.0f + a)*0.1f);
                glColor3f(1.0f, 0.6f - i/720.0f, 0.2f - i/1440.0f);
                glVertex2f(cosf(a)*r, sinf(a)*r);
            }
            glEnd();
            break;

        case BALL_FIRE:
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(1.0f, 0.8f, 0.0f);
            glVertex2f(0,0);
            for (int i = 0; i <= 360; i += 15) {
                float a = i * PI / 180.0f;
                float r = ball->radius * (0.8f + sinf(animation_time*5.0f + a)*0.2f);
                glColor3f(1.0f, 0.3f + 0.5f*sinf(animation_time*2.0f + a), 0.0f);
                glVertex2f(cosf(a)*r, sinf(a)*r);
            }
            glEnd();
            break;

        case BALL_ICE:
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.6f, 0.8f, 1.0f);
            glVertex2f(0,0);
            for (int i = 0; i <= 360; i += 15) {
                float a = i * PI / 180.0f;
                float r = ball->radius * (0.9f + sinf(animation_time*2.0f + a)*0.1f);
                glColor3f(0.4f + 0.2f*sinf(animation_time + a),
                          0.6f + 0.2f*sinf(animation_time*1.5f + a),
                          1.0f);
                glVertex2f(cosf(a)*r, sinf(a)*r);
            }
            glEnd();
            break;

        case BALL_MAGNETIC:
            glBegin(GL_TRIANGLE_FAN);
            glColor3f(0.8f, 0.0f, 0.8f);
            glVertex2f(0,0);
            for (int i = 0; i <= 360; i += 15) {
                float a = i * PI / 180.0f;
                float r = ball->radius * (0.85f + sinf(animation_time*4.0f + a)*0.15f);
                glColor3f(0.6f + 0.2f*sinf(animation_time*3.0f + a), 0.0f,
                          0.6f + 0.2f*sinf(animation_time*2.0f + a));
                glVertex2f(cosf(a)*r, sinf(a)*r);
            }
            glEnd();
            break;
    }

    // Small white highlight
    glColor4f(1.0f, 1.0f, 1.0f, 0.6f);
    drawCircle(ball->radius * 0.3f, ball->radius * 0.3f, ball->radius * 0.2f, 16);

    // Slow-motion ring effect
    if (ball->type == BALL_NORMAL && slow_time_timer > 0) {
        glColor4f(1.0f, 1.0f, 1.0f, 0.3f);
        drawCircle(0, 0, ball->radius * 1.5f, 32);
    }

    glPopMatrix();
}

// Draw rotating glowing power-up cube
void drawPowerUp(PowerUp p) {
    if (!p.active) return;

    glPushMatrix();
    glTranslatef(p.x, p.y, 0);
    glRotatef(p.rotation, 0,0,1);

    switch (p.type) {
        case POWERUP_BIG_PADDLE:     glColor3f(0.0f,1.0f,0.0f); break;
        case POWERUP_SLOW_BALL:      glColor3f(0.0f,0.5f,1.0f); break;
        case POWERUP_EXTRA_POINTS:   glColor3f(1.0f,1.0f,0.0f); break;
        case POWERUP_SLOW_TIME:      glColor3f(1.0f,0.0f,1.0f); break;
        case POWERUP_FAST_PADDLE:    glColor3f(0.5f,0.5f,1.0f); break;
        case POWERUP_INVISIBLE_BALL: glColor3f(0.8f,0.8f,0.8f); break;
        case POWERUP_SPLIT_BALL:     glColor3f(1.0f,0.5f,0.0f); break;
        default:                     glColor3f(0.7f,0.7f,0.7f);
    }

    float s = p.size + sinf(animation_time * 3.0f) * 0.2f;
    glScalef(s, s, s);

    glBegin(GL_QUADS);
    glVertex2f(-10,-10); glVertex2f(10,-10);
    glVertex2f(10,10);   glVertex2f(-10,10);
    glEnd();

    glColor3f(1,1,1);
    glLineWidth(2.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(-10,-10); glVertex2f(10,-10);
    glVertex2f(10,10);   glVertex2f(-10,10);
    glEnd();
    glLineWidth(1.0f);

    glPopMatrix();
}

// Place ball back in center with random angle
void resetBall(Ball* ball) {
    ball->x = (float)((rand() % 200) - 100);
    ball->y = 0;

    float angle = (float)((rand() % 60 - 30) * PI / 180.0f);
    float speed = ball_speed;

    ball->vy = speed * cosf(angle) * ((rand() % 2) ? 1.0f : -1.0f);
    ball->vx = speed * sinf(angle);

    ball->radius = BALL_RADIUS;
    ball->type = BALL_NORMAL;
    ball->effectTimer = 0.0f;

    for (int i = 0; i < MAX_TRAIL; i++)
        ball->trail[i].life = 0.0f;
}

// Try to spawn one new random power-up
void spawnPowerUp() {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) {
            powerups[i].x = (float)((rand() % 800) - 400);
            powerups[i].y = (float)((rand() % 500) - 250);
            powerups[i].type = (rand() % 7) + 1;
            powerups[i].active = 1;
            powerups[i].size = 1.0f;
            powerups[i].rotation = 0.0f;
            break;
        }
    }
}

// AI paddle control logic
void updateAI(int player) {
    // Skip if this paddle isn't AI-controlled
    if (currentMode == MODE_PVP) {
        if (player == 1 && player1_control != CONTROL_AUTO) return;
        if (player == 2 && player2_control != CONTROL_AUTO) return;
    } else if (currentMode == MODE_PVC) {
        if (player != 2) return;
    } else return;

    // Difficulty tuning values
    float reaction = 0.95f, accuracy = 0.85f, errChance = 0.3f, maxErr = 50.0f;
    float speedMult = 1.2f, anticipate = 0.6f, adapt = 0.15f;

    if (currentDifficulty == DIFFICULTY_HARD) {
        reaction = 1.1f; accuracy = 0.95f; errChance = 0.0f; maxErr = 0.0f;
        speedMult = 1.6f; anticipate = 0.8f; adapt = 0.2f;
    }

    // Learn from long rallies
    if (consecutive_hits > 5) {
        accuracy = fminf(1.0f, accuracy + adapt * 0.1f);
        reaction = fminf(1.2f, reaction + adapt * 0.05f);
    }
    if (total_hits > 50) {
        float learn = fminf(0.3f, total_hits * 0.005f);
        accuracy = fminf(1.0f, accuracy + learn * 0.05f);
        reaction = fminf(1.3f, reaction + learn * 0.02f);
    }

    // Find closest ball heading towards this paddle
    Ball* target = NULL;
    float minTime = 9999.0f;
    float ballSpd = 0;

    for (int i = 0; i < 3; i++) {
        if (!balls[i].active) continue;
        Ball* b = &balls[i];

        int incoming = 0;
        float t = 0;

        if (player == 1) { // bottom paddle
            if (b->vy < 0 && b->y > orthoBottom + paddle_height) {
                t = (b->y - (orthoBottom + paddle_height)) / -b->vy;
                incoming = 1;
            }
        } else { // top paddle
            if (b->vy > 0 && b->y < orthoTop - paddle_height) {
                t = ((orthoTop - paddle_height) - b->y) / b->vy;
                incoming = 1;
            }
        }

        if (incoming && t < minTime) {
            minTime = t;
            target = b;
            ballSpd = sqrtf(b->vx*b->vx + b->vy*b->vy);
        }
    }

    float* targetX = (player == 1) ? &player1_target_x : &player2_target_x;

    if (!target) {
        // No threat → slowly go back to center
        *targetX += (0 - *targetX) * 0.05f * reaction;
    } else {
        // Basic prediction
        float predict = target->x + target->vx * minTime * accuracy;

        // Wall bounce prediction (better on hard)
        if (fabsf(target->vx) > 0.1f) {
            float tLeft = minTime;
            float cx = target->x, cvx = target->vx;
            while (tLeft > 0) {
                float tWall = (cvx > 0)
                    ? (orthoRight - target->radius - cx) / cvx
                    : (orthoLeft  + target->radius - cx) / cvx;

                if (tWall > 0 && tWall <= tLeft) {
                    cx += cvx * tWall;
                    cvx = -cvx * 0.98f;
                    tLeft -= tWall;
                } else {
                    cx += cvx * tLeft;
                    break;
                }
            }
            predict = (currentDifficulty == DIFFICULTY_MEDIUM)
                ? predict * 0.7f + cx * 0.3f
                : predict * 0.4f + cx * 0.6f;
        }

        // Add human-like mistake on medium
        if (currentDifficulty != DIFFICULTY_HARD && (rand() % 100 < errChance * 100)) {
            float err = ((rand() % (int)maxErr*2) - maxErr) * (1.0f + minTime*0.5f);
            predict += err;
        }

        // Move towards predicted spot
        float dist = predict - *targetX;
        float step = paddle_velocity * reaction * speedMult * 0.05f;

        if (fabsf(dist) > 20) step *= 1.5f;

        // Snap instantly on very close balls in hard mode
        if (currentDifficulty == DIFFICULTY_HARD && fabsf(dist) < 50 && minTime < 0.3f)
            *targetX = predict;
        else if (fabsf(dist) > 2.0f)
            *targetX += (dist > 0 ? step : -step);
    }

    // Clamp target position
    float l = orthoLeft  + paddle_width/2;
    float r = orthoRight - paddle_width/2;
    if (*targetX < l) *targetX = l;
    if (*targetX > r) *targetX = r;
}

// Check if any achievement should be unlocked now
void checkAchievements() {
    if (!achievements[0].unlocked && (player1_score > 0 || player2_score > 0)) {
        achievements[0].unlocked = 1; achievements_unlocked++; playSound(800,300);
    }
    if (!achievements[1].unlocked && consecutive_hits >= 5) {
        achievements[1].unlocked = 1; achievements_unlocked++; playSound(1000,300);
    }
    if (!achievements[2].unlocked && max_ball_speed >= 20.0f) {
        achievements[2].unlocked = 1; achievements_unlocked++; playSound(1200,300);
    }
    if (!achievements[3].unlocked && powerups_collected >= 10) {
        achievements[3].unlocked = 1; achievements_unlocked++; playSound(700,300);
    }
    if (!achievements[4].unlocked && currentDifficulty == DIFFICULTY_HARD &&
        currentMode == MODE_PVC && player1_score >= 5) {
        achievements[4].unlocked = 1; achievements_unlocked++; playSound(2000,500);
    }
    if (!achievements[6].unlocked && consecutive_hits >= 20) {
        achievements[6].unlocked = 1; achievements_unlocked++; playSound(1600,400);
    }
}

void drawAchievements() {
    if (achievements_unlocked == 0) return;

    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    glOrtho(0, windowWidth, 0, windowHeight, -1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    wglMakeCurrent(NULL, NULL);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255,255,0));

    char buf[100];
    sprintf(buf, "ACHIEVEMENTS: %d/7", achievements_unlocked);
    TextOutA(hdc, 10, windowHeight - 30, buf, (int)strlen(buf));

    wglMakeCurrent(hdc, hrc);

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// Check if ball touched any active power-up
void checkPowerUpCollision(Ball* ball) {
    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (!powerups[i].active) continue;

        float dx = ball->x - powerups[i].x;
        float dy = ball->y - powerups[i].y;
        float dist = sqrtf(dx*dx + dy*dy);

        if (dist < ball->radius + 15) {
            powerups_collected++;

            switch (powerups[i].type) {
                case POWERUP_BIG_PADDLE:
                    if (ball->vy > 0) player1_big_paddle = 1;
                    else              player2_big_paddle = 1;
                    powerup_duration = 10.0f;
                    playSound(800,200);
                    break;

                case POWERUP_SLOW_BALL:
                    ball_speed *= 0.7f;
                    for (int j = 0; j < 3; j++)
                        if (balls[j].active) {
                            balls[j].vx *= 0.7f;
                            balls[j].vy *= 0.7f;
                        }
                    playSound(600,200);
                    break;

                case POWERUP_EXTRA_POINTS:
                    if (ball->vy > 0) player1_score += 2;
                    else              player2_score += 2;
                    combo_multiplier = 2; combo_timer = 5.0f;
                    playSound(1000,200);
                    break;

                case POWERUP_SLOW_TIME:
                    slow_time_factor = 0.5f;
                    slow_time_timer = 5.0f;
                    playSound(700,200);
                    break;

                case POWERUP_FAST_PADDLE:
                    if (ball->vy > 0) player1_paddle_speed = 1.5f;
                    else              player2_paddle_speed = 1.5f;
                    playSound(900,200);
                    break;

                case POWERUP_INVISIBLE_BALL:
                    ball->type = BALL_NORMAL;
                    ball->effectTimer = 5.0f;
                    playSound(500,200);
                    break;

                case POWERUP_SPLIT_BALL:
                    for (int j = 0; j < 3; j++) {
                        if (!balls[j].active && activeBalls < 3) {
                            balls[j].active = 1;
                            balls[j].x = ball->x;
                            balls[j].y = ball->y;
                            balls[j].vx = -ball->vx;
                            balls[j].vy = -ball->vy;
                            balls[j].radius = ball->radius;
                            balls[j].type = ball->type;
                            activeBalls++;
                            break;
                        }
                    }
                    playSound(1200,200);
                    break;
            }

            powerups[i].active = 0;
            needsRedraw = 1;
            checkAchievements();
        }
    }
}

// Manage power-up timers and spawning
void updatePowerUps() {
    powerup_timer += 0.016f;
    if (powerup_timer >= 8.0f) {
        spawnPowerUp();
        powerup_timer = 0.0f;
    }

    for (int i = 0; i < MAX_POWERUPS; i++) {
        if (powerups[i].active) {
            powerups[i].size = 0.8f + sinf(animation_time * 3.0f + i) * 0.2f;
            powerups[i].rotation += 1.0f;
        }
    }

    if (powerup_duration > 0) {
        powerup_duration -= 0.016f;
        if (powerup_duration <= 0)
            player1_big_paddle = player2_big_paddle = 0;
    }

    if (slow_time_timer > 0) {
        slow_time_timer -= 0.016f;
        if (slow_time_timer <= 0)
            slow_time_factor = 1.0f;
    }
}

void updateCombo() {
    if (combo_timer > 0) {
        combo_timer -= 0.016f;
        if (combo_timer <= 0) {
            combo_multiplier = 1;
            consecutive_hits = 0;
            needsRedraw = 1;
        }
    }
}

void redraw() {
    InvalidateRect(hwnd, NULL, FALSE);
}

//              Menu screens

void drawDifficultyMenu() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // Dark background
    glBegin(GL_QUADS);
    glColor3f(0.1f,0.1f,0.2f); glVertex2f(orthoLeft, orthoTop);
    glVertex2f(orthoRight, orthoTop);
    glColor3f(0.05f,0.05f,0.15f);
    glVertex2f(orthoRight, orthoBottom);
    glVertex2f(orthoLeft, orthoBottom);
    glEnd();

    // Random twinkling dots
    glPointSize(2.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < 50; i++) {
        float rx = rand() / (float)RAND_MAX;
        float ry = rand() / (float)RAND_MAX;
        float br = 0.5f + 0.5f * sinf(animation_time * 2.0f + i);
        glColor3f(br, br, br);
        glVertex2f(orthoLeft + (orthoRight-orthoLeft)*rx,
                   orthoBottom + (orthoTop-orthoBottom)*ry);
    }
    glEnd();

    SwapBuffers(hdc);

    drawText("SELECT DIFFICULTY", -180, 300, 1);

    const char* opts[] = {"1 - MEDIUM", "2 - HARD"};
    const char* desc[] = {"Challenging AI", "Extreme challenge!"};
    float ys[] = {200, 150};

    for (int i = 0; i < 2; i++) {
        if (currentDifficulty == i) {
            char buf[100]; sprintf(buf, "> %s <", opts[i]);
            drawText(buf, -100, ys[i], 0);
        } else {
            drawText(opts[i], -80, ys[i], 0);
        }
        drawText(desc[i], -150, ys[i]-30, 0);
    }

    drawText("PRESS ENTER TO START", -150, -100, 0);
    drawText("PRESS ESC TO GO BACK",  -150, -150, 0);

    float spd = (currentDifficulty == DIFFICULTY_MEDIUM) ? 16.0f : 18.0f;
    char buf[50]; sprintf(buf, "BALL SPEED: %.1f", spd);
    drawText(buf, -120, -200, 0);
}

void drawSpeedMenu() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // Reddish dark bg
    glBegin(GL_QUADS);
    glColor3f(0.2f,0.1f,0.1f); glVertex2f(orthoLeft, orthoTop);
    glVertex2f(orthoRight, orthoTop);
    glColor3f(0.1f,0.05f,0.05f);
    glVertex2f(orthoRight, orthoBottom);
    glVertex2f(orthoLeft, orthoBottom);
    glEnd();

    SwapBuffers(hdc);

    drawText("SELECT BALL SPEED FOR PvP", -220, 300, 1);

    char buf[100];
    sprintf(buf, "CURRENT SPEED: %.1f", pvp_ball_speed);
    drawText(buf, -120, 200, 1);

    drawText("UP ARROW - INCREASE SPEED",   -180, 100, 0);
    drawText("DOWN ARROW - DECREASE SPEED", -180, 70,  0);

    float ratio = (pvp_ball_speed - 6.0f) / (25.0f - 6.0f);
    float barW = 400.0f * ratio;

    SwapBuffers(hdc);

    // Bar background
    glColor3f(0.3f,0.3f,0.3f);
    glBegin(GL_QUADS);
    glVertex2f(-200,150); glVertex2f(200,150);
    glVertex2f(200,170);  glVertex2f(-200,170);
    glEnd();

    // Gradient bar
    glBegin(GL_QUADS);
    glColor3f(1,0,0);    glVertex2f(-200,150);
    glColor3f(0,1,0);    glVertex2f(-200+barW,150);
    glColor3f(0,0.5f,0); glVertex2f(-200+barW,170);
    glColor3f(0.5f,0,0); glVertex2f(-200,170);
    glEnd();

    SwapBuffers(hdc);

    drawText("ENTER - START GAME",    -120, 0,   0);
    drawText("ESC - BACK TO MENU",    -120, -30, 0);
    drawText("SLOW", -220, 180, 0);
    drawText("FAST", 180,  180, 0);

    // Sparks for high speed
    if (pvp_ball_speed > 15.0f) {
        glColor4f(1,0,0,0.3f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 20; i++) {
            float x = -500 + fmod(animation_time*100 + i*20, 1000);
            float y = (rand()%400) - 200;
            glVertex2f(x,y);
        }
        glEnd();
        SwapBuffers(hdc);
    }
}

// Handle input → update paddle target positions smoothly
void updateControls(float dt) {
    float base = paddle_velocity * 1.5f;

    // Bottom player controls
    switch (player1_control) {
        case CONTROL_KEYBOARD_1:
            if (key_d_pressed)     player1_target_x += base * player1_paddle_speed * dt * 40;
            if (key_a_pressed)     player1_target_x -= base * player1_paddle_speed * dt * 40;
            break;
        case CONTROL_KEYBOARD_2:
            if (key_right_pressed) player1_target_x += base * player1_paddle_speed * dt * 40;
            if (key_left_pressed)  player1_target_x -= base * player1_paddle_speed * dt * 40;
            break;
        case CONTROL_AUTO:
            updateAI(1);
            break;
        case CONTROL_MOUSE:
            break;
    }

    // Top player controls (PvP only)
    if (currentMode == MODE_PVP) {
        switch (player2_control) {
            case CONTROL_KEYBOARD_1:
                if (key_right_pressed) player2_target_x += base * player2_paddle_speed * dt * 40;
                if (key_left_pressed)  player2_target_x -= base * player2_paddle_speed * dt * 40;
                break;
            case CONTROL_KEYBOARD_2:
                if (key_right_pressed) player2_target_x += base * player2_paddle_speed * dt * 40;
                if (key_left_pressed)  player2_target_x -= base * player2_paddle_speed * dt * 40;
                break;
            case CONTROL_AUTO:
                updateAI(2);
                break;
            case CONTROL_MOUSE:
                break;
        }
    } else if (currentMode == MODE_PVC) {
        updateAI(2);
    }

    // Smooth interpolation
    player1_paddle_x += (player1_target_x - player1_paddle_x) * paddle_acceleration;
    player2_paddle_x += (player2_target_x - player2_paddle_x) * paddle_acceleration;

    // Clamp everything
    float ml = orthoLeft  + paddle_width/2;
    float mr = orthoRight - paddle_width/2;

    player1_target_x = fmaxf(ml, fminf(mr, player1_target_x));
    player2_target_x = fmaxf(ml, fminf(mr, player2_target_x));
    player1_paddle_x = fmaxf(ml, fminf(mr, player1_paddle_x));
    player2_paddle_x = fmaxf(ml, fminf(mr, player2_paddle_x));
}

// Main rendering when in gameplay mode
void display() {
    if (currentMode == MODE_MENU)           { drawMenu(); return; }
    if (currentMode == MODE_DIFFICULTY_SELECT) { drawDifficultyMenu(); return; }
    if (currentMode == MODE_SPEED_SELECT)   { drawSpeedMenu(); return; }

    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    drawCenterLine();
    drawPaddle(player1_paddle_x, 0, player1_big_paddle, 1);
    drawPaddle(player2_paddle_x, 0, player2_big_paddle, 2);

    for (int i = 0; i < 3; i++)
        if (balls[i].active) {
            drawTrail(&balls[i]);
            drawBall(&balls[i]);
        }

    for (int i = 0; i < MAX_POWERUPS; i++)
        if (powerups[i].active)
            drawPowerUp(powerups[i]);

    drawParticles();

    SwapBuffers(hdc);

    char s1[50], s2[50];
    sprintf(s1, "PLAYER 1: %d", player1_score);
    sprintf(s2, "PLAYER 2: %d", player2_score);
    drawText(s1, -550, orthoBottom + 30, 0);
    drawText(s2, -550, orthoTop - 30, 0);
}

// Fancy animated main menu
void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    float t = animation_time;

    glBegin(GL_QUADS);
    glColor3f(0.1f + 0.05f*sinf(t*0.5f), 0.1f + 0.05f*sinf(t*0.7f+1), 0.2f + 0.05f*sinf(t*0.3f+2));
    glVertex2f(orthoLeft, orthoTop);

    glColor3f(0.15f + 0.05f*sinf(t*0.6f), 0.15f + 0.05f*sinf(t*0.8f+0.5f), 0.25f + 0.05f*sinf(t*0.4f+1.5f));
    glVertex2f(orthoRight, orthoTop);

    glColor3f(0.05f + 0.05f*sinf(t*0.4f), 0.05f + 0.05f*sinf(t*0.6f+2), 0.15f + 0.05f*sinf(t*0.2f+3));
    glVertex2f(orthoRight, orthoBottom);

    glColor3f(0.0f + 0.05f*sinf(t*0.3f), 0.0f + 0.05f*sinf(t*0.5f+1.5f), 0.1f + 0.05f*sinf(t*0.1f+2.5f));
    glVertex2f(orthoLeft, orthoBottom);
    glEnd();

    SwapBuffers(hdc);

    drawText("PING PONG", -80, 300, 1);

    const char* items[] = {
        "1 - PLAYER VS PLAYER",
        "2 - PLAYER VS COMPUTER",
        "3 - SELECT BALL SPEED (PvP)"
    };
    float ys[] = {150, 100, 50};

    for (int i = 0; i < 3; i++)
        drawText(items[i], -180, ys[i], 0);

    drawText("PRESS SPACE TO START SELECTED GAME", -250, -200, 0);
    drawText("ESC - EXIT", -180, -230, 0);

    char buf[100];
    sprintf(buf, "ACHIEVEMENTS: %d/7   MAX SPEED: %.1f", achievements_unlocked, max_ball_speed);
    drawText(buf, -200, 380, 0);
}

// Main game loop logic — physics, collisions, scoring
void update() {
    if (!game_running) return;

    needsRedraw = 1;
    float dt = 0.016f * slow_time_factor;

    updateControls(dt);
    updatePowerUps();
    updateCombo();
    updateParticles();

    animation_time += dt;

    for (int i = 0; i < 3; i++) {
        if (!balls[i].active) continue;
        Ball* b = &balls[i];

        if (b->effectTimer > 0) b->effectTimer -= dt;

        b->x += b->vx;
        b->y += b->vy;

        updateTrail(b);

        float spd = sqrtf(b->vx*b->vx + b->vy*b->vy);
        if (spd > max_ball_speed) {
            max_ball_speed = spd;
            checkAchievements();
        }

        // Left/right wall bounce
        if (b->x + b->radius > orthoRight) {
            b->x = orthoRight - b->radius;
            b->vx = -b->vx;
            addParticle(b->x, b->y, 1,1,1);
            playSound(300,50);
        }
        if (b->x - b->radius < orthoLeft) {
            b->x = orthoLeft + b->radius;
            b->vx = -b->vx;
            addParticle(b->x, b->y, 1,1,1);
            playSound(300,50);
        }

        checkPowerUpCollision(b);

        float p1y = orthoBottom + paddle_height;
        float p2y = orthoTop    - paddle_height;

        // Bottom paddle hit
        int w = player1_big_paddle ? (int)(paddle_width*1.5f) : paddle_width;
        if (b->y - b->radius < p1y + paddle_height && b->vy < 0) {
            if (b->x >= player1_paddle_x - w/2 && b->x <= player1_paddle_x + w/2) {
                b->y = p1y + paddle_height + b->radius;

                float hit = (b->x - player1_paddle_x) / (w / 2.0f);

                float base = fabsf(b->vy);
                b->vy = base * 1.2f;
                b->vx = hit * 8.0f + b->vx * 0.5f;

                float len = sqrtf(b->vx*b->vx + b->vy*b->vy);
                float maxS = (currentMode == MODE_PVP) ? 25.0f : 30.0f;

                if (len > maxS) {
                    b->vx = (b->vx / len) * maxS;
                    b->vy = (b->vy / len) * maxS;
                } else {
                    b->vx *= 1.05f;
                    b->vy *= 1.05f;
                }
            }
        }

        // Top paddle hit
        w = player2_big_paddle ? (int)(paddle_width*1.5f) : paddle_width;
        if (b->y + b->radius > p2y - paddle_height && b->vy > 0) {
            if (b->x >= player2_paddle_x - w/2 && b->x <= player2_paddle_x + w/2) {
                b->y = p2y - paddle_height - b->radius;

                float hit = (b->x - player2_paddle_x) / (w / 2.0f);

                b->vy = -fabsf(b->vy) - 1.5f;
                b->vx += hit * 3.0f;

                float len = sqrtf(b->vx*b->vx + b->vy*b->vy);
                b->vx = (b->vx / len) * ball_speed;
                b->vy = (b->vy / len) * ball_speed;

                consecutive_hits++;
                total_hits++;
                if (consecutive_hits >= 3) {
                    combo_multiplier = 2;
                    combo_timer = 3.0f;
                }

                switch (b->type) {
                    case BALL_FIRE:    addParticle(b->x,b->y,1,0,0); ball_speed += 0.5f; break;
                    case BALL_ICE:     player1_paddle_speed = 0.5f; addParticle(b->x,b->y,0.5f,0.8f,1); break;
                    case BALL_MAGNETIC:b->vx += (player2_paddle_x - b->x) * 0.1f; break;
                }

                addParticle(b->x, b->y, 1.0f, 0.2f, 0.1f);
                playSound(500 + (int)(fabsf(hit)*200), 100);
            }
        }

        // Score & respawn logic
        if (b->y < orthoBottom) {
            player2_score += combo_multiplier;
            b->active = 0;
            activeBalls--;
            if (activeBalls <= 0) {
                resetBall(&balls[0]);
                balls[0].active = 1;
                activeBalls = 1;
            }
            combo_multiplier = 1;
            consecutive_hits = 0;
            playSound(200,200);
        }

        if (b->y > orthoTop) {
            player1_score += combo_multiplier;
            b->active = 0;
            activeBalls--;
            if (activeBalls <= 0) {
                resetBall(&balls[0]);
                balls[0].active = 1;
                activeBalls = 1;
            }
            combo_multiplier = 1;
            consecutive_hits = 0;
            playSound(200,200);
        }
    }

    if (needsRedraw) redraw();
}

// Windows message handler
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static int mouseX, mouseY;
    static int lastMouseX = 0;

    switch (uMsg) {
        case WM_CREATE:
            needsRedraw = 1;
            return 0;

        case WM_SHOWWINDOW:
            if (wParam) { needsRedraw = 1; redraw(); }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            display();
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_F11) {
                // Toggle fullscreen mode
                if (!fullscreen) {
                    GetWindowRect(hwnd, &windowRect);
                    windowStyle   = GetWindowLong(hwnd, GWL_STYLE);
                    windowExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

                    SetWindowLong(hwnd, GWL_STYLE,   windowStyle   & ~(WS_CAPTION | WS_THICKFRAME));
                    SetWindowLong(hwnd, GWL_EXSTYLE, windowExStyle & ~(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE));

                    int sw = GetSystemMetrics(SM_CXSCREEN);
                    int sh = GetSystemMetrics(SM_CYSCREEN);
                    SetWindowPos(hwnd, HWND_TOP, 0,0, sw,sh, SWP_FRAMECHANGED | SWP_NOZORDER);

                    windowWidth = sw; windowHeight = sh;
                    updateOrthoBounds();
                    glViewport(0,0,sw,sh);
                    glMatrixMode(GL_PROJECTION); glLoadIdentity();
                    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
                    glMatrixMode(GL_MODELVIEW);
                    correctPaddlePositions();
                    fullscreen = 1;
                } else {
                    SetWindowLong(hwnd, GWL_STYLE,   windowStyle);
                    SetWindowLong(hwnd, GWL_EXSTYLE, windowExStyle);
                    SetWindowPos(hwnd, HWND_TOP,
                                 windowRect.left, windowRect.top,
                                 windowRect.right - windowRect.left,
                                 windowRect.bottom - windowRect.top,
                                 SWP_FRAMECHANGED | SWP_NOZORDER);

                    windowWidth  = windowRect.right  - windowRect.left;
                    windowHeight = windowRect.bottom - windowRect.top;
                    updateOrthoBounds();
                    glViewport(0,0,windowWidth,windowHeight);
                    glMatrixMode(GL_PROJECTION); glLoadIdentity();
                    glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
                    glMatrixMode(GL_MODELVIEW);
                    correctPaddlePositions();
                    fullscreen = 0;
                }
                needsRedraw = 1;
                redraw();
                return 0;
            }

            if (currentMode == MODE_MENU) {
                switch (wParam) {
                    case '1': currentMode = MODE_PVP; initGame(); needsRedraw=1; break;
                    case '2': currentMode = MODE_DIFFICULTY_SELECT; needsRedraw=1; break;
                    case '3': currentMode = MODE_SPEED_SELECT; needsRedraw=1; break;
                    case VK_SPACE:
                        if ((currentMode == MODE_PVP || currentMode == MODE_PVC) && !game_running) {
                            resetBall(&balls[0]);
                            game_running = 1;
                            SetTimer(hwnd, 1, 16, NULL);
                            needsRedraw = 1;
                        }
                        break;
                    case VK_ESCAPE: PostQuitMessage(0); break;
                }
            }
            else if (currentMode == MODE_DIFFICULTY_SELECT) {
                switch (wParam) {
                    case '1': currentDifficulty = DIFFICULTY_MEDIUM; needsRedraw=1; break;
                    case '2': currentDifficulty = DIFFICULTY_HARD;   needsRedraw=1; break;
                    case VK_RETURN: currentMode = MODE_PVC; initGame(); needsRedraw=1; break;
                    case VK_ESCAPE: currentMode = MODE_MENU; needsRedraw=1; break;
                }
            }
            else if (currentMode == MODE_SPEED_SELECT) {
                switch (wParam) {
                    case VK_UP:
                        if (pvp_ball_speed < 25.0f) { pvp_ball_speed += 1.0f; playSound(600,100); }
                        needsRedraw=1; break;
                    case VK_DOWN:
                        if (pvp_ball_speed > 5.0f)  { pvp_ball_speed -= 1.0f; playSound(400,100); }
                        needsRedraw=1; break;
                    case VK_RETURN: currentMode = MODE_PVP; initGame(); needsRedraw=1; break;
                    case VK_ESCAPE: currentMode = MODE_MENU; needsRedraw=1; break;
                }
            }
            else {
                switch (wParam) {
                    case 'M': case 'm':
                        game_running = 0; KillTimer(hwnd,1);
                        currentMode = MODE_MENU; initGame(); needsRedraw=1; break;
                    case 'R': case 'r':
                        if (game_running) {
                            player1_score = player2_score = 0;
                            for (int j=0; j<3; j++) if (balls[j].active) resetBall(&balls[j]);
                            needsRedraw=1;
                        }
                        break;
                    case VK_SPACE:
                        if (!game_running) {
                            resetBall(&balls[0]);
                            game_running = 1;
                            SetTimer(hwnd,1,16,NULL);
                        } else {
                            game_running = 0;
                            KillTimer(hwnd,1);
                        }
                        needsRedraw=1; break;
                    case VK_ESCAPE: PostQuitMessage(0); break;
                }

                // Update key states
                switch (wParam) {
                    case 'A': case 'a': key_a_pressed = 1; break;
                    case 'D': case 'd': key_d_pressed = 1; break;
                    case 'O': case 'o': key_o_pressed = 1; break;
                    case 'L': case 'l': key_l_pressed = 1; break;
                    case VK_UP:    key_up_pressed    = 1; break;
                    case VK_DOWN:  key_down_pressed  = 1; break;
                    case VK_LEFT:  key_left_pressed  = 1; break;
                    case VK_RIGHT: key_right_pressed = 1; break;
                    case 'W': case 'w': key_w_pressed = 1; break;
                    case 'S': case 's': key_s_pressed = 1; break;
                }
            }

            if (needsRedraw) redraw();
            return 0;
        }

        case WM_KEYUP: {
            switch (wParam) {
                case 'A': case 'a': key_a_pressed = 0; break;
                case 'D': case 'd': key_d_pressed = 0; break;
                case 'O': case 'o': key_o_pressed = 0; break;
                case 'L': case 'l': key_l_pressed = 0; break;
                case VK_UP:    key_up_pressed    = 0; break;
                case VK_DOWN:  key_down_pressed  = 0; break;
                case VK_LEFT:  key_left_pressed  = 0; break;
                case VK_RIGHT: key_right_pressed = 0; break;
                case 'W': case 'w': key_w_pressed = 0; break;
                case 'S': case 's': key_s_pressed = 0; break;
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            mouseX = LOWORD(lParam);
            mouseY = HIWORD(lParam);
            lastMouseX = mouseX;

            if (currentMode != MODE_MENU && currentMode != MODE_DIFFICULTY_SELECT &&
                currentMode != MODE_SPEED_SELECT && !game_running) {
                resetBall(&balls[0]);
                game_running = 1;
                SetTimer(hwnd, 1, 16, NULL);
                needsRedraw = 1;
                redraw();
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            mouseX = LOWORD(lParam);

            if (game_running && (currentMode == MODE_PVP || currentMode == MODE_PVC)) {
                float glX = orthoLeft + (orthoRight - orthoLeft) * mouseX / windowWidth;

                if (player1_control == CONTROL_MOUSE) player1_target_x = glX;
                if (player2_control == CONTROL_MOUSE) player2_target_x = glX;

                // Split control if both players use mouse
                if (player1_control == CONTROL_MOUSE && player2_control == CONTROL_MOUSE) {
                    int my = HIWORD(lParam);
                    if (my > windowHeight / 2)
                        player1_target_x = glX;
                    else
                        player2_target_x = glX;
                }

                needsRedraw = 1;
            }

            lastMouseX = mouseX;
            return 0;
        }

        case WM_TIMER:
            update();
            return 0;

        case WM_DESTROY:
            if (gameFont)  DeleteObject(gameFont);
            if (largeFont) DeleteObject(largeFont);
            if (hrc) {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(hrc);
            }
            if (hdc) ReleaseDC(hwnd, hdc);
            PostQuitMessage(0);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_SIZE: {
            windowWidth  = LOWORD(lParam);
            windowHeight = HIWORD(lParam);

            updateOrthoBounds();
            glViewport(0, 0, windowWidth, windowHeight);
            glMatrixMode(GL_PROJECTION); glLoadIdentity();
            glOrtho(orthoLeft, orthoRight, orthoBottom, orthoTop, -1,1);
            glMatrixMode(GL_MODELVIEW);

            correctPaddlePositions();
            needsRedraw = 1;
            redraw();
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Program entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "PongWindowClass";

    WNDCLASS wc = {0};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;

    RegisterClassA(&wc);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);
    int wx = (sw - WINDOW_WIDTH)  / 2;
    int wy = (sh - WINDOW_HEIGHT) / 2;

    hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "Ping Pong",
        WS_OVERLAPPEDWINDOW,
        wx, wy, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    initOpenGL();

    MSG msg = {0};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
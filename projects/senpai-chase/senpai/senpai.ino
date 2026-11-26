// Senpai Chase: you (sprite from the photo) follow your finger,
// the 3D anime girl (pre-rendered frames in 8 directions) follows you.
// Board: ESP32-2432S0xx "Cheap Yellow Display", TFT_eSPI + XPT2046 touch on the shared SPI bus.
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "assets.h"

#ifndef INVERT_COLORS
#define INVERT_COLORS 0
#endif

TFT_eSPI tft;
TFT_eSprite strip[2] = {TFT_eSprite(&tft), TFT_eSprite(&tft)};
Preferences prefs;

static const int SW = 320, SH = 240, STRIP_H = 60, NSTRIP = SH / STRIP_H;

// ------------------------------------------------------------ touch calibration
struct Cal { uint8_t swapXY; float ax, bx, ay, by; } cal;
bool calOk = false;

bool rawTouch(uint16_t &x, uint16_t &y) {
  if (tft.getTouchRawZ() < 600) return false;
  tft.getTouchRaw(&x, &y);
  return tft.getTouchRawZ() >= 600;
}

void waitPoint(int sx, int sy, uint16_t &rx, uint16_t &ry) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM); tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Calibrazione: tocca la croce", 160, 120, 2);
  tft.drawLine(sx - 10, sy, sx + 10, sy, TFT_MAGENTA); tft.drawLine(sx, sy - 10, sx, sy + 10, TFT_MAGENTA);
  tft.drawCircle(sx, sy, 6, TFT_WHITE);
  uint16_t x, y; uint32_t n = 0, sx_ = 0, sy_ = 0;
  while (!rawTouch(x, y)) delay(10);
  delay(60);
  while (n < 24) { if (rawTouch(x, y)) { sx_ += x; sy_ += y; n++; } delay(8); }
  rx = sx_ / n; ry = sy_ / n;
  tft.fillCircle(sx, sy, 6, TFT_GREEN);
  uint32_t t = millis();
  while (millis() - t < 250) { if (rawTouch(x, y)) t = millis(); delay(10); }
}

void calibrate() {
  uint16_t x0, y0, x1, y1, x2, y2;
  waitPoint(30, 30, x0, y0);
  waitPoint(290, 30, x1, y1);
  waitPoint(30, 210, x2, y2);
  int dxr = abs((int)x1 - (int)x0), dyr = abs((int)y1 - (int)y0);
  cal.swapXY = dyr > dxr;               // the raw axis that changes moving horizontally
  float a0 = cal.swapXY ? y0 : x0, a1 = cal.swapXY ? y1 : x1;
  float b0 = cal.swapXY ? x0 : y0, b2 = cal.swapXY ? x2 : y2;
  cal.ax = (290 - 30) / (a1 - a0); cal.bx = 30 - a0 * cal.ax;
  cal.ay = (210 - 30) / (b2 - b0); cal.by = 30 - b0 * cal.ay;
  prefs.putBytes("cal", &cal, sizeof(cal));
  calOk = true;
}

bool screenTouch(int &sx, int &sy) {
  uint16_t x, y;
  if (!rawTouch(x, y)) return false;
  float a = cal.swapXY ? y : x, b = cal.swapXY ? x : y;
  sx = constrain((int)(a * cal.ax + cal.bx), 0, SW - 1);
  sy = constrain((int)(b * cal.ay + cal.by), 0, SH - 1);
  return true;
}

// ------------------------------------------------------------ drawing into the strip
static inline uint16_t sw16(uint16_t c) { return (c >> 8) | (c << 8); }

void blit(uint16_t *buf, int sy0, const Frame &f, int fx, int fy) {
  int x0 = fx + f.ox, y0 = fy + f.oy;
  int ya = max(y0, sy0), yb = min(y0 + (int)f.h, sy0 + STRIP_H);
  if (ya >= yb) return;
  int xa = max(x0, 0), xb = min(x0 + (int)f.w, SW);
  for (int y = ya; y < yb; y++) {
    const uint16_t *src = f.px + (y - y0) * f.w + (xa - x0);
    uint16_t *dst = buf + (y - sy0) * SW + xa;
    for (int x = xa; x < xb; x++, src++, dst++) if (*src != KEY_COLOR) *dst = *src;
  }
}

void shadow(uint16_t *buf, int sy0, int cx, int cy, int rx, int ry) {
  for (int y = cy - ry; y <= cy + ry; y++) {
    if (y < sy0 || y >= sy0 + STRIP_H) continue;
    float t = (float)(y - cy) / ry; int hw = rx * sqrtf(max(0.f, 1 - t * t));
    uint16_t *row = buf + (y - sy0) * SW;
    for (int x = max(0, cx - hw); x <= min(SW - 1, cx + hw); x++) {
      uint16_t c = sw16(row[x]); c = (c >> 1) & 0x7BEF; c += (c >> 2) & 0x39E7; row[x] = sw16(c);
    }
  }
}

static inline void px(uint16_t *buf, int sy0, int x, int y, uint16_t cs) {
  if (x >= 0 && x < SW && y >= sy0 && y < sy0 + STRIP_H) buf[(y - sy0) * SW + x] = cs;
}

static const char *HEART[6] = {".##.##.", "#######", "#######", ".#####.", "..###..", "...#..."};
void heart(uint16_t *buf, int sy0, int x, int y, uint16_t cs) {
  for (int r = 0; r < 6; r++) for (int c = 0; c < 7; c++) if (HEART[r][c] == '#') px(buf, sy0, x + c - 3, y + r - 3, cs);
}

// ------------------------------------------------------------ game state
struct Actor { float x, y; float phase; int dir; bool walking; };
Actor man{160, 150, 0, 0, false}, girl{70, 200, 0, 0, false};
int manFace = 1;               // 1 right, 0 left
int targetX = -1, targetY = -1;
uint32_t blinkUntil = 0, nextBlink = 2000;

struct Petal { float x, y, vx, vy, ph; } petals[36];
struct Heart { float x, y, life; } hearts[14];
float closeTime = 0, heartTimer = 0;
const char *bubbleTxt = nullptr; float bubbleLeft = 0; bool farSaid = false;

const char *PHRASES[] = {"Senpai~! <3", "Ti ho preso!", "Notice me, senpai!", "Kyaa~!", "Daisuki! <3"};

void say(const char *t, float secs) { bubbleTxt = t; bubbleLeft = secs; }

int dirFromVec(float vx, float vy) {
  float th = atan2f(vx, vy);                       // 0 = towards the viewer (down)
  int d = (int)lroundf(th / (PI / 4));
  return (d % 8 + 8) % 8;
}

void update(float dt) {
  int tx, ty;
  if (screenTouch(tx, ty)) { targetX = tx; targetY = constrain(ty + 10, 40, SH - 3); }
  // --- man towards the touch point
  man.walking = false;
  if (targetX >= 0) {
    float dx = targetX - man.x, dy = targetY - man.y, d = sqrtf(dx * dx + dy * dy);
    if (d > 2.5f) {
      float sp = min(d * 4.0f, 95.0f) * dt;
      man.x += dx / d * sp; man.y += dy / d * sp; man.phase += sp;
      if (fabsf(dx) > 1) manFace = dx > 0;
      man.walking = true;
    } else targetX = -1;
  }
  if (!man.walking) man.phase += dt * 8;
  // --- girl follows the man
  float dx = man.x - girl.x, dy = man.y - girl.y, d = sqrtf(dx * dx + dy * dy);
  girl.walking = false;
  if (d > 30) {
    float sp = constrain(45 + (d - 30) * 1.6f, 0.f, 125.f) * dt;
    girl.x += dx / d * sp; girl.y += dy / d * sp; girl.phase += sp;
    girl.dir = dirFromVec(dx, dy); girl.walking = true;
  } else if (d > 1) girl.dir = dirFromVec(dx, dy);
  // --- speech bubbles and hearts
  if (d > 130 && !farSaid) { say("Aspettami senpai!", 2.0f); farSaid = true; }
  if (d < 70) farSaid = false;
  if (!man.walking && !girl.walking && d < 40) {
    closeTime += dt;
    if (closeTime > 0.7f && closeTime - dt <= 0.7f) say(PHRASES[random(5)], 2.5f);
    heartTimer -= dt;
    if (closeTime > 0.7f && heartTimer <= 0) {
      heartTimer = 0.35f;
      for (auto &h : hearts) if (h.life <= 0) { h.x = girl.x + random(-8, 9); h.y = girl.y - 60; h.life = 1.6f; break; }
    }
  } else closeTime = 0;
  if (bubbleLeft > 0) bubbleLeft -= dt;
  for (auto &h : hearts) if (h.life > 0) { h.life -= dt; h.y -= 18 * dt; h.x += sinf(h.life * 6) * 0.3f; }
  for (auto &p : petals) {
    p.ph += dt * 2; p.x += (p.vx + sinf(p.ph) * 10) * dt; p.y += p.vy * dt;
    if (p.y > SH + 2) { p.y = -2; p.x = random(-20, SW); }
    if (p.x > SW + 2) p.x = -2;
  }
  // --- blink
  uint32_t now = millis();
  if (now > nextBlink) { blinkUntil = now + 140; nextBlink = now + random(1800, 4800); }
}

const Frame &manFrame() {
  int k = man.walking ? ((int)(man.phase / 3.0f)) % 8 : 8 + (((int)man.phase / 5) & 1);
  return MAN[manFace ? 0 : 1][k];
}
const Frame &girlFrame() {
  int k = girl.walking ? ((int)(girl.phase / 3.2f)) % 8 : (millis() < blinkUntil ? 9 : 8);
  return GIRL[girl.dir][k];
}

void drawStrip(int s) {
  TFT_eSprite &sp = strip[s & 1];
  uint16_t *buf = (uint16_t *)sp.getPointer();
  int sy0 = s * STRIP_H;
  memcpy(buf, BG + sy0 * SW, SW * STRIP_H * 2);
  int mx = lroundf(man.x), my = lroundf(man.y), gx = lroundf(girl.x), gy = lroundf(girl.y);
  shadow(buf, sy0, mx, my, 12, 4);
  shadow(buf, sy0, gx, gy, 13, 4);
  if (gy < my) { blit(buf, sy0, girlFrame(), gx, gy); blit(buf, sy0, manFrame(), mx, my); }
  else { blit(buf, sy0, manFrame(), mx, my); blit(buf, sy0, girlFrame(), gx, gy); }
  const uint16_t P1 = sw16(tft.color565(255, 190, 215)), P2 = sw16(tft.color565(255, 225, 235));
  for (auto &p : petals) { int x = p.x, y = p.y; px(buf, sy0, x, y, P1); px(buf, sy0, x + 1, y, P2); px(buf, sy0, x, y + 1, P1); }
  const uint16_t HC = sw16(tft.color565(255, 60, 120));
  for (auto &h : hearts) if (h.life > 0) heart(buf, sy0, h.x, h.y, HC);
  if (targetX >= 0) { // touch target marker
    uint16_t c = sw16(TFT_WHITE);
    for (int i = -3; i <= 3; i++) { px(buf, sy0, targetX + i, targetY - 10, c); px(buf, sy0, targetX, targetY - 10 + i, c); }
  }
  if (bubbleLeft > 0 && bubbleTxt) {
    sp.setTextFont(2);
    int w = sp.textWidth(bubbleTxt) + 12, bx = constrain(gx - w / 2, 2, SW - w - 2), by = max(2, gy - 98);
    sp.fillRoundRect(bx, by - sy0, w, 20, 8, TFT_WHITE);
    sp.drawRoundRect(bx, by - sy0, w, 20, 8, tft.color565(230, 80, 150));
    sp.fillTriangle(gx - 4, by + 19 - sy0, gx + 4, by + 19 - sy0, gx, by + 26 - sy0, TFT_WHITE);
    sp.setTextColor(tft.color565(200, 40, 120)); sp.setTextDatum(MC_DATUM);
    sp.drawString(bubbleTxt, bx + w / 2, by + 10 - sy0);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(27, OUTPUT); digitalWrite(27, HIGH);   // alternative backlight (2.4" model)
  tft.begin();
  tft.setRotation(1);
  tft.invertDisplay(INVERT_COLORS);              // this panel shows inverted colours without it
  tft.setSwapBytes(false);
  prefs.begin("senpai", false);
  // splash: hold your finger down to recalibrate
  tft.fillScreen(tft.color565(40, 20, 50));
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(tft.color565(255, 150, 200)); tft.drawString("Senpai Chase", 160, 100, 4);
  tft.setTextColor(TFT_WHITE); tft.drawString("tieni premuto per ricalibrare il touch", 160, 140, 2);
  calOk = prefs.getBytes("cal", &cal, sizeof(cal)) == sizeof(cal);
  bool hold = false; uint32_t t = millis();
  while (millis() - t < 1500) { uint16_t x, y; if (rawTouch(x, y)) hold = true; delay(20); }
  if (!calOk || hold) {
    if (hold) { tft.fillScreen(TFT_BLACK); tft.drawString("Lascia il dito...", 160, 120, 2); uint16_t x, y; t = millis(); while (millis() - t < 400) { if (rawTouch(x, y)) t = millis(); delay(10); } }
    calibrate();
  }
  for (int i = 0; i < 2; i++) { strip[i].setColorDepth(16); if (!strip[i].createSprite(SW, STRIP_H)) Serial.println("sprite alloc FAILED"); }
  for (auto &p : petals) { p.x = random(SW); p.y = random(SH); p.vx = random(8, 22); p.vy = random(14, 30); p.ph = random(100) / 10.0f; }
  for (auto &h : hearts) h.life = 0;
  tft.initDMA();
  say("Senpai! Toccami lo schermo~", 3.0f);
}

void loop() {
  static uint32_t last = millis(), fpsT = millis(); static int frames = 0;
  uint32_t now = millis();
  float dt = min(0.05f, (now - last) / 1000.0f); last = now;
  update(dt);
  tft.startWrite();
  for (int s = 0; s < NSTRIP; s++) {
    drawStrip(s);
    tft.pushImageDMA(0, s * STRIP_H, SW, STRIP_H, (uint16_t *)strip[s & 1].getPointer());
  }
  tft.dmaWait();
  tft.endWrite();
  if (++frames, now - fpsT >= 2000) { Serial.printf("%d fps, heap %u\n", frames / 2, ESP.getFreeHeap()); frames = 0; fpsT = now; }
}

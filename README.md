# Portfolio — Cosmin

> 计算机工程学生 · 嵌入式系统。个人作品集网站及项目源代码。

I study Computer Engineering at Politecnico di Torino, and this year I'm on exchange at Tongji University (EIE, Politong program). I like building things where software meets hardware.

**Site:** https://cosmin702.github.io/portfolio/ · **GitHub:** https://github.com/Cosmin702

## What's in this repo
| Path | What it is |
|---|---|
| `index.html`, `data.js`, `assets/` | The portfolio site: static, no build step, no external requests (it has to load fast from mainland China). All content is in `data.js`. |
| [`projects/esp32-espnow-sensor-network/`](projects/esp32-espnow-sensor-network/) | Battery ESP32 sensor nodes → receiver over ESP-NOW, with deep sleep, an OLED display and live web charts (C++/Arduino). |
| [`projects/memory-game/`](projects/memory-game/) | Browser memory game with accounts and an online leaderboard on CouchDB (HTML/CSS/JS). |
| [`projects/password-checker/`](projects/password-checker/) | Checks a password-manager export against Have I Been Pwned using k-anonymity (Python). |
| [`projects/recipe-scraper/`](projects/recipe-scraper/) | Scrapes a random recipe from GialloZafferano (Python, BeautifulSoup). |

Each project folder has its own README: what it does, how the code works, setup and the problems I ran into.

## Run the site locally
```bash
python3 -m http.server 8000
```
Then open http://localhost:8000.

## Secrets
Credentials never go in the code. The ESP32 sketches read them from `secrets.h`, which is git-ignored; copy `secrets.example.h` to create it. CSV exports are git-ignored too.

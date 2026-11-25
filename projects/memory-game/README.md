# Memory game

> 带账号系统和在线排行榜的网页记忆翻牌游戏。

<!-- TODO: screenshot or GIF of a game -->

## What it does
A remake of the classic memory card game in the browser. Players can create an account, choose one of three difficulty levels and get a score based on the number of attempts and the time. The best scores are saved on the server and shown in a global leaderboard.

## How the code works
- **Pages**: `index.html` (difficulty choice and leaderboard), `login.html` / `register.html`, and one page per difficulty (`facile`, `medio`, `difficile`, with 8, 16 and 24 cards).
- **Cards**: when a game page loads, `js/game-script.js` builds a random sequence of pairs and assigns the images from `img/` to the cards with `querySelectorAll`. Selected cards flip with a CSS animation.
- **Score**: computed from attempts and time. On a win, a confetti animation plays (`canvas-confetti`) and the score is saved if it is the player's best. Guests' scores are kept only in the browser.
- **Database**: CouchDB, a NoSQL database that stores JSON. The page talks to it directly from JavaScript with the Fetch API (GET to read, PUT to write, with CORS). The leaderboard sorts users by the average of their best scores.
- **Passwords**: at registration the password is encrypted with AES (`crypto-js`), using the password itself as the key. As the server admin, I never see the users' passwords.

## Setup
1. Run CouchDB, create a `memory_game` database and enable CORS.
2. Set `DB_BASE` at the top of `js/register-script.js` to your CouchDB address (default `http://localhost:5984/memory_game`).
3. Serve the folder with any static web server and open `index.html`.

## Known limits
Everything runs on the client side:
- the database URL is visible in the code, and anyone who has it can read or change the data;
- adding authentication wouldn't help, because the keys would also be visible in the code;
- a player can also read the card sequence and fake the score.

## What I'd do differently
Move login and score saving to a server-side backend, so users can't touch the database directly. When I wrote this I hadn't learned server-side programming (PHP) yet.

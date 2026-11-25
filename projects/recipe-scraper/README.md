# Recipe scraper

> 抓取 GialloZafferano 网站的随机菜谱（标题、配料、步骤）。

## What it does
It searches GialloZafferano for a keyword ("limoni"), picks a random recipe from the results and prints its title, image link, intro data, ingredients, steps and the storage and tips notes.

## How the code works
- `requests` downloads the search page, and `BeautifulSoup` picks a random `<article>` result.
- It downloads the recipe page and extracts each part with CSS selectors (`.gz-ingredients .gz-ingredient`, `.gz-content-recipe-step`, `.gz-content-readmore`, …).
- The texts are also collected in strings (`listaingredienti`, `listaistruzioni`, …) so they can be passed on to another step.

## Usage
```bash
pip install -r requirements.txt
python recipe_scraper.py
```

## Notes
- The selectors depend on the site's HTML, so they break if the site changes its layout.
- The part that sends the recipe to IBM watsonx isn't in this folder. <!-- TODO: add it or describe it -->

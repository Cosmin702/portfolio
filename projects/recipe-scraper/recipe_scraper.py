from bs4 import BeautifulSoup
import requests
import random


listaintro = ""
listaingredienti = ""
listaistruzioni = ""

# 1a parte: prendere il link di una ricetta
page = requests.get("https://www.giallozafferano.it/ricerca/limoni+/", timeout=10)
zuppa = BeautifulSoup(page.content, "html.parser")

x = random.randint(1, 14)

risultato = zuppa.find_all("article")[x].find("a")["href"]
print(risultato)

# 2a parte: prendere un istantanea della pagina della ricetta
page = requests.get(risultato, timeout=10)
zuppa = BeautifulSoup(page.content, "html.parser")

# 3a parte: prendere il titlo di una ricetta
titolo = zuppa.find("h1")
print(titolo.text)

# 4a parte: prendere l'immagine di una ricetta
immagine = zuppa.find("img")
print(immagine.get("src") if immagine else "nessuna immagine")

# 5a parte: prendere l'introduzione della ricetta
intro = zuppa.find_all(class_="gz-name-featured-data")
for introduzione in intro:
    print(introduzione.text)
    listaintro += introduzione.text + "\n"

    
# 6a parte: prendere gli ingredienti della ricetta 
ingredienti = zuppa.select(".gz-ingredients .gz-ingredient")
for ingrediente in ingredienti:
    print(" ".join(ingrediente.text.split()))
    listaingredienti += ingrediente.text + "\n"

# 7a parte: prendere le istruzioni della ricetta
istruzioni = zuppa.select(".gz-content-recipe-step")
for istruzione in istruzioni:
    print(" ".join(istruzione.text.split()) + "\n")
    listaistruzioni += istruzione.text + "\n"

# 8a parte: prendere i consilgli di conservazione della ricetta
altro = zuppa.select(".gz-content-readmore")
if len(altro) > 0:
    conservazione = " ".join(altro[0].text.split()) + "\n"
    print(conservazione)
if len(altro) > 1:
    consiglio = " ".join(altro[1].text.split()) + "\n"
    print(consiglio)


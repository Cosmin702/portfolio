import csv, requests, hashlib


def main():
    path = input("Inserisci dove si trova il file da analizzare, deve essere un csv: ")
    lista_login = leggiFile(path)
    if not lista_login:
        return
    passwords_notsafe = controlla_password(lista_login)

    if not passwords_notsafe:
        print("Nessuna password trovata nei data breach.")
        return

    with open("export.csv", "w", newline="", encoding="UTF-8") as outputfile:
        writer = csv.DictWriter(outputfile, passwords_notsafe[0].keys())
        writer.writeheader()
        writer.writerows(passwords_notsafe)
    print(f"{len(passwords_notsafe)} password compromesse salvate in export.csv")


def controlla_password(lista_login): 
    url = "https://api.pwnedpasswords.com/range/"
    password_notsafe = []
    
    for password in lista_login:
        caratteri_password = password["login_password"]
        hash = hashlib.sha1(caratteri_password.encode("UTF-8")).hexdigest().upper()
        
        # k-anonymity: al server arrivano solo i primi 5 caratteri dell'hash, mai la password
        risposta = requests.get(url + hash[0:5], timeout=10)
        
        lista_hash = risposta.text.splitlines()

        for h in lista_hash:
            elementi = h.split(":")
            
            if(elementi[0] == hash[5:]):
                password.update({"times": elementi[1]})
                password_notsafe.append(password)
                break
    
    return password_notsafe    
            

def leggiFile(path):
    try:
        file = open(path,"r",encoding="UTF-8")
        try:
            lista_login = []
            
            for riga in csv.DictReader(file):
                # salto le voci che non sono login o che non hanno una password
                if riga["type"] != "login" or riga["login_password"] == "":
                    continue
                
                record = {
                    "name": riga["name"],
                    "login_username": riga["login_username"],
                    "login_password": riga["login_password"],
                    "login_uri": riga["login_uri"]
                }
                
                lista_login.append(record)
            
            return lista_login
                            
        except Exception as ex:
            print(ex)
        finally:
            file.close()
    except FileNotFoundError:
        print("Il file non é stato trovato")


if __name__ == "__main__":
    main()

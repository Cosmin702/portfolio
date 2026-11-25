# Password checker

> 检查导出的密码是否出现在已知数据泄露中（Have I Been Pwned，k-匿名查询）。

## What it does
It reads a CSV export from a password manager and checks every login password against the [Have I Been Pwned](https://haveibeenpwned.com/Passwords) database of leaked passwords. Passwords that appear in a breach are saved to `export.csv`, with the number of times each one was found.

## How the code works
- It reads the CSV and keeps only entries of type `login` that have a password (columns `type`, `name`, `login_username`, `login_password`, `login_uri`).
- For each password it computes the SHA-1 hash and sends **only the first 5 characters** of the hash to `api.pwnedpasswords.com/range/`. This is the k-anonymity model: the full password and hash never leave the computer.
- The API answers with all the hash suffixes that start with those 5 characters, and the script compares them locally with the rest of the hash.

## Usage
```bash
pip install -r requirements.txt
python main.py
```
Enter the path of the CSV when asked.

> **Warning:** the input CSV and `export.csv` contain real passwords. Never commit them. `*.csv` files are git-ignored in this repo.

<!-- TODO: which password manager the export format comes from; what I'd do differently -->

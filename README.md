# monorepo

Regroupement de tous mes anciens dépôts GitHub en un seul repo : un dossier par ancien dépôt, code inchangé, historique complet conservé.

## Projets

| Dossier | Stack | Description |
|---|---|---|
| `abdessamed-bank-kata` | Java / Maven | Kata de la banque (énoncé : [kirinux/ing-bank-account-kata](https://github.com/kirinux/ing-bank-account-kata)) |
| `app` | Java / Maven | Application multi-modules (api-gateway, user, shared) |
| `coffee-kata` | Java / Maven | Coffee kata |
| `coin-machine` | Java / Maven | Coin machine kata |
| `fizzbuzz-kata` | Java / Maven | FizzBuzz kata |
| `fizzbuzz-refucktoring` | TypeScript | FizzBuzz, session refucktoring (branche `socrates-2023`) |
| `fraction-promo15` | Java / Maven | Kata fraction fait en mob (Abdessamed, Issam, Younouss, encadré par Hadrien) |
| `game-of-life-kata` | Java / Maven | Game of Life kata |
| `kata-pacifica` | Angular / TypeScript | Kata Pacifica |
| `mars-rover` | Java / Gradle | Mars Rover kata |
| `nodejs-learning` | JavaScript | Apprentissage Node.js (back + front) |
| `nullobject` | Java | Null Object pattern |
| `passwords` | Java / Gradle | Passwords kata |
| `roman-numerals-refucktoring` | TypeScript | Roman numerals, refucktoring |
| `SingSongKata` | Java / Maven | Sing Song kata |
| `Tennis-Refactoring-Kata` | TypeScript (multi-langages) | Fork du Tennis Refactoring Kata d'Emily Bache |
| `tictactoe-ai-coded` | C / CMake | Tic Tac Toe codé avec une IA |
| `tondeuse` | Java / Gradle | Kata de la tondeuse |

## Historique et branches

- `main` contient un commit de merge par dépôt importé (`git log --first-parent`).
- L'historique de chaque dépôt a été réécrit avec `git filter-repo --to-subdirectory-filter`, donc `git log` / `git blame` fonctionnent fichier par fichier dans chaque dossier.
- Les branches autres que la branche principale de chaque dépôt sont conservées sous la forme `<dossier>/<branche>` : `app/dev-1.0`, `fizzbuzz-refucktoring/socrates-2023`, `order/master`, `tictactoe-ai-coded/docs/add-readme`.

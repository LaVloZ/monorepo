# Announce Debugger

Outil Windows (C11, API Win32) pour déboguer les endpoints `/announce` des trackers BitTorrent :

- **HTTP / HTTPS** (BEP 3, avec `peers` compact, `peers6` et format dictionnaire) ;
- **UDP** (BEP 15 : connect, announce, erreurs, retransmissions `15 × 2^n` s).

Le résultat est **un seul `.exe`, sans DLL à installer** : il n'utilise que des composants de Windows
(Win32, WinHTTP, Winsock, BCrypt pour le SHA-1).

On ouvre un `.torrent` (bouton, glisser-déposer ou ligne de commande) : l'outil affiche les infos du torrent
(info_hash, taille, fichiers, pièces…) et **tous** ses trackers (`announce` + `announce-list`). **Aucun announce
n'est envoyé automatiquement.** On choisit un tracker, on règle les paramètres, on clique sur **Announce** et on
voit :

- **Réponse** : transport, code HTTP / action UDP, temps de réponse, champs décodés (erreurs en rouge, champs
  inexistants pour le transport grisés) ;
- **Requête** : l'URL HTTP finale (bouton copier) ou, en UDP, l'adresse résolue et les paquets connect/announce
  envoyés, champ par champ + hex dump ;
- **Peers** : IP, port, peer_id (si fourni), avec une recherche (IP, port, peer_id, client…) ; le bouton
  **Sonder les peers** se connecte à chacun (TCP, handshake BitTorrent, 16 en parallèle, 5 s max par peer) et
  affiche s'il est joignable, son pourcentage de complétion (`bitfield` / `have` / `have_all` / `have_none`),
  les seeders en vert, son client (handshake étendu BEP 10 ou peer_id) et ses extensions. Rien n'est téléchargé.
  Les résultats restent attachés à l'announce dans l'historique. Le pourcentage exact demande le `.torrent`
  chargé (nombre de pièces) ;
- **Raw** : hex dump de la réponse brute, annoté en UDP, avec le bencode décodé en HTTP ;
- **Historique** : les 100 derniers announces de chaque tracker (en mémoire), cliquables ;
- **Journal** horodaté : announces, retransmissions, réutilisation/renouvellement du `connection_id`, erreurs.

Les announces tournent en arrière-plan : on peut en lancer plusieurs en même temps vers des trackers différents
(un seul à la fois par tracker), et **Annuler** agit sur le tracker sélectionné.

On peut aussi ajouter une URL de tracker à la main et saisir un info_hash sans fichier `.torrent`.
Un double-clic sur une ligne (infos, trackers, réponse, peers) copie la valeur.

## Prérequis

**Build Tools pour Visual Studio** (ou Visual Studio Community) avec la charge de travail
**« Développement Desktop en C++ »** (compilateur MSVC + SDK Windows). Windows 10 ou plus récent.

Rien d'autre à installer : pas de bibliothèque externe.

## Compilation et tests

Depuis un terminal ouvert dans ce dossier (le script trouve MSVC tout seul) :

```bat
build.bat
```

| Commande | Effet |
|---|---|
| `build.bat` | build release : `build\release\announce-debugger.exe` |
| `build.bat test` | build release puis tests unitaires et d'intégration |
| `build.bat check` | build debug + détection des fuites mémoire (CRT debug), puis tests |
| `build.bat asan` | build AddressSanitizer (débordements, use-after-free…), puis tests |
| `build.bat clean` | supprime `build\` |

Le code compile en `/W4` (équivalent de `-Wall -Wextra`) sans warning.

Tests :

- `test_unit` : bencode (encode/decode, cas invalides), info_hash (y compris dictionnaire non canonique), parsing
  `.torrent`, peers compact IPv4/IPv6 et dictionnaire, sérialisation exacte des paquets UDP connect (16 octets) et
  announce (98 octets), parsing des réponses UDP announce/erreur, transaction_id invalide, URL HTTP, historique,
  messages peer wire (handshake, bitfield, have, have_all, handshake étendu, client deviné) ;
- `test_integration` : faux tracker HTTP et faux tracker UDP sur `127.0.0.1` (port éphémère) : succès, tracker id,
  failure reason, réponse non bencodée, connexion refusée, paquet avec mauvais transaction_id, réutilisation et
  expiration du connection_id, retransmission, timeout, annulation, port UDP fermé, DNS ; faux peers TCP pour le
  sondage (seeder avec bitfield et handshake étendu, `have_all`, mauvais info_hash, peer muet, port fermé).

## Utilisation

```bat
build\release\announce-debugger.exe
build\release\announce-debugger.exe C:\chemin\vers\fichier.torrent
```

### Exemple HTTP

1. Ouvrir un `.torrent` dont un tracker est en `http(s)://`, ou taper une URL dans le champ sous la liste des
   trackers, par exemple `http://tracker.example.org:8080/announce`, puis Entrée.
2. Vérifier l'info_hash (40 hex) et les paramètres, choisir `event = started`, cliquer sur **Announce**.
3. L'onglet **Requête** montre l'URL envoyée, par exemple
   `http://tracker.example.org:8080/announce?info_hash=%A0%A1…&peer_id=-DB0001-…&port=6881&…&event=started`.
   Si l'URL contient déjà une query (`?passkey=…`), les paramètres sont ajoutés avec `&`.

### Exemple UDP

1. Ajouter `udp://opentor.net:6969/announce` (le chemin est ignoré, seuls l'hôte et le port comptent).
2. Cliquer sur **Announce** : le journal montre la résolution DNS, le `connect`, puis l'`announce`.
3. Un nouvel announce dans la minute réutilise le `connection_id` (pas de connect). Au-delà, il est renouvelé.
4. Sans réponse, l'outil retransmet après 15 s, 30 s, 60 s… jusqu'à `n = 8`, puis abandonne. Le bouton
   **Annuler** interrompt l'attente à tout moment.

## Organisation du code

| Fichier | Rôle |
|---|---|
| `src/bencode.c/h` | encodage/décodage bencode (strict, avec position brute de chaque valeur) |
| `src/torrent.c/h` | lecture du `.torrent`, info_hash (SHA-1 des octets bruts de `info`), trackers, fichiers |
| `src/tracker.c/h` | interface commune : `tr_announce()`, parsing d'URL, résultats, mise en forme Requête/Raw |
| `src/http_tracker.c/h` | construction de l'URL, requête WinHTTP (asynchrone, annulable), décodage de la réponse |
| `src/udp_tracker.c/h` | paquets BEP 15, envoi/réception Winsock, retransmissions, annotations |
| `src/peer_probe.c/h` | sondage d'un peer : handshake BitTorrent, bitfield/have, BEP 6 et BEP 10 |
| `src/history.c/h` | historique des announces par tracker (100 max, en mémoire), avec les résultats de sondage |
| `src/ui.c`, `src/main.c` | interface Win32 ; le réseau tourne dans un thread séparé |

## Notes

- Le `connection_id` UDP est réutilisé pendant **1 minute** (règle client du BEP 15 ; le tracker l'accepte
  2 minutes).
- Les torrents BitTorrent v2 purs (BEP 52) ne sont pas supportés ; les torrents hybrides utilisent l'info_hash v1.
- `peer_id` par défaut : `-DB0001-` + 12 caractères alphanumériques aléatoires. `key` : 32 bits aléatoires par
  session (envoyée en 8 hex en HTTP).
- HTTP : le proxy configuré pour WinHTTP (`netsh winhttp show proxy`) est utilisé, sinon connexion directe.

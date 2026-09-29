---
name: analyse-fondamentale-ticker
description: >-
  Analyse fondamentale et narrative d'un actif (action, crypto ou ETF), désignable par symbole, nom OU code ISIN, en deux temps — PHOTO DU PRÉSENT puis JUGEMENT SUR L'AVENIR à 2-5 ans déduit des données. Pas d'analyse technique. Action : rentabilité, earnings/bilan, dette, cashflow, dividendes/EPS, insiders, narratif et bulle IA, et exposition ETF (combien d'ETF détiennent le titre, avec quel poids). Crypto : narratif, open interest, flux ETF. ETF : taille (AUM), frais, flux entrants/sortants (inflow/outflow), composition (holdings, secteurs, pays). À utiliser quand l'utilisateur demande "analyse fondamentale de X", "qu'est-ce qui se passe avec X", "X a-t-elle du potentiel", "analyse l'ISIN/l'ETF X", "pourquoi X monte/baisse", "dans combien d'ETF est X", pour une action US, une action française (.PA), une crypto ou un ETF. NE PAS utiliser pour de l'analyse technique (chandeliers, RSI, moyennes mobiles, supports/résistances).
---

# Analyse fondamentale d'un ticker — présent + avenir à 2-5 ans

## But et logique

Enquêter sur un actif (action ou crypto) en **deux temps clairement séparés** :

1. **La photo du présent** — où en est l'entreprise *aujourd'hui* : son positionnement, sa santé financière, ce qui se passe en ce moment. C'est le constat factuel.
2. **Le jugement sur l'avenir (2-5 ans)** — à partir des données réunies dans la photo, déduire si l'entreprise a du potentiel pour les années à venir : forces durables, risques, conditions de la thèse.

L'ordre compte : **on établit d'abord le positionnement, puis on s'en sert pour juger l'avenir**. Le jugement n'est pas une intuition isolée ; il découle explicitement des faits de la première partie. Aucune analyse technique. Sortie : une **synthèse concise en chat**, factuelle et sourcée.

## Règle d'or : ne jamais inventer de chiffre

Un chiffre faux ruine la confiance dans toute l'analyse.

- N'écris un chiffre QUE s'il apparaît dans une réponse d'outil ou dans le texte d'une news retournée.
- Ne devine pas, n'estime pas, n'annualise pas, ne calcule pas un ratio « au jugé ». Si la donnée manque, écris **« non disponible »**.
- Rattache chaque chiffre tiré d'une actualité à sa source, et **date** l'information.
- Reste sur les **grandes lignes** : les chiffres qui comptent (taille, croissance, marge, dette, cash, rendement), pas un mur de décimales.
- Le jugement sur l'avenir reste une **interprétation explicitement étiquetée comme telle**, jamais présentée comme un fait, et **sans prédiction de prix**.

## Outils (connecteur de données financières connecté, type FMP)

- `search` → `search-symbol`, `search-name`, `search-exchange-variants`
- `quote` → `quote`
- `company` → `profile-symbol`, `market-cap`, `peers`
- `statements` → `key-metrics-ttm`, `metrics-ratios-ttm`, `income-statement-growth`, `financial-scores`, `cashflow-statement`, `balance-sheet-statement`, `income-statement`, `financial-statement-growth`
- `calendar` → `earnings-company` (dernier & prochain earnings, surprises), `dividends-company` (dates de dividende), `earnings-calendar` / `dividends-calendar` (événements à venir sur une fenêtre)
- `news` → `search-stock-news`, `search-press-releases`, `crypto-news` / `search-crypto-news`
- `secFilings` → `8k-latest`, `search-by-symbol`
- `earningsTranscript` → `transcripts-dates-by-symbol`, `search-transcripts`
- `insiderTrades` → `insider-trade-statistics`, `search-insider-trades`
- `commitmentOfTraders` → positionnement futures (proxy d'open interest, crypto/commodités) *(plan Premium+)*
- `etfAndMutualFunds` → `etf-asset-exposure` (quels ETF détiennent un titre + poids), `holdings` (composition d'un ETF), `information` (AUM, frais, infos fonds), `sector-weighting`, `country-weighting`
- **`WebSearch`** → repli pour le narratif/news ET pour open interest crypto + flux ETF (voir ci-dessous)
- **Skill `narratif-ai-secteur`** → pour situer l'actif dans la bulle/narratif IA au niveau marché (l'invoquer si l'utilisateur veut creuser le thème IA)

Si le connecteur n'est pas connecté, le dire et proposer de le connecter plutôt que répondre de mémoire.

## Repli web quand le connecteur bloque (important)

Selon le plan FMP, certains outils renvoient **« ACCESS DENIED — requires a higher plan »** : typiquement `news`, `search-press-releases`, `insiderTrades`, `earningsTranscript`, `commitmentOfTraders`, et certains paramètres de `quote`/`statements`.

Quand un outil renvoie ACCESS DENIED ou ne répond pas :
- **Ne pas réessayer** le même outil ni ses jumeaux du même palier dans la session.
- **Basculer sur `WebSearch`** pour reconstruire l'info manquante (earnings, dette, cashflow, insiders, open interest, flux ETF), à partir de sources fiables (Reuters, Bloomberg, FT, Les Échos, communiqués officiels, CoinDesk/CoinGlass pour la crypto).
- Toujours **citer et dater** ce qui vient du web, et le signaler brièvement à l'utilisateur.

Les chiffres de base (`quote`, `company/profile`, une partie de `statements`) passent généralement même sur plan bas : les utiliser, et compléter le reste par le web.

## Étape 0 — Résoudre l'actif (symbole, nom ou ISIN) et son type

L'utilisateur peut désigner l'actif par **symbole**, **nom**, ou **code ISIN** (12 caractères, 2 lettres pays + 9 alphanumériques + 1 clé, ex. `US5949181045`, `FR0000121014`).

- **Si l'entrée ressemble à un ISIN** → la résoudre avec `search/search-ISIN` (paramètre `isin`) pour obtenir le symbole, puis continuer normalement. Afficher l'ISIN ET le symbole résolu.
- **Si c'est un nom ou un symbole ambigu** → `search/search-symbol` ou `search/search-name`. Confirmer le symbole retenu.

Déterminer ensuite le **type d'actif**, qui choisit le mode :
- **Action US** (`AAPL`, `NVDA`) → tous les volets (Parties A & B).
- **Action française / européenne** → suffixe Euronext, souvent `.PA` (LVMH = `MC.PA`, Thales = `HO.PA`). Fondamentaux + news dispos ; **8-K, transcripts, insiders SEC absents** hors US.
- **Crypto** (`BTCUSD`, `BNBUSD`) → mode crypto (narratif + open interest + flux ETF).
- **ETF** (le profil/recherche indique `isEtf: true`, ou le nom contient « ETF / UCITS / iShares / SPDR / Amundi / Lyxor… ») → **mode ETF** (voir section dédiée). Ne pas appliquer la grille action (pas de marges/earnings d'entreprise).

Toujours afficher l'identifiant résolu (symbole + ISIN si fourni) et la devise.

---

# PARTIE A — La photo du présent (positionnement actuel)

On commence toujours par établir où en est l'entreprise. C'est le matériau brut du jugement qui suivra.

## A1 — Snapshot & positionnement

`quote/quote` + `company/profile-symbol` / `market-cap` : cours, variation, secteur, taille, plage 52 s, position dans son industrie. Situer l'entreprise : leader ou challenger, méga-cap ou small-cap, secteur porteur ou mûr.

## A2 — Santé fondamentale (actions)

Extraire l'essentiel de chaque dimension (interpréter la tendance, pas un mur de nombres) :

- **Rentabilité** : marges (brute, opérationnelle, nette), ROE/ROIC, qualité du résultat (`key-metrics-ttm`, `metrics-ratios-ttm`, `financial-scores`). Gagne-t-elle de l'argent, et de mieux en mieux ?
- **Dernier earnings / bilan** : `calendar/earnings-company` (dernier résultat + surprise vs attentes) ; `income-statement` (dernier trimestre/exercice) ; `income-statement-growth` / `financial-statement-growth` (croissance CA & bénéfices : accélère ou décélère ?).
- **Dette** : `balance-sheet-statement` + ratios (`netDebtToEBITDA`, current ratio). Bilan sain ou tendu ? Capacité à encaisser une hausse des taux ?
- **Cashflow** : `cashflow-statement` (FCF, conversion du résultat en cash, capex). Génère-t-elle du cash ou en brûle-t-elle ?
- **Dividendes / EPS / rendement** : `calendar/dividends-company` + `quote` (lastDividend, EPS, dividend yield). Croissance de l'EPS ? Dividende soutenable et en hausse ? Rendement ?

Si la boîte n'est pas rentable (biotech clinical-stage, jeune tech), le dire et regarder plutôt : trésorerie/runway, pipeline/jalons, chemin vers la rentabilité.

## A3 — News & narratif (« ce qui se passe »)

`news/search-stock-news` + `search-press-releases` (fenêtre ~30-60 j) ; `secFilings/8k-latest` (US) ; dernier `earningsTranscript` pour le ton du management. Repérer les **catalyseurs** (résultats, guidance, contrats, produits, régulation, M&A, direction) et construire le narratif du moment.

## A4 — Insiders

`insiderTrades/insider-trade-statistics` (tendance achats vs ventes) + `search-insider-trades`. Lecture nuancée : achats en open market de dirigeants = conviction ; ventes souvent routinières. Hors US → repli web si signal connu, sinon « non disponible ».

## A5 — Positionnement face au narratif IA

Où se situe l'entreprise par rapport au thème IA, moteur dominant du marché :
- **Exposition** : l'IA est-elle un moteur de revenus réel, un risque de disruption, ou neutre ?
- **Bénéficiaire vs perdant** : capte-t-elle la demande IA (capex, datacenters, logiciels) ou la subit-elle ?
- **Déjà monétisé ou narratif** : l'effet est-il déjà dans les chiffres ou promis ?

Pour le contexte de cycle/bulle au niveau marché, invoquer le skill **`narratif-ai-secteur`** si l'utilisateur veut creuser.

## A6 — Exposition ETF (actions)

Mesurer à quel point le titre est « détenu passivement » via `etfAndMutualFunds/etf-asset-exposure` :
- **Dans combien d'ETF** le titre est présent (nombre d'ETF détenteurs).
- **Quel poids** il représente dans les principaux ETF qui le détiennent (les plus grosses pondérations).

Lecture : une forte présence dans de gros ETF (S&P 500, Nasdaq-100, ETF sectoriels/IA) signifie que le titre reçoit des flux passifs réguliers tant que ces indices/ETF collectent — un soutien en marché porteur, mais aussi une vulnérabilité en cas de décollecte ou de sortie d'indice. Citer 3-5 ETF représentatifs avec leur poids, pas la liste exhaustive. Donnée souvent disponible même sur plan bas ; sinon « non disponible ».

## A7 — Agenda des catalyseurs à venir

Recenser les **événements datés ou attendus** qui peuvent faire bouger le titre dans les prochaines semaines/mois. C'est le pont vers la Partie B : ces rendez-vous sont les moments où la thèse se valide ou se casse. Chercher activement, dans chacune de ces familles :

- **Earnings & événements société** : prochaine date de résultats (`calendar/earnings-company`), journée investisseurs, AG, dates de dividende/détachement (`calendar/dividends-company`), expiration de lock-up, split.
- **Macro & banques centrales** : prochaines décisions **Fed / BCE** (et BoE/BoJ si pertinent), publications d'inflation/emploi clés — surtout pour les valeurs sensibles aux taux (tech/croissance, immobilier, banques) et les exportateurs (effet change).
- **Régulation & politique** : **votes/décisions réglementaires** (UE, FDA pour la santé, antitrust, FCC, décisions ETF SEC pour la crypto), procès/échéances juridiques, sanctions/contrôles export (ex. puces vers la Chine), élections ou budgets (ex. budget défense).
- **Produits & technologie** : **lancements produits** annoncés, mises sur le marché, fin d'essais cliniques (read-outs), keynotes.
- **Salons & conférences sectorielles** : grands rendez-vous qui concentrent les annonces — ex. **Computex**, CES, GTC (Nvidia), WWDC/Apple events, Mobile World Congress, JP Morgan Healthcare, Eurosatory (défense), conférences OPEP pour l'énergie.

Pour chaque événement pertinent : **dater** (date précise si connue, sinon fenêtre type « T3 2026 »), dire **pourquoi il compte** pour ce titre, et le sens probable (haussier/baissier/incertain). Utiliser `news`/`search-press-releases` et **WebSearch** pour les dates (calendriers Fed/BCE, dates de salons, échéances réglementaires). Ne jamais inventer une date : si elle est incertaine, l'indiquer (« attendu vers… »).

Si rien de notable n'est identifié, le dire en une ligne plutôt que de meubler.

---

# PARTIE B — Le jugement sur l'avenir (2-5 ans)

À partir UNIQUEMENT des éléments réunis en Partie A, déduire le potentiel de l'entreprise à 2-5 ans. Chaque point doit s'appuyer sur un fait de la photo (« parce que… »). C'est une interprétation, étiquetée comme telle, jamais une prédiction de prix.

Raisonner sur ces axes :

- **Moteurs de croissance durables** : la croissance actuelle peut-elle tenir 2-5 ans ? (tendance du CA, carnet/backlog, segments porteurs, position IA déjà monétisée vs promise).
- **Qualité & défense du modèle** : marges et rentabilité défendables ? avantage concurrentiel (moat) ? menace de disruption (IA, concurrence) ?
- **Solidité financière** : le bilan et le cashflow permettent-ils d'investir, de traverser un cycle de taux/récession, de financer la croissance sans se diluer ?
- **Rémunération de l'actionnaire** : trajectoire EPS, soutenabilité et croissance du dividende, buybacks — créent-ils de la valeur dans la durée ?
- **Position dans le cycle IA** : si le secteur est en euphorie, une boîte de qualité peut rester chère ; distinguer valeur intrinsèque et prime de narratif.

Conclure par un **verdict nuancé** : forces durables, faiblesses/risques majeurs, et les **conditions concrètes** qui valideraient ou casseraient la thèse à 2-5 ans (ex. « si la marge cloud continue de monter et la dépendance à X se réduit, la thèse tient ; sinon… »). Pas de recommandation achat/vente, pas d'objectif de cours.

---

## Format de sortie (deux sections, concis)

Rendre la synthèse en deux blocs nettement séparés :

**🔵 PHOTO DU PRÉSENT**
1. **TL;DR** — 2-3 phrases : positionnement et ce qui se passe.
2. **Chiffres clés** — cours & variation, market cap, croissance CA, marge/rentabilité, dette, FCF, EPS, dividende & rendement (ceux disponibles ; sinon « non disponible »).
3. **Dernier earnings** — résultat récent vs attentes.
4. **Narratif & insiders** — catalyseurs du moment + une ligne insiders.
5. **Positionnement IA** — bénéficiaire / neutre / menacé.
5b. **Exposition ETF** — nombre d'ETF détenteurs + 3-5 ETF avec poids (une ligne).

**🗓️ CATALYSEURS À VENIR**
6. **Agenda** — événements datés à surveiller, classés par proximité : earnings, décisions Fed/BCE, votes/décisions réglementaires, lancements produits, salons (Computex, GTC, CES…). Pour chacun : date (ou fenêtre), pourquoi ça compte, sens probable.

**🟢 JUGEMENT À 2-5 ANS**
7. **Thèse** — potentiel déduit des faits ci-dessus (forces durables, moteurs de croissance).
8. **Risques & angles morts** — ce qui pourrait casser la thèse.
9. **Conditions à surveiller** — lesquels des catalyseurs ci-dessus valideront ou invalideront la thèse.
10. **Sources** — liens datés.

Ton factuel et nuancé. **Pas de conseil en investissement.** Distinguer faits (Partie A) et interprétation (Partie B).

---

## Mode crypto

Pas de fondamentaux d'entreprise. Garder la structure en deux temps :

**Photo du présent :**
- **Snapshot & contexte** : `quote/quote` (paire USD) ; utilité du protocole, catégorie (L1, L2, exchange token, DeFi…).
- **Narratif** : `news/crypto-news` / `search-crypto-news` ou web — régulation, mises à jour réseau, événements d'exchange/émetteur, macro/liquidité.
- **Open interest** : levier et appétit spéculatif sur les futures. Via `commitmentOfTraders` si dispo, sinon **WebSearch** (CoinGlass, CoinDesk) pour l'OI agrégé et sa tendance (hausse = levier croissant, risque de squeeze/liquidations). Dater et sourcer.
- **Flux ETF** : pour BTC/ETH, les flux entrants/sortants des ETF spot sont un moteur de demande majeur. Via `etfAndMutualFunds` si dispo, sinon **WebSearch** (flux nets récents : entrées = demande institutionnelle, sorties = pression). Dater impérativement.

**Catalyseurs à venir (crypto) :** recenser les événements datés — décisions ETF de la SEC, échéances réglementaires (MiCA, etc.), halving, mises à jour réseau/hard forks, déblocages de tokens (unlocks), grandes conférences. Dater et sourcer (web si besoin).

**Jugement à 2-5 ans :** recentré sur adoption, narratif structurel, dynamique ETF/OI, et risques (régulation, liquidité, concurrence entre chaînes), sans inventer de métrique on-chain non fournie.

Ne pas afficher de sections financières/insiders/filings d'entreprise pour une crypto.

## Mode ETF

Quand l'actif est un ETF (détecté à l'étape 0), on n'analyse pas une « entreprise » mais un **véhicule** : sa composition, son coût, sa taille et sa dynamique de flux. Garder la structure en deux temps.

**Photo du présent :**
- **Identité & taille** : `etfAndMutualFunds/information` → AUM (taille des actifs sous gestion), **frais (expense ratio)**, émetteur, indice répliqué, date de création, devise. `quote/quote` pour le cours et la variation.
- **Composition** : `etfAndMutualFunds/holdings` (top holdings et leurs poids → degré de concentration), `sector-weighting` (répartition sectorielle), `country-weighting` (répartition géographique). Dire en une phrase ce que l'ETF détient vraiment et s'il est concentré ou diversifié.
- **Flux (inflow/outflow)** : les flux nets récents sont le signal de demande clé. Si le connecteur ne les expose pas directement, **WebSearch** (sources type etf.com, Morningstar, issuer) pour les entrées/sorties nettes récentes — entrées = collecte/appétit, sorties = décollecte. Dater impérativement.
- **Performance & contexte** : performance récente et ce qui la dirige (le thème sous-jacent : IA, défense, obligataire, or…).

**Jugement à 2-5 ans :**
- **Coût** : l'expense ratio est-il compétitif vs ETF équivalents ? (un écart de frais se paie sur la durée).
- **Thème & durabilité** : l'exposition de l'ETF a-t-elle du sens à 2-5 ans (secteur porteur vs mode passagère) ? Concentration = risque si quelques lignes dominent.
- **Liquidité & pérennité** : un AUM important et des flux positifs réduisent le risque de fermeture/écart de liquidité ; un petit ETF en décollecte est plus fragile.
- **Risques** : risque de change (ETF en devise étrangère), risque de concentration, risque thématique (si la bulle du thème dégonfle).

Conclure par un verdict nuancé sur la pertinence de l'ETF comme véhicule d'exposition au thème, sans recommandation d'achat/vente. Ne pas afficher de sections earnings/insiders d'entreprise pour un ETF.

## Règles

- Ne jamais fabriquer de chiffre ; donnée manquante = le dire.
- Accepter symbole, nom OU ISIN en entrée ; toujours afficher l'identifiant résolu (symbole + ISIN si fourni) et la devise.
- Choisir le bon mode selon le type : action / crypto / ETF. Ne pas appliquer la grille action à un ETF.
- Deux sections distinctes : présent (faits) puis avenir (jugement déduit). Ne pas mélanger.
- Pas de sections vides : omettre proprement un volet non applicable.
- Pas d'analyse technique. Citer et dater les sources.

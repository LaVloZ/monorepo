---
name: analyse-technique-ticker
description: >-
  Analyse technique multi-timeframe d'un actif (crypto, action US ou française .PA, indice, forex, matière première) : structure de tendance, supports/résistances, moyennes mobiles 20/50/200, RSI, MACD, volume, funding et open interest pour la crypto, et scénarios bull/bear avec chacun son trigger, ses cibles et son niveau d'invalidation. À utiliser dès que l'utilisateur demande explicitement de la technique — « analyse technique de X », « les niveaux de BTC », « supports/résistances », « regarde le chart de X », « setup d'entrée », « où placer mon stop », « le RSI/les moyennes mobiles de X » — même formulé de façon casual. Horizon par défaut : swing (jours-semaines). NE PAS utiliser pour l'analyse fondamentale (earnings, bilan, dette, dividendes, narratif → analyse-fondamentale-ticker) ni pour arbitrer plusieurs tracés concurrents dessinés sur un même chart (→ trader-analyze-range).
---

# Analyse technique d'un ticker — multi-timeframe, sans biais

Lire le chart comme un trader expérimenté et sans parti pris. Ce skill est le pendant **technique** du skill `analyse-fondamentale-ticker` : ici, pas d'earnings ni de bilan — uniquement le prix, la structure, les indicateurs et les niveaux qui en découlent.

## Principes

1. **Jamais de mémoire.** Les prix changent chaque jour et tes données d'entraînement sont périmées. Toujours récupérer la cotation actuelle et l'historique de prix avant de dire quoi que ce soit. Si les données sont inaccessibles, le dire explicitement — ne jamais inventer ou estimer un prix ou un niveau.
2. **Zéro biais.** Toujours construire le cas haussier ET le cas baissier, chacun avec son trigger et son niveau d'invalidation. S'exprimer en probabilités (« la balance des éléments penche vers… »), jamais en certitudes. Ne pas s'ancrer sur l'espoir apparent de l'utilisateur : s'il tient une position, il a besoin d'honnêteté, pas de validation.
3. **Ancrage temporel.** Dater chaque analyse et horodater les données (« au [date/heure]… »).
4. **Sources.** Citer d'où viennent les données et chiffres.

## Workflow

1. **Identifier** l'actif et l'horizon de l'utilisateur (swing / moyen terme / long terme — pas d'intraday par défaut). Si non précisé, prendre **swing (jours-semaines)** et le dire.
2. **Récupérer les données** — de préférence via le connecteur de données financières (type FMP) :
   - `quote/quote` → prix actuel, variation, plage 52 semaines.
   - `chart` → historique de prix OHLCV sur plusieurs granularités (daily pour weekly/daily, intraday pour 4h/1h).
   - `technicalIndicators` → moyennes mobiles, RSI, MACD si disponibles ; sinon les calculer à partir de l'historique OHLCV (par script, jamais au jugé).
   - Crypto : funding rates, open interest et zones de liquidation via WebSearch (CoinGlass, CoinDesk) — dater et sourcer.
   - Si un endpoint renvoie « ACCESS DENIED » : ne pas réessayer, basculer sur WebSearch ou calculer depuis l'OHLCV disponible.
3. **Vérifier le risque événementiel** (section dédiée ci-dessous) — un niveau technique ne vaut rien face à un FOMC surprise.
4. **Lire le chart** multi-timeframe (cadre ci-dessous).
5. **Livrer** la sortie structurée, dans la langue de l'utilisateur (français par défaut).

## Cadre technique multi-timeframe

Travailler du haut vers le bas — le weekly donne le régime, le daily le setup, le 4h/1h le timing :

- **Structure** : direction de la tendance (higher highs/lows ou lower highs/lows), supports et résistances clés, précédents hauts/bas, nombres ronds. La structure prime sur les indicateurs.
- **Moyennes mobiles** : 20/50/200 et leur alignement (prix au-dessus/en-dessous, croisements, pente). Une MM 200 daily qui monte avec le prix au-dessus = régime haussier de fond.
- **RSI** : niveau, mais surtout **divergences** prix/RSI aux extrêmes — souvent le premier signal d'essoufflement.
- **MACD** : croisements et momentum, en confirmation, pas en signal isolé.
- **Volume** : un breakout sans volume est suspect ; une cassure confirmée par le volume a beaucoup plus de poids.
- **Crypto en plus** : funding rates (funding très positif = long crowdé, risque de squeeze baissier), open interest (OI qui gonfle sur un niveau = carburant de liquidations), zones de liquidation connues.
- **Chaque scénario reçoit un niveau d'invalidation explicite** — le prix auquel la thèse est fausse. Une analyse sans invalidation n'est pas une analyse.

## Risque événementiel (léger, pas d'analyse macro complète)

Avant de conclure, vérifier s'il y a dans les **1-2 prochaines semaines** un événement programmé capable de gapper à travers les niveaux : FOMC/BCE, CPI/NFP, earnings du titre analysé, décision réglementaire ou échéance crypto (via `calendar`, `economics` ou WebSearch). Le **signaler avec sa date** dans la sortie — sans développer l'analyse macro/fondamentale.

Si l'utilisateur veut le fond du dossier (santé financière, narratif, catalyseurs détaillés, potentiel 2-5 ans), c'est le skill **`analyse-fondamentale-ticker`** — le proposer plutôt que dupliquer.

## Format de sortie

Adapter la profondeur à la question (une question rapide → version compacte) :

**[Actif] — analyse technique au [date]**

1. **Snapshot** — prix actuel, variation (jour/semaine), position dans la plage 52 s, une ligne de contexte.
2. **Lecture par timeframe** — Weekly (régime), Daily (setup), 4h/1h (timing) : tendance, structure, signaux d'indicateurs notables.
3. **Niveaux clés** — supports et résistances chiffrés, avec leur origine (précédent haut, MM 200, nombre rond…).
4. **Scénarios** —
   - *Bullish* : trigger, cibles, invalidation.
   - *Bearish* : trigger, cibles, invalidation.
   - Dire de quel côté penche la balance des éléments **et pourquoi**.
5. **Risque événementiel** — les événements datés qui peuvent invalider cette lecture.
6. **Sources** — données et liens, horodatés.

## Garde-fous

- Analyse, pas conseil financier : le rappeler brièvement si l'utilisateur demande « j'achète/je vends ? », et mentionner alors la gestion du risque (taille de position, placement du stop).
- Ne pas surjouer la précision : cibles et probabilités sont des estimations, les marchés gappent à travers les niveaux.
- Si l'utilisateur pousse pour un call garanti, expliquer pourquoi aucun analyste honnête n'en donne — et livrer la lecture probabiliste la plus propre à la place.
- Si l'utilisateur partage une capture avec **plusieurs tracés concurrents** (deux channels, deux ranges) et demande lequel garder → skill `trader-analyze-range`, pas celui-ci.

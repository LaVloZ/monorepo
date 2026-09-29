# Design — Indicateur « Mean Departure » (validation visuelle)

**Date** : 2026-06-14
**Statut** : **validé visuellement** (v1 de référence, commitée). Haute précision /
recall partiel assumé — voir « Philosophie » plus bas.
**But** : valider à l'œil l'idée du screener « mean departure » AVANT d'investir dans un
backtest statistique. Si les setups marqués ne ressemblent pas à de bons points d'entrée
sur l'historique, on arrête là.

**Fichier** : `tradingview/mean_departure.pine`.

## Concept

Repérer une **continuation de tendance après repli** : tendance haussière établie → le prix
retrace vers sa moyenne → on marque le moment où il **repart concrètement**, à condition que
le momentum soit sain et que le volume ait confirmé la hausse précédente.

## Livrable

Un **seul indicateur Pine Script v5**, `overlay=true`, à coller dans TradingView et à
balader sur les graphes (daily par défaut, fonctionne sur tout timeframe). Aucune
dépendance, aucune install.

## Affichage

- **PAS de tracé des EMA** sur le graphe (l'utilisateur les a déjà dans TradingView).
  Les EMA sont calculées en interne uniquement pour la logique.
- **Marqueur** : triangle vert sous la barre, uniquement sur la barre de **départ**.
- **Tableau** dans un coin : les 4 conditions en ✓/✗ sur la barre courante + une ligne
  « DÉPART ».

## Les 4 conditions (état = setup armé)

1. **Tendance haussière** : `EMA50 > EMA200` ET `close > EMA200`.
2. **Repli récent** : dans les ~20 dernières barres, le prix a plongé de **2 à 50 %** sous
   le plus-haut ~20 barres ET est passé **sous l'EMA20** (en restant > EMA200).
   → flag « un repli sain a eu lieu récemment ». (Plafond porté de 12 % à **50 %** : les
   valeurs momentum retracent souvent jusqu'aux Fibo 50-61,8 % avant de repartir.)
3. **Momentum sain** sur le 3/10 de Raschke (`osc = EMA3 − EMA10`) : le **pic de
   l'oscillateur sur la fenêtre récente** (`swingLen` barres) doit être **≥** le pic sur la
   fenêtre précédente. Sommets ascendants → momentum qui se renforce (sain). Pic récent plus
   bas → essoufflement → rejet. **Fenêtres glissantes, pas de pivots → pas de retard.**
4. **Volume confirmant** : volume moyen des barres haussières > barres baissières (sur la
   fenêtre) ET > SMA20 du volume.

## Déclenchement (le marqueur)

`TRIGGER` = conditions **1, 3, 4** vraies sur la barre courante
**+** un repli qualifiant (cond. 2) survenu dans les **N** dernières barres
**+** `close` repasse **au-dessus de l'EMA20** sur cette barre (le « départ »).

Note : sur la barre de départ, le prix est de nouveau au-dessus de l'EMA20, donc la
sous-condition « sous l'EMA20 » de la cond. 2 n'est plus vraie *sur cette barre* — c'est
pourquoi la cond. 2 est évaluée comme « un repli a eu lieu **récemment** » (flag sur fenêtre
glissante), pas sur la barre courante.

## Paramètres (inputs TradingView, calibrables en live)

- Longueurs EMA : 20 / 50 / 200 (et 3 / 10 / 16 pour Raschke).
- Fenêtre de repli / recence : défaut **20** / **15** barres.
- Profondeur du repli : min **2 %**, max **50 %**.
- **`swingLen`** (fenêtre swing momentum) : défaut **10** barres. **Le bouton clé** de la
  cond. 3 : règle la séparation entre « pic récent » et « pic précédent » de l'oscillateur.
- Fenêtre volume : défaut 20 barres.
- `gateCond3` : la cond. 3 bloque-t-elle le signal (on) ou est-elle seulement affichée (off,
  mode diagnostic).

## Leçon : pourquoi la cond. 3 n'utilise PAS de divergence par pivots

Première implémentation : divergence baissière classique (prix plus-haut plus haut + osc
plus-haut plus bas) sur `ta.pivothigh`. **Quatre échecs successifs** avant de comprendre
deux choses :

1. **Mauvaise définition.** « Divergence » au sens de l'utilisateur = l'**oscillateur** fait
   des sommets plus bas (momentum qui s'essouffle), *peu importe le prix*. La divergence du
   manuel exige un plus-haut de prix plus haut — elle rate les cas où le prix fait un
   plus-bas plus haut mais le momentum meurt (cas réel observé).
2. **Retard structurel.** `ta.pivothigh` ne confirme un sommet que `right` barres après. Or
   l'entrée se déclenche juste après le sommet → le sommet décisif n'est pas encore confirmé
   au moment de l'entrée. Aucun réglage de fenêtre ne corrige ça.

→ Solution retenue : comparaison de **pics d'oscillateur sur fenêtres glissantes**
(`ta.highest`), qui mesure « sommets ascendants » directement, **sans pivot et sans lag**.
Validée sur un exemple étiqueté (entrées 👍 captées, entrée 👎 d'essoufflement rejetée).

## Philosophie : heuristique > précision absolue

Le signal capte « pas mal » de mouvements, **pas tous** — et c'est voulu. Trop de cas
particuliers rendent une détection exhaustive quasi impossible à coder proprement. On
préfère un filet **à haute précision** (les prises sont fiables, taux de réussite post-entrée
élevé) quitte à laisser passer des coups (recall partiel). Un signal heuristique « assez
bon » bat un signal « parfait » impossible.

## Hors périmètre v1 (idées futures)

- **Niveaux de Fibonacci** pour affiner la zone de repli / le déclenchement.
- Backtest statistique Python (event study) — viendra SI la validation visuelle est
  concluante.
- Étape 1 screener TradingView natif + étape 2 Python (le pipeline complet de l'idée
  d'origine).

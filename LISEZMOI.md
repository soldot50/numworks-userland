# NumWorks Userland — N0120, firmware 26.4.0

Méthode officielle de NumWorks : on compile ton propre « userland » (l'interface) avec le noyau officiel,
et on l'installe dans l'emplacement de firmware inactif (A ou B). Après un reset, la calculatrice revient au système officiel.

## Contenu
- `.github/workflows/main.yml`  : compilation automatique sur GitHub (gratuit si le dépôt est public)
- `COPIER_DANS_GITHUB__main.yml` : même fichier, visible (si le dossier caché .github ne s'envoie pas)
- `web/index.html`               : site pour installer sur la calculatrice (WebUSB, Chrome/Edge)
- `theme/userland_theme.json`    : couleurs du thème (jaune NumWorks)
- `serve.sh`                     : test local du site (facultatif)

## Étapes (tout se fait dans le navigateur)
1. Dépôt GitHub **public** (ex. numworks-userland). Supprime les anciens fichiers du dépôt.
2. Envoie `web/` et `theme/` (glisser-déposer). Pour le workflow : *Add file > Create new file*,
   nom `.github/workflows/main.yml`, colle le contenu de `COPIER_DANS_GITHUB__main.yml`, Commit.
3. *Settings > Pages > Source : GitHub Actions*.
4. *Actions > Build Userland > Run workflow* (la version est déjà 26.4.0).
5. Télécharge l'artefact `userland-n0120` (userland.A.dfu et userland.B.dfu).
6. Ouvre https://TON_PSEUDO.github.io/NOM_DU_DEPOT/ , branche la calculatrice allumée
   (message « La calculatrice est connectée »), ferme my.numworks.com, clique Connecter,
   choisis le .dfu de l'emplacement détecté, Installer.

## Limites (honnêteté)
- Rien n'a été testé sur une vraie N0120. Le build peut demander des corrections au premier essai.
- Le build utilise `make -C epsilon PLATFORM=n0120 EPSILON_VERSION=26.4.0` (structure actuelle du dépôt NumWorks). Si le code récent est plus neuf que ton firmware, le noyau peut refuser de lancer le userland.
- Le thème ne s'applique que si les noms de couleurs existent dans escher/include/escher/palette.h.
- Le menu complet de la maquette (bloc-notes, minuteur, mini-jeux...) reste à programmer après validation du circuit.

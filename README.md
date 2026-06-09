# IslandViewerStudents

Rapport — Island Viewer

la code à été réalisé dans le langage c++ sous les platformes windows et macos.

1. les choix algorithmiques

a. Bruit fractal
Le bruit de base utilisé est le bruit de Perlin, via la fonction glm::perlin() de la bibliothèque GLM. Bien que plus lisse que le bruit de valeur simple, le Perlin seul manque de détail. Pour y remédier, nous avons implémenté un FBM (Fractional Brownian Motion), aussi appelé bruit fractal.
Paramètres retenus et impact visuel:

• Octaves (1–8) : chaque octave ajoute un niveau de détail.
1 octave : le terrain est très lisse et générique.[octave 1](image-2.png)
5–6 octaves : on obtient des reliefs naturels avec côtes dentelées et collines internes. [octave 5](image-3.png)
Au-delà de 7 : le terrain devient excessivement bruité.[octave 8](image-4.png)

• Lacunarité (1–4) : contrôle l'écart de fréquence entre octaves.
À 2.0 , chaque octave est deux fois plus fine. Des valeurs proches de 1.0 donnent un terrain très doux et homogène.![lacunarité 2](<capture readme/lacunarité2.png>) ![lacunarité 4](<capture readme/lacunarité4.png>)

• Gain (0.0–1.0) : facteur multiplicateur de l'amplitude pour chaque octave.
0.5 signifie que l'amplitude de chaque octave est réduite de moitié par rapport à la précédente.![gain 0.5](<capture readme/gain0.5.png>)
Un gain élevé (> 0.7) rend les détails fins aussi importants que la forme générale, créant un terrain très chaotique.![gain 0.8](<capture readme/gain0.8.png>)

• Scale (0.01–10.0) :facteur d'échelle pour contrôler la "taille" du bruit généré.
Une grande valeur agrandit les détails![scale 2](<capture readme/scale2.png>)
une petite valeur étale le relief![scale 8.6](<capture readme/scale8.6.png>)

b. Génération de heightmap et couleurs

La heightmap est générée dans un format float 32 bits par pixel . Sur l'image la valeur de chaque pixel est une altitude. (PIXELFORMAT_UNCOMPRESSED_R32), ce qui évite toute perte de précision lors de la génération du mesh.
Chaque pixel est calculé par la fonction suivante :
• Calcul du bruit FBM → génère des formes aléatoires naturelles (comme des montagnes ou des nuages) valeur dans [0, 1].
• Application du masque radial (island mask) : Plus on s'éloigne du centre, plus on force l'altitude à descendre vers 0. Cela garantit que les bords de la carte sont sous l'eau.
• Clamp au waterLevel : toute valeur en dessous du niveau d'eau est forcée à waterLevel, aplatissant le fond marin.

La mise en couleur (Coloration par palier)
La conversion heightmap → couleur utilise un système de zones avec interpolation linéaire (glm::mix) entre les paliers, évitant les transitions brusques :
• v < 0.28 : eau profonde (bleu acier, 70/130/180).
• 0.28–0.30 : transition eau → sable (2% de la plage de hauteur seulement), créant une plage étroite et réaliste.
• 0.30–0.55 : transition sable → herbe, la zone la plus large correspondant aux plaines côtières et aux collines basses.
• v > 0.55 : transition herbe → roche claire, représentant les hauteurs et sommets.
L'utilisation de glm::mix() plutôt que de couleurs solides par palier donne un aspect naturel en évitant les contours artificiels entre biomes.

c. Distribution de points par Poisson disk sampling

Cet alogorithme génère des points aléatoires avec une contrainte : deux points ne peuvent jamais être plus proches que la distance r. L'idée principal est une liste active de points "candidats" qui génèrent des voisins, et une grille d'accélération pour tester rapidement les collisions.
les 4 étapes du code en résumé :

-Grille d'accélération (cellSize = r / √2) — chaque cellule ne contient qu'un seul point, donc chercher des voisins se réduit à regarder les 25 cellules autour (5×5).

-Point de départ — tirage aléatoire jusqu'à trouver un point isOnIsland().
Filtrage sur l'île
Un filtre isOnIsland() rejette tout candidat dont la hauteur dans la heightmap est inférieure au waterLevel. Le point de départ est lui-même cherché par tirage aléatoire avec jusqu'à 10 000 tentatives. Les points sont ainsi automatiquement contraints à la surface terrestre, sans post-traitement.

-Boucle principale — un point actif génère k candidats dans l'anneau [r, 2r]. Si le candidat est sur l'île et assez loin de tous ses voisins → ajouté. Sinon → ignoré. Si aucun candidat ne passe après k essais → le point est retiré de la liste active.
maxAttempts (k) : nombre de candidats testés autour de chaque point actif avant de le retirer de la liste active. La valeur standard k=30 est un bon compromis entre qualité de remplissage et temps de calcul. Un k élevé donne un remplissage quasi-optimal mais ralentit la génération.
minDistance (r) : détermine la densité maximale. Une valeur faible produit des centaines d'arbres/objets très proches ; une valeur grande donne une distribution clairsemée. C'est le principal levier de contrôle de la densité.

-Fin — quand la liste active est vide, le terrain est saturé.
![poisson disk sampling](<capture readme/poissondisksampling.png>)

Les points doivent estres réinistialiser a chaque dois que je change un paramètre sinon les points sont moins visualiser comme sur cet exemple quand j'augmente le scale. ![scale augmenté](<capture readme/disksampligscaleevoluer.png>)


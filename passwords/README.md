# passwords

Comparaison de deux façons de modéliser la mise à jour d'un mot de passe : `classic` (modèle anémique, la logique dans `UserService`) et `rich1` (modèle plus riche).

- Java 17, Spring Boot 3.4, jOOQ, Liquibase, HSQLDB
- Un test par variante : `ClassicUpdatePasswordTest`, `Rich1UpdatePasswordTest`

```
./gradlew test
```

# mars-rover

Mars Rover kata : un rover sur une grille reçoit une suite de commandes (`L`, `R`, `M`), tourne, avance, et s'arrête devant un obstacle.

- Java, Gradle, JUnit 5, AssertJ
- Pattern Command (`MoveCommand`, `LeftCommand`, `RightCommand`) et State (`MovableState`, `BlockedState`)

```
./gradlew test
```

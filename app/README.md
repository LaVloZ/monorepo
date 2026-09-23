# app

Expérimentation (2018) d'une architecture micro-services en Java, multi-modules Maven.

- `api-gateway` : point d'entrée (`Main`)
- `user` : domaine utilisateur (`User`, `UserFactory`, `UserRepository`, persistance CouchDB)
- `shared/shared-domain` : abstractions communes (événements, `CrudRepository`)
- `shared/couchdb-client` : client CouchDB maison sur Apache HttpClient + JSON-B
- `shared/shared-ms` : communication inter-services par JMS/AMQP (Qpid)

Tests : JUnit 5, Mockito, AssertJ.

```
mvn test
```

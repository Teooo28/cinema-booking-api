# Sistem de Rezervari Cinema - Web API

### 1. Descrierea Proiectului
Acest proiect este o aplicatie backend C++ creata pentru a gestiona sistemul de rezervari al unui cinematograf. Sistemul ruleaza ca un Web API (folosind framework-ul Crow), permitand interogarea filmelor, cumpararea de bilete si anularea rezervarilor prin cereri HTTP. Proiectul pune in practica concepte avansate de Programare Orientata pe Obiecte (POO), Design Patterns si persistenta datelor folosind o baza de date SQLite.

### 2. Structura Ierarhica a Proiectului si Design Patterns

Sistemul are codul impartit logic, folosind principii arhitecturale clare:

**Ierarhia de Baza (Filme)**
```text
Event (Abstracta)
 ├── Movie2D
 └── Movie3D
```
* **Event:** Clasa de baza de la care pleaca ierarhia. Contine datele comune (ID, titlu, pret de baza, locuri disponibile) si o variabila statica `totalEventsCreated` pentru a tine evidenta globala a tuturor instantelor generate in sistem, accesibila prin metoda statica `static int getTotalEventsCreated()`. Aici este definita metoda virtuala `getFinalPrice()`.
* **Movie2D & Movie3D:** Clase derivate care suprascriu calculul pretului (ex: `Movie3D` adauga o taxa suplimentara pentru ochelari).

**Design Patterns Implementate**
```text
DiscountStrategy (Interfata)
 ├── NoDiscount
 └── StudentDiscount
```
* **Strategy Pattern:** Implementat prin interfata `DiscountStrategy`. Permite schimbarea dinamica a modului in care se calculeaza pretul (ex: aplicarea reducerii pentru studenti) injectand o clasa diferita la runtime, fara a modifica logica clasei `Event` si fara a folosi instructiuni `if` complexe.
* **Factory Pattern:** Clasa `EventFactory` este responsabila pentru crearea instantelor corecte de filme (2D sau 3D), ascunzand logica de instantiere de restul programului.

**Clase de Management si Utilitare**
* **CinemaRepository:** Gestioneaza stocarea datelor. Sincronizeaza datele din memoria RAM (un `std::vector<std::unique_ptr<Event>>`) cu hard disk-ul, executand interogari SQL (INSERT, UPDATE) prin SQLite.
* **ApiResponse\<T\>:** O clasa *Template* utilizata pentru a standardiza absolut toate raspunsurile trimise catre client sub forma unui pachet JSON uniform (succes, mesaj, date).
* **ExceptiiCustom:** Clase derivate din `std::exception` create pentru a gestiona erorile de business (`InvalidDataException`, `EventNotFoundException`).

**Integrarea Bibliotecilor Externe**
* **Crow (C++ Microframework):** Utilizat pentru a expune functionalitatile claselor pe internet. Gestioneaza un server HTTP multithreaded si rute RESTful (GET, POST, DELETE).
* **SQLite3:** Baza de date relationala integrata direct in aplicatie (fara server separat). Clasa `CinemaRepository` converteste obiectele C++ in comenzi SQL (INSERT, UPDATE) pentru a stoca si actualiza inventarul biletelor pe disc.
* **Nlohmann/json:** Utilizata pentru parsarea si construirea pachetelor de date complexe care circula pe retea (serializare/deserializare intre obiecte C++ si format text web).

### 3. Concepte POO Implementate

* **Polimorfism la Runtime**
    * **Metode Virtuale:** Functia `getFinalPrice()` se comporta diferit in functie de tipul obiectului (Movie2D vs Movie3D). Sistemul calculeaza costurile automat prin polimorfism.
    * **Injectia Dependentelor:** Interfata `DiscountStrategy` este apelata polimorfic in interiorul functiei de pret a evenimentului.
* **Incapsulare si Abstractizarea Datelor**
    * S-a respectat principiul ascunderii datelor. Atributele sensibile ale filmelor sunt `private` sau `protected`, fiind modificate doar prin metode valide (ex: functia `bookSeats()` valideaza numarul de bilete inainte de a scadea locurile).
* **Gestiunea Moderna a Resurselor (Smart Pointers)**
    * Spre deosebire de gestionarea manuala a memoriei, acest proiect previne scurgerile de memorie (*memory leaks*) folosind **pointeri inteligenti** din standardul C++ modern: `std::unique_ptr` pentru stocarea unica a evenimentelor in Repository si `std::shared_ptr` pentru partajarea strategiilor de discount.

### 4. Functionalitati si Exemple de Utilizare (API Endpoints)

Aplicatia expune urmatoarele rute RESTful care pot fi accesate din terminal folosind utilitarul `curl`:

**1. Afisarea filmelor (GET `/movies`)**
Returneaza un array JSON cu toate filmele disponibile, incluzand preturile dinamice si locurile ramase in sala.
```bash
curl -X GET http://localhost:8080/movies
```

**2. Rezervarea biletelor (POST `/movies/<id>/book`)**
Clientul trimite un JSON cu numarul de bilete dorite si statusul (optional, pentru discount). Sistemul aplica reducerea, verifica locurile, salveaza in SQLite si genereaza un Cod Unic de Rezervare.
*Exemplu comanda (2 bilete pentru student la filmul cu ID 2):*
```bash
curl -X POST http://localhost:8080/movies/2/book -H "Content-Type: application/json" -d "{\"bilete\": 2, \"status\": \"student\"}"
```
*Raspunsul serverului:*
```json
{"data":"Ai rezervat 2 bilete. Total de plata: 70.00 RON. Cod intrare: #TKT-4892","message":"Rezervare Confirmata","success":true}
```

**3. Anularea rezervarii (DELETE `/cancel`)**
Clientul trimite doar codul primit pe bilet. Sistemul consulta registrul intern (`std::unordered_map`), identifica rezervarea, returneaza locurile in baza de date si invalideaza codul pentru a preveni fraudele.
*Exemplu comanda:*
```bash
curl -X DELETE http://localhost:8080/cancel -H "Content-Type: application/json" -d "{\"cod_intrare\": \"#TKT-4892\"}"
```
*Raspunsul serverului:*
```json
{"data":"Au fost returnate 2 locuri pentru codul #TKT-4892","message":"Anulare reusita","success":true}
```

### 5. Rulare si Testare

Proiectul foloseste **CMake** pentru build. Necesita un compilator modern de C++ (minim C++14).
* Librarii externe integrate: `Crow` (pentru serverul web HTTP), `nlohmann/json` (pentru parsarea JSON) si `sqlite3` (pentru baza de date).

### 6. Fluxul Aplicatiei

La rularea executabilului, fluxul este urmatorul:
1. Se instantiaza baza de date SQLite. Daca fisierul `cinema.db` nu exista pe disc, este creat automat.
2. Serverul Crow porneste in regim *multithreaded* pe portul `8080` si asculta cereri din internet.
3. La primirea unui request (ex: POST rezervare), input-ul este validat riguros (se verifica sintaxa JSON si regulile de business).
4. Daca datele sunt invalide (ex: se cer mai multe bilete decat capacitatea), o exceptie custom este aruncata, prinsa in blocul `try-catch`, iar serverul raspunde elegant cu un cod HTTP 400 (Bad Request).
5. Daca tranzactia are succes, modificarea are loc in RAM, se noteaza in registrul aplicatiei, iar `CinemaRepository` executa comanda SQL de UPDATE pentru a stoca permanent noul numar de locuri.
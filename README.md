# Symulacja systemu obsługi pasażerów i promów

Symulacja odprawy pasażerów oraz obsługi promów, uwzględniająca kontrolę bagażu, kontrolę bezpieczeństwa,sygnały kapitana portu oraz obsługę pasażerów VIP.

## 1. Temat projektu

Symulacja systemu odprawy pasażerów oraz obsługi promów z uwzględnieniem:

- kontroli bagażu
- kontroli bezpieczeństwa
- sygnałów kapitana portu
- obsługi pasażerów VIP

### 1.1 Procesy występujące w projekcie

- Kapitan Portu
- Kapitan Promu
- Pasażer

## 2. Testy jednostkowe

### TEST 1 — Limit bagażu i sygnał2

Cel
: Zweryfikować, że mechanizm odprawy odrzuca pasażerów z nadwagą bagażu i że po otrzymaniu sygnału2 wpuszczanie nowych pasażerów jest zatrzymywane.

Parametry

- Limit miejsc na promie (P): 5
- Dopuszczalny bagaż (Mp): 10 kg
- VIP: brak (0)
- Sygnał2 wysyłany po: 4 s

Dane wejściowe (wag pasażerów)

- 8, 12, 9, 11, 10, 7

Oczekiwany wynik

- Pasażerowie z wagą > 10 kg są odrzucani (log: „bagaż odrzucony”).
- Po sygnale2 nowe procesy pasażerów nie wchodzą do sekcji odprawy.
- Prom załaduje maksymalnie 5 osób spełniających warunki.
- Brak procesów wiszących; brak oczekiwania na nieistniejące sygnały.

### TEST 2 — Kontrola bezpieczeństwa

Cel
: Sprawdzić mechanizmy kolejkowania i kontroli bezpieczeństwa: przypisanie do stanowisk według płci, limity stanowisk oraz ograniczenie przepuszczania.

Parametry

- Kolejność płci: K, K, M, M, K, K, M
- Liczba stanowisk: 3
- Limit na stanowisko: 2 osoby
- Trap K (maks. przepuszczeń): 3
- VIP: brak

Oczekiwany wynik

- Brak mieszania płci na stanowiskach (log: przypisanie do stanowiska).
- Osoba, która przepuściła 3 osoby, nie może przepuścić czwartej.
- Wszystkie stanowiska wykorzystane zgodnie z limitem.
- Brak zakleszczeń wynikających z oczekiwania na „pasującego pasażera”.

### TEST 3 — Priorytet VIP i sygnał1

Cel
: Zweryfikować priorytet VIP przy wejściu oraz działanie trapu o niskiej przepustowości w połączeniu z wymuszonym wypłynięciem (sygnał1).

Parametry

- P = 8
- Trap K = 2
- VIP: 3 osoby
- Kolejność: N, N, VIP, N, VIP, VIP, N
- Sygnał1 wysłany przy zajętym trapie

Oczekiwany wynik

- VIP wchodzą przed zwykłymi (log: „VIP priorytet”).
- Po sygnale1 prom nie przyjmuje nowych pasażerów.
- Prom czeka na opróżnienie trapu, potem wypływa.
- Pozostali pasażerowie nie są blokowani — czekają na kolejny prom.

### TEST 4 — Obsługa wielu promów

Cel
: Sprawdzić rotację promów oraz zmianę parametrów limitów bagażu (Mdi) i czasów powrotu (Ti).

Parametry

- Liczba promów (N): 3
- P = 10
- Limity bagażu per prom: 8 kg, 12 kg, 9 kg
- Czasy powrotów (Ti): 10 s, 7 s, 12 s
- Pula pasażerów: 40 osób (wagi 5–15 kg)

Oczekiwany wynik

- Każdy prom działa z własnym limitem Mdi.
- Po odpłynięciu prom oznaczany jako „niedostępny”; po Ti znów jest „dostępny”.
- Kolejne promy podmieniane zgodnie z rotacją.
- Grupa 40 pasażerów zostaje przewieziona po kilku cyklach bez zatorów.

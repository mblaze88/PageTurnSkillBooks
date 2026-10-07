# PageTurnSkillBooks (SKSE, Skyrim AE 1.6.1170)

Książki z bonusem do umiejętności (np. Destruction) nie dają go już w chwili
otwarcia. Bonus pojawia się dopiero po przewróceniu kilku stron (domyślnie 3).
Zamknięcie książki wcześniej = brak bonusu, a przy następnym otwarciu liczymy od nowa.
Księgi czarów i zwykłe książki działają jak w vanilli.

## Wymagania (dependencies)

- Skyrim Anniversary Edition **1.6.1170**
- **SKSE64** dla 1.6.1170 (2.2.6)
- **Address Library for SKSE Plugins** (wersja All-in-One dla AE)

Żadne inne mody nie są potrzebne.

## WAŻNE: to jest kod źródłowy, nie gotowy DLL

W środowisku, w którym pracuję, nie ma kompilatora Windows (MSVC), więc nie mogłem
zbudować pliku `.dll` ani przetestować go w grze. API (nazwy klas, flagi, ID funkcji)
sprawdziłem w nagłówkach CommonLibSSE-NG, ale kod nie był kompilowany ani uruchamiany.

### Jak dostać DLL bez instalowania Visual Studio (GitHub Actions)

1. Załóż darmowe konto na github.com i utwórz nowe repozytorium (może być prywatne).
2. Wgraj do niego całą zawartość tego folderu (z ukrytym folderem `.github`).
3. Zakładka **Actions** → workflow **Build PageTurnSkillBooks.dll** → **Run workflow**
   (uruchomi się też samo po wgraniu).
4. Po kilku–kilkunastu minutach pobierz artefakt **PageTurnSkillBooks**. W środku jest
   gotowa struktura `Data/SKSE/Plugins/` z DLL i plikiem INI.

### Albo lokalnie (Visual Studio 2022 + vcpkg)

```
git clone https://github.com/CharmedBaryon/CommonLibSSE-NG.git extern/CommonLibSSE-NG
git -C extern/CommonLibSSE-NG checkout b93280e
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md
cmake --build build --config Release
```

## Instalacja

Skopiuj `PageTurnSkillBooks.dll` i `PageTurnSkillBooks.ini` do
`Skyrim Special Edition\Data\SKSE\Plugins\` (albo zainstaluj jako mod w MO2/Vortex,
zachowując strukturę folderów). Log: `Documents\My Games\Skyrim Special Edition\SKSE\PageTurnSkillBooks.log`.

## Konfiguracja (`PageTurnSkillBooks.ini`)

- `iPageTurns` – ile stron do przodu (domyślnie 3)
- `sForwardEvents` / `sBackEvents` – nazwy zdarzeń wejścia liczonych jako strona do przodu/do tyłu
- `bLogInput=1` – zapisuje do logu nazwy wciskanych klawiszy w książce (pomocne, gdy strony nie są liczone)

## Jak to działa

Plugin podpina się (MinHook) pod `TESObjectBOOK::Read`. Przy otwarciu książki
z umiejętnością wywołuje oryginał z chwilowo zdjętą flagą „uczy umiejętności”,
a potem ją przywraca. Zdarzenia wejścia liczą przewrócone strony, a po progu plugin
wywołuje `Read` jeszcze raz już normalnie. Bonus, komunikat i zapis stanu robi więc
sama gra, bez własnego pliku zapisu.

## Znane ograniczenia i ryzyka (nietestowane)

- Nie mogłem sprawdzić w grze, czy wywołanie `Read` poza momentem otwarcia książki
  zachowuje się dokładnie jak oczekiwano. To pierwsza rzecz do przetestowania.
- Plugin nie wie, ile książka ma stron. Jeśli ma ich mniej niż `iPageTurns`, nie da
  bonusu – wtedy zmniejsz próg.
- Kliknięcie myszą jest liczone jako strona do przodu (nie wiadomo, po której stronie
  kliknięto). Klawiatura i pad liczą się dokładnie, o ile nazwy zdarzeń są trafne.
- Statystyka „przeczytane książki” może zliczyć jedną książkę dwa razy.
- Inne mody hakujące `TESObjectBOOK::Read` mogą kolidować.
- Przed pierwszym użyciem zrób kopię zapisu gry.

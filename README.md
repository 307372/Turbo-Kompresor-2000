# Turbo Kompresor 2000

## 1. Czym jest Turbo Kompresor 2000?
- Turbo Kompresor 2000 to prosty archiwizer z minimalistycznym graficznym interfejsem użytkownika, wykonanym w Qt ze stylesheetami z https://github.com/Alexhuszagh/BreezeStyleSheets.

## 2. Jakie algorytmy zostały zaimplementowane?
Z algorytmów kompresji bezstratnej:
- Burrows-Wheeler transform
- move-to-front
- run-length encoding
- arithmetic coding (2 wersje)
- Asymmetric Numeral Systems (range variant)

Do sprawdzania poprawności działania, zastosowałem:
- CRC32
- SHA-1
- SHA-256

Szyfrowanie jest realizowane za pomocą AES-128 CTR

## 3. Co jest potrzebne do skompilowania tego programu?
- C++20 (stosowałem g++)
- Qt6
- libdivsufsort
- Crypto++

## 4. Harmonogram pracy
Zrobione:
- 16.03.2021 - Przygotowanie kodu do tego projektu - refaktoryzacja
- 01.04.2021 - Implementacja DC3 / Skew lub innego algorytmu generowania suffix array działającego w O(n) i opracowanie wersji transformaty Burrowsa-Wheelera niewymagającej podwojenia danych wejściowych do poprawnego działania
- 15.04.2021 - Implementacja AES
- 22.04.2021 - Dodanie HMAC i KDF
- 13.05.2021 - Modyfikacje pozwalające na zaszyfrowanie i odszyfrowanie pliku z poziomu GUI
- 27.05.2021 - Implementacja ranged Asymmetric Numeral Systems (rANS)
- 10.06.2021 - Optymalizacja dodanych algorytmów
- 17.06.2021 - Ponowna refaktoryzacja, ogólne poprawienie jakości i czytelności nowego kodu

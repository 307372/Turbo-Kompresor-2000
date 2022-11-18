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

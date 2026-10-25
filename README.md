# CYD-Buddy

Ein Wesen mit großen Augen auf dem ESP32-2432S028 (Cheap Yellow Display, Variante mit 2 USB-Buchsen). Es schaut sich um, blinzelt, freut sich über Streicheln und schläft im Dunkeln.

- Bauen: `pio run -e cyd2usb`
- Flashen: `pio run -e cyd2usb -t upload`
- Tests (am PC): `pio test -e native`

Design und Pläne: `docs/`. Vorlage für das Online-Update: `reference/ha_buddy_ota/`.

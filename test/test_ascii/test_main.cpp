#include <unity.h>
#include "logic/ascii.h"
void setUp() {}
void tearDown() {}

void test_umlauts() {
  char b[64];
  ascii_de("Lädt 5 %", b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Laedt 5 %", b);
  ascii_de("Prüft die Datei …", b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Prueft die Datei ...", b);
  ascii_de("Update bereit – tippen", b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Update bereit - tippen", b);
  ascii_de("Größe ÄÖÜ", b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Groesse AeOeUe", b);
}
void test_truncates() {
  char b[5];
  ascii_de("Müller", b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("Muel", b);
  ascii_de(nullptr, b, sizeof b);
  TEST_ASSERT_EQUAL_STRING("", b);
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_umlauts);
  RUN_TEST(test_truncates);
  return UNITY_END();
}

#pragma once

#include <cstdint>
#include <string>

namespace esphome {
namespace sensmos {

// Jedno zadanie HTTP wykonywane przez WSPÓLNY, trwały worker (publish + readbacki).
// run() robi blokujące HTTP i dostarcza wynik (zdefiniowane w pliku komponentu — bez zależności krzyżowych).
struct SensmosJob {
  void (*run)(SensmosJob *);
  void *self;
  std::string url;
  std::string body;
};

// Kolejkuje zadanie do TRWAŁEGO workera (task tworzony RAZ, nigdy nie kasowany → brak churnu/fragmentacji).
// stack_bytes — rozmiar stosu workera, użyty TYLKO przy pierwszej inicjalizacji (HTTP ~4 KB wystarcza,
// HTTPS/TLS potrzebuje ~10 KB). Stos alokowany jednorazowo ze sterty → RAM zajęty tylko gdy komponent
// używany (inaczej niż static .bss, które rezerwuje pamięć zawsze i może uwalić boot na ciasnym nodzie).
// Zwraca false gdy worker/kolejka nie wstały albo kolejka pełna. Wołane z pętli głównej (jednowątkowo).
bool net_submit(SensmosJob *job, uint32_t stack_bytes);

}  // namespace sensmos
}  // namespace esphome

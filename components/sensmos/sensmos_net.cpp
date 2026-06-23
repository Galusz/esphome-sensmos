#include "sensmos_net.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

namespace esphome {
namespace sensmos {

static QueueHandle_t g_queue = nullptr;

// Trwały worker (tworzony RAZ). Nie ma alokacji/zwalniania stosu per request, więc sterta się
// nie fragmentuje (to był powód crashy OOM po kilku godzinach). Stos alokowany jednorazowo.
static void worker(void *) {
  for (;;) {
    SensmosJob *job = nullptr;
    if (xQueueReceive(g_queue, &job, portMAX_DELAY) == pdTRUE && job != nullptr) {
      job->run(job);   // blokujące HTTP + finish()/finish_body()
      delete job;      // mała struktura — nie fragmentuje jak stos 4–10 KB
    }
  }
}

bool net_submit(SensmosJob *job, uint32_t stack_bytes) {
  // leniwa, jednorazowa inicjalizacja (wołane z pętli głównej → bez wyścigu)
  if (g_queue == nullptr) {
    QueueHandle_t q = xQueueCreate(3, sizeof(SensmosJob *));
    if (q == nullptr)
      return false;
    // ESP-IDF: usStackDepth jest w BAJTACH. Worker żyje wiecznie → jedna alokacja stosu, zero churnu.
    if (xTaskCreate(worker, "sensmos_net", stack_bytes, nullptr,
                    tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
      vQueueDelete(q);
      return false;  // brak RAM na stos teraz → spróbuje przy kolejnym submit
    }
    g_queue = q;  // ustaw dopiero gdy worker wstał
  }
  return xQueueSend(g_queue, &job, 0) == pdTRUE;
}

}  // namespace sensmos
}  // namespace esphome

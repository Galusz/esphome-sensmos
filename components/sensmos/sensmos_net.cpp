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
  // leniwa, jednorazowa inicjalizacja (wołane z pętli głównej)
  if (g_queue == nullptr) {
    // g_queue MUSI być ustawione PRZED utworzeniem workera: worker od razu woła
    // xQueueReceive(g_queue), a na dual-core task startuje zanim wrócimy z xTaskCreate.
    g_queue = xQueueCreate(3, sizeof(SensmosJob *));
    if (g_queue == nullptr)
      return false;
    // ESP-IDF: usStackDepth jest w BAJTACH. Worker żyje wiecznie → jedna alokacja stosu, zero churnu.
    if (xTaskCreate(worker, "sensmos_net", stack_bytes, nullptr,
                    tskIDLE_PRIORITY + 1, nullptr) != pdPASS) {
      vQueueDelete(g_queue);
      g_queue = nullptr;
      return false;  // brak RAM na stos teraz → spróbuje przy kolejnym submit
    }
  }
  return xQueueSend(g_queue, &job, 0) == pdTRUE;
}

}  // namespace sensmos
}  // namespace esphome

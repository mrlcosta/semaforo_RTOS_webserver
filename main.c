#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"

#include "lib/ssd1306.h"
#include "lib/font.h"

#include "pio_matrix.pio.h"
#include "config.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

ssd1306_t ssd;
PIO pio = pio0;
uint sm;
volatile bool modo_diurno = true;
volatile bool alerta_sonoro = true;
volatile int estado_semaforo = VERDE;

double COORDENADA_VERDE[PIXELS][3] = {
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

double COORDENADA_AMARELO[PIXELS][3] = {
    {0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 1, 0}, {0, 0, 0}, {0, 0, 0}};

double COORDENADA_VERMELHO[PIXELS][3] = {
    {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0}};

void display_task()
{
    char modo[16];
    char message[32];
    char texto[32];

    while (true)
    {
        ssd1306_fill(&ssd, false); // Limpa o display

        ssd1306_rect(&ssd, 3, 3, 122, 60, true, false); // moldura

        // escreve o texto de acordo com o testado
        switch (estado_semaforo)
        {
        case VERMELHO:
            sprintf(modo, "Modo diurno");
            sprintf(texto, "Pare");
            ssd1306_draw_string(&ssd, texto, 45, 35);
            break;

        case AMARELO:
            sprintf(modo, "Modo diurno");
            sprintf(texto, "Atencao");
            ssd1306_draw_string(&ssd, texto, 40, 35);
            break;

        case VERDE:
            sprintf(modo, "Modo diurno");
            sprintf(texto, "Prossiga");
            ssd1306_draw_string(&ssd, texto, 35, 35);
            break;

        case NOTURNO:
            sprintf(modo, "Modo noturno");
            sprintf(texto, "Preste atencao");
            ssd1306_draw_string(&ssd, texto, 10, 35);
            break;
        }

        ssd1306_draw_string(&ssd, modo, 15, 15); // envia texto ao display
        ssd1306_send_data(&ssd);                 // atualiza o display
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void button_task() // verificação dos botões por pooling
{
    while (true)
    {

        if (!gpio_get(BOTAO_A))
        {
            modo_diurno = !modo_diurno; // Alterna entre o modo normal e noturno
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        // verificação para o modo BOOTSEL ao pressionar o botão B
        if (!gpio_get(BOTAO_B))
        {
            reset_usb_boot(0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void buzzer_task() // task do buzzer. atualiza de acordo com o estado do semáforo
{
    while (true)
    {
        if (alerta_sonoro)
        {
            switch (estado_semaforo)
            {
            case VERDE:
                buzzer(BUZZER_PIN, 1000);
                vTaskDelay(pdMS_TO_TICKS(3000));
                break;

            case AMARELO:
                buzzer(BUZZER_PIN, 100);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case VERMELHO:
                buzzer(BUZZER_PIN, 500);
                vTaskDelay(pdMS_TO_TICKS(1500));
                break;

            case NOTURNO:
                buzzer(BUZZER_PIN, 300);
                vTaskDelay(pdMS_TO_TICKS(2000));
                break;
            }
        }
    }
}

void led_task() // task do led RGB. atualiza de acordo com o estado do semáforo
{
    while (true)
    {
        switch (estado_semaforo)
        {
        case VERDE:
            gpio_put(LED_VERDE, true);
            gpio_put(LED_VERMELHO, false);
            vTaskDelay(pdMS_TO_TICKS(DELAY));

            if (modo_diurno)
            {
                estado_semaforo = AMARELO; // PROXIMO ESTADO
            }
            else
            {
                estado_semaforo = NOTURNO;
            }

            break;

        case AMARELO:
            gpio_put(LED_VERDE, true);
            gpio_put(LED_VERMELHO, true);
            vTaskDelay(pdMS_TO_TICKS(DELAY));

            if (modo_diurno)
            {
                estado_semaforo = VERMELHO; // PROXIMO ESTADO
            }
            else
            {
                estado_semaforo = NOTURNO;
            }
            break;

        case VERMELHO:
            gpio_put(LED_VERDE, false);
            gpio_put(LED_VERMELHO, true);
            vTaskDelay(pdMS_TO_TICKS(DELAY));

            if (modo_diurno)
            {
                estado_semaforo = VERDE; // PROXIMO ESTADO
            }
            else
            {
                estado_semaforo = NOTURNO;
            }
            break;

        case NOTURNO: // amarelo piscante
            gpio_put(LED_VERDE, true);
            gpio_put(LED_VERMELHO, true);
            vTaskDelay(pdMS_TO_TICKS(300));
            gpio_put(LED_VERDE, false);
            gpio_put(LED_VERMELHO, false);
            vTaskDelay(pdMS_TO_TICKS(2000));
            if (modo_diurno)
            {
                estado_semaforo = VERDE; // PROXIMO ESTADO
            }
            break;
        }
    }
}
void buzzer(uint pin, uint delay)
{                                                // ativa o buzzer
    uint slice_num = pwm_gpio_to_slice_num(pin); // escolhe o slice
    pwm_set_gpio_level(pin, 2048);               // ativa o buzer com duty 50%
    sleep_ms(delay);                             // delay
    pwm_set_gpio_level(pin, 0);                  // desliga o buzzer
}

int32_t rgb_matrix(double r, double g, double b)
{
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}
void pio_matrix(double coordenada[][3], int32_t valor, PIO pio, uint sm)
{
    for (int16_t i = 0; i < PIXELS; i++)
    {
        double r = coordenada[i][0];
        double g = coordenada[i][1];
        double b = coordenada[i][2];

        valor = rgb_matrix(r, g, b);
        pio_sm_put_blocking(pio, sm, valor);
    }
}

void matriz_led_task()
{
    while (true)
    {
        switch (estado_semaforo) // atualiza de acordo com o estado do semáforo
        {
        case VERDE:
            pio_matrix(COORDENADA_VERDE, 0, pio, sm);
            break;

        case AMARELO:
            pio_matrix(COORDENADA_AMARELO, 0, pio, sm);
            break;

        case VERMELHO:
            pio_matrix(COORDENADA_VERMELHO, 0, pio, sm);
            break;

        case NOTURNO:
            pio_matrix(COORDENADA_AMARELO, 0, pio, sm);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100)); // Atualiza o padrão a cada 100ms
    }
}

void user_request(char **request)
{
    if (strstr(*request, "GET /modo") != NULL)
    {
        modo_diurno = !modo_diurno; // Alterna entre o modo normal e noturno
    }
    if (strstr(*request, "GET /buzzer") != NULL)
    {
        alerta_sonoro = !alerta_sonoro; // Alterna entre o modo sonoro e sem som
    }
};

static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (!p)
    {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("GET: %s\n", request);
    user_request(&request);
    char html[1024];
    snprintf(html, sizeof(html),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title>Semaforo IOT</title>\n"
             "<style>\n"
             "body { background-color: #8aefff; max-width: 600px; margin: 0 auto; padding: 2rem; line-height: 1.5; }\n"
             "h1 { text-align: center; margin-bottom: 2rem; }\n"
             "form { margin: 0.5rem 0; text-align: center; }\n"
             "button { padding: 15px; font-size: 1rem; background: #ffa807; cursor: pointer; border: none; border-radius: 10px;}\n"
             "button:hover { background: #f0853e; }\n"
             ".texto { text-align: center; margin-top: 2rem; font-size: 1.25rem; }\n"
             "</style>\n"
             "</head>\n"
             "<body>\n"
             "<h1>Semaforo IOT com RTOS</h1>\n"
             "<form action=\"./modo\"><button>Alternar modo</button></form>\n"
             "<form action=\"./buzzer\"><button>Alerta sonoro</button></form>\n"
             "<p class=\"texto\">Modo semaforo: %s</p>\n"
             "<p class=\"texto\">Alerta sonoro: %s</p>\n"
             "</body>\n"
             "</html>\n",
             modo_diurno ? "Diurno" : "Noturno", 
             alerta_sonoro ? "Ativado" : "Desativado");

    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
    free(request);
    pbuf_free(p);
    return ERR_OK;
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}
void init_perifericos()
{

    // Inicializa display
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, 128, 64, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);

    // Inicializa botoes
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);

    // inicializa matriz de led
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    // Configurar o pino como saída de PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);

    // Obter o slice do PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // seta o PWM
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (BUZZER_FREQUENCY * 4096)); // Divisor de clock
    pwm_init(slice_num, &config, true);

    // Iniciar o PWM no nível baixo
    pwm_set_gpio_level(BUZZER_PIN, 0);

    // Inicializa os Leds
    gpio_init(LED_VERDE);
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);

    while (cyw43_arch_init())
    {
        printf("Erro ao iniciar rede\n");
        sleep_ms(100);
        return -1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    cyw43_arch_enable_sta_mode();

    printf("Conectando no Wi-Fi...\n");
    ssd1306_draw_string(&ssd, "Conectando", 10, 29);
    ssd1306_send_data(&ssd);
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Erro ao conectar Wi-Fi\n");
        ssd1306_draw_string(&ssd, "Erro ao conectar", 10, 29);
        ssd1306_send_data(&ssd);
        sleep_ms(100);
        return -1;
    }
    printf("conectando rede\n");

    if (netif_default)
    {
        printf("IP: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Erro ao iniciar webserver\n");
        return -1;
    }

    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao iniciar webserver no IP e porta indicado\n");
        return -1;
    }
    server = tcp_listen(server);
    tcp_accept(server, tcp_server_accept);
    printf("webserver: porta 80\n");
}

void wifi_task()
{
    while (1)
    {
        cyw43_arch_poll(); // polling do wifi
        sleep_ms(50);
    }
}

int main()
{
    stdio_init_all();
    init_perifericos(); // inicializa os perifericos

    // tarefas do FreeRTOS
    xTaskCreate(wifi_task, "wifi", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(button_task, "Botao", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(buzzer_task, "Buzzer", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(display_task, "Display", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(led_task, "Led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(matriz_led_task, "Matriz led", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}

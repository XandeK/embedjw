#include "FreeRTOS.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "task.h"
#include "lwip/tcp.h"
#include "lwip/udp.h"
#include "lwip/init.h"
#include "lwip/ip_addr.h"

#define START_PORT 20
#define END_PORT 1024
#define TARGET_IP_ADDR "18.142.229.102"
#define MY_STACK_SIZE 2000  

// Callback function for connection result
static err_t tcp_result_cb(void *arg, struct tcp_pcb *tpcb, err_t err) {
    u16_t port = (u16_t)(uintptr_t)arg; // Recover the port number from the argument

    if (err == ERR_OK) {
        printf("Port %u is open\n", port);
        return ERR_OK;
    } else {
        printf("Port %u is closed or filtered\n", port);
        return ERR_TIMEOUT;
    }
}

void tcp_scan(const ip_addr_t *ipaddr, u16_t start_port, u16_t end_port) {
    for(u16_t port = start_port; port <= end_port; port++) {
        struct tcp_pcb *pcb = tcp_new();
        if (pcb == NULL) {
            // Handle memory allocation failure
            continue;
        }

        err_t err = tcp_bind(pcb, IP_ADDR_ANY, 0);  // Binding to any local address and ephemeral port
        if (err != ERR_OK) {
            // Handle binding failure
            tcp_abort(pcb);
            continue;
        }

        tcp_arg(pcb, (void*)(uintptr_t)port); // Pass the port number to the callback as an argument

        printf("Trying Port %d\n", port);
        err = tcp_connect(pcb, ipaddr, port, tcp_result_cb);
        vTaskDelay(pdMS_TO_TICKS(100));
        if (err != ERR_OK) {
            tcp_abort(pcb);  // Aborting the connection on error
        } else {
            tcp_abort(pcb);
        }
        // Short delay to allow processing of the callback
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Extended delay to allow all connections to complete or time out
    vTaskDelay(pdMS_TO_TICKS(2000));
}

void main_task(__unused void *params) {
    if (cyw43_arch_init()) {
        printf("failed to initialize\n");
        return;
    }
    cyw43_arch_enable_sta_mode();
    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        exit(1);
    } else {
        printf("Connected.\n");
    }

    // After connecting to Wi-Fi, start the TCP scan
    ip_addr_t target_ip;
    ip4addr_aton(TARGET_IP_ADDR, &target_ip);
    tcp_scan(&target_ip, START_PORT, END_PORT);
    /* xTaskCreate(TcpScanTask, "TCP Scan", MY_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL); */
}

int main(void) {
    // ... initialize hardware, FreeRTOS, lwIP, etc. ...
    stdio_init_all();
    
    // ... create tasks for TCP and UDP scanning ...
    xTaskCreate(main_task, "Main Task", MY_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    // ... start the scheduler, etc. ...
    vTaskStartScheduler();

    for (;;);
    return 0;
}
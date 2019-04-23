// Created by Ajay Curam
/*  Lock program
 	
    This program simulates the LOCK that is driven by pwm attached to the
    ESP32.

    Design:
	- This program sets up a TCP socket to talk to backend server.
	- waits for message from server.
	- For now, server received OPEN, CLOS messages as examples
	- What one needs to do to make this complete
		- Define states for the lock: OPEN, CLOSE
		- When you get the message from server, drive the PWM
		  to open or close. (Ref TBD: Lock_control()in code below)
		- When OPEN state, set up a timer, so that it goes
		  automatically into CLOSE state after some time ( security
		  reasons !)

    Key things to remember:
		- Ensure that you know the details of the access point
		  and change the wifi and IP variables in 
			#define CONFIG_EXAMPLE_IPV4(If IPV6 set this accordingl)

			#define EXAMPLE_WIFI_SSID "Kcuram_EXT"
			#define EXAMPLE_WIFI_PASS "<REDACTED>"
			#define HOST_IP_ADDR "10.0.0.231" (host = Server)

*/
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


//Servo motor lock init and definitions
#define LOCK 1
#define UNLK 2 
extern void    mcpwm_servo_control_init();
int gate_state;
extern void lockup();
extern void unlocked();

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/

#define CONFIG_EXAMPLE_IPV4
/*
#define EXAMPLE_WIFI_SSID "Kcuram_EXT"
#define EXAMPLE_WIFI_PASS "<REDACTED>"
#define HOST_IP_ADDR "10.0.0.231" 
*/

#define EXAMPLE_WIFI_SSID "OnePlus 6"
#define EXAMPLE_WIFI_PASS "CuramFamily"
#define HOST_IP_ADDR "192.168.43.133" 

#define PORT 10034

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

static const char *TAG = "EspLock : ";

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
//        /* enable ipv6 */
        printf("SYSTEM_EVENT_STA_CONNECTED\r\n");
//        tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, IPV4_GOTIP_BIT);
        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
	printf("%s:SYSTEM_EVENT_STA_DISCONNECTED  \n", TAG);
        /* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, IPV4_GOTIP_BIT);
//        xEventGroupClearBits(wifi_event_group, IPV6_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_AP_STA_GOT_IP6:
        printf("System event AP got IP6\r\n");
//        xEventGroupSetBits(wifi_event_group, IPV6_GOTIP_BIT);
//        ESP_LOGI(TAG, "SYSTEM_EVENT_STA_GOT_IP6");
//        char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
//        ESP_LOGI(TAG, "IPv6: %s", ip6);
    default:
        break;
    }
    return ESP_OK;
}
static void	disable_brown_out() //Enables Low Power mode.
{
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
}

static void initialise_wifi(void)
{

    disable_brown_out();
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void wait_for_ip()
{
    uint32_t bits = IPV4_GOTIP_BIT | IPV6_GOTIP_BIT ;//possibly get rid of IPV6 bits.

    ESP_LOGI(TAG, "Waiting for AP connection...");
//    xEventGroupWaitBits(wifi_event_group, bits, false, true, portMAX_DELAY);
    xEventGroupWaitBits(wifi_event_group, bits, false, true, 1000);
    ESP_LOGI(TAG, "Connected to AP");
}

static void tcp_client_task(void *pvParameters)
{
    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;


printf("%sRunning tcp client task \n", TAG);

#ifdef CONFIG_EXAMPLE_IPV4
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = inet_addr(HOST_IP_ADDR);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#endif
/*    //Attempt to disable IPv6 functionality. Force IPv4?
#else // IPV6
        struct sockaddr_in6 destAddr;
        inet6_aton(HOST_IP_ADDR, &destAddr.sin6_addr);
        destAddr.sin6_family = AF_INET6;
        destAddr.sin6_port = htons(PORT);
        addr_family = AF_INET6;
        ip_protocol = IPPROTO_IPV6;
        inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif
*/
        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
	    goto done;
        } 
        ESP_LOGI(TAG, "Socket created");

        int err = connect(sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving. Shut down socket.
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
            	shutdown(sock, 0);
           	 close(sock);
                break;

            } else if (len == 0) { //Means that the Camera script (TCP Server) is not up & running. Need to initialize TCP Server before starting ESP device.
                ESP_LOGI(TAG, "Received 0 bytes: Closing Socket");
            	shutdown(sock, 0);
           	 close(sock);
                break;
            } else { // Valid data reveived
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG, "%s", rx_buffer);
		printf("%s Gate Current state = (%d) LOCK = (%d) UNLK = (%d) \n", TAG, gate_state, LOCK, UNLK);
		if (strncmp(rx_buffer,"LOCK", 4) == 0) {
			printf("%sreceived LOCK message\n", TAG);
			if( gate_state == UNLK) {
				gate_state = LOCK;
				lockup();
				printf("%sGates Locked\n", TAG);
			} else {
				printf("%sAlready in lock state\n", TAG);
			}
		} else if (strncmp(rx_buffer,"UNLK", 4) == 0) {
			printf("%s received UNLK message\n gate_state = (%d)",TAG, gate_state);
			if( gate_state == LOCK) {
				gate_state = UNLK;
				unlocked();
				printf("%sGates UnLocked\n", TAG);
			} else {
				printf("%sAlready in Unlock state\n", TAG);
			}
		}
            }

//            vTaskDelay(2000 / portTICK_PERIOD_MS);
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
done:
	printf("%sLock Task exitting \n", TAG);
}

void app_main()
{
	char c = 0;

	printf("%s Press any key to start\n", TAG);
	// run some 100ms delay (not sure how to do this)
	fflush(stdout);
	c = fgetc(stdin);

	printf("%sStarting program\n", TAG);

    ESP_ERROR_CHECK( nvs_flash_init() );
    mcpwm_servo_control_init();
    gate_state = LOCK; 
    lockup(); // Start with Gates locked
    initialise_wifi();
    wait_for_ip();

    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}

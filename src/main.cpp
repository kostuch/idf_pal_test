#include "stddef.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <soc/rtc_wdt.h>
//#include "AudioSystem.h"
//#include "AudioOutput.h"
#include "Graphics.h"
#include "Image.h"
#include "SimplePALOutput.h"
#include "Sprites.h"
#include "gfx/font5x7.h"
#include "gfx/font8x8.h"
#include "gfx/font16x16.h"
#include "gfx/icons.h"
//#include "sfx/music.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

Font font57(5, 7, font5x7);
Font font88(8, 8, font8x8);
Font font1616(16, 16, font16x16);
//Image obrazek;
////////////////////////////
//audio configuration
//const int SAMPLING_RATE = 20000;
//const int BUFFER_SIZE = 4000;
//AudioSystem audioSystem(SAMPLING_RATE, BUFFER_SIZE);
//AudioOutput audioOutput;

///////////////////////////
//Video configuration
//PAL MAX, half: 324x268 full: 648x536
//NTSC MAX, half: 324x224 full: 648x448
const int XRES = 320;
const int YRES = 240;
//const int XRES = 648;
//const int YRES = 536;
Graphics graphics(XRES, YRES);
SimplePALOutput composite;

#define EXAMPLE_SSID "padaka4"
#define EXAMPLE_PASS "megapadaka"
#define EXAMPLE_ESP_MAXIMUM_RETRY  3
static int s_retry_num = 0;
static const char *TAG = "wifi station";
/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
		{
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG, "retry to connect to the AP");
		}
		else xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);

		ESP_LOGI(TAG, "connect to the AP fail");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
		ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void wifi_init_sta(void)
{
	s_wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	esp_event_handler_instance_t instance_any_id;
	esp_event_handler_instance_t instance_got_ip;
	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
	                ESP_EVENT_ANY_ID,
	                &event_handler,
	                NULL,
	                &instance_any_id));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
	                IP_EVENT_STA_GOT_IP,
	                &event_handler,
	                NULL,
	                &instance_got_ip));
	wifi_config_t wifi_config = { .sta =  {
			/* ssid            */ EXAMPLE_SSID,
			/* password        */ EXAMPLE_PASS,
			/* scan_method     */ {},
			/* bssid_set       */ {},
			/* bssid           */ {},
			/* channel         */ {},
			/* listen_interval */ {},
			/* sort_method     */ {},
			/* threshold       */ {
				/* rssi            */ {},
				/* authmode        */ WIFI_AUTH_WPA2_PSK
			},
			/* pmf_cfg         */ {
				/* capable         */ true,
				/* required        */ false
			},
			/* rm_enabled      */ {},
			/* btm_enabled     */ {},
			//      /* mbo_enabled     */ {}, // For IDF 4.4 and higher
			/* reserved        */ {}
		}
	};
	/*
	//Allocate storage for the struct
	wifi_config_t sta_config = {};
	//Assign ssid & password strings
	strcpy((char*)sta_config.sta.ssid, "ssid");
	strcpy((char*)sta_config.sta.password, "password");
	sta_config.sta.bssid_set = false;
	*/
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
	ESP_ERROR_CHECK(esp_wifi_start() );
	ESP_LOGI(TAG, "wifi_init_sta finished.");
	/* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
	 * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
	                                       WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
	                                       pdFALSE,
	                                       pdFALSE,
	                                       portMAX_DELAY);

	/* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
	 * happened. */
	if (bits & WIFI_CONNECTED_BIT) ESP_LOGI(TAG, "connected to ap SSID:%s password:%s", EXAMPLE_SSID, EXAMPLE_PASS);
	else if (bits & WIFI_FAIL_BIT) ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s", EXAMPLE_SSID, EXAMPLE_PASS);
	else ESP_LOGE(TAG, "UNEXPECTED EVENT");

	/* The event will not be processed after unregister */
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
	vEventGroupDelete(s_wifi_event_group);
}

void compositeCore(void *data)
{
	while (true) composite.sendFrame(&graphics.frame);
}

void make_screen(uint16_t val)
{
	// Teoretycznie rozdzielczosc 40x30 znakow (8x8)
	// 20x15 znakow (16x16)
	//fill audio buffer
	//audioSystem.calcSamples();
	//graphics.begin(0);

	graphics.fillRect(0, 0, 324, 40, RED);
	graphics.fillRect(0, 40, 324, 40, GREEN);
	graphics.fillRect(0, 80, 324, 40, BLUE);
	graphics.fillRect(0, 120, 324, 40, WHITE);
	graphics.fillRect(0, 160, 324, 40, BLACK);
	graphics.fillRect(0, 200, 324, 40, YELLOW);
	graphics.setTextColor(WHITE);
	graphics.setFont(font88);
	graphics.setCursor(0, 0);
	graphics.out_txt("ABC");
	graphics.setCursor(0, 232);
	graphics.out_txt("QWE");
	graphics.setCursor(296, 0);
	graphics.out_txt("123");
	graphics.setCursor(296, 232);
	graphics.out_txt("XYZ");
	graphics.setCursor(0, 180);
	graphics.out_txt("0123456789012345678901234567890123456789");
	graphics.setFont(font57);
	graphics.setCursor(0, 160);
	graphics.out_txt("0123456789012345678901234567890123456789012345678901234567890123456789");
	for (size_t i = 0; i < 40; i++)
	{
		graphics.line(i * 8, 200, i * 8, 210, BLACK);
	}
	
/* 
	graphics.setCursor(0, 0);
	graphics.setTextColor(BLACK);
	graphics.setFont(font57);
	graphics.out_txt("0123456789abcdefABCDEF");
	graphics.setCursor(0, 40);
	graphics.setFont(font88);
	graphics.out_txt("0123456789abcdefABCDEF");
	graphics.setCursor(0, 60);
	graphics.setFont(font1616);
	graphics.out_txt("1234567890");
	graphics.setCursor(0, 80);
	char napis[6];
	itoa(val, napis, 10);
	graphics.out_txt(napis);
	graphics.fillRect(0, 100, 324, 10, BLACK);
	graphics.fillRect(0, 110, 324, 10, WHITE);
	graphics.fillRect(0, 120, 324, 20, RED);
	graphics.fillRect(0, 140, 324, 20, GREEN);
	graphics.fillRect(0, 160, 324, 20, BLUE);
	graphics.fillRect(0, 180, 324, 20, YELLOW);
	graphics.fillRect(0, 200, 324, 20, MAGENTA);
	graphics.fillRect(0, 220, 324, 20, CYAN);
	graphics.setCursor(0, 150);
	graphics.setTextColor(BLACK, TRANSPARENT);
	graphics.out_txt("Skeletondevices !");
	graphics.drawMonoBMP(280, 100, 32, 32, satellite_sym, YELLOW, BLACK);
	graphics.drawMonoBMP(280, 150, 32, 32, satellite_sym,  BLACK, TRANSPARENT);
 */	
	graphics.end();
}

void refreshScreen(void *data)
{
	static uint16_t counter;

	while (true)
	{
		//make_screen(counter++);
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}


extern "C" void app_main()
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();

	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);
	ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
	composite.init();
	 
	for (size_t i = 0; i < lineSamples; i++)
	{
		printf("blank[%d]=%d short[%d]=%d mix[%d]=%d long[%d]=%d\n",
		 i, composite.blank[i], i, composite.shortSync[i],
		 i, composite.mixSync[i], i, composite.longSync[i]);
	}
	
	graphics.init();
	make_screen(0);
	xTaskCreatePinnedToCore(compositeCore, "c", 1024, NULL, 5, NULL, 0);
	xTaskCreatePinnedToCore(refreshScreen, "rs", configMINIMAL_STACK_SIZE, NULL, 1, NULL, 1);
	//initialize audio output
	//audioOutput.init(audioSystem);
	//Play first sound in loop (music)
	//music.play(audioSystem, 0);
	wifi_init_sta();
}

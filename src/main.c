/**
 * @file main.c
 * @brief Application main entry point
 *
 * This file contains the main entry point for the application.
 */
/**< Author: Dhruv Menon */
/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>		  /**< Include Zephyr types */
#include <stddef.h>				  /**< Include standard definitions */
#include <string.h>				  /**< Include string manipulation functions */
#include <errno.h>				  /**< Include error handling */
#include <zephyr/sys/printk.h>	  /**< Include printk for printing */
#include <zephyr/sys/byteorder.h> /**< Include byte order conversion */
#include <zephyr/kernel.h>		  /**< Include kernel related functionalities */

#include <zephyr/settings/settings.h> /**< Include Zephyr settings */

#include <zephyr/bluetooth/bluetooth.h> /**< Include Bluetooth stack */
#include <zephyr/bluetooth/hci.h>		/**< Include Bluetooth HCI */
#include <zephyr/bluetooth/conn.h>		/**< Include Bluetooth connection */
#include <zephyr/bluetooth/uuid.h>		/**< Include Bluetooth UUID */
#include <zephyr/bluetooth/gatt.h>		/**< Include Bluetooth GATT */

#include "hog.h" /**< Include Human Interface Device (HID) over GATT */

#include <zephyr/usb/usb_device.h> /**< Include USB device stack */
#include <zephyr/usb/usbd.h>	   /**< Include USB device functions */
#include <zephyr/drivers/uart.h>   /**< Include UART driver */
#include <stdio.h>				   /**< Include standard I/O */
#include <string.h>				   /**< Include string manipulation functions */
#include <stdlib.h>				   /**< Include standard library */
#include <ctype.h>
#include <zephyr/drivers/gpio.h>

#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);
int ret;

#define STACKSIZE 1024 /**< Define stack size */

#define MAX_DATA_LEN 50 /**< Define maximum data length */

static char report[3][MAX_DATA_LEN]; /**< Define report data array */
uint8_t global_report[3];			 /**< Define global report array */
struct bt_conn *default_conn = NULL;
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
USBD_CONFIGURATION_DEFINE(config_1,
						  USB_SCD_SELF_POWERED,
						  200);

USBD_DESC_LANG_DEFINE(sample_lang);									 /**< Define USB language descriptor */
USBD_DESC_MANUFACTURER_DEFINE(sample_mfr, "ZEPHYR");				 /**< Define USB manufacturer descriptor */
USBD_DESC_PRODUCT_DEFINE(sample_product, "Zephyr USBD ACM console"); /**< Define USB product descriptor */
USBD_DESC_SERIAL_NUMBER_DEFINE(sample_sn, "0123456789ABCDEF");		 /**< Define USB serial number descriptor */

USBD_DEVICE_DEFINE(sample_usbd,
				   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
				   0x2fe3, 0x0001); /**< Define USB device */

static int enable_usb_device_next(void)
{
	int err;

	err = usbd_add_descriptor(&sample_usbd, &sample_lang); /**< Add language descriptor */
	if (err)
	{
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_mfr); /**< Add manufacturer descriptor */
	if (err)
	{
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_product); /**< Add product descriptor */
	if (err)
	{
		return err;
	}

	err = usbd_add_descriptor(&sample_usbd, &sample_sn); /**< Add serial number descriptor */
	if (err)
	{
		return err;
	}

	err = usbd_add_configuration(&sample_usbd, &config_1); /**< Add USB configuration */
	if (err)
	{
		return err;
	}

	err = usbd_register_class(&sample_usbd, "cdc_acm_0", 1); /**< Register USB class */
	if (err)
	{
		return err;
	}

	err = usbd_init(&sample_usbd); /**< Initialize USB device */
	if (err)
	{
		return err;
	}

	err = usbd_enable(&sample_usbd); /**< Enable USB device */
	if (err)
	{
		return err;
	}

	return 0;
}
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK_NEXT) */

/**
 * @brief Process the received data
 *
 * This function processes the received data.
 *
 * @param[in] data Data to be processed
 */
static void process_data(const char *data)
{
	char *data_copy = strdup(data); /**< Create a copy to modify */
	size_t len = strlen(data);
	while (len > 0 && isspace((unsigned char)data[len - 1]))
	{
		len--;
	}
	data_copy[len] = '\0'; // Trimmed string

	printk("Received data: %s\n", data);

	if (strcmp(data_copy, "getDeviceId") == 0)
	{
		// Print the Bluetooth device name
		printk("Bluetooth Device Name: %s\n", CONFIG_BT_DEVICE_NAME);
		return;
	}
	else if (strcmp(data_copy, "getConnectedAddress") == 0)
	{
		// Print the connected device address if a connection exists
		if (default_conn)
		{
			char addr[BT_ADDR_LE_STR_LEN];
			bt_addr_le_to_str(bt_conn_get_dst(default_conn), addr, sizeof(addr));
			printk("Connected Device Address: %s\n", addr);
		}
		else
		{
			printk("No device connected\n");
		}
		return;
	}

	if (data_copy == NULL)
	{
		printk("Failed to allocate memory for data copy\n");
		return;
	}

	char *token;
	uint8_t *ptr = global_report;
	int i = 0;

	/* Split the data by space and store in global_report array */
	token = strtok(data_copy, " ");
	while (token != NULL && i < 3)
	{
		/* Convert token to uint8_t and store in global_report */
		*ptr = (uint8_t)strtoul(token, NULL, 10);
		ptr++;
		token = strtok(NULL, " ");
		strcpy(report[i], token);
		report[i][0] = '\0';
		i++;
	}

	while (i < 3)
	{
		report[i][0] = '\0';
		i++;
	}
	free(data_copy); /**< Free the allocated memory */
}

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				  BT_UUID_16_ENCODE(BT_UUID_HIDS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

// Function to control RGB LED
void set_rgb_led_state(uint8_t r, uint8_t g, uint8_t b)
{

	if (ret < 0)
	{
		printk("Error: Failed to initialize LED GPIO port\n");
		return;
	}
	if (r == 1)
	{
		ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
	}
	else
	{
		ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
	}
	if (g == 1)
	{
		ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
	}
	else
	{
		ret = gpio_pin_configure_dt(&led2, GPIO_OUTPUT_INACTIVE);
	}
	if (b == 1)
	{
		ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);
	}
	else
	{
		ret = gpio_pin_configure_dt(&led3, GPIO_OUTPUT_INACTIVE);
	}
	// Set LED pins to desired state
}
/**
 * @brief Callback for connection establishment
 *
 * This function is called when a connection is established.
 *
 * @param[in] conn Connection object
 * @param[in] err Error code
 */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err)
	{
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}
	default_conn = conn;

	printk("Connected %s\n", addr);
	set_rgb_led_state(0, 1, 0);

	if (bt_conn_set_security(conn, BT_SECURITY_L2))
	{
		printk("Failed to set security\n");
	}
}

/**
 * @brief Callback for disconnection
 *
 * This function is called when a connection is disconnected.
 *
 * @param[in] conn Connection object
 * @param[in] reason Disconnection reason code
 */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason 0x%02x)\n", addr, reason);
	set_rgb_led_state(1, 0, 0);
	if (conn == default_conn) // Clear the stored connection object if it matches the disconnected one
	{
		default_conn = NULL;
	}
}

/**
 * @brief Callback for security change
 *
 * This function is called when security level changes.
 *
 * @param[in] conn Connection object
 * @param[in] level Security level
 * @param[in] err Error code
 */
static void security_changed(struct bt_conn *conn, bt_security_t level,
							 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		printk("Security changed: %s level %u\n", addr, level);
	}
	else
	{
		printk("Security failed: %s level %u err %d\n", addr, level,
			   err);
	}
}

/**
 * @brief Bluetooth initialization callback
 *
 * This function is called when Bluetooth initialization is completed.
 *
 * @param[in] err Error code
 */
BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

/**
 * @brief Bluetooth ready callback
 *
 * This function is called when Bluetooth is ready.
 *
 * @param[in] err Error code
 */
static void bt_ready(int err)
{
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	hog_init();

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/**
 * @brief Display passkey callback
 *
 * This function is called to display passkey for pairing.
 *
 * @param[in] conn Connection object
 * @param[in] passkey Passkey to be displayed
 */
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Passkey for %s: %06u\n", addr, passkey);
}

/**
 * @brief Authentication cancel callback
 *
 * This function is called when pairing is cancelled.
 *
 * @param[in] conn Connection object
 */
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = auth_passkey_display,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
};

/**
 * @brief Main function
 *
 * The main entry point of the application.
 *
 * @return Returns 0 on successful execution
 */
int main(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	uint32_t dtr = 0;

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	if (enable_usb_device_next())
	{
		return 0;
	}
#else
	if (usb_enable(NULL))
	{
		return 0;
	}
#endif

	/* Poll if the DTR flag was set */
	while (!dtr)
	{
		uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		/* Give CPU resources to low priority threads. */
		k_sleep(K_MSEC(10));
	}

	int err;

	err = bt_enable(bt_ready);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SAMPLE_BT_USE_AUTHENTICATION))
	{
		bt_conn_auth_cb_register(&auth_cb_display);
		printk("Bluetooth authentication callbacks registered.\n");
	}

	char data[MAX_DATA_LEN];
	while (1)
	{
		/* Check if any data is available from USB */
		ssize_t bytes_read = uart_fifo_read(dev, data, sizeof(data));
		if (bytes_read > 0)
		{
			printk("%d\n", bytes_read);
			data[bytes_read] = '\0'; /**< Null-terminate the string */
			process_data(data);		 /**< Process the received data */
		}

		/* Print the report array contents */
		printk("Report: %d, %d, %d\n", global_report[0], global_report[1], global_report[2]);

		// Reset global_report

		hog_button_loop(global_report);
		global_report[1] = 0;
		global_report[2] = 0;
		k_sleep(K_MSEC(1));
	}

	return 0;
}

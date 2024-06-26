/** @file
 *  @brief HoG Service sample for Keyboard
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

enum
{
	HIDS_REMOTE_WAKE = BIT(0),
	HIDS_NORMALLY_CONNECTABLE = BIT(1),
};

struct hids_info
{
	uint16_t version;
	uint8_t code;
	uint8_t flags;
} __packed;

struct hids_report
{
	uint8_t id;
	uint8_t type;
} __packed;

static struct hids_info info = {
	.version = 0x0000,
	.code = 0x00,
	.flags = HIDS_NORMALLY_CONNECTABLE,
};

enum
{
	HIDS_INPUT = 0x01,
	HIDS_OUTPUT = 0x02,
	HIDS_FEATURE = 0x03,
};

static struct hids_report input = {
	.id = 0x01,
	.type = HIDS_INPUT,
};

static uint8_t simulate_input;
static uint8_t ctrl_point;
static uint8_t report_map[] = {
	0x05, 0x01, /* Usage Page (Generic Desktop) */
	0x09, 0x06, /* Usage (Keyboard) */
	0xA1, 0x01, /* Collection (Application) */
	0x85, 0x01, /* Report ID (1) */
	0x05, 0x07, /* Usage Page (Key Codes) */
	0x19, 0xE0, /* Usage Minimum (224) */
	0x29, 0xE7, /* Usage Maximum (231) */
	0x15, 0x00, /* Logical Minimum (0) */
	0x25, 0x01, /* Logical Maximum (1) */
	0x75, 0x01, /* Report Size (1) */
	0x95, 0x08, /* Report Count (8) */
	0x81, 0x02, /* Input (Data, Variable, Absolute) */
	0x95, 0x01, /* Report Count (1) */
	0x75, 0x08, /* Report Size (8) */
	0x81, 0x03, /* Input (Constant, Variable, Absolute) */
	0x95, 0x05, /* Report Count (5) */
	0x75, 0x01, /* Report Size (1) */
	0x05, 0x08, /* Usage Page (LEDs) */
	0x19, 0x01, /* Usage Minimum (1) */
	0x29, 0x05, /* Usage Maximum (5) */
	0x91, 0x02, /* Output (Data, Variable, Absolute) */
	0x95, 0x01, /* Report Count (1) */
	0x75, 0x03, /* Report Size (3) */
	0x91, 0x03, /* Output (Constant, Variable, Absolute) */
	0x95, 0x06, /* Report Count (6) */
	0x75, 0x08, /* Report Size (8) */
	0x15, 0x00, /* Logical Minimum (0) */
	0x25, 0x65, /* Logical Maximum (101) */
	0x05, 0x07, /* Usage Page (Key Codes) */
	0x19, 0x00, /* Usage Minimum (0) */
	0x29, 0x65, /* Usage Maximum (101) */
	0x81, 0x00, /* Input (Data, Array) */
	0xC0		/* End Collection */
};

static ssize_t read_info(struct bt_conn *conn,
						 const struct bt_gatt_attr *attr, void *buf,
						 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
							 sizeof(struct hids_info));
}

static ssize_t read_report_map(struct bt_conn *conn,
							   const struct bt_gatt_attr *attr, void *buf,
							   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, report_map,
							 sizeof(report_map));
}

static ssize_t read_report(struct bt_conn *conn,
						   const struct bt_gatt_attr *attr, void *buf,
						   uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, attr->user_data,
							 sizeof(struct hids_report));
}

static void input_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	simulate_input = (value == BT_GATT_CCC_NOTIFY) ? 1 : 0;
}

static ssize_t read_input_report(struct bt_conn *conn,
								 const struct bt_gatt_attr *attr, void *buf,
								 uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, NULL, 0);
}

static ssize_t write_ctrl_point(struct bt_conn *conn,
								const struct bt_gatt_attr *attr,
								const void *buf, uint16_t len, uint16_t offset,
								uint8_t flags)
{
	uint8_t *value = attr->user_data;

	if (offset + len > sizeof(ctrl_point))
	{
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy(value + offset, buf, len);

	return len;
}

#if CONFIG_SAMPLE_BT_USE_AUTHENTICATION
#define SAMPLE_BT_PERM_READ BT_GATT_PERM_READ_AUTHEN
#define SAMPLE_BT_PERM_WRITE BT_GATT_PERM_WRITE_AUTHEN
#else
#define SAMPLE_BT_PERM_READ BT_GATT_PERM_READ_ENCRYPT
#define SAMPLE_BT_PERM_WRITE BT_GATT_PERM_WRITE_ENCRYPT
#endif

BT_GATT_SERVICE_DEFINE(hog_svc,
					   BT_GATT_PRIMARY_SERVICE(BT_UUID_HIDS),
					   BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_INFO, BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ, read_info, NULL, &info),
					   BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT_MAP, BT_GATT_CHRC_READ,
											  BT_GATT_PERM_READ, read_report_map, NULL, NULL),
					   BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_REPORT,
											  BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
											  SAMPLE_BT_PERM_READ,
											  read_input_report, NULL, NULL),
					   BT_GATT_CCC(input_ccc_changed,
								   SAMPLE_BT_PERM_READ | SAMPLE_BT_PERM_WRITE),
					   BT_GATT_DESCRIPTOR(BT_UUID_HIDS_REPORT_REF, BT_GATT_PERM_READ,
										  read_report, NULL, &input),
					   BT_GATT_CHARACTERISTIC(BT_UUID_HIDS_CTRL_POINT,
											  BT_GATT_CHRC_WRITE_WITHOUT_RESP,
											  BT_GATT_PERM_WRITE,
											  NULL, write_ctrl_point, &ctrl_point), );

void hog_init(void)
{
}

void hog_button_loop(int8_t global_report[8])
{
	int8_t report[8] = {0, 0, 0, 0, 0, 0, 0, 0};

	for (int i = 0; i < 8; i++)
	{
		memcpy(&report[i], &global_report[i], sizeof(int8_t));
	}

	bt_gatt_notify(NULL, &hog_svc.attrs[5],
				   report, sizeof(report));
	printk("advertised: %d %d %d %d %d %d %d %d\n", report[0], report[1], report[2], report[3], report[4], report[5], report[6], report[7]);
}

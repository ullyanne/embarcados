/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
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
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>

#define SERVICE 0xe1
#define SERVICE_UUID BT_UUID_DECLARE_16(SERVICE)

#define CHARACT1 0xe2
#define CHARACT1_UUID BT_UUID_DECLARE_16(CHARACT1)

#define CHARACT2 0xe3
#define CHARACT2_UUID BT_UUID_DECLARE_16(CHARACT2)

struct bt_gatt_cb gatt_callbacks;
struct bt_conn_cb conn_callbacks;
//gatt_callbacks.att_mtu_updated = ble_peripheral_mtu_updated;

/* OK */
static int ble_uppercase_write(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     const void *buf, uint16_t len,
                                     uint16_t offset, uint8_t flags);

static void bt_gatt_ccc_cfg(const struct bt_gatt_attr *attr,
                                       uint16_t value);

BT_GATT_SERVICE_DEFINE(
	uppercase_service, 
	BT_GATT_PRIMARY_SERVICE(SERVICE_UUID),
    BT_GATT_CHARACTERISTIC(	CHARACT1_UUID, 
							BT_GATT_CHRC_WRITE, 
							BT_GATT_PERM_WRITE, 
							NULL,
                        	ble_uppercase_write, 
							NULL),
    BT_GATT_CHARACTERISTIC(	CHARACT2_UUID, 
							BT_GATT_CHRC_NOTIFY,
                           	BT_GATT_PERM_NONE, 
							NULL, 
							NULL, 
							NULL),
    BT_GATT_CCC(	bt_gatt_ccc_cfg, 
					BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),);

static int ble_uppercase_write(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     const void *buf, uint16_t len,
                                     uint16_t offset, uint8_t flags) {
  int err = 0;
  char data[len + 1];

  /* Copia dados recebidos. */
  memcpy(data, buf, len);
  data[len] = '\0';

  printk("|BLE PERIPHERAL| CHEGOU A INFORMAÇÃO %s.\n", data);

  /* Converte letras minúsculas para maiúsculas. */
  for (int i = 0; i < len; i++) {
    if ((data[i] >= 'a') && ((data[i] <= 'z'))) {
      data[i] = 'A' + (data[i] - 'a');
    }
  }

  printk("|BLE PERIPHERAL| MANDANDO A INFORMÇÃO DE VOLTA %s.\n", data);

  /* Notifica Central com o dados convertidos. */
  err = bt_gatt_notify(NULL, &uppercase_service.attrs[0], data, len);
  if (err) {
    printk("|BLE PERIPHERAL| DEU RUIM :(\n");
  }

  return 0;
}

/* OK */
static void bt_gatt_ccc_cfg(const struct bt_gatt_attr *attr,
                                       uint16_t value) {
  ARG_UNUSED(attr);

  bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);

  printk("|BLE PERIPHERAL| Notify %s.\n",
         (notify_enabled ? "enabled" : "disabled"));
}

/**
 * @brief Define os dados condificados no adversiting.
 *
 */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(0xe1), ),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;
	 
	bt_conn_cb_register(&conn_callbacks);
	bt_gatt_cb_register(&gatt_callbacks);
	
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

static void hrs_notify(void)
{
	static uint8_t heartrate = 90U;

	/* Heartrate measurements simulation */
	heartrate++;
	if (heartrate == 160U) {
		heartrate = 90U;
	}

	bt_hrs_notify(heartrate);
}

int main(void)
{
	bt_ready();

	// bt_conn_auth_cb_register(&auth_cb_display);

	// /* Implement notification. At the moment there is no suitable way
	//  * of starting delayed work so we do it here
	//  */
	// while (1) {
	// 	k_sleep(K_SECONDS(1));

	// 	/* Heartrate measurements simulation */
	// 	hrs_notify();

	// 	/* Battery level simulation */
	// 	bas_notify();
	// }
	return 0;
}

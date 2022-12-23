/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <device.h>
#include <devicetree.h>
#include <kernel.h>
#include <zephyr.h>
#include <zephyr/drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(sensor);

#include "app_bt.h"
#include "led_matrix.h"

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 5000

typedef struct {
    int dummy;
} send_data_t;

static send_data_t send_data = {
    .dummy = 0,
};

static void app_work_handler(struct k_work *work);
static void app_timer_handler(struct k_timer *dummy);

K_WORK_DEFINE(app_work, app_work_handler);
K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);

static void app_work_handler(struct k_work *work)
{
    k_msleep(100);

    send_data.dummy++;
    bt_app_send_data(&send_data, sizeof(send_data_t));
}

static void app_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&app_work);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected    = connected,
    .disconnected = disconnected,
};

static int app_bt_cb(void *data)
{
    // Note: The maximum data size is defined by APP_BT_MAX_ATTR_LEN.
    if (APP_BT_MAX_ATTR_LEN < sizeof(send_data_t)) {
        LOG_ERR("app_bt_cb(): Too large data size");
        return -EINVAL;
    }
    memcpy(data, &send_data, sizeof(send_data_t));
    return sizeof(send_data_t);
}

static struct bt_app_cb app_callbacks = {
    .app_bt_cb = app_bt_cb,
};

void main(void)
{
    LOG_MODULE_DECLARE(sensor);
    int ret;

    ret = bt_app_init(&app_callbacks);
    if (ret) {
        LOG_ERR("Failed to init bt [%d]", ret);
        return;
    }

    ret = bt_app_advertise_start();
    if (ret) {
        LOG_ERR("Failed to start advertising [%d]", ret);
        return;
    }

    /* start periodic timer that expires once every 10 seconds */
    k_timer_start(&app_timer, K_SECONDS(1), K_MSEC(SLEEP_TIME_MS));

    led_matrix_test();

    // do nothing
}

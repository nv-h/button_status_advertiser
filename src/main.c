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

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 1000

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_NODELABEL(ring0), gpios);
static struct gpio_callback      button_cb;

static bool is_connected = false;

static void app_send_handler(struct k_work *work);
static void app_update_handler(struct k_work *work);
static void app_timer_handler(struct k_timer *dummy);

K_WORK_DEFINE(app_send, app_send_handler);
K_WORK_DEFINE(app_update, app_update_handler);
K_TIMER_DEFINE(app_timer, app_timer_handler, NULL);

static void button_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    if (pins & BIT(button.pin)) {
        if (is_connected) {
            k_work_submit(&app_send);
        } else {
            k_work_submit(&app_update);
        }
    }
}

static void button_init(void)
{
    gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
    gpio_init_callback(&button_cb, button_handler, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb);
}

static void app_send_handler(struct k_work *work)
{
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_DISABLE);

    uint8_t button_status = gpio_pin_get_dt(&button);
    LOG_INF("app_send_handler(): button_status=%d", button_status);
    bt_app_send_data(button_status);

    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
}

static void app_update_handler(struct k_work *work)
{
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_DISABLE);

    uint8_t button_status = gpio_pin_get_dt(&button);
    LOG_INF("app_update_handler(): button_status=%d", button_status);
    bt_app_advertise_update(button_status);

    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_BOTH);
}

static void app_timer_handler(struct k_timer *dummy)
{
    k_work_submit(&app_send);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        printk("Connection failed (err %u)\n", err);
        return;
    }

    printk("Connected\n");
    is_connected = true;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason %u)\n", reason);
    is_connected = false;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected    = connected,
    .disconnected = disconnected,
};

static int app_bt_cb(void *data)
{
    // Note: The maximum data size is defined by APP_BT_MAX_ATTR_LEN.
    uint8_t button_status = gpio_pin_get_dt(&button);
    memcpy(data, &button_status, sizeof(button_status));
    return sizeof(button_status);
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

    button_init();

    /* start periodic timer that expires once every `SLEEP_TIME_MS` milliseconds */
    k_timer_start(&app_timer, K_SECONDS(1), K_MSEC(SLEEP_TIME_MS));

    // do nothing
}

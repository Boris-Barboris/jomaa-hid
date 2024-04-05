/*
 *   Jomaa LTP-03 bluetooth touchpad driver
 *
 *   LTP-03 report desctriptor:
	R: 483 05 01 09 06 a1 01 85 01 05 07 19 e0 29 e7 15 00 25 01 75 01 95 08 81 02 95 01 75 08 81 01 95 05
	75 01 05 08 19 01 29 05 91 02 95 01 75 03 91 01 95 06 75 08 15 00 26 ff 00 05 07 19 00 2a ff 00 81 00 c0
	05 01 09 02 a1 01 85 02 09 01 a1 00 05 09 19 01 29 08 15 00 25 01 95 08 75 01 81 02 05 01 16 01 80 26 ff
	7f 75 10 95 02 09 30 09 31 81 06 15 81 25 7f 75 08 95 01 09 38 81 06 05 0c 0a 38 02 95 01 81 06 c0 c0 05
	0c 09 01 a1 01 85 03 19 00 2a 9c 02 15 00 26 9c 02 75 10 95 03 81 00 c0 05 01 09 80 a1 01 85 04 1a 81 00
	2a 83 00 15 00 25 01 75 01 95 03 81 02 95 05 81 01 c0 06 00 ff 09 01 a1 01 25 ff 75 08 95 14 85 05 09 01
	b1 02 c0 05 0d 09 05 a1 01 85 0b 25 01 75 01 09 57 b1 02 26 ff 7f 75 0f 09 a1 b1 02 27 ff ff 00 00 09 56
	75 10 95 01 81 02 05 09 25 01 19 01 29 01 95 02 75 01 81 02 95 06 81 01 05 0d 09 22 a1 00 09 42 09 47 75
	01 95 02 81 02 25 05 09 38 75 06 95 01 81 02 05 01 46 06 07 35 00 26 50 06 75 0c 55 0e 65 11 09 30 81 02
	46 10 05 26 d0 04 09 31 81 02 c0 05 0d 09 22 a1 00 25 01 09 42 09 47 75 01 95 02 81 02 09 38 25 05 75 06
	95 01 81 02 05 01 46 06 07 26 50 06 75 0c 09 30 81 02 46 10 05 26 d0 04 09 31 81 02 c0 05 0d 09 22 a1 00
	25 01 09 42 09 47 75 01 95 02 81 02 09 38 25 05 75 06 95 01 81 02 05 01 46 06 07 26 50 06 75 0c 09 30 81
	02 46 10 05 26 d0 04 09 31 81 02 c0 05 0d 09 22 a1 00 25 01 09 42 09 47 75 01 95 02 81 02 09 38 25 05 75
	06 95 01 81 02 05 01 46 06 07 26 50 06 75 0c 09 30 81 02 46 10 05 26 d0 04 09 31 81 02 c0 c0
	N: device 0:0
	I: 3 0001 0001

	Device is used as a standard multitouch trackpad, but it cannot be handled by hid-multitouch because
	it does not conform to Windows Precision Touchpad spec. It uses fixed-sized 4-finger report, consisting
	of 4 records with "Tip Switch" usages. "Transducer Index" is used instead of "Contact ID".

	Example report of ID 11:

	# ReportID: 11 / Scan Time:  31680 | Button: 1 , 0 | #
	#              | Tip Switch: 1 | Confidence: 1 | Transducer Index:   1 | X:   421 | Y:   272
	#              | Tip Switch: 1 | Confidence: 1 | Transducer Index:   2 | X:   206 | Y:   178
	#              | Tip Switch: 1 | Confidence: 1 | Transducer Index:   3 | X:   129 | Y:    42
	#              | Tip Switch: 0 | Confidence: 0 | Transducer Index:   0 | X:     0 | Y:     0
	E: 000122.850982 20 0b c0 7b 01 07 a5 01 11 0b ce 20 0b 0f 81 a0 02 00 00 00 00


	# ReportID: 11 / Scan Time:  32480 | Button: 0 , 0 | #
	#              | Tip Switch: 0 | Confidence: 1 | Transducer Index:   1 | X:   422 | Y:   272
	#              | Tip Switch: 0 | Confidence: 0 | Transducer Index:   0 | X:     0 | Y:     0
	#              | Tip Switch: 0 | Confidence: 0 | Transducer Index:   0 | X:     0 | Y:     0
	#              | Tip Switch: 0 | Confidence: 0 | Transducer Index:   0 | X:     0 | Y:     0
	E: 000122.903707 20 0b e0 7e 00 06 a6 01 11 00 00 00 00 00 00 00 00 00 00 00 00
 *
 *   Copyright (c) 2024 Alexander Baranin <ismailsiege@gmail.com>
 */

/*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "hid-ids.h"


#define TRACKPAD_REPORT_ID 0x0b

#define TRACKPAD_MIN_X 0
#define TRACKPAD_MAX_X 1616
#define TRACKPAD_MIN_Y 0
#define TRACKPAD_MAX_Y 1232
#define TOUCH_COUNT 4

/**
 * struct jomaa_sc - LTP-03-specific data.
 * @input: Input device through which we report events.
 * @touches: Most recent data for a touch, indexed by tracking ID.
 */
struct jomaa_sc {
	struct input_dev *input;
	struct hid_device *hdev;
	struct delayed_work work;
};

static void jomaa_emit_buttons(struct jomaa_sc *msc, int state)
{
	input_report_key(msc->input, BTN_LEFT, state & 1);
	input_report_key(msc->input, BTN_RIGHT, state & 2);
}

static bool jomaa_emit_touch(struct jomaa_sc *msc, int raw_id,
		u8 *tdata, int scantime)
{
	struct input_dev *input = msc->input;
	short id, x, y, state;

	id = (tdata[0] & 0xfc) >> 2;	// Transducer indexes start from 1
	if (id < 1 || id >= TOUCH_COUNT)
		return false;
	x = (tdata[1] & 0xff) | ((short)(tdata[2] & 0x0f) << 8);
	y = ((tdata[2] & 0xf0) >> 4) | ((tdata[3] & 0xff) << 4);
	state = tdata[0] & 0x2;

	input_mt_slot(input, input_mt_get_slot_by_key(input, id));
	input_mt_report_slot_state(input, MT_TOOL_FINGER, state);

	/* Generate the input events for this touch. */
	if (state) {
		input_report_abs(input, ABS_MT_POSITION_X, x);
		input_report_abs(input, ABS_MT_POSITION_Y, TRACKPAD_MAX_Y - y);		// y is inverted
	}
	return state;
}

static int jomaa_raw_event(struct hid_device *hdev,
		struct hid_report *report, u8 *data, int size)
{
	/*
	0b = 11 // report-id
	80 23   // scan time (16 bit)
	00  // 2 bits for buttons, 6 bits of padding
		// figer 1 section
	07  // 6 bits of transducer (finger) index (starts from 1), 1 bit of tip switch, 1 bit of confidence
	fe e1 1e	// 12 bits for X, 12 bits for Y
		// finger 2 section
	0b  // 6 bits of transducer (finger) index (starts from 1), 1 bit of tip switch, 1 bit of confidence
	dd 12 1b
	// fingers 3 and 4
	00 00 00 00
	00 00 00 00
	*/
	struct jomaa_sc *msc = hid_get_drvdata(hdev);
	struct input_dev *input = msc->input;
	unsigned short ii, scantime, clicks = 0;
	bool use_count = false;

	switch (data[0]) {
	case TRACKPAD_REPORT_ID:
		if (size < 20) {
			return 0;
		}
		scantime = ((int)data[2] << 8) | data[1];
		for (ii = 0; ii < TOUCH_COUNT; ii++)
			use_count |= jomaa_emit_touch(msc, ii, data + 4 + ii * 4, scantime);
		clicks = data[3] & 0x3;
		break;
	default:
		return 0;
	}

	input_mt_sync_frame(input);
	jomaa_emit_buttons(msc, clicks);
	// input_mt_report_pointer_emulation(input, true);
	input_sync(input);
	return 1;
}

static int jomaa_setup_input(struct input_dev *input, struct hid_device *hdev)
{
	int error;
	int mt_flags = 0;

	__set_bit(EV_KEY, input->evbit);
	__clear_bit(EV_MSC, input->evbit);
	__clear_bit(BTN_0, input->keybit);
	__clear_bit(BTN_MIDDLE, input->keybit);
	__clear_bit(BTN_RIGHT, input->keybit);
	__set_bit(BTN_MOUSE, input->keybit);
	__set_bit(BTN_RIGHT, input->keybit);
	__set_bit(BTN_TOOL_FINGER, input->keybit);
	__set_bit(BTN_TOOL_DOUBLETAP, input->keybit);
	__set_bit(BTN_TOOL_TRIPLETAP, input->keybit);
	__set_bit(BTN_TOOL_QUADTAP, input->keybit);
	__set_bit(BTN_TOUCH, input->keybit);
	__set_bit(INPUT_PROP_POINTER, input->propbit);
	__set_bit(INPUT_PROP_BUTTONPAD, input->propbit);
	mt_flags = INPUT_MT_POINTER | INPUT_MT_DROP_UNUSED |
			INPUT_MT_TRACK;
	__set_bit(EV_ABS, input->evbit);

	error = input_mt_init_slots(input, TOUCH_COUNT, mt_flags);
	if (error)
		return error;

	input_set_abs_params(input, ABS_MT_POSITION_X,
					TRACKPAD_MIN_X, TRACKPAD_MAX_X, 0, 0);
	input_set_abs_params(input, ABS_MT_POSITION_Y,
					TRACKPAD_MIN_Y, TRACKPAD_MAX_Y, 0, 0);

	input_set_events_per_packet(input, 20);

	/*
	 * hid-input may mark device as using autorepeat, but neither
	 * the trackpad, nor the mouse actually want it.
	 */
	__clear_bit(EV_REP, input->evbit);

	return 0;
}

static int jomaa_input_mapping(struct hid_device *hdev,
		struct hid_input *hi, struct hid_field *field,
		struct hid_usage *usage, unsigned long **bit, int *max)
{
	struct jomaa_sc *sc = hid_get_drvdata(hdev);

	if (!sc->input)
		sc->input = hi->input;

	/* Trackpad does not give relative data after switching to MT */
	if (field->flags & HID_MAIN_ITEM_RELATIVE)
		return -1;

	return 0;
}

static int jomaa_input_configured(struct hid_device *hdev,
		struct hid_input *hi)
{
	struct jomaa_sc *msc = hid_get_drvdata(hdev);
	int ret;

	ret = jomaa_setup_input(msc->input, hdev);
	if (ret) {
		hid_err(hdev, "jomaa setup input failed (%d)\n", ret);
		/* clean msc->input to notify probe() of the failure */
		msc->input = NULL;
		return ret;
	}

	return 0;
}

static int jomaa_enable_multitouch(struct hid_device *hdev)
{
	const u8 *feature;
	const u8 feature_mt[] = { TRACKPAD_REPORT_ID, 0x01 };
	u8 *buf;
	int ret;
	int feature_size;

	feature_size = sizeof(feature_mt);
	feature = feature_mt;

	buf = kmemdup(feature, feature_size, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = hid_hw_raw_request(hdev, buf[0], buf, feature_size,
				HID_FEATURE_REPORT, HID_REQ_SET_REPORT);
	kfree(buf);
	return ret;
}

static void jomaa_enable_mt_work(struct work_struct *work)
{
	struct jomaa_sc *msc =
		container_of(work, struct jomaa_sc, work.work);
	int ret;

	ret = jomaa_enable_multitouch(msc->hdev);
	if (ret < 0)
		hid_err(msc->hdev, "unable to switch trackpad to mt mode (%d)\n", ret);
}

static int jomaa_probe(struct hid_device *hdev,
	const struct hid_device_id *id)
{
	struct jomaa_sc *msc;
	struct hid_report *report;
	int ret;

	msc = devm_kzalloc(&hdev->dev, sizeof(*msc), GFP_KERNEL);
	if (msc == NULL) {
		hid_err(hdev, "can't alloc jomaa descriptor\n");
		return -ENOMEM;
	}

	msc->hdev = hdev;
	INIT_DEFERRABLE_WORK(&msc->work, jomaa_enable_mt_work);

	hid_set_drvdata(hdev, msc);

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "jomaa hid parse failed\n");
		return ret;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "jomaa hw start failed\n");
		return ret;
	}

	if (!msc->input) {
		hid_err(hdev, "jomaa input not registered\n");
		ret = -ENOMEM;
		goto err_stop_hw;
	}

	report = hid_register_report(hdev, HID_INPUT_REPORT,
		TRACKPAD_REPORT_ID, 0);

	if (!report) {
		hid_err(hdev, "unable to register touch report\n");
		ret = -ENOMEM;
		goto err_stop_hw;
	}

	/*
	 * Some devices repond with 'invalid report id' when feature
	 * report switching it into multitouch mode is sent to it.
	 *
	 * This results in -EIO from the _raw low-level transport callback,
	 * but there seems to be no other way of switching the mode.
	 * Thus the super-ugly hacky success check below.
	 */
	ret = jomaa_enable_multitouch(hdev);
	if (ret != -EIO && ret < 0) {
		hid_err(hdev, "unable to request touch data (%d)\n", ret);
		goto err_stop_hw;
	}
	if (ret == -EIO) {
		schedule_delayed_work(&msc->work, msecs_to_jiffies(500));
	}

	return 0;
err_stop_hw:
	hid_hw_stop(hdev);
	return ret;
}

static void jomaa_remove(struct hid_device *hdev)
{
	struct jomaa_sc *msc = hid_get_drvdata(hdev);
	if (msc)
		cancel_delayed_work_sync(&msc->work);
	hid_hw_stop(hdev);
}

static const struct hid_device_id jomaa_device_ids[] = {
	{ HID_BLUETOOTH_DEVICE(0x093A, 0x2860), .driver_data = 0 },
	{ }
};
MODULE_DEVICE_TABLE(hid, jomaa_device_ids);

static struct hid_driver jomaa_driver = {
	.name = "jomaa",
	.id_table = jomaa_device_ids,
	.probe = jomaa_probe,
	.remove = jomaa_remove,
	.raw_event = jomaa_raw_event,
	.input_mapping = jomaa_input_mapping,
	.input_configured = jomaa_input_configured,
};
module_hid_driver(jomaa_driver);

MODULE_AUTHOR("Alexander Baranin");

MODULE_DESCRIPTION("Jomaa LTP-03 trackpad driver for Linux");

MODULE_LICENSE("GPL");
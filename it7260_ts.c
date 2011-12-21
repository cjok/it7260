/*
* touch screen driver for it7260
*/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/pm.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/i2c.h>
#include <linux/timer.h>

#define DEBUG 1
#if DEBUG
#	define dbg_pr(fmt, args...)	printk(fmt, ##args)
#else
#	define dbg_pr(fmt, args...)
#endif

struct it7260_ts_priv {
	struct i2c_client *client;
	struct input_dev *input;
	struct delayed_work work;
	int irq;
};

static void it7260_ts_poscheck(struct work_struct *work)
{
	struct it7260_ts_priv *priv = container_of(work, 
							struct it7260_ts_priv, work.work);
	unsigned char buf[14];
	unsigned short xpos, ypos;
	unsigned char event;

	memset(buf, 0, sizeof(buf));

	if (i2c_master_recv(priv->client, buf, sizeof(buf)) != sizeof(buf)) {
		dbg_pr("unable to read i2c data\n");
		goto out;
	}

	xpos = ((buf[3] & 0x0F) << 8) | buf[2];
	ypos = ((buf[3] & 0xF0) << 4) | buf[4];
	event = buf[5];

	if (event != 0) {
		input_report_key(priv->input, BTN_TOUCH, 1);
		input_report_abs(priv->input, ABS_X, ypos);
		input_report_abs(priv->input, ABS_Y, xpos);
		input_sync(priv->input);
	} else {
		input_report_key(priv->input, BTN_TOUCH, 0);
		input_sync(priv->input);
	}

out:
	enable_irq(priv->irq);
}

static irqreturn_t it7260_ts_isr(int irq, void *dev_id)
{
	struct it7260_ts_priv *priv = dev_id;

	dbg_pr("get irq[8]\n");
	disable_irq_nosync(irq);
	schedule_delayed_work(&priv->work, HZ / 20);

	return IRQ_HANDLED;
}

static int it7260_ts_open(struct input_dev *dev)
{
	struct it7260_ts_priv *priv = input_get_drvdata(dev);

//	enable_irq(priv->irq);
//	it7260_ts_poscheck(&priv->work.work);

	return 0;
}

static void it7260_ts_close(struct input_dev *dev)
{
	struct it7260_ts_priv *priv = input_get_drvdata(dev);

//	disable_irq(priv->irq);
}

static int it7260_ts_probe(struct i2c_client *client, 
			const struct i2c_device_id *idp)
{
	struct it7260_ts_priv *priv;
	struct input_dev *input;
	int error;

	dbg_pr("1.client and driver matched\n");
	client->irq = IRQ_EINT8;
	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dbg_pr("failed to allocate driver data\n");
		error = -ENOMEM;
		goto err0;
	}
	
	dev_set_drvdata(&client->dev, priv);

	input = input_allocate_device();
	if (!input) {
		dbg_pr("failed to allocate input device\n");
		error = -ENOMEM;
		goto err1;
	}

	input->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input, ABS_X, 0, 600, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, 1024, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE, 0, 1, 0, 0);

	input->name = client->name;
	input->phys = "I2C";
	input->id.bustype = BUS_I2C;

	input->open = it7260_ts_open;
	input->close = it7260_ts_close;
	
	input_set_drvdata(input, priv);

	priv->client = client;
	priv->input = input;
	INIT_DELAYED_WORK(&priv->work, it7260_ts_poscheck);
	priv->irq = client->irq;

	error = input_register_device(input);
	if (error) {
		dbg_pr("failed to register input device\n");
		goto err1;
	}

	error = request_irq(priv->irq, it7260_ts_isr, IRQF_TRIGGER_LOW,
						client->name, priv);
	if (error) {
		dbg_pr("unable to request touchscreen IRQ\n");
		goto err2;
	}
	dbg_pr("registered irq[%d]\n", priv->irq);

	device_init_wakeup(&client->dev, 1);
	dbg_pr("2.client and driver matched\n");
	return 0;
	
err2:
	input_unregister_device(input);
	input = NULL;
err1:
	input_free_device(input);
	kfree(priv);
err0:
	dev_set_drvdata(&client->dev, NULL);
	return error;
}

static int it7260_ts_remove(struct i2c_client *client)
{
	struct it7260_ts_priv *priv = dev_get_drvdata(&client->dev);

	free_irq(priv->irq, priv);
	input_unregister_device(priv->input);
	kfree(priv);

	dev_set_drvdata(&client->dev, NULL);

	return 0;
}

static const struct i2c_device_id it7260_ts_id[] = {
	{"IT7260", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, it7260_ts_id);

static struct i2c_driver it7260_ts_driver = {
	.driver = {
		.name = "IT7260-ts",
	},
	.probe = it7260_ts_probe,
	.remove = it7260_ts_remove,
	.id_table = it7260_ts_id,
};


static int __init it7260_ts_init(void)
{
	int ret;
	ret = i2c_add_driver(&it7260_ts_driver);	
	if (ret) {
		dbg_pr("failed to register i2c device driver\n");
	}

	dbg_pr("register i2c device driver suc\n");
	return ret;
}

static void __exit it7260_ts_exit(void)
{
	i2c_del_driver(&it7260_ts_driver);
}

module_init(it7260_ts_init);
module_exit(it7260_ts_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CJOK <cjok.liao@gmail.com>");
MODULE_DESCRIPTION("it7260 touchscreen driver");


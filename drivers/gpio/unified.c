#include <phabos/gpio/unified.h>
#include <errno.h>

static struct gpio_device *find_device(unsigned int line)
{
    struct gpio_device *dev;
    int line_offset = 0;

    for (int devid = 0; (dev = gpio_device_get(devid)); devid++) {
        if (line >= line_offset && line < line_offset + dev->line_count()) {
            return dev;
        }

        line_offset += dev->line_count();
    }

    return NULL;
}

int gpio_get_direction(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->get_direction(line);
}

void gpio_direction_in(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->direction_in(line);
}

void gpio_direction_out(unsigned int line, unsigned int value)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->direction_out(line, value);
}

void gpio_activate(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->activate(line);
}

void gpio_deactivate(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->deactivate(line);
}

int gpio_get_value(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->get_value(line);
}

int gpio_set_value(unsigned int line, unsigned int value)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->set_value(line, value);
}

int gpio_set_debounce(unsigned int line, uint16_t delay)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->set_debounce(line, delay);
}

int gpio_irqattach(unsigned int line, gpio_irq_handler_t handler)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->irqattach(line, handler);
}

int gpio_set_triggering(unsigned int line, int trigger)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->set_triggering(line, trigger);
}

int gpio_mask_irq(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->mask_irq(line);
}

int gpio_unmask_irq(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->unmask_irq(line);
}

int gpio_clear_interrupt(unsigned int line)
{
    struct gpio_device *dev = find_device(&line);

    RET_IF_FAIL(dev, -ENODEV);
    RET_IF_FAIL(dev->ops, -ENODEV);

    return dev->ops->clear_interrupt(line);
}

unsigned int gpio_line_count(void)
{
    static unsigned int line_count;
    struct gpio_device *dev;

    if (line_count != 0)
        return line_count;

    foreach_gpio(dev) {
        line_count += dev->line_count(dev);
    }

    return line_count;
}

static int gpio_probe(struct driver *driver)
{
    return 0;
}

static int gpio_remove(struct driver *driver)
{
    return 0;
}

struct device_driver unified_gpio_device = {
    .name = "unified-gpio"
    .description = "unified GPIO controller interface",
    .class = DEVICE_CLASS_GPIO,
};

struct driver unified_gpio_driver = {
    .name = "unified-gpio",
    .dev = &unified_gpio_device,

    .probe = gpio_probe,
    .remove = gpio_remove,
};

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/math64.h>
#include <linux/input.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Antigravity");
MODULE_DESCRIPTION("Universal Touch Remap LKM via Kprobes");
MODULE_VERSION("1.0");

/* Sysfs Parameter for WebUI */
static int idle_poll_ms = 0;
module_param(idle_poll_ms, int, 0644);
MODULE_PARM_DESC(idle_poll_ms, "Touch Y-Offset (Target 700, Default 0)");

/* Cache variables */
static int cached_offset = 0;
static u64 cached_q = 0;

static int touch_remap_y(int y)
{
    int current_offset = READ_ONCE(idle_poll_ms);
    if (current_offset == 0)
        return y;

    if (current_offset != cached_offset) {
        cached_offset = current_offset;
        /* Q = ( (3200 - offset) << 30 ) / 3200 */
        cached_q = div_u64((u64)(3200 - current_offset) << 30, 3200);
    }

    if (y > current_offset) {
        /* new_y = offset + ( (y - offset) * Q ) >> 30 */
        return current_offset + (int)(( (u64)(y - current_offset) * cached_q ) >> 30);
    }
    
    return y;
}

/* Kprobe pre_handler */
static int handler_pre(struct kprobe *p, struct pt_regs *regs)
{
#ifdef CONFIG_ARM64
    /*
     * input_inject_event(struct input_handle *handle, unsigned int type, unsigned int code, int value)
     * ARM64 calling convention:
     * x0 = handle (regs[0])
     * x1 = type   (regs[1])
     * x2 = code   (regs[2])
     * x3 = value  (regs[3])
     */
    unsigned int type = (unsigned int)regs->regs[1];
    unsigned int code = (unsigned int)regs->regs[2];
    int value = (int)regs->regs[3];

    if (type == EV_ABS && (code == ABS_Y || code == ABS_MT_POSITION_Y || code == ABS_MT_TOOL_Y)) {
        regs->regs[3] = (unsigned long)touch_remap_y(value);
    }
#else
    pr_warn("touch_remap: Only ARM64 is supported for register manipulation.\\n");
#endif
    return 0; // Return 0 to allow the original function to execute
}

static struct kprobe kp = {
    .symbol_name = "input_inject_event",
    .pre_handler = handler_pre,
};

static int __init touch_remap_init(void)
{
    int ret;
    ret = register_kprobe(&kp);
    if (ret < 0) {
        pr_err("touch_remap: register_kprobe failed, returned %d\\n", ret);
        return ret;
    }
    pr_info("touch_remap: Kprobe registered at %p\\n", kp.addr);
    return 0;
}

static void __exit touch_remap_exit(void)
{
    unregister_kprobe(&kp);
    pr_info("touch_remap: Kprobe unregistered\\n");
}

module_init(touch_remap_init);
module_exit(touch_remap_exit);

/*
 * 获取内核全部描述符表
 *
 * 当前仅支持x86
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <asm/desc.h>

/* GDT寄存器结构 */
struct gdtr {
	__u16 limit;
#if   defined(CONFIG_X86)
	__u32 base;
#elif defined(CONFIG_X86_64)
	__u64 base;
#else
#error "not support"
#endif
} __attribute__((packed));

/* GDT描述符结构 - 8字节 */
struct gdt_entry {
	__u16 limit_low;    /* 段界限低16位 */
	__u16 base_low;     /* 基地址低16位 */
	__u8  base_middle;  /* 基地址中8位 */
	__u8  access;       /* 访问权限字节 */
	__u8  granularity;  /* 粒度字节 */
	__u8  base_high;    /* 基地址高8位 */
} __attribute__((packed));

/* 获取段选择子的值 */
static inline __u16 get_segment_selector(int seg)
{
	__u16 val = 0;
	switch(seg) {
	case 0:  /* CS */
		asm volatile("mov %%cs, %0" : "=r"(val));
		break;
	case 1:  /* SS */
		asm volatile("mov %%ss, %0" : "=r"(val));
		break;
	case 2:  /* DS */
		asm volatile("mov %%ds, %0" : "=r"(val));
		break;
	case 3:  /* ES */
		asm volatile("mov %%es, %0" : "=r"(val));
		break;
	case 4:  /* FS */
		asm volatile("mov %%fs, %0" : "=r"(val));
		break;
	case 5:  /* GS */
		asm volatile("mov %%gs, %0" : "=r"(val));
		break;
	}
	return val;
}

/* 打印GDT项的详细信息 */
static void print_gdt_entry(int index, struct gdt_entry *entry)
{
	__u32 base;
	__u32 limit;
	__u8 type, dpl, p, avl, l, d, g;

	/* 计算基地址 */
	base = entry->base_low | (entry->base_middle << 16) | (entry->base_high << 24);

	/* 计算段界限 */
	limit = entry->limit_low | ((entry->granularity & 0x0F) << 16);

	/* 解析访问权限字节 */
	type = entry->access & 0x0F;      /* 段类型 */
	dpl = (entry->access >> 5) & 0x03; /* 特权级 */
	p = (entry->access >> 7) & 0x01;   /* 存在位 */

	/* 解析粒度字节 */
	avl = (entry->granularity >> 4) & 0x01; /* 可用位 */
	l = (entry->granularity >> 5) & 0x01;   /* 64位代码段标志 */
	d = (entry->granularity >> 6) & 0x01;   /* 默认操作数大小 */
	g = (entry->granularity >> 7) & 0x01;   /* 粒度 */

	/* 如果是空描述符，跳过详细信息 */
	if (entry->access == 0 && entry->granularity == 0) {
		printk(KERN_INFO "  [%02d]: NULL\n", index);
		return;
	}

	printk(KERN_INFO "  [%02d]: base=0x%08x limit=0x%05x%s type=0x%x DPL=%d %s%s%s\n",
	       index, base, limit, g ? "(4KB)" : "(1B)",
	       type, dpl,
	       p ? "P" : "-",
	       d ? "D" : "-",
	       avl ? "A" : "-");
}

static int __init dump_gdt_init(void)
{
	struct gdtr gdt_reg;
	struct gdt_entry *gdt_table;
	int i, entries;
	__u16 seg_regs[6];
	const char *seg_names[] = {"CS", "SS", "DS", "ES", "FS", "GS"};

	printk(KERN_INFO "===== GDT Dump Module =====\n");

	/* 获取GDTR寄存器的值 */
	asm volatile("sgdt %0" : "=m"(gdt_reg));
	printk(KERN_INFO "GDTR: base=0x%08x, limit=0x%04x\n",
	       (unsigned int)gdt_reg.base, gdt_reg.limit);

	/* 显示段寄存器的值 */
	printk(KERN_INFO "\nSegment Registers:\n");
	for (i = 0; i < 6; i++) {
		seg_regs[i] = get_segment_selector(i);
		printk(KERN_INFO "  %s = 0x%04x (index=%d, TI=%d, RPL=%d)\n",
		       seg_names[i], seg_regs[i],
		       seg_regs[i] >> 3,           /* 索引 */
		       (seg_regs[i] >> 2) & 0x01,  /* TI位 */
		       seg_regs[i] & 0x03);         /* RPL */
	}

	/* 显示GDT表内容 */
	printk(KERN_INFO "\nGDT Entries:\n");
	gdt_table = (struct gdt_entry *)gdt_reg.base;
	entries = (gdt_reg.limit + 1) / sizeof(struct gdt_entry);

	/* 限制显示的条目数，避免太多输出 */
	if (entries > 32) {
		entries = 32;
		printk(KERN_INFO "  (Showing first 32 entries)\n");
	}

	for (i = 0; i < entries; i++) {
		print_gdt_entry(i, &gdt_table[i]);
	}

	printk(KERN_INFO "===== GDT Dump Complete =====\n");
	return 0;
}

static void __exit dump_gdt_exit(void)
{
	printk(KERN_INFO "GDT Dump Module removed\n");
}

module_init(dump_gdt_init);
module_exit(dump_gdt_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("rivsidn");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Dump x86 Global Descriptor Table");

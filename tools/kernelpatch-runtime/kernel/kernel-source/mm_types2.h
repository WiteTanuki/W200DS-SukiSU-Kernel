#ifndef _LINUX_MM_TYPES_H
#define _LINUX_MM_TYPES_H

#include <ktypes.h>
#include <linux/mm.h>
#include "rbtree.h"
#include "prio_tree.h"
#include "android_vendor.h"

#define _ANDROID_KABI_RESERVE(n)		u64 android_kabi_reserved##n

typedef atomic64_t atomic_long_t;

/*
 * Macros to use _before_ the ABI is frozen
 */

/*
 * ANDROID_KABI_RESERVE
 *   Reserve some "padding" in a structure for potential future use.
 *   This normally placed at the end of a structure.
 *   number: the "number" of the padding variable in the structure.  Start with
 *   1 and go up.
 */
#ifdef CONFIG_ANDROID_KABI_RESERVE
#define ANDROID_KABI_RESERVE(number)	_ANDROID_KABI_RESERVE(number)
#else
#define ANDROID_KABI_RESERVE(number)
#endif

#ifdef CONFIG_USERFAULTFD
#define NULL_VM_UFFD_CTX ((struct vm_userfaultfd_ctx) { NULL, })
struct vm_userfaultfd_ctx {
	struct userfaultfd_ctx *ctx;
};
#else /* CONFIG_USERFAULTFD */
#define NULL_VM_UFFD_CTX ((struct vm_userfaultfd_ctx) {})
struct vm_userfaultfd_ctx {};
#endif /* CONFIG_USERFAULTFD */


struct address_space;
struct mem_cgroup;

struct page;

struct rw_semaphore {
	atomic_long_t count;
	/*
	 * Write owner or one of the read owners as well flags regarding
	 * the current state of the rwsem. Can be used as a speculative
	 * check to see if the write owner is running on the cpu.
	 */
	atomic_long_t owner;
#ifdef CONFIG_RWSEM_SPIN_ON_OWNER
	struct optimistic_spin_queue osq; /* spinner MCS lock */
#endif
	raw_spinlock_t wait_lock;
	struct list_head wait_list;
#ifdef CONFIG_DEBUG_RWSEMS
	void *magic;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	struct lockdep_map	dep_map;
#endif
	ANDROID_VENDOR_DATA(1);
	ANDROID_OEM_DATA_ARRAY(1, 2);
};

struct mm_struct;


typedef struct { unsigned long pgprot; } __pgprot_t;


/*
 * This struct describes a virtual memory area. There is one of these
 * per VM-area/task. A VM area is any part of the process virtual memory
 * space that has a special rule for the page-fault handlers (ie a shared
 * library, the executable area etc).
 */
struct vm_area_struct {
	/* The first cache line has the info for VMA tree walking. */

	unsigned long vm_start;		/* Our start address within vm_mm. */
	unsigned long vm_end;		/* The first byte after our end address
					   within vm_mm. */

	/* linked list of VM areas per task, sorted by address */
	struct vm_area_struct *vm_next, *vm_prev;

	struct rb_node vm_rb;

	/*
	 * Largest free memory gap in bytes to the left of this VMA.
	 * Either between this VMA and vma->vm_prev, or between one of the
	 * VMAs below us in the VMA rbtree and its ->vm_prev. This helps
	 * get_unmapped_area find a free area of the right size.
	 */
	unsigned long rb_subtree_gap;

	/* Second cache line starts here. */

	struct mm_struct *vm_mm;	/* The address space we belong to. */

	/*
	 * Access permissions of this VMA.
	 * See vmf_insert_mixed_prot() for discussion.
	 */
	__pgprot_t vm_page_prot;
	unsigned long vm_flags;		/* Flags, see mm.h. */

	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 *
	 * For private anonymous mappings, a pointer to a null terminated string
	 * in the user process containing the name given to the vma, or NULL
	 * if unnamed.
	 */
	union {
		struct {
			struct rb_node rb;
			unsigned long rb_subtree_last;
		} shared;
		const char __user *anon_name;
	};

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_lock &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */

	/* Function pointers to deal with this struct. */
	const struct vm_operations_struct *vm_ops;

	/* Information about our backing store: */
	unsigned long vm_pgoff;		/* Offset (within vm_file) in PAGE_SIZE
					   units */
	struct file * vm_file;		/* File we map to (can be NULL). */
	void * vm_private_data;		/* was vm_pte (shared mem) */

#ifdef CONFIG_SWAP
	atomic_long_t swap_readahead_info;
#endif
#ifndef CONFIG_MMU
	struct vm_region *vm_region;	/* NOMMU mapping region */
#endif
#ifdef CONFIG_NUMA
	struct mempolicy *vm_policy;	/* NUMA policy for the VMA */
#endif
	struct vm_userfaultfd_ctx vm_userfaultfd_ctx;
#ifdef CONFIG_SPECULATIVE_PAGE_FAULT
	seqcount_t vm_sequence;
	atomic_t vm_ref_count;		/* see vma_get(), vma_put() */
#endif
	ANDROID_KABI_RESERVE(1);
	ANDROID_KABI_RESERVE(2);
	ANDROID_KABI_RESERVE(3);
	ANDROID_KABI_RESERVE(4);
} __randomize_layout;

struct vm_area_struct_offset
{
    int16_t vm_start_offset;
    int16_t vm_end_offset;
    int16_t vm_next_offset, vm_prev_offset;
    int16_t vm_rb_offset;
    int16_t rb_subtree_gap_offset;
    int16_t vm_mm_offset;
    int16_t vm_page_prot_offset;
    int16_t vm_flags_offset;
    int16_t shared_rb_offset;
    int16_t shared_rb_subtree_last_offset;
    int16_t anon_name_offset;
    int16_t anon_vma_chain_offset;
    int16_t anon_vma_offset;
    int16_t vm_ops_offset;
    int16_t vm_pgoff_offset;
    int16_t vm_file_offset;
    int16_t vm_private_data_offset;
    int16_t swap_readahead_info_offset;
    int16_t vm_region_offset;
    int16_t vm_policy_offset;
    int16_t vm_userfaultfd_ctx_offset;
    int16_t vm_sequence_offset;
    int16_t vm_ref_count_offset;
};

struct mm_struct_offset
{
    int16_t mmap_base_offset;
    int16_t task_size_offset;
    int16_t pgd_offset;
    int16_t map_count_offset;
    int16_t total_vm_offset;
    int16_t locked_vm_offset;
    int16_t pinned_vm_offset;
    int16_t data_vm_offset;
    int16_t exec_vm_offset;
    int16_t stack_vm_offset;
    int16_t start_code_offset, end_code_offset, start_data_offset, end_data_offset;
    int16_t start_brk_offset, brk_offset, start_stack_offset;
    int16_t arg_start_offset, arg_end_offset, env_start_offset, env_end_offset;
};

extern struct mm_struct_offset mm_struct_offset;

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

#endif
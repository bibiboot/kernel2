#include "types.h"
#include "globals.h"
#include "kernel.h"

#include "util/gdb.h"
#include "util/init.h"
#include "util/debug.h"
#include "util/string.h"
#include "util/printf.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/pagetable.h"
#include "mm/pframe.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "main/acpi.h"
#include "main/apic.h"
#include "main/interrupt.h"
#include "main/cpuid.h"
#include "main/gdt.h"

#include "proc/sched.h"
#include "proc/proc.h"
#include "proc/kthread.h"

#include "drivers/dev.h"
#include "drivers/blockdev.h"
#include "drivers/tty/virtterm.h"

#include "api/exec.h"
#include "api/syscall.h"

#include "fs/vfs.h"
#include "fs/vnode.h"
#include "fs/vfs_syscall.h"
#include "fs/fcntl.h"
#include "fs/stat.h"

#include "test/kshell/kshell.h"

GDB_DEFINE_HOOK(boot)
GDB_DEFINE_HOOK(initialized)
GDB_DEFINE_HOOK(shutdown)

static void      *bootstrap(int arg1, void *arg2);
static void      *idleproc_run(int arg1, void *arg2);
static kthread_t *initproc_create(void);
static void      *initproc_run(int arg1, void *arg2);
static void       hard_shutdown(void);
extern void *vfstest_main(int, void*);
extern void *testproc();
static context_t bootstrap_context;
extern int Sunghantest(kshell_t *k,int arg1, char **);
extern int sunghan_test();
extern int sunghan_deadlock_test();
static int gdb_wait = GDBWAIT;
/**
 * This is the first real C function ever called. It performs a lot of
 * hardware-specific initialization, then creates a pseudo-context to
 * execute the bootstrap function in.
 */
void
kmain()
{
        GDB_CALL_HOOK(boot);

        dbg_init();
        dbgq(DBG_CORE, "Kernel binary:\n");
        dbgq(DBG_CORE, "  text: 0x%p-0x%p\n", &kernel_start_text, &kernel_end_text);
        dbgq(DBG_CORE, "  data: 0x%p-0x%p\n", &kernel_start_data, &kernel_end_data);
        dbgq(DBG_CORE, "  bss:  0x%p-0x%p\n", &kernel_start_bss, &kernel_end_bss);

        page_init();

        pt_init();
        slab_init();
        pframe_init();

        acpi_init();
        apic_init();
        intr_init();

        gdt_init();

        /* initialize slab allocators */
#ifdef __VM__
        anon_init();
        shadow_init();
#endif
        vmmap_init();
        proc_init();
        kthread_init();

#ifdef __DRIVERS__
        bytedev_init();
        blockdev_init();
#endif

        void *bstack = page_alloc();
        pagedir_t *bpdir = pt_get();
        KASSERT(NULL != bstack && "Ran out of memory while booting.");
	/* This little loop gives gdb a place to synch up with weenix.  In the
	 * past the weenix command started qemu was started with -S which
	 * allowed gdb to connect and start before the boot loader ran, but
	 * since then a bug has appeared where breakpoints fail if gdb connects
	 * before the boot loader runs.  See
	 *
	 * https://bugs.launchpad.net/qemu/+bug/526653
	 *
	 * This loop (along with an additional command in init.gdb setting
	 * gdb_wait to 0) sticks weenix at a known place so gdb can join a
	 * running weenix, set gdb_wait to zero  and catch the breakpoint in
	 * bootstrap below.  See Config.mk for how to set GDBWAIT correctly.
	 *
	 * DANGER: if GDBWAIT != 0, and gdb isn't run, this loop will never
	 * exit and weenix will not run.  Make SURE the GDBWAIT is set the way
	 * you expect.
	 */
      	while (gdb_wait) ;
        context_setup(&bootstrap_context, bootstrap, 0, NULL, bstack, PAGE_SIZE, bpdir);
        context_make_active(&bootstrap_context);

        panic("\nReturned to kmain()!!!\n");
}

#ifdef __DRIVERS__

        int Sunghantest(kshell_t *kshell, int argc, char **argv)
        {
            KASSERT(kshell != NULL);
        
	   sunghan_test();
	 
	    
	 /*  sunghan_deadlock_test();*/
	   
/*	    dbg(DBG_INIT, "(GRADING): do_foo() is invoked, argc = %d, argv = 0x%08x\n",
            argc, (unsigned int)argv);*/
            return 0;
        }

	int Sunghandeadlock(kshell_t *kshell, int argc, char **argv)
	{
	KASSERT(kshell!=NULL);
	sunghan_deadlock_test();
	return 0;
	}

	int fabertest(kshell_t *kshell, int argc, char **argv)
	{

		KASSERT(kshell!=NULL);
		testproc();
		return 0;

	}

       int vfstestmain(kshell_t *kshell, int argc, char **argv)
        {

                KASSERT(kshell!=NULL);
                vfstest_main(1, NULL);
                return 0;

        }

#endif /* __DRIVERS__ */

/**
 * This function is called from kmain, however it is not running in a
 * thread context yet. It should create the idle process which will
 * start executing idleproc_run() in a real thread context.  To start
 * executing in the new process's context call context_make_active(),
 * passing in the appropriate context. This function should _NOT_
 * return.
 *
 * Note: Don't forget to set curproc and curthr appropriately.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
bootstrap(int arg1, void *arg2)
{
        /* necessary to finalize page table information */
   
    
      
	dbg(DBG_INIT, "\nBOOTSTRAP: INSIDE BOOTSTRAP\n");
        pt_template_init();
	
 	proc_t* process0=proc_create("Idle_Process");
	kthread_t* thread0=kthread_create(process0,idleproc_run,1,(void*)1);
	
	curproc=process0;
	curthr=thread0;    
	
        KASSERT(NULL != curproc); /* make sure that the "idle" process has been created successfully */
        dbg(DBG_INIT,"(GRADING1  1.a)  Idle Process Created Successfully\n");
	KASSERT(PID_IDLE == curproc->p_pid); /* make sure that what has been created is the "idle" process */
	dbg(DBG_INIT,"(GRADING1 1.a)  Idle Process's pid assigned properly\n");
        KASSERT(NULL != curthr); /* make sure that the thread for the "idle" process has been created successfully */
	dbg(DBG_INIT,"(GRADING1 1.a)  Thread for idle process created successfully\n");
/*	dbg(DBG_INIT, "\nBOOTSTRAP: MOVING FROM BOOTSTRAP TO IDLEPROC_RUN\n");*/
	/*sched_make_runnable(curthr); */    
	context_make_active(&thread0->kt_ctx);

        return 0;

}

/**
 * Once we're inside of idleproc_run(), we are executing in the context of the
 * first process-- a real context, so we can finally begin running
 * meaningful code.
 *
 * This is the body of process 0. It should initialize all that we didn't
 * already initialize in kmain(), launch the init process (initproc_run),
 * wait for the init process to exit, then halt the machine.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
idleproc_run(int arg1, void *arg2)
{
        int status;
        pid_t child;
        /* create init proc */
        kthread_t *initthr = initproc_create();
/*	dbg(DBG_INIT,"created initproc");
        dbg(DBG_INIT,"%d",curproc->p_pid);*/
	init_call_all();
        GDB_CALL_HOOK(initialized);

        /* Create other kernel threads (in order) */

#ifdef __VFS__
        /* Once you have VFS remember to set the current working directory
         * of the idle and init processes */
         curproc->p_cwd = vfs_root_vn;
         initthr->kt_proc->p_cwd = vfs_root_vn;
         vref(vfs_root_vn);
         vref(vfs_root_vn);

        /* Here you need to make the null, zero, and tty devices using mknod */
        /* You can't do this until you have VFS, check the include/drivers/dev.h
         * file for macros with the device ID's you will need to pass to mknod */
        /*NOT_YET_IMPLEMENTED("VFS: idleproc_run");*/
        /*TODO Dont know When VFS will be formed*/

        if(do_mkdir("/dev") >=  0)
        {
        dbg(DBG_PRINT,"(GRADING2C) Creating null, zero and tty0\n");
        /*do_mkdir("/dev");*/
        /*Block devices*/
        int status1=do_mknod("/dev/null", S_IFCHR, MEM_NULL_DEVID); 
	
        int status2=do_mknod("/dev/zero", S_IFCHR, MEM_ZERO_DEVID); 
	 
        int status3=do_mknod("/dev/tty0", S_IFCHR, MKDEVID(2, 0)); 
          
        }
#endif

        /* Finally, enable interrupts (we want to make sure interrupts
         * are enabled AFTER all drivers are initialized) */
        intr_enable();

        /* Run initproc */
        sched_make_runnable(initthr);
        /* Now wait for it */
        child = do_waitpid(-1, 0, &status);
        KASSERT(PID_INIT == child);

#ifdef __MTP__
        kthread_reapd_shutdown();
#endif


#ifdef __VFS__
        /* Shutdown the vfs: */
        dbg_print("weenix: vfs shutdown...\n");
        vput(curproc->p_cwd);
        if (vfs_shutdown())
                panic("vfs shutdown FAILED!!\n");

#endif

        /* Shutdown the pframe system */
#ifdef __S5FS__
        pframe_shutdown();
#endif

        dbg_print("\nweenix: halted cleanly!\n");
        GDB_CALL_HOOK(shutdown);
        hard_shutdown();
        return NULL;
}

/**
 * This function, called by the idle process (within 'idleproc_run'), creates the
 * process commonly refered to as the "init" process, which should have PID 1.
 *
 * The init process should contain a thread which begins execution in
 * initproc_run().
 *
 * @return a pointer to a newly created thread which will execute
 * initproc_run when it begins executing
 */
static kthread_t *
initproc_create(void)
{
      	
	proc_t *process1=proc_create("Init_process");
 	KASSERT(process1!=NULL);
	dbg(DBG_INIT,"(GRADING1 1.b)  Init Process created successfully\n" );
	kthread_t *thread1=kthread_create(process1,initproc_run,1,(void*)1);
	
	KASSERT(PID_INIT == process1->p_pid);
	dbg(DBG_INIT,"(GRADING1 1.b)  Init process's pid assigned properly\n");
        KASSERT(thread1 != NULL);
	dbg(DBG_INIT,"(GRADING1 1.b)  Init process's thread created successfully\n");
/*	dbg(DBG_INIT,"thread1_returned");*/
	

	return thread1;
}

/**
 * The init thread's function changes depending on how far along your Weenix is
 * developed. Before VM/FI, you'll probably just want to have this run whatever
 * tests you've written (possibly in a new process). After VM/FI, you'll just
 * exec "/bin/init".
 *
 * Both arguments are unused.
 *
 * @param arg1 the first argument (unused)
 * @param arg2 the second argument (unused)
 */
static void *
initproc_run(int arg1, void *arg2)
{
       
#ifdef __DRIVERS__
	
        kshell_add_command("Sunghan test", Sunghantest, "\n");
	kshell_add_command("Faber Test",fabertest,"\n");
	kshell_add_command("deadlock test",Sunghandeadlock,"\n");
	kshell_add_command("vfstest_main", vfstestmain,"\n");

        kshell_t *kshell = kshell_create(0);
        if (NULL == kshell) panic("init: Couldn't create kernel shell\n");
        while(kshell_execute_next(kshell));
        kshell_destroy(kshell);

#endif /* __DRIVERS__ */
	
    /*   dbg(DBG_INIT, "\n..........RUNNING INITPROC AWESOMELY .......\n");*/
        return NULL;
}
	
/**
 * Clears all interrupts and halts, meaning that we will never run
 * again.
 */
static void
hard_shutdown()
{
#ifdef __DRIVERS__
        vt_print_shutdown();
#endif
        __asm__ volatile("cli; hlt");
}

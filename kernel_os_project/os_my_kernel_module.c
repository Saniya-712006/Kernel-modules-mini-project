#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/list.h>


#define NUM_CHILDREN 5

static int input_a=20;
static int input_b=5;

module_param(input_a,int,0644);
module_param(input_b,int,0644);
MODULE_PARM_DESC(input_a,"First integer for arithmetic operations");
MODULE_PARM_DESC(input_b,"Second integer for arithmetic operations");


int calc_gcd(int a, int b);
int child_function(void *data);


struct arithmetic_data {
    int a,b;
    int sum,difference,product;
    int gcd;

};

struct child_info{
    struct task_struct *task;
    struct list_head list;
};

static LIST_HEAD(child_list);



int calc_gcd(int a, int b)
{
    while(b!=0)
    {
        int temp=b;
        b=a%b;
        a=temp;
    }
    return a;
}


int child_function(void *data)
{
    struct arithmetic_data *arith= kmalloc(sizeof(struct arithmetic_data),GFP_KERNEL);
    if(!arith)
    {
        printk(KERN_ERR "Memory allocation failed for arithmetic data\n");
        return -ENOMEM;
    }

    arith->a=input_a;
    arith->b=input_b;
    arith->sum= arith->a + arith->b;
    arith->difference= arith->a - arith->b;
    arith->product= (arith->a)*(arith->b);
    arith->gcd=calc_gcd(arith->a,arith->b);

    printk(KERN_INFO "Child Process (PID: %d): GCD= %d ,Sum= %d, Difference= %d, Product= %d\n",current->pid,arith->gcd,arith->sum,arith->difference,arith->product);
    
     kfree(arith);
    while(!kthread_should_stop())
    {
        msleep(500);
    }
    
    printk(KERN_INFO "Child thread %d stopping.\n", current->pid);
   
 
    return 0;
}

static int __init my_kern_module_init(void)
{
    int i;
    printk(KERN_INFO "Kernel Module Loaded: Parent PID= %d\n",current->pid);

    for(i=0;i<NUM_CHILDREN;i++)
    {
       struct task_struct *child_task = kthread_run(child_function,NULL,"childThread-%d",i);

        if(IS_ERR(child_task))
        {
            printk(KERN_ERR "Failed to create child thread %d\n",i);
            
        }
        else
        {
            struct child_info *child= kmalloc(sizeof(struct child_info),GFP_KERNEL);
            if(!child)
            {
                printk(KERN_ERR "Memory allocation failed for child_info\n");
                continue;
            }
            child->task=child_task;
            list_add(&child->list,&child_list);
            printk(KERN_INFO "Created child thread %d with PID %d\n", i,child_task->pid);
        }
    }
    return 0;
}

static void __exit my_kern_module_exit(void)
{
    struct child_info *child,*tmp;
    printk(KERN_INFO "Kernel module unloaded. Printing child process tree:\n");
    
    list_for_each_entry(child,&child_list,list)
    {
        printk(KERN_INFO "Child PID : %d (Parent PID : %d)\n",child->task->pid, child->task->real_parent->pid);
    }

    list_for_each_entry_safe(child,tmp,&child_list,list)
    {
        if(child->task && !IS_ERR(child->task))
        {
            
                kthread_stop(child->task);
            
            
        }
        list_del(&child->list);
        kfree(child);
    }
}    

module_init(my_kern_module_init);
module_exit(my_kern_module_exit);

//METADATA INFO

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Saniya");
MODULE_DESCRIPTION("Kernel module that created child processes, performs arithmetic operations with gcd computation, and tracks memory.");
    .text

    /* Switch from current_thread to next_thread. Make next_thread
    * the current_thread, and set next_thread to 0.
    * Use eax as a temporary register; it is caller saved.
    */
    .globl thread_switch
    thread_switch:
        /* YOUR CODE HERE */
        // save current state to current_thread
        pushal
        movl current_thread, %eax
        movl %esp, (%eax)

        // set current_thread to next_thread
        // set next_thread to 0

        //movl next_thread,	current_thread
        //movl 0, next_thread

        leal current_thread, %eax
        movl next_thread, %ebx
        movl %ebx, (%eax)
        leal next_thread, %eax
        movl $0, (%eax)

        // restore from current_thread
        movl current_thread, %eax
        movl (%eax), %esp
        popal
        // pop return address from stack
        ret

#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>



mailbox_t mbox_open(char *name)
{
    mailbox_t mailbox;
    mailbox.id = invoke_syscall(SYSCALL_MAILBOX_OPEN, (uintptr_t)name, IGNORE, IGNORE,IGNORE);
    return mailbox;
}

void mbox_close(mailbox_t mailbox)
{
    // TODO:
    invoke_syscall(SYSCALL_MAILBOX_CLOSE, mailbox.id, IGNORE, IGNORE,IGNORE);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MAILBOX_SEND, mailbox.id, (uintptr_t)msg, msg_length,IGNORE);
}

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    return invoke_syscall(SYSCALL_MAILBOX_RECV, mailbox.id, (uintptr_t)msg, msg_length,IGNORE);

}

int multi_mail(mailbox_t mailbox, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MULMAIL ,mailbox.id, (uintptr_t)msg, msg_length,IGNORE);
}

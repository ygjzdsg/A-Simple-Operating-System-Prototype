#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#define MAX_MBOX_LENGTH (64)

#include <os/list.h>
// TODO: please define mailbox_t;
// mailbox_t is just an id of kernel's mail box.
typedef struct mailbox
{
    int id; 
} mailbox_t;


typedef struct mthread_mailbox_t
{
    char name[MAX_MBOX_LENGTH];                   
    char msg[MAX_MBOX_LENGTH];       
    int index;                                     
    int valid;
    list_head full_queue;
    list_head empty_queue;
} mthread_mailbox_t;


mthread_mailbox_t whole_mailboxes[16];


mailbox_t mbox_open(char *);
void mbox_close(mailbox_t);
int mbox_send(mailbox_t, void *, int);
int mbox_recv(mailbox_t, void *, int);
int multi_mail(mailbox_t, void *, int);
#endif

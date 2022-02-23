#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>


EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;
int recv_flag = 0;
LIST_HEAD(recv_block_queue);

volatile int rx_curr = 0, rx_tail = 0;


long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // TODO: 
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    uint64_t cpu_id = get_current_cpu_id();
    while (recv_flag){
        do_block(&current_running[cpu_id]->list, &recv_block_queue);
    }
        
    recv_flag = 1;
    long status;
    int current_packet = 0;
    int remain_packet;
    while (current_packet < num_packet){
        remain_packet = num_packet - current_packet;
        if (remain_packet <= 32){
            EmacPsRecv(&EmacPsInstance, (EthernetFrame *)rx_buffers, (remain_packet));
            EmacPsWaitRecv(&EmacPsInstance, remain_packet, rx_len);
            for (int i=0;i<remain_packet; i++){
                kmemcpy((uint8_t *)addr,(uint8_t *)&rx_buffers[i], rx_len[i]);
                addr=addr+rx_len[i];
                frLength[i+current_packet]=rx_len[i];
            }
        }else{
            EmacPsRecv(&EmacPsInstance, (EthernetFrame *)rx_buffers, 32);
            EmacPsWaitRecv(&EmacPsInstance, 32, rx_len);
            for (int i = 0; i < 32; i++){
                kmemcpy((uint8_t *)addr, (uint8_t *)&rx_buffers[i], rx_len[i]);
                addr=addr+rx_len[i];
                frLength[i+current_packet]=rx_len[i];
            }
        }
        current_packet=current_packet+32;
    }
    recv_flag = 0;
    while (!is_empty(&recv_block_queue)){
        do_unblock(recv_block_queue.next);
    }
    return status;

}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    // send all packet
    // maybe you need to call drivers' send function multiple times ?
    kmemcpy((uint8_t *)tx_buffer, (uint8_t *)addr, sizeof(EthernetFrame));
    // set TX descripter and enable mac
    EmacPsSend(&EmacPsInstance, (EthernetFrame *)tx_buffer, length); 
    EmacPsWaitSend(&EmacPsInstance); 
}

void do_net_irq_mode(int mode)
{
    // TODO:
    // turn on/off network driver's interrupt mode
    if (mode==1){ 
       XEmacPs_IntEnable(&EmacPsInstance, (XEMACPS_IXR_TX_ERR_MASK | XEMACPS_IXR_RX_ERR_MASK | (u32)XEMACPS_IXR_FRAMERX_MASK | (u32)XEMACPS_IXR_TXCOMPL_MASK))
    }else{
       XEmacPs_IntDisable(&EmacPsInstance, (XEMACPS_IXR_TX_ERR_MASK | XEMACPS_IXR_RX_ERR_MASK | (u32)XEMACPS_IXR_FRAMERX_MASK | (u32)XEMACPS_IXR_TXCOMPL_MASK))
    }
    net_poll_mode = mode;
}

#include <stdio.h>

#define FIFO_CTRL_RADDR 1
#define FIFO_CTRL_WADDR 2
#define READ_LIMIT 1000
typedef unsigned int uint32;
typedef unsigned int U32;

typedef struct
{
  uint32 a;
  uint32 b;
} FifoCmd;

FifoCmd hw_add_cmd_queue[32];

union FifoCtrlWrite
{
   uint32 value;
   struct
   {
      uint32 w_idx : 15; 
      uint32 w_page: 1;
      uint32 DUMMY : 16;
   };
};
U32 read_reg32(U32 addr)
{
    return 1;
}/* return FifoCtrlWrite register*/
void write_reg32(U32 addr, U32 value_in)
{
  return;
} /*write value_in into FifoCtrlWrite register*/

union FifoCtrlRead
{
   uint32 value;
   struct
   {
      uint32 r_idx : 15; 
      uint32 r_page: 1;
      uint32 DUMMY : 16;
   };
};

uint32 hw_add(uint32 a, uint32 b, uint32 *sum)
{
    uint32 w_idx, r_idx, read_count=0;

    union FifoCtrlRead rinfo;
    union FifoCtrlWrite winfo;
    
    rinfo.value = read_reg32(FIFO_CTRL_RADDR); /* Read Register */
    winfo.value = read_reg32(FIFO_CTRL_WADDR); /* Write Register */
    r_idx = rinfo.r_idx;
    w_idx = winfo.w_idx;

    /*
     * Check if FIFO cmd queue if full by comparing w_idx and r_idx.
     * If queue is full, keep polling r_idx and comparing again, 
     * until HW-ADD finishes adding and updates r_idx, so that
     * FIFO cmd queue has enough space for next input cmd.
     * Assume that:
     *   Queue is empty if w_idx = r_idx
     *   Queue is full if (w_idx+1)%32 = r_idx
     */
    while ( (w_idx+1)%32 == r_idx) {
       rinfo.value = read_reg32(FIFO_CTRL_RADDR);
       r_idx = rinfo.r_idx;

       read_count++;
       if (read_count > READ_LIMIT)/* Wait too long, return fail*/
           return 0;
    }

    /* Fill input cmd with number a and b */
    hw_add_cmd_queue[w_idx].a = a;
    hw_add_cmd_queue[w_idx].b = b;

    /* Update w_idx to indicate HW-ADD with new input cmd */
    winfo.w_idx = (winfo.w_idx+1)%32;

    /* Input new cmd to HW-ADD */
    write_reg32(FIFO_CTRL_WADDR, winfo.value);
    
    /*
     * Check if HW-ADD finishes the new input cmd by comparing w_idx and r_idx.
     * If r_idx equals to w_idx, the HW-ADD finishes the new input cmd.
     * If not, keep monitoring r_idx until it is equal to w_idx.
     */
    rinfo.value = read_reg32(FIFO_CTRL_RADDR); /* Read Register */
    read_count = 0;
    while ( w_idx != r_idx) {
       rinfo.value = read_reg32(FIFO_CTRL_RADDR);
       r_idx = rinfo.r_idx;

       read_count++;
       if (read_count > READ_LIMIT)/* Wait too long, return fail*/
           return 0;
    }
    
    /* Get the sum from cmd queues's a */
    *sum = hw_add_cmd_queue[w_idx].a;

    return 1;
}


int main()
{
    uint32 num1, num2, sum, status=1;

    num1 = 123;
    num2 = 456;

    if ( !hw_add(num1, num2, &sum)) {
        printf("Failed\n");
        status = 0;
    } 

    return status;
}

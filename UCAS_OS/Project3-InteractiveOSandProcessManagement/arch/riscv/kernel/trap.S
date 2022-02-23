#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
/*  la t0,exception_handler_entry 
  csrw CSR_STVEC,t0

  csrr t0, CSR_MHARTID
  bnez t0, core1_exception
  csrw CSR_SIP, zero

core1_exception:
  li s0, SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw CSR_SIE, s0
  jr ra */

  la t0, exception_handler_entry 
  csrw CSR_STVEC, t0
  li t0, SIE_STIE
  csrw CSR_SIE  , t0
  jr ra
ENDPROC(setup_exception)

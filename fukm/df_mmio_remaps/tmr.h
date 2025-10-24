/**
 * The function values directly comes from the PPR.
 * For DF_TMR_FUNC:        D18F7xA00...D18F7xAA8 DF::TmrBaseAddr
 */
#define DF_TMR_FUNC                     (0x7)

#define TMR_BASE_ADDR_OFFSET    (0xA00)   /* DF::TmrBaseAddr  */
#define TMR_LIMIT_ADDR_OFFSET   (0xA04)   /* DF::TmrLimitAddr */
#define TMR_CTL_OFFSET          (0xA08)   /* DF::TmrCtl       */
#define TMR_FIDA_OFFSET         (0xA0C)   /* DF::TmrFidA      */
#define TMR_FIDB_OFFSET         (0xA10)   /* DF::TmrFidB      */
#define TMR_EXT_ADDR_OFFSET     (0xA14)   /* DF::TmrExtAddr   */

#define TMR_OFFSET              (0x18)

#define TMR_TRUST_LVL_LSBS_SHIFT    (0)
#define TMR_TRUST_LVL_LSBS_MASK     (0xFul << (TMR_TRUST_LVL_LSBS_SHIFT))

#define TMR_TRUST_LVL_SHIFT         (17)
#define TMR_TRUST_LVL_MASK          (0x7 << TMR_TRUST_LVL_SHIFT)

#define TMR_ADDR_MASK               (0xFFFFFFFFull << TMR_X86_PHYS_ADDR_SHIFT)

/* DF::TmrExtAddr */
#define TMR_EXT_SHIFT               (48)
#define TMR_EXT_MASK                (0xFFull << TMR_EXT_SHIFT)
#define TMR_BASE_ADDR_EXT_SHIFT     (0)
#define TMR_BASE_ADDR_EXT_MASK      (0xFFull << TMR_BASE_ADDR_EXT_SHIFT)
#define TMR_LIMIT_ADDR_EXT_SHIFT    (16)
#define TMR_LIMIT_ADDR_EXT_MASK     (0xFFull << TMR_LIMIT_ADDR_EXT_SHIFT)


/**
 * Trusted memory region registers
 * http://twiki.amd.com/twiki/bin/view/DFArch/CoherentSlave#MAS
 * Genoa has 9 TMRs. TMR0 + 8 additional TMRs
 * There are 6 registers per TMR
 *      Offset 0xA00 = TmrBaseAddr0
 *      Offset 0xA04 = TmrLimitAddr0
 *      Offset 0xA08 = TmrCtl
 *      Offset 0xA0C = TmrFidA
 *      Offset 0xA10 = TmrFidB
 *      Offset 0xA14 = TmrExtAddr
 */
/*
 * CNLI + UMC keys are propagated, what are CNLI Keys??????
*/
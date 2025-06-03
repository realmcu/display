#include "rtl876x.h"
#include "stdbool.h"

/*============================================================================*
 *                         PPE Registers Memory Map
 *============================================================================*/
typedef struct
{
    __IO uint32_t REG_GLB_STATUS;                       /* offset 0x00*/
    __IO uint32_t REG_LYR_ENABLE;                       /* offset 0x04*/
    __IO uint32_t REG_RELOAD_CFG;                       /* offset 0x08*/
    __IO uint32_t REG_LL_CFG;                           /* offset 0x0C*/
    __IO uint32_t REG_LLP;                              /* offset 0x10*/
    __I  uint32_t REG_RESERVED1;                        /* offset 0x14*/
    __IO uint32_t REG_SECURE;                           /* offset 0x18*/
    __IO uint32_t REG_LINE_OVER_INDEX;                  /* offset 0x1C*/
    __IO uint32_t CLUT_INDEX;                           /* offset 0x20*/
    __IO uint32_t CLUT_CONT;                            /* offset 0x24*/
    __I  uint32_t REG_RESERVED4[6];                     /* offset 0x28-0x3C*/
    __I  uint32_t REG_INTR_STATUS;                      /* offset 0x40*/
    __I  uint32_t REG_INTR_RAW;                         /* offset 0x44*/
    __O  uint32_t REG_INTR_CLR;                         /* offset 0x48*/
    __IO uint32_t REG_INTR_MASK;                        /* offset 0x4C*/
    __I  uint32_t REG_BUS_ERR_DETAIL;                   /* offset 0x50*/
    __I  uint32_t REG_DIV0_ERR_DETAIL;                  /* offset 0x54*/
    __I  uint32_t REG_RESERVED5[7];                     /* offset 0x58-0x70*/\
    __I  uint32_t REG_IP_ID;                            /* offset 0x74*/
    __I  uint32_t REG_RELEASE_DATA;                     /* offset 0x78*/
    __I  uint32_t REG_RTL_VERSION;                      /* offset 0x7C*/
} PPE_Typedef;

typedef struct
{
    __IO uint32_t REG_LYR0_PIC_CFG;             /* offset 0x80*/
    __IO uint32_t REG_LYR0_BG;                  /* offset 0x84*/
    __IO uint32_t REG_LYR0_WIN_MIN;             /* offset 0x88*/
    __IO uint32_t REG_LYR0_WIN_MAX;             /* offset 0x8C*/
    __IO uint32_t REG_LYR0_BUS_CFG;             /* offset 0x90*/
    __IO uint32_t REG_LYR0_HS_CFG;              /* offset 0x94*/
    __IO uint32_t REG_LYR0_ADDR;                /* offset 0x98*/
    __I  uint32_t REG_RESERVED1;                /* offset 0x9C*/
    __IO uint32_t REG_LYR0_LINE_LEN;            /* offset 0xA0*/
    __IO uint32_t REG_LYR0_BLK_SIZE;            /* offset 0xA4*/
    __I  uint32_t REG_RESERVED2[22];            /* offset 0xA8-0xFC*/
} PPE_ResultLayer_Typedef;


typedef struct
{
    __IO uint32_t REG_LYRx_PIC_CFG;                             /* offset 0x00 + X * 0x80*/
    __IO uint32_t REG_LYRx_CONST_PIX;                           /* offset 0x04 + X * 0x80*/
    __IO uint32_t REG_LYRx_WIN_MIN;                             /* offset 0x08 + X * 0x80*/
    __IO uint32_t REG_LYRx_WIN_MAX;                             /* offset 0x0C + X * 0x80*/
    __IO uint32_t REG_LYRx_BUS_CFG;                             /* offset 0x14 + X * 0x80*/
    __IO uint32_t REG_LYRx_HS_CFG;                              /* offset 0x18 + X * 0x80*/
    __IO uint32_t REG_LYRx_ADDR;                                /* offset 0x1C + X * 0x80*/
    __I  uint32_t REG_RESERVED1;                                /* offset 0x20 + X * 0x80*/
    __IO uint32_t REG_LYRx_LINE_LEN;                            /* offset 0x24 + X * 0x80*/
    __IO uint32_t REG_LYRx_PIC_SIZE;                            /* offset 0x28 + X * 0x80*/
    __IO uint32_t REG_LYRx_KEY_MIN;                             /* offset 0x2C + X * 0x80*/
    __IO uint32_t REG_LYRx_KEY_MAX;                             /* offset 0x30 + X * 0x80*/
    __IO uint32_t REG_LYRx_KEY_REPLACE;                         /* offset 0x34 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E11;                    /* offset 0x38 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E12;                    /* offset 0x3C + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E13;                    /* offset 0x40 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E21;                    /* offset 0x44 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E22;                    /* offset 0x48 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E23;                    /* offset 0x4C + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E31;                    /* offset 0x50 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E32;                    /* offset 0x54 + X * 0x80*/
    __IO uint32_t REG_LYRx_TRANS_MATRIX_E33;                    /* offset 0x58 + X * 0x80*/
    __IO uint32_t REG_RESERVED2[10];                            /* offset (0x58-0x7C) + X * 0x80*/
} PPE_Input_Layer_Typedef;

/*============================================================================*
 *                         Registers Definitions
 *============================================================================*/
/* Peripheral:PPE*/
/* Description:PPEregister defines */


/* Register: PPE_REG_GLB_STATUS -----------------------------------------------------*/
/* Description: . Offset: 0x0000. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t run_state:     3;
        __I  uint32_t reserved:     12;
        __I  uint32_t chn_inact:     17;
    } b;
} PPE_REG_GLB_STATUS_TypeDef;

/* Register: PPE_REG_LYR_ENABLE -----------------------------------------------------*/
/* Description: . Offset: 0x0004. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t rsvd:     1;
        __IO uint32_t input_lyr_1_en:     1;
        __IO uint32_t input_lyr_2_en:     1;
        __I  uint32_t reserved:     29;
    } b;
} PPE_REG_LYR_ENABLE_TypeDef;

/* Register: PPE_REG_LD_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0008. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t input_lyr_1_reload_en:     1;
        __IO uint32_t input_lyr_2_reload_en:     1;
        __I  uint32_t reserved1:         14;
        __I  uint32_t set_valid:     1;
        __I  uint32_t auto_clr:     1;
        __I  uint32_t reserved2:     14;
    } b;
} PPE_REG_RELOAD_CFG_TypeDef;

/* Register: PPE_REG_LL_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x000C. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t input_lyr_1_ll_en:     1;
        __IO uint32_t input_lyr_2_ll_en:     1;
        __I  uint32_t reserved1:         14;
        __I  uint32_t ll_arsize:     3;
        __IO uint32_t ll_incr:     1;
        __IO uint32_t ll_arcache:     4;
        __IO uint32_t ll_max_arlen_log:     3;
        __I  uint32_t reserved2:     1;
        __IO uint32_t ll_prior:     4;
    } b;
} PPE_REG_LL_CFG_TypeDef;

/* Register: PPE_REG_LLP -----------------------------------------------------*/
/* Description: . Offset: 0x0010. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t reserved:     2;
        __IO uint32_t llp:     30;
    } b;
} PPE_REG_LLP_TypeDef;

/* Register: PPE_REG_SECURE -----------------------------------------------------*/
/* Description: . Offset: 0x0018. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t secure:     1;
        __I  uint32_t rsvd:     31;
    } b;
} PPE_REG_SECURE_TypeDef;

/* Register: PPE_REG_LINE_OVER_INDEX -----------------------------------------------------*/
/* Description: . Offset: 0x001C. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t line_over_int_index:     11;
        __I  uint32_t reserved:      21;
    } b;
} PPE_REG_LINE_OVER_INDEX_TypeDef;

/* Register: PPE_REG_INTR_STATUS -----------------------------------------------------*/
/* Description: . Offset: 0x0040. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t all_over:     1;
        __I  uint32_t fr_over:     1;
        __I  uint32_t load_over:     1;
        __I  uint32_t line_over:     1;
        __I  uint32_t susp_inac:     1;
        __I  uint32_t secure_err:     1;
        __I  uint32_t set_err:     1;
        __I  uint32_t bus_err:     1;
        __I  uint32_t div0_err:     1;
        __I  uint32_t rsvd:     23;
    } b;
} PPE_REG_INTR_STATUS_TypeDef;

/* Register: PPE_REG_INTR_RAW -----------------------------------------------------*/
/* Description: . Offset: 0x0044. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t all_over:     1;
        __I  uint32_t fr_over:     1;
        __I  uint32_t load_over:     1;
        __I  uint32_t line_over:     1;
        __I  uint32_t susp_inac:     1;
        __I  uint32_t secure_err:     1;
        __I  uint32_t set_err:     1;
        __I  uint32_t bus_err:     1;
        __I  uint32_t div0_err:     1;
        __I  uint32_t rsvd:     23;
    } b;
} PPE_REG_INTR_RAW_TypeDef;

/* Register: PPE_REG_INTR_CLR -----------------------------------------------------*/
/* Description: . Offset: 0x0048. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __O  uint32_t all_over:     1;
        __O  uint32_t fr_over:     1;
        __O  uint32_t load_over:     1;
        __O  uint32_t line_over:     1;
        __O  uint32_t susp_inac:     1;
        __O  uint32_t secure_err:     1;
        __I  uint32_t set_err:     1;
        __O  uint32_t bus_err:     1;
        __O  uint32_t div0_err:     1;
        __I  uint32_t rsvd:     23;
    } b;
} PPE_REG_INTR_CLR_TypeDef;

/* Register: PPE_REG_INTR_MASK -----------------------------------------------------*/
/* Description: . Offset: 0x004C. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t all_over:     1;
        __IO uint32_t fr_over:     1;
        __IO uint32_t load_over:     1;
        __IO uint32_t line_over:     1;
        __IO uint32_t susp_inac:     1;
        __IO uint32_t secure_err:     1;
        __I  uint32_t set_err:     1;
        __IO uint32_t bus_err:     1;
        __IO uint32_t div0_err:     1;
        __I  uint32_t rsvd:     23;
    } b;
} PPE_REG_INTR_MASK_TypeDef;

/* Register: PPE_REG_BUS_ERR_DETAIL -----------------------------------------------------*/
/* Description: . Offset: 0x0050. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t bus_err_detail_rsvd:     17;
        __I  uint32_t rsvd:     15;
    } b;
} PPE_REG_BUS_ERR_DETAIL_TypeDef;

/* Register: PPE_REG_DIV0_ERR_DETAIL -----------------------------------------------------*/
/* Description: . Offset: 0x0054. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t rsvd:     2;
        __I  uint32_t input_layer2_div0_err_detail:     1;
        __I  uint32_t rsvd1:     16;
    } b;
} PPE_REG_DIV0_ERR_DETAIL_TypeDef;

/* Register: PPE_REG_LYR0_BUS_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0074. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t ip_id:     32;
    } b;
} PPE_REG_LYR0_IP_ID_TypeDef;

/* Register: PPE_REG_RELEASE_DATE -----------------------------------------------------*/
/* Description: . Offset: 0x0078. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t release_date:     32;
    } b;
} PPE_REG_RELEASE_DATE_TypeDef;

/* Register: PPE_REG_RTL_VER -----------------------------------------------------*/
/* Description: . Offset: 0x007C. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t rtl_ver:     32;
    } b;
} PPE_REG_RTL_VER_TypeDef;

/* Register: PPE_REG_LYR0_PIC_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t format:     8;
        __I  uint32_t reserved:     24;
    } b;
} PPE_REG_LYR0_PIC_CFG_TypeDef;

/* Register: PPE_REG_BACKGROUND -----------------------------------------------------*/
/* Description: . Offset: 0x0084. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t background:     32;
    } b;
} PPE_REG_LYR0_BACKGROUND_TypeDef;

/* Register: PPE_REG_LYR0_WIN_MIN -----------------------------------------------------*/
/* Description: . Offset: 0x0088. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t win_x_min:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t win_y_min:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYR0_WIN_MIN_TypeDef;

/* Register: PPE_REG_LYR0_WIN_MAX -----------------------------------------------------*/
/* Description: . Offset: 0x008C. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t win_x_max:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t win_y_max:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYR0_WIN_MAX_TypeDef;

/* Register: PPE_REG_LYR0_BUS_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0090. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t axsize:     3;
        __IO uint32_t incr:     1;
        __IO uint32_t axcache:     4;
        __IO uint32_t max_awlen_log:     3;
        __I  uint32_t rsvd:     1;
        __I  uint32_t priority:     4;
        __I  uint32_t rsvd1:     16;
    } b;
} PPE_REG_LYR0_BUS_CFG_TypeDef;

/* Register: PPE_REG_LYR0_HS_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0094. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I uint32_t reserved: 32;
    } b;
} PPE_REG_LYR0_HS_CFG_TypeDef;

/* Register: PPE_REG_LYR0_ADDR -----------------------------------------------------*/
/* Description: . Offset: 0x0098. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t addr:     32;
    } b;
} PPE_REG_LYR0_ADDR_TypeDef;

/* Register: PPE_REG_LYR0_LINE_LEN -----------------------------------------------------*/
/* Description: . Offset: 0x00A0. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t line_len:     32;
    } b;
} PPE_REG_LYR0_LINE_LEN_TypeDef;

/* Register: PPE_REG_LYR0_LINE_LEN -----------------------------------------------------*/
/* Description: . Offset: 0x00A0. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t width:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t height:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYR0_BLK_SIZE_TypeDef;


/* Register: PPE_REG_LYRx_PIC_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0080 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t format:     8;
        __IO uint32_t pic_src:     1;
        __I  uint32_t rsvd1:     1;
        __IO uint32_t interpolation:     1;
        __I  uint32_t rsvd2:     1;
        __IO uint32_t abf:     4;
        __IO uint32_t key_en:     4;
        __IO uint32_t key_mode:     1;
        __I  uint32_t reserved:     11;
    } b;
} PPE_REG_LYRx_PIC_CFG_TypeDef;

/* Register: PPE_REG_LYRx_FIXED_COLOR -----------------------------------------------------*/
/* Description: . Offset: 0x0084 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t const_pixel:     32;
    } b;
} PPE_REG_LYRx_CONST_PIX_TypeDef;

/* Register: PPE_REG_LYRx_WIN_MIN -----------------------------------------------------*/
/* Description: . Offset: 0x0088 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t win_x_min:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t win_y_min:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYRx_WIN_MIN_TypeDef;

/* Register: PPE_REG_LYRx_WIN_MAX -----------------------------------------------------*/
/* Description: . Offset: 0x008C + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t win_x_max:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t win_y_max:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYRx_WIN_MAX_TypeDef;

/* Register: PPE_REG_LYRx_BUS_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0090 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t arsize:     3;
        __IO uint32_t incr:     1;
        __IO uint32_t arcache:     4;
        __IO uint32_t max_arlen_log:     3;
        __I  uint32_t rsvd:     21;
    } b;
} PPE_REG_LYRx_BUS_CFG_TypeDef;

/* Register: PPE_REG_LYRx_HS_CFG -----------------------------------------------------*/
/* Description: . Offset: 0x0094 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __I  uint32_t rsvd: 32;
    } b;
} PPE_REG_LYRx_HS_CFG_TypeDef;

/* Register: PPE_REG_LYRx_ADDR -----------------------------------------------------*/
/* Description: . Offset: 0x0098 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t addr: 32;
    } b;
} PPE_REG_LYRx_ADDR_TypeDef;

/* Register: PPE_REG_LYRx_LINE_LEN -----------------------------------------------------*/
/* Description: . Offset: 0x00A0 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t line_len:     16;
        __I  uint32_t rsvd:     16;
    } b;
} PPE_REG_LYRx_LINE_LEN_TypeDef;

/* Register: PPE_REG_LYRx_PIC_SIZE -----------------------------------------------------*/
/* Description: . Offset: 0x00A4 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t width:     11;
        __I  uint32_t reserved1:     5;
        __IO uint32_t height:     11;
        __I  uint32_t reserved2:     5;
    } b;
} PPE_REG_LYRx_PIC_SIZE_TypeDef;

/* Register: PPE_REG_LYRx_KEY_MIN -----------------------------------------------------*/
/* Description: . Offset: 0x00A8 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t r:     8;
        __IO uint32_t g:     8;
        __IO uint32_t b:     8;
        __I  uint32_t reserved:     8;
    } b;
} PPE_REG_LYRx_KEY_MIN_TypeDef;

/* Register: PPE_REG_LYRx_KEY_MAX -----------------------------------------------------*/
/* Description: . Offset: 0x00AC + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t r:     8;
        __IO uint32_t g:     8;
        __IO uint32_t b:     8;
        __I  uint32_t reserved:     8;
    } b;
} PPE_REG_LYRx_KEY_MAX_TypeDef;

/* Register: PPE_REG_LYRx_KEY_REPLACE -----------------------------------------------------*/
/* Description: . Offset: 0x00B0 + x * 0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t r:     8;
        __IO uint32_t g:     8;
        __IO uint32_t b:     8;
        __IO uint32_t a:     8;
    } b;
} PPE_REG_LYRx_KEY_REPLACE_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E11 -----------------------------------------------------*/
/* Description: . Offset: 0x00B4 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e11:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E11_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E12 -----------------------------------------------------*/
/* Description: . Offset: 0x00B8 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e12:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E12_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E13 -----------------------------------------------------*/
/* Description: . Offset: 0x00BC +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e13:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E13_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E21 -----------------------------------------------------*/
/* Description: . Offset: 0x00C0 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e21:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E21_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E22 -----------------------------------------------------*/
/* Description: . Offset: 0x00C4 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e22:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E22_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E23 -----------------------------------------------------*/
/* Description: . Offset: 0x00C8 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e23:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E23_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E31 -----------------------------------------------------*/
/* Description: . Offset: 0x00CC +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e31:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E31_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E32 -----------------------------------------------------*/
/* Description: . Offset: 0x00D0 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e32:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E32_TypeDef;

/* Register: PPE_REG_LYRx_TRANS_MATRIX_E33 -----------------------------------------------------*/
/* Description: . Offset: 0x00D4 +  x *0x0080. Address: . */
typedef union
{
    uint32_t d32;
    uint8_t d8[4];
    struct
    {
        __IO uint32_t matrix_e33:     32;
    } b;
} PPE_REG_LYRx_TRANS_MATRIX_E33_TypeDef;

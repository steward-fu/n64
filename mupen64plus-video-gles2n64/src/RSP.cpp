#include <math.h>
#include "Common.h"
#include "gles2N64.h"
#include "OpenGL.h"
#include "Debug.h"
#include "RSP.h"
#include "RDP.h"
#include "N64.h"
#include "F3D.h"
#include "3DMath.h"
#include "VI.h"
#include "ShaderCombiner.h"
#include "DepthBuffer.h"
#include "GBI.h"
#include "gSP.h"
#include "Textures.h"
#ifdef __ARM_NEON__
#include "arm_neon.h"
#endif

//#define PRINT_DISPLAYLIST
//#define PRINT_DISPLAYLIST_NUM 1

RSPInfo     RSP;

void RSP_LoadMatrix( f32 mtx[4][4], u32 address )
{

    f32 recip = 1.5258789e-05f;

    struct _N64Matrix
    {
        s16 integer[4][4];
        u16 fraction[4][4];
    } *n64Mat = (struct _N64Matrix *)&RDRAM[address];
#ifdef __ARM_NEON__
    // Load recip
    float32_t _recip = recip;

    // Load integer
    int16x4_t _integer0_s16 = vld1_s16(n64Mat->integer[0]);
    int16x4_t _integer1_s16 = vld1_s16(n64Mat->integer[1]);
    int16x4_t _integer2_s16 = vld1_s16(n64Mat->integer[2]);
    int16x4_t _integer3_s16 = vld1_s16(n64Mat->integer[3]);

    // Load fraction
    uint16x4_t _fraction0_u16 = vld1_u16(n64Mat->fraction[0]);
    uint16x4_t _fraction1_u16 = vld1_u16(n64Mat->fraction[1]);
    uint16x4_t _fraction2_u16 = vld1_u16(n64Mat->fraction[2]);
    uint16x4_t _fraction3_u16 = vld1_u16(n64Mat->fraction[3]);

    // Reverse 16bit values --> j^1
    _integer0_s16 = vrev32_s16 (_integer0_s16);                 // 0 1 2 3 --> 1 0 3 2
    _integer1_s16 = vrev32_s16 (_integer1_s16);                 // 0 1 2 3 --> 1 0 3 2 
    _integer2_s16 = vrev32_s16 (_integer2_s16);                 // 0 1 2 3 --> 1 0 3 2
    _integer3_s16 = vrev32_s16 (_integer3_s16);                 // 0 1 2 3 --> 1 0 3 2
    _fraction0_u16 = vrev32_u16 (_fraction0_u16);               // 0 1 2 3 --> 1 0 3 2
    _fraction1_u16 = vrev32_u16 (_fraction1_u16);               // 0 1 2 3 --> 1 0 3 2 
    _fraction2_u16 = vrev32_u16 (_fraction2_u16);               // 0 1 2 3 --> 1 0 3 2 
    _fraction3_u16 = vrev32_u16 (_fraction3_u16);               // 0 1 2 3 --> 1 0 3 2
    
    // Expand to 32Bit int/uint
    int32x4_t _integer0_s32 = vmovl_s16(_integer0_s16);         // _integer0_s32 = (i32)_integer0_s16
    int32x4_t _integer1_s32 = vmovl_s16(_integer1_s16);         // _integer1_s32 = (i32)_integer1_s16
    int32x4_t _integer2_s32 = vmovl_s16(_integer2_s16);         // _integer2_s32 = (i32)_integer2_s16
    int32x4_t _integer3_s32 = vmovl_s16(_integer3_s16);         // _integer3_s32 = (i32)_integer3_s16
    uint32x4_t _fraction0_u32 = vmovl_u16(_fraction0_u16);      // _fraction0_u32 = (u32)_fraction0_u16
    uint32x4_t _fraction1_u32 = vmovl_u16(_fraction1_u16);      // _fraction1_u32 = (u32)_fraction1_u16
    uint32x4_t _fraction2_u32 = vmovl_u16(_fraction2_u16);      // _fraction2_u32 = (u32)_fraction2_u16
    uint32x4_t _fraction3_u32 = vmovl_u16(_fraction3_u16);      // _fraction3_u32 = (u32)_fraction3_u16
    
    // Convert to Float
    float32x4_t _integer0_f32 = vcvtq_f32_s32 (_integer0_s32);  // _integer0_f32 = (f32)_integer0_s32
    float32x4_t _integer1_f32 = vcvtq_f32_s32 (_integer1_s32);  // _integer1_f32 = (f32)_integer1_s32 
    float32x4_t _integer2_f32 = vcvtq_f32_s32 (_integer2_s32);  // _integer2_f32 = (f32)_integer2_s32 
    float32x4_t _integer3_f32 = vcvtq_f32_s32 (_integer3_s32);  // _integer3_f32 = (f32)_integer3_s32 
    float32x4_t _fraction0_f32 = vcvtq_f32_u32 (_fraction0_u32);// _fraction0_f32 = (f32)_fraction0_u32 
    float32x4_t _fraction1_f32 = vcvtq_f32_u32 (_fraction1_u32);// _fraction1_f32 = (f32)_fraction1_u32 
    float32x4_t _fraction2_f32 = vcvtq_f32_u32 (_fraction2_u32);// _fraction2_f32 = (f32)_fraction2_u32
    float32x4_t _fraction3_f32 = vcvtq_f32_u32 (_fraction3_u32);// _fraction3_f32 = (f32)_fraction3_u32

    // Multiply and add
    _integer0_f32 = vmlaq_n_f32(_integer0_f32,_fraction0_f32,_recip);// _integer0_f32 = _integer0_f32 + _fraction0_f32* _recip
    _integer1_f32 = vmlaq_n_f32(_integer1_f32,_fraction1_f32,_recip);// _integer1_f32 = _integer1_f32 + _fraction1_f32* _recip
    _integer2_f32 = vmlaq_n_f32(_integer2_f32,_fraction2_f32,_recip);// _integer2_f32 = _integer2_f32 + _fraction2_f32* _recip
    _integer3_f32 = vmlaq_n_f32(_integer3_f32,_fraction3_f32,_recip);// _integer3_f32 = _integer3_f32 + _fraction3_f32* _recip

    // Store in mtx
    vst1q_f32(mtx[0], _integer0_f32);
    vst1q_f32(mtx[1], _integer1_f32);
    vst1q_f32(mtx[2], _integer2_f32);
    vst1q_f32(mtx[3], _integer3_f32);
#else
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            mtx[i][j] = (GLfloat)(n64Mat->integer[i][j^1]) + (GLfloat)(n64Mat->fraction[i][j^1]) * recip;
#endif
}

void RSP_ProcessDList()
{
    VI_UpdateSize();
    OGL_UpdateScale();
    TextureCache_ActivateNoise(2);

    RSP.PC[0] = *(u32*)&DMEM[0x0FF0];
    RSP.PCi = 0;
    RSP.count = 0;

    RSP.halt = FALSE;
    RSP.busy = TRUE;

#ifdef __TRIBUFFER_OPT
    __indexmap_clear();
#endif

    gSP.matrix.stackSize = min( 32, *(u32*)&DMEM[0x0FE4] >> 6 );
    gSP.matrix.modelViewi = 0;
    gSP.changed |= CHANGED_MATRIX;

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            gSP.matrix.modelView[0][i][j] = 0.0f;

    gSP.matrix.modelView[0][0][0] = 1.0f;
    gSP.matrix.modelView[0][1][1] = 1.0f;
    gSP.matrix.modelView[0][2][2] = 1.0f;
    gSP.matrix.modelView[0][3][3] = 1.0f;

    u32 uc_start = *(u32*)&DMEM[0x0FD0];
    u32 uc_dstart = *(u32*)&DMEM[0x0FD8];
    u32 uc_dsize = *(u32*)&DMEM[0x0FDC];

    if ((uc_start != RSP.uc_start) || (uc_dstart != RSP.uc_dstart))
        gSPLoadUcodeEx( uc_start, uc_dstart, uc_dsize );

    gDPSetAlphaCompare(G_AC_NONE);
    gDPSetDepthSource(G_ZS_PIXEL);
    gDPSetRenderMode(0, 0);
    gDPSetAlphaDither(G_AD_DISABLE);
    gDPSetColorDither(G_CD_DISABLE);
    gDPSetCombineKey(G_CK_NONE);
    gDPSetTextureConvert(G_TC_FILT);
    gDPSetTextureFilter(G_TF_POINT);
    gDPSetTextureLUT(G_TT_NONE);
    gDPSetTextureLOD(G_TL_TILE);
    gDPSetTextureDetail(G_TD_CLAMP);
    gDPSetTexturePersp(G_TP_PERSP);
    gDPSetCycleType(G_CYC_1CYCLE);
    gDPPipelineMode(G_PM_NPRIMITIVE);

#ifdef PRINT_DISPLAYLIST
    if ((RSP.DList%PRINT_DISPLAYLIST_NUM) == 0) LOG(LOG_VERBOSE, "BEGIN DISPLAY LIST %i \n", RSP.DList);
#endif

    while (!RSP.halt)
    {
        u32 pc = RSP.PC[RSP.PCi];

        if ((pc + 8) > RDRAMSize)
        {
#ifdef DEBUG
            DebugMsg( DEBUG_LOW | DEBUG_ERROR, "ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n" );
#endif
            break;
        }


        u32 w0 = *(u32*)&RDRAM[pc];
        u32 w1 = *(u32*)&RDRAM[pc+4];
        RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[pc+8], 24, 8 );
        RSP.cmd = _SHIFTR( w0, 24, 8 );
        RSP.PC[RSP.PCi] += 8;

#ifdef PROFILE_GBI
        GBI_ProfileBegin(RSP.cmd);
#endif

#ifdef PRINT_DISPLAYLIST
        if ((RSP.DList%PRINT_DISPLAYLIST_NUM) == 0) LOG(LOG_VERBOSE, "%s: w0=0x%x w1=0x%x\n", GBI_GetFuncName(GBI.current->type, RSP.cmd), w0, w1);
#endif

        GBI.cmd[RSP.cmd]( w0, w1 );

#ifdef PROFILE_GBI
        GBI_ProfileEnd(RSP.cmd);
#endif
    }

#ifdef PRINT_DISPLAYLIST
        if ((RSP.DList%PRINT_DISPLAYLIST_NUM) == 0) LOG(LOG_VERBOSE, "END DISPLAY LIST %i \n", RSP.DList);
#endif

    RSP.busy = FALSE;
    RSP.DList++;
    gSP.changed |= CHANGED_COLORBUFFER;
}

void RSP_Init()
{
    RDRAMSize = 1024 * 1024 * 8;
    RSP.DList = 0;
    RSP.uc_start = RSP.uc_dstart = 0;
    gDP.loadTile = &gDP.tiles[7];
    gSP.textureTile[0] = &gDP.tiles[0];
    gSP.textureTile[1] = &gDP.tiles[1];

    DepthBuffer_Init();
    GBI_Init();
}


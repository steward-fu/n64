#include "gSP.h"
#include "OpenGL.h"

// Clobber lists for vfp registers
#define VFP_CLOBBER_HALF1 \
    "s0", "s1", "s2", "s3"

#define VFP_CLOBBER_HALF2 \
    "s8", "s9", "s10", "s11"

#define VFP_CLOBBER_HALF3 \
    "s16", "s17", "s18", "s19"

#define VFP_CLOBBER_HALF4 \
    "s24", "s25", "s26", "s27"

#define VFP_CLOBBER_BANK1 \
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7"

#define VFP_CLOBBER_BANK2 \
    "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15"

#define VFP_CLOBBER_BANK3 \
    "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23"

#define VFP_CLOBBER_BANK4 \
    "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"

#define VFP_CLOBBER_ALL \
    VFP_CLOBBER_BANK1, \
    VFP_CLOBBER_BANK2, \
    VFP_CLOBBER_BANK3, \
    VFP_CLOBBER_BANK4

// Reset vector length & stride
#define VFP_VEC_LEN_RESET \
    "fmrx       r0, fpscr           \n\t" \
    "bic        r0, r0, #0x00370000 \n\t" \
    "fmxr       fpscr, r0           \n\t"

// Set vector length. VEC_LENGTH must be between 0 and 3. The VFP vector
// length will be set to VEC_LENGTH+1
#define VFP_VEC_LEN(VEC_LEN) \
    "fmrx       r0, fpscr                      \n\t" \
    "bic        r0, r0, #0x00370000            \n\t" \
    "orr        r0, r0, #0x000" #VEC_LEN "0000 \n\t" \
    "fmxr       fpscr, r0                      \n\t"

static void gSPTransformVertex_vfp(float vtx[4], const float mtx[4][4])
{
    // transforms vertex 'vtx' using tranform matrix 'mtx'
    // loads and fmacs are interleaved to better utilise pipelines
    volatile void* m2 = (void*) &mtx[2][0];
    asm volatile(
        "fldmias        %[v], {s0-s2}      \n\t"    // load vtx
        "fldmias        %[m2]!, {s16-s19}  \n\t"    // load mtx[2]
        "fldmias        %[m2], {s8-s11}    \n\t"    // load mtx[3]

        "fmacs          s8, s16, s2        \n\t"    // s8 = mtx[3][0] + z * mtx[2][0]
        "fmacs          s9, s17, s2        \n\t"    // s9 = mtx[3][1] + z * mtx[2][1]
        "fldmias        %[m]!, {s24-s27}   \n\t"    // load mtx[0]
        "fmacs          s10, s18, s2       \n\t"    // s10 = mtx[3][2] + z * mtx[2][2]
        "fmacs          s11, s19, s2       \n\t"    // s11 = mtx[3][3] + z * mtx[2][3]

        "fmacs          s8, s24, s0        \n\t"    // s8 += x * mtx[0][0]
        "fmacs          s9, s25, s0        \n\t"    // s9 += x * mtx[0][1]
        "fldmias        %[m]!, {s16-s19}   \n\t"    // load m[1]
        "fmacs          s10, s26, s0       \n\t"    // s10 += x * mtx[0][2]
        "fmacs          s11, s27, s0       \n\t"    // s11 += x * mtx[0][3]

        "fmacs          s8, s16, s1        \n\t"    // s8 += y * mtx[1][0]
        "fmacs          s9, s17, s1        \n\t"    // s9 += y * mtx[1][1]
        "fmacs          s10, s18, s1       \n\t"    // s10 += y * mtx[1][2]
        "fmacs          s11, s19, s1       \n\t"    // s11 += y * mtx[1][3]

        "fstmias        %[v], {s8-s11}     \n\t"    // save vtx
 
        : [v] "+r" (vtx), [m] "+r" (mtx)
        : [m2] "r" (m2)
        : "memory", VFP_CLOBBER_HALF1, VFP_CLOBBER_HALF2, VFP_CLOBBER_HALF3, VFP_CLOBBER_HALF4
    );
}

static void gSPLightVertex_vfp(u32 v)
{

    volatile int i = gSP.numLights;
    volatile void *ptr0 = &gSP.lights[0].r;
    volatile void *ptr1 = &gSP.lights[gSP.numLights].r;
    volatile void *ptr2 = &OGL.triangles.vertices[v].nx;
    volatile void *ptr3 = gSP.matrix.modelView[gSP.matrix.modelViewi];

    volatile int one = 0x3f800000;
    asm volatile(
        "flds       s0, [%[ptr2], #0]     \n\t"    // load vec[0]
        "fldmias    %[ptr3]!, {s1-s3}     \n\t"    // load mtx[0][0] to mtx[0][2]
        "add        %[ptr3], %[ptr3], #4  \n\t"    // skip mtx[0][3]

        "fmuls      s12, s0, s1           \n\t"    // s12 = vec[0] * m[0][0]
        "flds       s4, [%[ptr2], #4]     \n\t"    // load vec[1]
        "fmuls      s13, s0, s2           \n\t"    // s13 = vec[0] * m[0][1]

        "fldmias    %[ptr3]!, {s5-s7}     \n\t"    // load mtx[1][0] to mtx[1][2]
        "add        %[ptr3], %[ptr3], #4  \n\t"    // skip mtx[1][3]
        "fmuls      s14, s0, s3           \n\t"    // s14 = vec[0] * m[0][2]
        "fmacs      s12, s4, s5           \n\t"    // s12 += vec[1] * mtx[1][0]
  
        "flds       s8, [%[ptr2], #8]     \n\t"    // load vec[2]
        "fmacs      s13, s4, s6           \n\t"    // s13 += vec[1] * mtx[1][1]
        "fldmias    %[ptr3], {s9-s11}     \n\t"    // load mtx into vec regs
        "fmacs      s14, s4, s7           \n\t"    // s14 += vec[2] * mtx[1][2]

        "fmacs      s12, s8, s9           \n\t"    // s12 += vec[2] * mtx[2][0]
        "fmacs      s13, s8, s10          \n\t"    // s13 += vec[2] * mtx[2][1]
        "fmacs      s14, s8, s11          \n\t"    // s14 += vec[2] * mtx[2][2]
        
        "vmov       s15, %[one]           \n\t"

        "fmuls      s1, s12, s12          \n\t"
        "fmacs      s1, s13, s13          \n\t"
        "fmacs      s1, s14, s14          \n\t"    // s1 = x*x + y*y + z*z

        "fmrs       r0, s1                \n\t"    // move s1 to arm reg
        "cmp        r0, #0                \n\t"    // if r0 == 0:
        "beq        0f                    \n\t"    //   return

        "fsqrts     s1, s1                \n\t"    // s1 = sqrt(s1) == len(v_trans)
        "fdivs      s15, s15, s1          \n\t"

        "fmuls      s0, s12, s15          \n\t"    // normalise v_trans
        "fmuls      s1, s13, s15          \n\t"    // normalise v_trans
        "fmuls      s2, s14, s15          \n\t"    // normalise v_trans
        
        "fstmias    %[ptr2]!, {s0-s2}     \n\t"

        "0:                              \n\t"
        "fldmias    %[ptr1]!, {s3-s5}    \n\t"

        "tst        %[nlights], #1       \n\t"
        "beq        2f                   \n"

        "1:                              \n\t"
        "fldmias    %[ptr0]!, {s6-s11}   \n\t"
        "fmuls      s9, s9, s0           \n\t"
        "fmacs      s9, s10, s1          \n\t"
        "fmacs      s9, s11, s2          \n\t"

        "fmrs       r0, s9               \n\t" 
        "ands       r0, r0, #0x80000000  \n\t"
        "fmsrne     s9, r0               \n\t"

        "fmacs      s3, s9, s6           \n\t"
        "fmacs      s4, s9, s7           \n\t"
        "fmacs      s5, s9, s8           \n"

        "subs       %[nlights], #1       \n\t"
        "beq        3f                   \n"

        "2:                              \n\t"
        "fldmias    %[ptr0]!, {s6-s11}   \n\t"
        "fmuls      s9, s9, s0           \n\t"
        "fmacs      s9, s10, s1          \n\t"
        "fmacs      s9, s11, s2          \n\t"

        "fldmias    %[ptr0]!, {s12-s17}  \n\t"
        "fmuls      s15, s15, s0         \n\t"
        "fmacs      s15, s16, s1         \n\t"
        "fmacs      s15, s17, s2         \n\t"

        "fmrs       r0, s9               \n\t" 
        "ands       r0, r0, #0x80000000  \n\t"
        "fmsrne     s9, r0               \n\t"

        "fmacs      s3, s9, s6           \n\t"
        "fmacs      s4, s9, s7           \n\t"
        "fmacs      s5, s9, s8           \n\t"

        "fmrs       r0, s15              \n\t" 
        "ands       r0, r0, #0x80000000  \n\t"
        "fmsrne     s15, r0              \n\t"

        "fmacs      s3, s15, s12         \n\t"
        "fmacs      s4, s15, s13         \n\t"
        "fmacs      s5, s15, s14         \n\t"

        "subs       %[nlights], #2       \n\t"
        "bne        2b                   \n"

        "3:                              \n\t"
        "vmov       r0, r1, s3, s4       \n\t"
        "vmov       r2, s5               \n\t"

        "cmp        r0, #0x3f800000      \n\t"
        "vmovgt     s3, %[one]         \n\t"

        "cmp        r1, #0x3f800000      \n\t"
        "vmovgt     s4, %[one]         \n\t"

        "cmp        r2, #0x3f800000      \n\t"
        "vmovgt     s5, %[one]         \n\t"

        "add        %[ptr2], %[ptr2], #4 \n\t"
        "fstmias    %[ptr2], {s3-s5}     \n\t"

        : [ptr0] "+r" (ptr0), [ptr1] "+r" (ptr1), [ptr2] "+r" (ptr2), 
          [ptr3] "+r" (ptr3), [one] "+r" (one)
        : [nlights] "r" (i)
        : "r0", "r1", "r2", "cc", "memory", VFP_CLOBBER_BANK1, VFP_CLOBBER_BANK2, "s16", "s17"
    );
}

void gSPInitVFP()
{
    gSPTransformVertex = gSPTransformVertex_vfp;
    gSPLightVertex = gSPLightVertex_vfp;
}

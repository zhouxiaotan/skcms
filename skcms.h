/*
 * Copyright 2018 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

// skcms.h contains the entire public API for skcms.

#ifndef SKCMS_API
    #define SKCMS_API
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

// A row-major 3x3 matrix (ie vals[row][col])
typedef struct skcms_Matrix3x3 {
    float vals[3][3];
} skcms_Matrix3x3;

// A row-major 3x4 matrix (ie vals[row][col])
typedef struct skcms_Matrix3x4 {
    float vals[3][4];
} skcms_Matrix3x4;

// A transfer function mapping encoded values to linear values,
// represented by this 7-parameter piecewise function:
//
//   linear = sign(encoded) *  (c*|encoded| + f)       , 0 <= |encoded| < d
//          = sign(encoded) * ((a*|encoded| + b)^g + e), d <= |encoded|
//
// (A simple gamma transfer function sets g to gamma and a to 1.)
typedef struct skcms_TransferFunction {
    float g, a,b,c,d,e,f;
} skcms_TransferFunction;

// Unified representation of 'curv' or 'para' tag data, or a 1D table from 'mft1' or 'mft2'
typedef union skcms_Curve {
    struct {
        uint32_t alias_of_table_entries;
        skcms_TransferFunction parametric;
    };
    struct {
        uint32_t table_entries;
        const uint8_t* table_8;
        const uint8_t* table_16;
    };
} skcms_Curve;

// Practical test that answers: Is curve roughly the inverse of inv_tf? Typically used by passing
// the inverse of a known parametric transfer function (like sRGB), to determine if a particular
// curve is very close to sRGB.
SKCMS_API bool skcms_AreApproximateInverses(const skcms_Curve* curve,
                                            const skcms_TransferFunction* inv_tf);

typedef struct skcms_A2B {
    // Optional: N 1D curves, followed by an N-dimensional CLUT.
    // If input_channels == 0, these curves and CLUT are skipped,
    // Otherwise, input_channels must be in [1, 4].
    uint32_t        input_channels;
    skcms_Curve     input_curves[4];
    uint8_t         grid_points[4];
    const uint8_t*  grid_8;
    const uint8_t*  grid_16;

    // Optional: 3 1D curves, followed by a color matrix.
    // If matrix_channels == 0, these curves and matrix are skipped,
    // Otherwise, matrix_channels must be 3.
    uint32_t        matrix_channels;
    skcms_Curve     matrix_curves[3];
    skcms_Matrix3x4 matrix;

    // Required: 3 1D curves. Always present, and output_channels must be 3.
    uint32_t        output_channels;
    skcms_Curve     output_curves[3];
} skcms_A2B;

typedef struct skcms_ICCProfile {
    const uint8_t* buffer;

    uint32_t size;
    uint32_t data_color_space;
    uint32_t pcs;
    uint32_t tag_count;

    // skcms_Parse() will set commonly-used fields for you when possible:

    // If we can parse red, green and blue transfer curves from the profile,
    // trc will be set to those three curves, and has_trc will be true.
    bool                   has_trc;
    skcms_Curve            trc[3];

    // If this profile's gamut can be represented by a 3x3 transform to XYZD50,
    // skcms_Parse() sets toXYZD50 to that transform and has_toXYZD50 to true.
    bool                   has_toXYZD50;
    skcms_Matrix3x3        toXYZD50;

    // If the profile has a valid A2B0 tag, skcms_Parse() sets A2B to that data,
    // and has_A2B to true.
    bool                   has_A2B;
    skcms_A2B              A2B;
} skcms_ICCProfile;

// The sRGB color profile is so commonly used that we offer a canonical skcms_ICCProfile for it.
SKCMS_API const skcms_ICCProfile* skcms_sRGB_profile(void);
// Ditto for XYZD50, the most common profile connection space.
SKCMS_API const skcms_ICCProfile* skcms_XYZD50_profile(void);

SKCMS_API const skcms_TransferFunction* skcms_sRGB_TransferFunction(void);
SKCMS_API const skcms_TransferFunction* skcms_sRGB_Inverse_TransferFunction(void);
SKCMS_API const skcms_TransferFunction* skcms_Linear_TransferFunction(void);

// Practical equality test for two skcms_ICCProfiles.
// The implementation is subject to change, but it will always try to answer
// "can I substitute A for B?" and "can I skip transforming from A to B?".
SKCMS_API bool skcms_ApproximatelyEqualProfiles(const skcms_ICCProfile* A,
                                                const skcms_ICCProfile* B);

// Parse an ICC profile and return true if possible, otherwise return false.
// The buffer is not copied, it must remain valid as long as the skcms_ICCProfile
// will be used.
SKCMS_API bool skcms_Parse(const void*, size_t, skcms_ICCProfile*);

// No-op, to be removed.
static inline void skcms_OptimizeForSpeed(skcms_ICCProfile* p) { (void)p; }

SKCMS_API bool skcms_ApproximateCurve(const skcms_Curve* curve,
                                      skcms_TransferFunction* approx,
                                      float* max_error);

typedef struct skcms_ICCTag {
    uint32_t       signature;
    uint32_t       type;
    uint32_t       size;
    const uint8_t* buf;
} skcms_ICCTag;

SKCMS_API void skcms_GetTagByIndex    (const skcms_ICCProfile*, uint32_t idx, skcms_ICCTag*);
SKCMS_API bool skcms_GetTagBySignature(const skcms_ICCProfile*, uint32_t sig, skcms_ICCTag*);

// These are common ICC signature values
enum {
    // data_color_space
    skcms_Signature_CMYK = 0x434D594B,
    skcms_Signature_Gray = 0x47524159,
    skcms_Signature_RGB  = 0x52474220,

    // pcs
    skcms_Signature_Lab  = 0x4C616220,
    skcms_Signature_XYZ  = 0x58595A20,
};

typedef enum skcms_PixelFormat {
    skcms_PixelFormat_RGB_565,
    skcms_PixelFormat_BGR_565,

    skcms_PixelFormat_RGB_888,
    skcms_PixelFormat_BGR_888,
    skcms_PixelFormat_RGBA_8888,
    skcms_PixelFormat_BGRA_8888,

    skcms_PixelFormat_RGBA_1010102,
    skcms_PixelFormat_BGRA_1010102,

    skcms_PixelFormat_RGB_161616,     // Big-endian.  Pointers must be 16-bit aligned.
    skcms_PixelFormat_BGR_161616,
    skcms_PixelFormat_RGBA_16161616,
    skcms_PixelFormat_BGRA_16161616,

    skcms_PixelFormat_RGB_hhh,        // 1-5-10 half-precision float.
    skcms_PixelFormat_BGR_hhh,        // Pointers must be 16-bit aligned.
    skcms_PixelFormat_RGBA_hhhh,
    skcms_PixelFormat_BGRA_hhhh,

    skcms_PixelFormat_RGB_fff,        // 1-8-23 single-precision float (the normal kind).
    skcms_PixelFormat_BGR_fff,        // Pointers must be 32-bit aligned.
    skcms_PixelFormat_RGBA_ffff,
    skcms_PixelFormat_BGRA_ffff,
} skcms_PixelFormat;

// We always store any alpha channel linearly.  In the chart below, tf-1() is the inverse
// transfer function for the given color profile (applying the transfer function linearizes).

// We treat opaque as a strong requirement, not just a performance hint: we will ignore
// any source alpha and treat it as 1.0, and will make sure that any destination alpha
// channel is filled with the equivalent of 1.0.

// When premultiplying and/or using a non-linear transfer function, it's important
// that we know the order the operations are applied.  If you're used to working
// with non-color-managed drawing systems, PremulAsEncoded is probably the "premul"
// you're looking for; if you want linear blending, PremulLinear is the choice for you.

typedef enum skcms_AlphaFormat {
    skcms_AlphaFormat_Opaque,          // alpha is always opaque
                                       //   tf-1(r),   tf-1(g),   tf-1(b),   1.0
    skcms_AlphaFormat_Unpremul,        // alpha and color are unassociated
                                       //   tf-1(r),   tf-1(g),   tf-1(b),   a
    skcms_AlphaFormat_PremulAsEncoded, // premultiplied while encoded
                                       //   tf-1(r)*a, tf-1(g)*a, tf-1(b)*a, a
    skcms_AlphaFormat_PremulLinear,    // premultiplied while linear
                                       //   tf-1(r*a), tf-1(g*a), tf-1(b*a), a
} skcms_AlphaFormat;

// Convert npixels pixels from src format and color profile to dst format and color profile
// and return true, otherwise return false.  It is safe to alias dst == src if dstFmt == srcFmt.
SKCMS_API bool skcms_Transform(const void*             src,
                               skcms_PixelFormat       srcFmt,
                               skcms_AlphaFormat       srcAlpha,
                               const skcms_ICCProfile* srcProfile,
                               void*                   dst,
                               skcms_PixelFormat       dstFmt,
                               skcms_AlphaFormat       dstAlpha,
                               const skcms_ICCProfile* dstProfile,
                               size_t                  npixels);

// If profile can be used as a destination in skcms_Transform, return true. Otherwise, attempt to
// rewrite it with approximations where reasonable. If successful, return true. If no reasonable
// approximation exists, leave the profile unchanged and return false.
SKCMS_API bool skcms_MakeUsableAsDestination(skcms_ICCProfile* profile);

// If profile can be used as a destination with a single parametric transfer function (ie for
// rasterization), return true. Otherwise, attempt to rewrite it with approximations where
// reasonable. If successful, return true. If no reasonable approximation exists, leave the
// profile unchanged and return false.
SKCMS_API bool skcms_MakeUsableAsDestinationWithSingleCurve(skcms_ICCProfile* profile);

SKCMS_API bool skcms_PrimariesToXYZD50(float rx, float ry,
                                       float gx, float gy,
                                       float bx, float by,
                                       float wx, float wy,
                                       skcms_Matrix3x3* toXYZD50);

// Utilities for programmatically constructing profiles
static inline void skcms_Init(skcms_ICCProfile* p) {
    memset(p, 0, sizeof(*p));
    p->data_color_space = skcms_Signature_RGB;
    p->pcs = skcms_Signature_XYZ;
}

static inline void skcms_SetTransferFunction(skcms_ICCProfile* p,
                                             const skcms_TransferFunction* tf) {
    p->has_trc = true;
    for (int i = 0; i < 3; ++i) {
        p->trc[i].table_entries = 0;
        p->trc[i].parametric = *tf;
    }
}

static inline void skcms_SetXYZD50(skcms_ICCProfile* p, const skcms_Matrix3x3* m) {
    p->has_toXYZD50 = true;
    p->toXYZD50 = *m;
}

#ifdef __cplusplus
}
#endif

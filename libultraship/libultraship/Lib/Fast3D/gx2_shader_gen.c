#ifdef __WIIU__

#include "gx2_shader_gen.h"
#include "gx2_shader_inl.h"

#include <malloc.h>
#include <gx2/mem.h>

#define ROUNDUP(x, align) (((x) + ((align) -1)) & ~((align) -1))

static const uint8_t reg_map[] = {
    ALU_SRC_0, // SHADER_0
    _R6, // SHADER_INPUT_1
    _R7, // SHADER_INPUT_2
    _R8, // SHADER_INPUT_3
    _R9, // SHADER_INPUT_4
    _R10, // SHADER_INPUT_5
    _R11, // SHADER_INPUT_6
    _R12, // SHADER_INPUT_7
    _R13, // SHADER_TEXEL0
    _R13, // SHADER_TEXEL0A
    _R14, // SHADER_TEXEL1
    _R14, // SHADER_TEXEL1A
    ALU_SRC_1, // SHADER_1
    _R1, // SHADER_COMBINED
};

#define ADD_INSTR(...) \
    uint64_t tmp[] = {__VA_ARGS__}; \
    memcpy(*alu_ptr, tmp, sizeof(tmp)); \
    *alu_ptr += sizeof(tmp) / sizeof(uint64_t)

static inline void add_tex_clamp_S_T(uint64_t **alu_ptr, uint8_t tex)
{
    uint8_t texinfo_reg = (tex == 0) ? _R15 : _R16;
    uint8_t texcoord_reg = (tex == 0) ? _R1 : _R3;
    uint8_t texclamp_reg = (tex == 0) ? _R2 : _R4;

    ADD_INSTR(
        /* R127.xy = (float) texinfo.xy */
        ALU_INT_TO_FLT(_R127, _x, texinfo_reg, _x) SCL_210
        ALU_LAST,

        ALU_INT_TO_FLT(_R127, _y, texinfo_reg, _y) SCL_210
        ALU_LAST,

        /* R127.xy = texSize / 0.5f */
        ALU_RECIP_IEEE(__, _x, _R127, _x) SCL_210
        ALU_LAST,

        ALU_MUL_IEEE(_R127, _x, ALU_SRC_PS, _x, ALU_SRC_0_5, _x),
        ALU_RECIP_IEEE(__, _y, _R127, _y) SCL_210
        ALU_LAST,

        ALU_MUL_IEEE(_R127, _y, ALU_SRC_PS, _y, ALU_SRC_0_5, _x)
        ALU_LAST,

        /* texCoord.xy = clamp(texCoord.xy, R127.xy, texClamp.xy) */
        ALU_MAX(__, _x, texcoord_reg, _x, _R127, _x),
        ALU_MAX(__, _y, texcoord_reg, _y, _R127, _y)
        ALU_LAST,

        ALU_MIN(texcoord_reg, _x, ALU_SRC_PV, _x, texclamp_reg, _x),
        ALU_MIN(texcoord_reg, _y, ALU_SRC_PV, _y, texclamp_reg, _y)
        ALU_LAST,
    );
}

static inline void add_tex_clamp_S(uint64_t **alu_ptr, uint8_t tex)
{
    uint8_t texinfo_reg = (tex == 0) ? _R15 : _R16;
    uint8_t texcoord_reg = (tex == 0) ? _R1 : _R3;
    uint8_t texclamp_reg = (tex == 0) ? _R2 : _R4;

    ADD_INSTR(
        /* R127.x = (float) texinfo.x */
        ALU_INT_TO_FLT(_R127, _x, texinfo_reg, _x) SCL_210
        ALU_LAST,

        /* R127.x = texSize / 0.5f */
        ALU_RECIP_IEEE(__, _x, _R127, _x) SCL_210
        ALU_LAST,

        ALU_MUL_IEEE(_R127, _x, ALU_SRC_PS, _x, ALU_SRC_0_5, _x)
        ALU_LAST,

        /* texCoord.xy = clamp(texCoord.xy, R127.xy, texClamp.xy) */
        ALU_MAX(__, _x, texcoord_reg, _x, _R127, _x)
        ALU_LAST,

        ALU_MIN(texcoord_reg, _x, ALU_SRC_PV, _x, texclamp_reg, _x)
        ALU_LAST,
    );
}

static inline void add_tex_clamp_T(uint64_t **alu_ptr, uint8_t tex)
{
    uint8_t texinfo_reg = (tex == 0) ? _R15 : _R16;
    uint8_t texcoord_reg = (tex == 0) ? _R1 : _R3;
    uint8_t texclamp_reg = (tex == 0) ? _R2 : _R4;

    ADD_INSTR(
        /* R127.y = (float) texinfo.y */
        ALU_INT_TO_FLT(_R127, _y, texinfo_reg, _y) SCL_210
        ALU_LAST,

        /* R127.y = 0.5f / texSize */
        ALU_RECIP_IEEE(__, _x, _R127, _y) SCL_210
        ALU_LAST,

        ALU_MUL_IEEE(_R127, _y, ALU_SRC_PS, _x, ALU_SRC_0_5, _x)
        ALU_LAST,

        /* texCoord.xy = clamp(texCoord.xy, R127.xy, texClamp.xy) */
        ALU_MAX(__, _y, texcoord_reg, _y, _R127, _y)
        ALU_LAST,

        ALU_MIN(texcoord_reg, _y, ALU_SRC_PV, _y, texclamp_reg, _y)
        ALU_LAST,
    );
}

static inline void add_mov(uint64_t **alu_ptr, uint8_t src, bool single) {
    bool src_alpha = (src == SHADER_TEXEL0A) || (src == SHADER_TEXEL1A);
    src = reg_map[src];

    /* texel = src */
    if (single) {
        ADD_INSTR(
            ALU_MOV(_R1, _w, src, _w)
            ALU_LAST,
        );
    } else {
        ADD_INSTR(
            ALU_MOV(_R1, _x, src, src_alpha ? _w :_x),
            ALU_MOV(_R1, _y, src, src_alpha ? _w :_y),
            ALU_MOV(_R1, _z, src, src_alpha ? _w :_z)
            ALU_LAST,
        );
    }
}

static inline void add_mul(uint64_t **alu_ptr, uint8_t src0, uint8_t src1, bool single) {
    bool src0_alpha = (src0 == SHADER_TEXEL0A) || (src0 == SHADER_TEXEL1A);
    bool src1_alpha = (src1 == SHADER_TEXEL0A) || (src1 == SHADER_TEXEL1A);
    src0 = reg_map[src0];
    src1 = reg_map[src1];

    /* texel = src0 * src1 */
    if (single) {
        ADD_INSTR(
            ALU_MUL(_R1, _w, src0, _w, src1, _w)
            ALU_LAST,
        );
    } else {
        ADD_INSTR(
            ALU_MUL(_R1, _x, src0, src0_alpha ? _w : _x, src1, src1_alpha ? _w : _x),
            ALU_MUL(_R1, _y, src0, src0_alpha ? _w : _y, src1, src1_alpha ? _w : _y),
            ALU_MUL(_R1, _z, src0, src0_alpha ? _w : _z, src1, src1_alpha ? _w : _z)
            ALU_LAST,
        );
    }
}

static inline void add_mix(uint64_t **alu_ptr, uint8_t src0, uint8_t src1, uint8_t src2, uint8_t src3, bool single) {
    bool src0_alpha = (src0 == SHADER_TEXEL0A) || (src0 == SHADER_TEXEL1A);
    bool src1_alpha = (src1 == SHADER_TEXEL0A) || (src1 == SHADER_TEXEL1A);
    bool src2_alpha = (src2 == SHADER_TEXEL0A) || (src2 == SHADER_TEXEL1A);
    bool src3_alpha = (src3 == SHADER_TEXEL0A) || (src3 == SHADER_TEXEL1A);
    src0 = reg_map[src0];
    src1 = reg_map[src1];
    src2 = reg_map[src2];
    src3 = reg_map[src3];

    /* texel = (src0 - src1) * src2 - src3 */
    if (single) {
        ADD_INSTR(
            ALU_ADD(__, _w, src0, _w, src1 _NEG, _w)
            ALU_LAST,

            ALU_MULADD(_R1, _w, ALU_SRC_PV, _w, src2, _w, src3, _w)
            ALU_LAST,
        );
    } else {
        ADD_INSTR(
            ALU_ADD(__, _x, src0, src0_alpha ? _w : _x, src1 _NEG, src1_alpha ? _w : _x),
            ALU_ADD(__, _y, src0, src0_alpha ? _w : _y, src1 _NEG, src1_alpha ? _w : _y),
            ALU_ADD(__, _z, src0, src0_alpha ? _w : _z, src1 _NEG, src1_alpha ? _w : _z)
            ALU_LAST,

            ALU_MULADD(_R1, _x, ALU_SRC_PV, _x, src2, src2_alpha ? _w : _x, src3, src3_alpha ? _w : _x),
            ALU_MULADD(_R1, _y, ALU_SRC_PV, _y, src2, src2_alpha ? _w : _y, src3, src3_alpha ? _w : _y),
            ALU_MULADD(_R1, _z, ALU_SRC_PV, _z, src2, src2_alpha ? _w : _z, src3, src3_alpha ? _w : _z)
            ALU_LAST,
        );
    }
}
#undef ADD_INSTR

static void append_tex_clamp(uint64_t **alu_ptr, uint8_t tex, bool s, bool t)
{
    if (s && t) {
        add_tex_clamp_S_T(alu_ptr, tex);
    } else if (s) {
        add_tex_clamp_S(alu_ptr, tex);
    } else {
        add_tex_clamp_T(alu_ptr, tex);
    }
}

static void append_formula(uint64_t **alu_ptr, uint8_t c[2][4], bool do_single, bool do_multiply, bool do_mix, bool only_alpha) {
    if (do_single) {
        add_mov(alu_ptr, c[only_alpha][3], only_alpha);
    } else if (do_multiply) {
        add_mul(alu_ptr, c[only_alpha][0], c[only_alpha][2], only_alpha);
    } else if (do_mix) {
        add_mix(alu_ptr, c[only_alpha][0], c[only_alpha][1], c[only_alpha][2], c[only_alpha][1], only_alpha);
    } else {
        add_mix(alu_ptr, c[only_alpha][0], c[only_alpha][1], c[only_alpha][2], c[only_alpha][3], only_alpha);
    }
}

static const uint64_t noise_instructions[] = {
    /* R127 = floor(gl_FragCoord.xy * (240.0f / window_params.x)) */
    ALU_RECIP_IEEE(__, _x, _C(0), _x) SCL_210
    ALU_LAST,

    ALU_MUL_IEEE(__, _x, ALU_SRC_PS, _x, ALU_SRC_LITERAL, _x)
    ALU_LAST,
    ALU_LITERAL(0x43700000 /* 240.0f */),

    ALU_MUL(__, _x, _R0, _x, ALU_SRC_PV, _x),
    ALU_MUL(__, _y, _R0, _y, ALU_SRC_PV, _x)
    ALU_LAST,

    ALU_FLOOR(_R127, _x, ALU_SRC_PV, _x),
    ALU_FLOOR(_R127, _y, ALU_SRC_PV, _y)
    ALU_LAST,

    /* R127 = sin(vec3(R127.x, R127.y, window_params.y)) */
    ALU_MULADD(_R127, _x, _R127, _x, ALU_SRC_LITERAL, _x, ALU_SRC_0_5, _x),
    ALU_MULADD(_R127, _y, _R127, _y, ALU_SRC_LITERAL, _x, ALU_SRC_0_5, _x),
    ALU_MULADD(_R127, _z, _C(0), _y, ALU_SRC_LITERAL, _x, ALU_SRC_0_5, _x)
    ALU_LAST,
    ALU_LITERAL(0x3E22F983 /* 0.1591549367f (radians -> revolutions) */),

    ALU_FRACT(__, _x, _R127, _x),
    ALU_FRACT(__, _y, _R127, _y),
    ALU_FRACT(__, _z, _R127, _z)
    ALU_LAST,

    ALU_MULADD(_R127, _x, ALU_SRC_PV, _x, ALU_SRC_LITERAL, _x, ALU_SRC_LITERAL, _y),
    ALU_MULADD(_R127, _y, ALU_SRC_PV, _y, ALU_SRC_LITERAL, _x, ALU_SRC_LITERAL, _y),
    ALU_MULADD(_R127, _z, ALU_SRC_PV, _z, ALU_SRC_LITERAL, _x, ALU_SRC_LITERAL, _y)
    ALU_LAST,
    ALU_LITERAL2(0x40C90FDB /* 6.283185482f (tau) */, 0xC0490FDB /* -3.141592741f (-pi) */),

    ALU_MUL(_R127, _x, ALU_SRC_PV, _x, ALU_SRC_LITERAL, _x),
    ALU_MUL(_R127, _y, ALU_SRC_PV, _y, ALU_SRC_LITERAL, _x),
    ALU_MUL(_R127, _z, ALU_SRC_PV, _z, ALU_SRC_LITERAL, _x)
    ALU_LAST,
    ALU_LITERAL(0x3E22F983 /* 0.1591549367f (radians -> revolutions) */),

    ALU_SIN(_R127, _x, _R127, _x) SCL_210
    ALU_LAST,

    ALU_SIN(_R127, _y, _R127, _y) SCL_210
    ALU_LAST,

    ALU_SIN(_R127, _z, _R127, _z) SCL_210
    ALU_LAST,

    /* R127.x = dot(R127.xyz, vec3(12.9898, 78.233, 37.719)); */
    ALU_DOT4(_R127, _x, _R127, _x, ALU_SRC_LITERAL, _x),
    ALU_DOT4(__, _y, _R127, _y, ALU_SRC_LITERAL, _y),
    ALU_DOT4(__, _z, _R127, _z, ALU_SRC_LITERAL, _z),
    ALU_DOT4(__, _w, ALU_SRC_LITERAL, _w, ALU_SRC_0, _x)
    ALU_LAST,
    ALU_LITERAL4(0x414FD639 /* 12.9898f */, 0x429C774C /* 78.233f */, 0x4216E042 /* 37.719f */, 0x80000000 /* -0.0f */),

    /* R127.x = fract(sin(R127.x) * 143758.5453); */
    ALU_MULADD(_R127, _x, _R127, _x, ALU_SRC_LITERAL, _x, ALU_SRC_0_5, _x)
    ALU_LAST,
    ALU_LITERAL(0x3E22F983 /* 0.1591549367f (radians -> revolutions) */),

    ALU_FRACT(__, _x, _R127, _x)
    ALU_LAST,

    ALU_MULADD(_R127, _x, ALU_SRC_PV, _x, ALU_SRC_LITERAL, _x, ALU_SRC_LITERAL, _y)
    ALU_LAST,
    ALU_LITERAL2(0x40C90FDB /* 6.283185482f (tau) */, 0xC0490FDB /* -3.141592741f (-pi) */),

    ALU_SIN(_R127, _x, _R127, _x) SCL_210
    ALU_LAST,

    ALU_MUL(__, _x, _R127, _x, ALU_SRC_LITERAL, _x)
    ALU_LAST,
    ALU_LITERAL(0x480C63A3 /* 143758.5453f */),

    ALU_FRACT( _R127, _x, ALU_SRC_PV, _x)
    ALU_LAST,

    /* texel.a *= floor(R127.x + 0.5); */
    ALU_ADD(__, _x, _R127, _x, ALU_SRC_0_5, _x)
    ALU_LAST,

    ALU_FLOOR(__, _x, ALU_SRC_PV, _x)
    ALU_LAST,

    ALU_MUL(_R1, _w, _R1, _w, ALU_SRC_PV, _x)
    ALU_LAST,
};

static GX2UniformVar uniformVars[] = {
    { "window_params", GX2_SHADER_VAR_TYPE_FLOAT2, 1, 0, -1, },
};

static GX2SamplerVar samplerVars[] = {
    { "uTex0", GX2_SAMPLER_VAR_TYPE_SAMPLER_2D, 0 },
    { "uTex1", GX2_SAMPLER_VAR_TYPE_SAMPLER_2D, 1 },
};

#define ADD_INSTR(...) \
    do { \
    uint64_t tmp[] = {__VA_ARGS__}; \
    memcpy(cur_buf, tmp, sizeof(tmp)); \
    cur_buf += sizeof(tmp) / sizeof(uint64_t); \
    } while (0)

static int generatePixelShader(GX2PixelShader *psh, struct CCFeatures *cc_features) {
    static const size_t max_program_buf_size = 512 * sizeof(uint64_t);
    uint64_t *program_buf = memalign(GX2_SHADER_PROGRAM_ALIGNMENT, max_program_buf_size);
    if (!program_buf) {
        return -1;
    }

    memset(program_buf, 0, max_program_buf_size);

    // start placing alus at offset 32
    static const uint32_t base_alu_offset = 32;
    uint64_t *cur_buf = NULL;

    // check if we need to clamp
    bool texclamp[2] = { false, false };
    for (int i = 0; i < 2; i++) {
        if (cc_features->used_textures[i]) {
            if (cc_features->clamp[i][0] || cc_features->clamp[i][1]) {
                texclamp[i] = true;
            }
        }
    }

    uint32_t texclamp_alu_offset = base_alu_offset;
    uint32_t texclamp_alu_size = 0;
    uint32_t texclamp_alu_cnt = 0;

    if (texclamp[0] || texclamp[1]) {
        // texclamp alu
        cur_buf = program_buf + texclamp_alu_offset;

        for (int i = 0; i < 2; i++) {
            if (cc_features->used_textures[i] && texclamp[i]) {
                append_tex_clamp(&cur_buf, i, cc_features->clamp[i][0], cc_features->clamp[i][1]);
            }
        }

        texclamp_alu_size = (uintptr_t) cur_buf - ((uintptr_t) (program_buf + texclamp_alu_offset));
        texclamp_alu_cnt = texclamp_alu_size / sizeof(uint64_t);
    }

    // main alu0
    uint32_t main_alu0_offset = texclamp_alu_offset + texclamp_alu_cnt;
    cur_buf = program_buf + main_alu0_offset;

    for (int c = 0; c < (cc_features->opt_2cyc ? 2 : 1); c++) {
        append_formula(&cur_buf, cc_features->c[c], cc_features->do_single[c][0], cc_features->do_multiply[c][0], cc_features->do_mix[c][0], false);
        if (cc_features->opt_alpha) {
            append_formula(&cur_buf, cc_features->c[c], cc_features->do_single[c][1], cc_features->do_multiply[c][1], cc_features->do_mix[c][1], true);
        }
    }

    if (cc_features->opt_fog) {
        ADD_INSTR(
            /* texel.rgb = mix(texel.rgb, vFog.rgb, vFog.a); */
            ALU_ADD(__, _x, _R5, _x, _R1 _NEG, _x),
            ALU_ADD(__, _y, _R5, _y, _R1 _NEG, _y),
            ALU_ADD(__, _z, _R5, _z, _R1 _NEG, _z)
            ALU_LAST,

            ALU_MULADD(_R1, _x, ALU_SRC_PV, _x, _R5, _w, _R1, _x),
            ALU_MULADD(_R1, _y, ALU_SRC_PV, _y, _R5, _w, _R1, _y),
            ALU_MULADD(_R1, _z, ALU_SRC_PV, _z, _R5, _w, _R1, _z)
            ALU_LAST,
        );
    }

    if (cc_features->opt_texture_edge && cc_features->opt_alpha) {
        ADD_INSTR(
            /* if (texel.a > 0.3) texel.a = 1.0; else discard; */
            ALU_KILLGT(__, _x, ALU_SRC_LITERAL, _x, _R1, _w),
            ALU_MOV(_R1, _w, ALU_SRC_1, _x)
            ALU_LAST,
            ALU_LITERAL(0x3e428f5c /*0.19f*/),
        );
    }

    const uint32_t main_alu0_size = (uintptr_t) cur_buf - ((uintptr_t) (program_buf + main_alu0_offset));
    const uint32_t main_alu0_cnt = main_alu0_size / sizeof(uint64_t);

    // main alu1
    // place the following instructions into a new alu, in case the other alu uses KILL
    const uint32_t main_alu1_offset = main_alu0_offset + main_alu0_cnt;
    cur_buf = program_buf + main_alu1_offset;

    if (cc_features->opt_alpha && cc_features->opt_noise) {
        memcpy(cur_buf, noise_instructions, sizeof(noise_instructions));
        cur_buf += sizeof(noise_instructions) / sizeof(uint64_t);
    }

    if (cc_features->opt_alpha) {
        if (cc_features->opt_alpha_threshold) {
            ADD_INSTR(
                /* if (texel.a < 8.0 / 256.0) discard; */
                ALU_KILLGT(__, _x, ALU_SRC_LITERAL, _x, _R1, _w)
                ALU_LAST,
                ALU_LITERAL(0x3d000000 /*0.03125f*/),
            );
        }

        if (cc_features->opt_invisible) {
            ADD_INSTR(
                /* texel.a = 0.0; */
                ALU_MOV(_R1, _w, ALU_SRC_0, _x)
                ALU_LAST,
            );
        }
    }

    const uint32_t main_alu1_size = (uintptr_t) cur_buf - ((uintptr_t) (program_buf + main_alu1_offset));
    const uint32_t main_alu1_cnt = main_alu1_size / sizeof(uint64_t);

    // tex
    const uint32_t num_texinfo = texclamp[0] + texclamp[1];
    const uint32_t num_textures = cc_features->used_textures[0] + cc_features->used_textures[1];

    uint32_t texinfo_offset = ROUNDUP(main_alu1_offset + main_alu1_cnt, 16);
    uint32_t cur_tex_offset = texinfo_offset;

    for (int i = 0; i < 2; i++) {
        if (cc_features->used_textures[i] && texclamp[i]) {
            uint8_t dst_reg = (i == 0) ? _R15 : _R16;

            uint64_t texinfo_buf[] = {
                TEX_GET_TEXTURE_INFO(dst_reg, _x, _y, _m, _m, _R1, _0, _0, _0, _0,  _t(i), _s(i))
            };

            memcpy(program_buf + cur_tex_offset, texinfo_buf, sizeof(texinfo_buf));
            cur_tex_offset += sizeof(texinfo_buf) / sizeof(uint64_t);
        }
    }

    uint32_t texsample_offset = cur_tex_offset;

    for (int i = 0; i < 2; i++) {
        if (cc_features->used_textures[i]) {
            uint8_t texcoord_reg = (i == 0) ? _R1 : _R3;
            uint8_t dst_reg = reg_map[(i == 0) ? SHADER_TEXEL0 : SHADER_TEXEL1];

            uint64_t tex_buf[] = {
                TEX_SAMPLE(dst_reg, _x, _y, _z, _w, texcoord_reg, _x, _y, _0, _x, _t(i), _s(i))
            };

            memcpy(program_buf + cur_tex_offset, tex_buf, sizeof(tex_buf));
            cur_tex_offset += sizeof(tex_buf) / sizeof(uint64_t);
        }
    }

    // make sure we didn't overflow the buffer
    const uint32_t total_program_size = cur_tex_offset * sizeof(uint64_t);
    assert(total_program_size <= max_program_buf_size);

    // cf
    uint32_t cur_cf_offset = 0;

    // if we use texclamp place those alus first
    if (texclamp[0] || texclamp[1]) {
        program_buf[cur_cf_offset++] = TEX(texinfo_offset, num_texinfo);
        program_buf[cur_cf_offset++] = ALU(texclamp_alu_offset, texclamp_alu_cnt);
    }

    if (num_textures > 0) {
        program_buf[cur_cf_offset++] = TEX(texsample_offset, num_textures) VALID_PIX;
    }

    program_buf[cur_cf_offset++] = ALU(main_alu0_offset, main_alu0_cnt);

    if (main_alu1_cnt > 0) {
        program_buf[cur_cf_offset++] = ALU(main_alu1_offset, main_alu1_cnt);
    }

    if (cc_features->opt_alpha) {
        program_buf[cur_cf_offset++] = EXP_DONE(PIX0, _R1, _x, _y, _z, _w) END_OF_PROGRAM;
    } else {
        program_buf[cur_cf_offset++] = EXP_DONE(PIX0, _R1, _x, _y, _z, _1) END_OF_PROGRAM;
    }

    // regs
    const uint32_t num_ps_inputs = 5 + cc_features->num_inputs;

    psh->regs.sq_pgm_resources_ps = 17; // num_gprs
    psh->regs.sq_pgm_exports_ps = 2; // export_mode
    psh->regs.spi_ps_in_control_0 = (num_ps_inputs + 1) // num_interp
        | (1 << 8) // position_ena
        | (1 << 26) // persp_gradient_ena
        | (1 << 28); // baryc_sample_cntl

    psh->regs.num_spi_ps_input_cntl = num_ps_inputs + 1;

    // frag pos
    psh->regs.spi_ps_input_cntls[0] = 0 | (1 << 8);

    // inputs
    for (int i = 0; i < num_ps_inputs; i++) {
        psh->regs.spi_ps_input_cntls[i + 1] = i | (1 << 8);
    }

    psh->regs.cb_shader_mask = 0xf; // output0_enable
    psh->regs.cb_shader_control = 1; // rt0_enable
    psh->regs.db_shader_control = (1 << 4) // z_order
        | (1 << 6); // kill_enable
    
    // program
    psh->size = total_program_size;
    psh->program = program_buf;

    psh->mode = GX2_SHADER_MODE_UNIFORM_REGISTER;

    // uniform vars
    psh->uniformVars = uniformVars;
    psh->uniformVarCount = sizeof(uniformVars) / sizeof(GX2UniformVar);

    // samplers
    psh->samplerVars = samplerVars;
    psh->samplerVarCount = sizeof(samplerVars) / sizeof(GX2SamplerVar);

    return 0;
}

static GX2AttribVar attribVars[] = {
    { "aVtxPos",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 0},
    { "aTexCoord0", GX2_SHADER_VAR_TYPE_FLOAT2, 0, 1},
    { "aTexClamp0", GX2_SHADER_VAR_TYPE_FLOAT2, 0, 2},
    { "aTexCoord1", GX2_SHADER_VAR_TYPE_FLOAT2, 0, 3},
    { "aTexClamp1", GX2_SHADER_VAR_TYPE_FLOAT2, 0, 4},
    { "aFog",       GX2_SHADER_VAR_TYPE_FLOAT4, 0, 5},
    { "aInput1",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 6},
    { "aInput2",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 7},
    { "aInput3",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 8},
    { "aInput4",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 9},
    { "aInput5",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 10},
    { "aInput6",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 11},
    { "aInput7",    GX2_SHADER_VAR_TYPE_FLOAT4, 0, 12},
};

static int generateVertexShader(GX2VertexShader *vsh, struct CCFeatures *cc_features) {
    static const size_t max_program_buf_size = 16 * sizeof(uint64_t);
    uint64_t *program_buf = memalign(GX2_SHADER_PROGRAM_ALIGNMENT, max_program_buf_size);
    if (!program_buf) {
        return -1;
    }

    const uint32_t num_ps_inputs = 5 + cc_features->num_inputs;

    uint64_t *cur_buf = program_buf;

    // aVtxPos
    ADD_INSTR(
        CALL_FS NO_BARRIER,
        EXP_DONE(POS0, _R1, _x, _y, _z, _w),
    );

    // params
    for (int i = 0; i < num_ps_inputs - 1; i++) {
        ADD_INSTR(
            EXP(PARAM(i), _R(i + 2), _x, _y, _z, _w) NO_BARRIER,
        );
    }

    // last param
    ADD_INSTR(
        (EXP_DONE(PARAM(num_ps_inputs - 1), _R(num_ps_inputs + 1), _x, _y, _z, _w) NO_BARRIER)
        END_OF_PROGRAM,
    );

    const uint32_t program_size = (uintptr_t) cur_buf - ((uintptr_t) program_buf);
    assert(program_size <= max_program_buf_size);

    // regs
    vsh->regs.sq_pgm_resources_vs = (num_ps_inputs + 2) // num_gprs
        | (1 << 8); // stack_size

    // num outputs minus 1
    vsh->regs.spi_vs_out_config = ((num_ps_inputs - 1) << 1);

    vsh->regs.num_spi_vs_out_id = 3;
    memset(vsh->regs.spi_vs_out_id, 0xff, sizeof(vsh->regs.spi_vs_out_id));
    vsh->regs.spi_vs_out_id[0] = (0) | (1 << 8) | (2 << 16) | (3 << 24);
    vsh->regs.spi_vs_out_id[1] = (4) | (5 << 8) | (6 << 16) | (7 << 24);
    vsh->regs.spi_vs_out_id[2] = (8) | (9 << 8) | (10 << 16) | (11 << 24);

    vsh->regs.sq_vtx_semantic_clear = ~((1 << 13) - 1);
    vsh->regs.num_sq_vtx_semantic = 13;
    memset(vsh->regs.sq_vtx_semantic, 0xff, sizeof(vsh->regs.sq_vtx_semantic));
    // aVtxPos
    vsh->regs.sq_vtx_semantic[0] = 0;
    // aTexCoord0
    vsh->regs.sq_vtx_semantic[1] = 1;
    // aTexClamp0
    vsh->regs.sq_vtx_semantic[2] = 2;
    // aTexCoord1
    vsh->regs.sq_vtx_semantic[3] = 3;
    // aTexClamp1
    vsh->regs.sq_vtx_semantic[4] = 4;
    // aFog
    vsh->regs.sq_vtx_semantic[5] = 5;
    // aInput1
    vsh->regs.sq_vtx_semantic[6] = 6;
    // aInput2
    vsh->regs.sq_vtx_semantic[7] = 7;
    // aInput3
    vsh->regs.sq_vtx_semantic[8] = 8;
    // aInput4
    vsh->regs.sq_vtx_semantic[9] = 9;
    // aInput5
    vsh->regs.sq_vtx_semantic[10] = 10;
    // aInput6
    vsh->regs.sq_vtx_semantic[11] = 11;
    // aInput7
    vsh->regs.sq_vtx_semantic[12] = 12;

    vsh->regs.vgt_vertex_reuse_block_cntl = 14; // vtx_reuse_depth
    vsh->regs.vgt_hos_reuse_depth = 16; // reuse_depth

    // program
    vsh->program = program_buf;
    vsh->size = program_size;

    vsh->mode = GX2_SHADER_MODE_UNIFORM_REGISTER;

    // attribs
    vsh->attribVarCount = sizeof(attribVars) / sizeof(GX2AttribVar);
    vsh->attribVars = attribVars;

    return 0;
}
#undef ADD_INSTR

int gx2GenerateShaderGroup(struct ShaderGroup *group, struct CCFeatures *cc_features) {
    memset(group, 0, sizeof(struct ShaderGroup));

    // generate the pixel shader
    if (generatePixelShader(&group->pixelShader, cc_features) != 0) {
        gx2FreeShaderGroup(group);
        return -1;
    }

    // generate the vertex shader
    if (generateVertexShader(&group->vertexShader, cc_features) != 0) {
        gx2FreeShaderGroup(group);
        return -1;
    }

    uint32_t attribOffset = 0;

    // aVtxPos
    group->attributes[group->numAttributes++] = 
        (GX2AttribStream) { 0, 0, attribOffset, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _z, _w), GX2_ENDIAN_SWAP_DEFAULT };
    attribOffset += 4 * sizeof(float);

    for (int i = 0; i < 2; i++) {
        if (cc_features->used_textures[i]) {
            // aTexCoordX
            group->attributes[group->numAttributes++] = 
                (GX2AttribStream) { 1 + (i * 2), 0, attribOffset, GX2_ATTRIB_FORMAT_FLOAT_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _0, _1), GX2_ENDIAN_SWAP_DEFAULT };
            attribOffset += 2 * sizeof(float);

            if (cc_features->clamp[i][0] || cc_features->clamp[i][1]) {
                // aTexClampX
                group->attributes[group->numAttributes++] = 
                    (GX2AttribStream) { 2 + (i * 2), 0, attribOffset, GX2_ATTRIB_FORMAT_FLOAT_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _0, _1), GX2_ENDIAN_SWAP_DEFAULT };
                attribOffset += 2 * sizeof(float);
            }
        }
    }

    // aFog
    if (cc_features->opt_fog) {
        group->attributes[group->numAttributes++] = 
            (GX2AttribStream) { 5, 0, attribOffset, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _z, _w), GX2_ENDIAN_SWAP_DEFAULT };
        attribOffset += 4 * sizeof(float);
    }

    // aInput
    for (int i = 0; i < cc_features->num_inputs; i++) {
        group->attributes[group->numAttributes++] = 
            (GX2AttribStream) { 6 + i, 0, attribOffset, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32, GX2_ATTRIB_INDEX_PER_VERTEX, 0, GX2_COMP_SEL(_x, _y, _z, _w), GX2_ENDIAN_SWAP_DEFAULT };
        attribOffset += 4 * sizeof(float);
    }

    group->stride = attribOffset;

    // init the fetch shader
    group->fetchShader.size = GX2CalcFetchShaderSizeEx(group->numAttributes, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);
    group->fetchShader.program = memalign(GX2_SHADER_PROGRAM_ALIGNMENT, group->fetchShader.size);
    if (!group->fetchShader.program) {
        gx2FreeShaderGroup(group);
        return -1;
    }

    GX2InitFetchShaderEx(&group->fetchShader, group->fetchShader.program, group->numAttributes, group->attributes, GX2_FETCH_SHADER_TESSELLATION_NONE, GX2_TESSELLATION_MODE_DISCRETE);

    // invalidate all programs
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, group->vertexShader.program, group->vertexShader.size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, group->pixelShader.program, group->pixelShader.size);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_SHADER, group->fetchShader.program, group->fetchShader.size);

    return 0;
}

void gx2FreeShaderGroup(struct ShaderGroup *group) {
    free(group->vertexShader.program);
    free(group->pixelShader.program);
    free(group->fetchShader.program);
}

#endif

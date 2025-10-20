// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    rgbneon.h

    NEON optimized RGB utilities.

***************************************************************************/

#ifndef MAME_EMU_VIDEO_RGBNEON_H
#define MAME_EMU_VIDEO_RGBNEON_H

#pragma once

#include <arm_neon.h>


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

class rgbaint_t
{
public:
	rgbaint_t() { }
	explicit rgbaint_t(u32 rgb) { set(rgb); }
	rgbaint_t(s32 a, s32 r, s32 g, s32 b) { set(a, r, g, b); }
	explicit rgbaint_t(const rgb_t &rgb) { set(rgb); }
	explicit rgbaint_t(int32x4_t rgba) { m_value = rgba; }

	rgbaint_t(const rgbaint_t &other) = default;
	rgbaint_t &operator=(const rgbaint_t &other) = default;

	void set(const rgbaint_t &other) { m_value = other.m_value; }
	void set(const u32 &rgba) { m_value = vreinterpretq_s32_u16(vzip1q_u16(vmovl_u8(vcreate_u8(rgba)), vdupq_n_u16(0))); }
	void set(s32 a, s32 r, s32 g, s32 b) { m_value = int32x4_t{ b, g, r, a }; }
	void set(const rgb_t &rgb) { set((const u32 &)rgb); }
	// This function sets all elements to the same val
	void set_all(s32 val) { m_value = vdupq_n_s32(val); }
	// This function zeros all elements
	void zero() { m_value = vdupq_n_s32(0); }
	// This function zeros only the alpha element
	void zero_alpha() { m_value = vsetq_lane_s32(0, m_value, 3); }

	rgb_t to_rgba() const
	{
		const int16x4_t temp = vmovn_s32(m_value);
		return vget_lane_u32(vreinterpret_u32_s8(vmovn_s16(vcombine_s16(temp, temp))), 0);
	}

	rgb_t to_rgba_clamp() const
	{
		const int16x4_t temp = vqmovn_s32(vmaxq_s32(m_value, vdupq_n_s32(0)));
		return vget_lane_u32(vreinterpret_u32_u8(vqmovn_u16(vreinterpretq_u16_s16(vcombine_s16(temp, temp)))), 0);
	}

	void set_a16(const s32 value) { m_value = vsetq_lane_s32(value, m_value, 3); }
	void set_a(const s32 value) { m_value = vsetq_lane_s32(value, m_value, 3); }
	void set_r(const s32 value) { m_value = vsetq_lane_s32(value, m_value, 2); }
	void set_g(const s32 value) { m_value = vsetq_lane_s32(value, m_value, 1); }
	void set_b(const s32 value) { m_value = vsetq_lane_s32(value, m_value, 0); }

	u8 get_a() const { return u8(u32(vgetq_lane_s32(m_value, 3))); }
	u8 get_r() const { return u8(u32(vgetq_lane_s32(m_value, 2))); }
	u8 get_g() const { return u8(u32(vgetq_lane_s32(m_value, 1))); }
	u8 get_b() const { return u8(u32(vgetq_lane_s32(m_value, 0))); }

	s32 get_a32() const { return vgetq_lane_s32(m_value, 3); }
	s32 get_r32() const { return vgetq_lane_s32(m_value, 2); }
	s32 get_g32() const { return vgetq_lane_s32(m_value, 1); }
	s32 get_b32() const { return vgetq_lane_s32(m_value, 0); }

	// These selects return an rgbaint_t with all fields set to the element chosen (a, r, g, or b)
	rgbaint_t select_alpha32() const { return rgbaint_t(vdupq_laneq_s32(m_value, 3)); }
	rgbaint_t select_red32() const { return rgbaint_t(vdupq_laneq_s32(m_value, 2)); }
	rgbaint_t select_green32() const { return rgbaint_t(vdupq_laneq_s32(m_value, 1)); }
	rgbaint_t select_blue32() const { return rgbaint_t(vdupq_laneq_s32(m_value, 0)); }

	void add(const rgbaint_t &color2) { m_value = vaddq_s32(m_value, color2.m_value); }
	void add_imm(s32 imm) { m_value = vaddq_s32(m_value, vdupq_n_s32(imm)); }
	void add_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vaddq_s32(m_value, int32x4_t{ b, g, r, a }); }

	void sub(const rgbaint_t &color2) { m_value = vsubq_s32(m_value, color2.m_value); }
	void sub_imm(s32 imm) { m_value = vsubq_s32(m_value, vdupq_n_s32(imm)); }
	void sub_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vsubq_s32(m_value, int32x4_t{ b, g, r, a }); }

	void subr(const rgbaint_t &color2) { m_value = vsubq_s32(color2.m_value, m_value); }
	void subr_imm(s32 imm) { m_value = vsubq_s32(vdupq_n_s32(imm), m_value); }
	void subr_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vsubq_s32(int32x4_t{ b, g, r, a }, m_value); }

	void mul(const rgbaint_t &color2) { m_value = vmulq_s32(m_value, color2.m_value); }
	void mul_imm(s32 imm) { m_value = vmulq_s32(m_value, vdupq_n_s32(imm)); }
	void mul_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vmulq_s32(m_value, int32x4_t{ b, g, r, a }); }

	void shl(const rgbaint_t &shift)
	{
		const int32x4_t mask = vreinterpretq_s32_u32(vcltq_u32(vreinterpretq_u32_s32(shift.m_value), vdupq_n_u32(32)));
		m_value = vandq_s32(vshlq_s32(m_value, shift.m_value), mask);
	}

	void shl_imm(u8 shift)
	{
		if (shift > 31)
			m_value = vdupq_n_s32(0);
		else
			m_value = vshlq_s32(m_value, vdupq_n_s32(shift));
	}

	void shr(const rgbaint_t &shift)
	{
		const uint32x4_t mask = vcltq_u32(vreinterpretq_u32_s32(shift.m_value), vdupq_n_u32(32));
		m_value = vreinterpretq_s32_u32(vandq_u32(vshlq_u32(vreinterpretq_u32_s32(m_value), vnegq_s32(shift.m_value)), mask));
	}

	void shr_imm(u8 shift)
	{
		if (shift > 31)
			m_value = vdupq_n_s32(0);
		else
			m_value = vreinterpretq_s32_u32(vshlq_u32(vreinterpretq_u32_s32(m_value), vdupq_n_s32(-s8(shift))));
	}

	void sra(const rgbaint_t &shift)
	{
		const uint32x4_t s = vminq_u32(vreinterpretq_u32_s32(shift.m_value), vdupq_n_u32(31));
		m_value = vshlq_s32(m_value, vnegq_s32(vreinterpretq_s32_u32(s)));
	}

	void sra_imm(u8 shift)
	{
		m_value = vshlq_s32(m_value, vdupq_n_s32(-s8(std::min(shift, u8(31)))));
	}

	void or_reg(const rgbaint_t &color2) { m_value = vorrq_s32(m_value, color2.m_value); }
	void and_reg(const rgbaint_t &color2) { m_value = vandq_s32(m_value, color2.m_value); }
	void xor_reg(const rgbaint_t &color2) { m_value = veorq_s32(m_value, color2.m_value); }

	void andnot_reg(const rgbaint_t &color2) { m_value = vandq_s32(m_value, vmvnq_s32(color2.m_value)); }

	void or_imm(s32 value) { m_value = vorrq_s32(m_value, vdupq_n_s32(value)); }
	void and_imm(s32 value) { m_value = vandq_s32(m_value, vdupq_n_s32(value)); }
	void xor_imm(s32 value) { m_value = veorq_s32(m_value, vdupq_n_s32(value)); }

	void or_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vorrq_s32(m_value, int32x4_t{ b, g, r, a }); }
	void and_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vandq_s32(m_value, int32x4_t{ b, g, r, a }); }
	void xor_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = veorq_s32(m_value, int32x4_t{ b, g, r, a }); }

	void clamp_and_clear(u32 sign)
	{
		int32x4_t vsign = vdupq_n_s32(s32(sign));
		m_value = vandq_s32(m_value, vceqq_s32(vandq_s32(m_value, vsign), vdupq_n_s32(0)));
		vsign = vmvnq_s32(vshrq_n_s32(vsign, 1));
		const int32x4_t mask = vcgtq_s32(m_value, vsign);
		m_value = vorrq_s32(vandq_s32(vsign, mask), vandq_s32(m_value, vmvnq_s32(mask)));
	}

	void clamp_to_uint8()
	{
		m_value = vminq_s32(vmaxq_s32(m_value, vdupq_n_s32(0)), vdupq_n_s32(255));
	}

	void sign_extend(u32 compare, u32 sign)
	{
		const int32x4_t compare_vec = vdupq_n_s32(s32(compare));
		m_value = vorrq_s32(m_value, vandq_s32(vceqq_s32(vandq_s32(m_value, compare_vec), compare_vec), vdupq_n_s32(s32(sign))));
	}

	void min(s32 value) { m_value = vminq_s32(m_value, vdupq_n_s32(value)); }
	void max(s32 value) { m_value = vmaxq_s32(m_value, vdupq_n_s32(value)); }

	void blend(const rgbaint_t &other, u8 factor)
	{
		m_value = vmulq_n_s32(m_value, s32(u32(factor)));
		m_value = vaddq_s32(m_value, vmulq_n_s32(other.m_value, 256 - s32(u32(factor))));
		m_value = vshrq_n_s32(m_value, 8);
	}

	void scale_and_clamp(const rgbaint_t &scale)
	{
		m_value = vshrq_n_s32(vmulq_s32(m_value, scale.m_value), 8);
		clamp_to_uint8();
	}

	void scale_imm_and_clamp(s32 scale)
	{
		m_value = vshrq_n_s32(vmulq_n_s32(m_value, scale), 8);
		clamp_to_uint8();
	}

	void scale_add_and_clamp(const rgbaint_t &scale, const rgbaint_t &other)
	{
		m_value = vaddq_s32(vshrq_n_s32(vmulq_s32(m_value, scale.m_value), 8), other.m_value);
		clamp_to_uint8();
	}

	void scale2_add_and_clamp(const rgbaint_t &scale, const rgbaint_t &other, const rgbaint_t &scale2)
	{
		m_value = vaddq_s32(vmulq_s32(m_value, scale.m_value), vmulq_s32(other.m_value, scale2.m_value));
		m_value = vshrq_n_s32(m_value, 8);
		clamp_to_uint8();
	}

	void cmpeq(const rgbaint_t &value) { m_value = vceqq_s32(m_value, value.m_value); }
	void cmpgt(const rgbaint_t &value) { m_value = vcgtq_s32(m_value, value.m_value); }
	void cmplt(const rgbaint_t &value) { m_value = vcltq_s32(m_value, value.m_value); }

	void cmpeq_imm(s32 value) { m_value = vceqq_s32(m_value, vdupq_n_s32(value)); }
	void cmpgt_imm(s32 value) { m_value = vcgtq_s32(m_value, vdupq_n_s32(value)); }
	void cmplt_imm(s32 value) { m_value = vcltq_s32(m_value, vdupq_n_s32(value)); }

	void cmpeq_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vceqq_s32(m_value, int32x4_t{ b, g, r, a }); }
	void cmpgt_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vcgtq_s32(m_value, int32x4_t{ b, g, r, a }); }
	void cmplt_imm_rgba(s32 a, s32 r, s32 g, s32 b) { m_value = vcltq_s32(m_value, int32x4_t{ b, g, r, a }); }

	rgbaint_t &operator+=(const rgbaint_t &other)
	{
		m_value = vaddq_s32(m_value, other.m_value);
		return *this;
	}

	rgbaint_t &operator+=(s32 other)
	{
		m_value = vaddq_s32(m_value, vdupq_n_s32(other));
		return *this;
	}

	rgbaint_t &operator-=(const rgbaint_t &other)
	{
		m_value = vsubq_s32(m_value, other.m_value);
		return *this;
	}

	rgbaint_t &operator*=(const rgbaint_t &other)
	{
		m_value = vmulq_s32(m_value, other.m_value);
		return *this;
	}

	rgbaint_t &operator*=(s32 other)
	{
		m_value = vmulq_s32(m_value, vdupq_n_s32(other));
		return *this;
	}

	rgbaint_t &operator>>=(s32 shift)
	{
		m_value = vshlq_s32(m_value, vdupq_n_s32(-s8(shift)));
		return *this;
	}

	void merge_alpha16(const rgbaint_t &alpha)
	{
		m_value = vcopyq_laneq_s32(m_value, 3, alpha.m_value, 3);
	}

	void merge_alpha(const rgbaint_t &alpha)
	{
		m_value = vcopyq_laneq_s32(m_value, 3, alpha.m_value, 3);
	}

	static u32 bilinear_filter(u32 rgb00, u32 rgb01, u32 rgb10, u32 rgb11, u8 u, u8 v)
	{
		// interpolate on u axis
		const uint16x8_t colorx0 = vmulq_n_u16(vmovl_u8(vcreate_u8((u64(rgb10) << 32) | rgb00)), 256U - u);
		const uint16x8_t colorx1 = vmulq_n_u16(vmovl_u8(vcreate_u8((u64(rgb11) << 32) | rgb01)), u);
		const uint32x4_t color0x = vaddq_u32(vmovl_u16(vget_low_u16(colorx0)), vmovl_u16(vget_low_u16(colorx1)));
		const uint32x4_t color1x = vaddq_u32(vmovl_u16(vget_high_u16(colorx0)), vmovl_u16(vget_high_u16(colorx1)));

		// interpolate on v axis
		const uint32x4_t color = vaddq_u32(vmulq_n_u32(color0x, 256U - v), vmulq_n_u32(color1x, v));

		// scale and saturate
		const uint16x4_t color16 = vqmovn_u32(vshrq_n_u32(color, 16));
		return vget_lane_u32(vreinterpret_u32_u8(vqmovn_u16(vcombine_u16(color16, color16))), 0);
	}

	void bilinear_filter_rgbaint(u32 rgb00, u32 rgb01, u32 rgb10, u32 rgb11, u8 u, u8 v)
	{
		// interpolate on u axis
		const uint16x8_t colorx0 = vmulq_n_u16(vmovl_u8(vcreate_u8((u64(rgb10) << 32) | rgb00)), 256U - u);
		const uint16x8_t colorx1 = vmulq_n_u16(vmovl_u8(vcreate_u8((u64(rgb11) << 32) | rgb01)), u);
		const uint32x4_t color0x = vaddq_u32(vmovl_u16(vget_low_u16(colorx0)), vmovl_u16(vget_low_u16(colorx1)));
		const uint32x4_t color1x = vaddq_u32(vmovl_u16(vget_high_u16(colorx0)), vmovl_u16(vget_high_u16(colorx1)));

		// interpolate on v axis
		const uint32x4_t color = vaddq_u32(vmulq_n_u32(color0x, 256U - v), vmulq_n_u32(color1x, v));

		// scale
		m_value = vshrq_n_u32(color, 16);
	}

protected:
	int32x4_t m_value;
};

#endif /* MAME_EMU_VIDEO_RGBNEON_H */

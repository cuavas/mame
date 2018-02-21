// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/*************************************************************************

    Exidy 6502 hardware

*************************************************************************/
#ifndef MAME_INCLUDES_EXIDY_H
#define MAME_INCLUDES_EXIDY_H

#pragma once

#include "sound/dac.h"
#include "sound/samples.h"
#include "screen.h"

#define EXIDY_MASTER_CLOCK              (XTAL(11'289'000))
#define EXIDY_CPU_CLOCK                 (EXIDY_MASTER_CLOCK / 16)
#define EXIDY_PIXEL_CLOCK               (EXIDY_MASTER_CLOCK / 2)
#define EXIDY_HTOTAL                    (0x150)
#define EXIDY_HBEND                     (0x000)
#define EXIDY_HBSTART                   (0x100)
#define EXIDY_HSEND                     (0x140)
#define EXIDY_HSSTART                   (0x120)
#define EXIDY_VTOTAL                    (0x118)
#define EXIDY_VBEND                     (0x000)
#define EXIDY_VBSTART                   (0x100)
#define EXIDY_VSEND                     (0x108)
#define EXIDY_VSSTART                   (0x100)


class exidy_state : public driver_device
{
public:
	enum
	{
		TIMER_COLLISION_IRQ
	};

	exidy_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_dac(*this, "dac"),
		m_samples(*this, "samples"),
		m_gfxdecode(*this, "gfxdecode"),
		m_screen(*this, "screen"),
		m_palette(*this, "palette"),
		m_videoram(*this, "videoram"),
		m_sprite1_xpos(*this, "sprite1_xpos"),
		m_sprite1_ypos(*this, "sprite1_ypos"),
		m_sprite2_xpos(*this, "sprite2_xpos"),
		m_sprite2_ypos(*this, "sprite2_ypos"),
		m_spriteno(*this, "spriteno"),
		m_sprite_enable(*this, "sprite_enable"),
		m_color_latch(*this, "color_latch"),
		m_characterram(*this, "characterram") { }


	required_device<cpu_device> m_maincpu;
	optional_device<dac_bit_interface> m_dac;
	optional_device<samples_device> m_samples;
	required_device<gfxdecode_device> m_gfxdecode;
	required_device<screen_device> m_screen;
	required_device<palette_device> m_palette;

	required_shared_ptr<uint8_t> m_videoram;
	required_shared_ptr<uint8_t> m_sprite1_xpos;
	required_shared_ptr<uint8_t> m_sprite1_ypos;
	required_shared_ptr<uint8_t> m_sprite2_xpos;
	required_shared_ptr<uint8_t> m_sprite2_ypos;
	required_shared_ptr<uint8_t> m_spriteno;
	required_shared_ptr<uint8_t> m_sprite_enable;
	required_shared_ptr<uint8_t> m_color_latch;
	required_shared_ptr<uint8_t> m_characterram;

	uint8_t m_last_dial;
	uint8_t m_collision_mask;
	uint8_t m_collision_invert;
	int m_is_2bpp;
	uint8_t m_int_condition;
	bitmap_ind16 m_background_bitmap;
	bitmap_ind16 m_motion_object_1_vid;
	bitmap_ind16 m_motion_object_2_vid;
	bitmap_ind16 m_motion_object_2_clip;

	DECLARE_READ8_MEMBER(exidy_interrupt_r);

	uint32_t screen_update_exidy(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);

	INTERRUPT_GEN_MEMBER(exidy_vblank_interrupt);

	void exidy_video_config(uint8_t _collision_mask, uint8_t _collision_invert, int _is_2bpp);
	inline void latch_condition(int collision);
	inline void set_1_color(int index, int which);
	void set_colors();
	void draw_background();
	inline int sprite_1_enabled();
	void draw_sprites(bitmap_ind16 &bitmap, const rectangle &cliprect);
	void check_collision();

	/* Targ and Spectar samples */
	int m_max_freq;
	uint8_t m_port_1_last;
	uint8_t m_port_2_last;
	uint8_t m_tone_freq;
	uint8_t m_tone_active;
	uint8_t m_tone_pointer;
	void adjust_sample(uint8_t freq);
	void common_audio_start(int freq);

protected:
	virtual void video_start() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;
	virtual void device_add_mconfig(machine_config &config) override;
	void spectar_audio(machine_config &config);
	void exidy_map(address_map &map);
	DECLARE_WRITE8_MEMBER(targ_audio_1_w);
	DECLARE_WRITE8_MEMBER(targ_audio_2_w);
	DECLARE_WRITE8_MEMBER(spectar_audio_2_w);
	SAMPLES_START_CB_MEMBER(spectar_audio_start);
};

class sidetrac_state : public exidy_state
{
public:
	using exidy_state::exidy_state;
	DECLARE_DRIVER_INIT(sidetrac);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void sidetrac_map(address_map &map);
};

class targ_state : public exidy_state
{
public:
	using exidy_state::exidy_state;
	DECLARE_DRIVER_INIT(targ);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void targ_audio(machine_config &config);
	void targ_map(address_map &map);
	SAMPLES_START_CB_MEMBER(targ_audio_start);
};

class spectar_state : public exidy_state
{
public:
	using exidy_state::exidy_state;
	DECLARE_DRIVER_INIT(spectar);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void spectar_map(address_map &map);
};

class rallys_state : public spectar_state
{
public:
	using spectar_state::spectar_state;
	DECLARE_DRIVER_INIT(rallys);
	DECLARE_DRIVER_INIT(phantoma);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void rallys_map(address_map &map);
};

class venture_state : public exidy_state
{
public:
	using exidy_state::exidy_state;
	DECLARE_DRIVER_INIT(venture);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void venture_audio(machine_config &config);
	void venture_map(address_map &map);
	void venture_audio_map(address_map &map);
};

class teetert_state : public venture_state
{
public:
	using venture_state::venture_state;
	DECLARE_DRIVER_INIT(teetert);
	DECLARE_CUSTOM_INPUT_MEMBER(teetert_input_r);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	DECLARE_MACHINE_START(teetert);
};

class mtrap_state : public venture_state
{
public:
	using venture_state::venture_state;
	DECLARE_DRIVER_INIT(mtrap);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void mtrap_cvsd_audio(machine_config &config);
	void cvsd_iomap(address_map &map);
	void cvsd_map(address_map &map);
};

class pepper2_state : public venture_state
{
public:
	using venture_state::venture_state;
	DECLARE_DRIVER_INIT(pepper2);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void pepper2_map(address_map &map);
};

class fax_state : public pepper2_state
{
public:
	using pepper2_state::pepper2_state;
	DECLARE_DRIVER_INIT(fax);

protected:
	virtual void device_add_mconfig(machine_config &config) override;
	void fax_map(address_map &map);
	DECLARE_WRITE8_MEMBER(fax_bank_select_w);
};

#endif // MAME_INCLUDES_EXIDY_H

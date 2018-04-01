// license:BSD-3-Clause
// copyright-holders:Vas Crabb
/***************************************************************************

    AES/MVS banked M1 ROM cartridges

    NEO-MVS CHA42G:
    * NEO-273 (latch C and S ROM addresses)
    * NEO-ZMC (M1 ROM banking control)
    * 32M C ROM as 4×512k×16 (C1/C2/C3/C4) mirrored every 1M×32
    * 1M S ROM as 128k×8 (S1) - supports ROM or 27C301 EPROM
    * 1M M1 ROM as 128k×8 (M1) mirrored every 128k×8

    NEO-MVS CHA42G-1:
    * Same active components as NEO-MVS CHA42G
    * Space for 150pF suppression capacitor from M1 /CE to ground
    * Only two of four VCC contacts present on each side of PCB
    * Additional components may not be populated
    * May use new layout with ROMs perpendicular to edge connector

***************************************************************************/
#ifndef MAME_BUS_NEOGEO_CART_M1ZMC_H
#define MAME_BUS_NEOGEO_CART_M1ZMC_H

#include "flatrom.h"

#include <memory>


namespace bus { namespace neogeo { namespace cart {

class m1_zmc_device : public flat_rom_device, public device_memory_interface
{
public:
	// construction/destruction
	m1_zmc_device(machine_config const &mconfig, char const *tag, device_t *owner, u32 clock);

	// software loading
	virtual image_init_result configure_software(get_feature_func const &get_feature) override;

	// 8-bit Z80 ROM and I/O
	virtual void install_m1_rom(address_space &space, offs_t base) override;
	virtual void install_m1_io(address_space &space, offs_t base) override;

protected:
	// device_memory_interface
	virtual space_config_vector memory_space_config() const override;

	// Z80 I/O handlers
	DECLARE_READ8_MEMBER(zmc_r);

	// dummy memory map
	void sdrom_map(address_map &map);

	// PCB configuration
	virtual std::unique_ptr<cha_desc const> config_cha(char const *name, get_feature_func const &get_feature) override;
	std::unique_ptr<cha_desc const> config_cha42g(char const *name, get_feature_func const &get_feature);

private:
	address_space_config const      m_space_config;
	optional_memory_bank_array<4>   m_m1_banks;
};

} } } // namespace bus::neogeo::cart


DECLARE_DEVICE_TYPE_NS(NG_CART_M1ZMC, bus::neogeo::cart, m1_zmc_device)

#endif // MAME_BUS_NEOGEO_CART_M1ZMC_H

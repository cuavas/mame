// license:BSD-3-Clause
// copyright-holders:windyfairy

#include "emu.h"
#include "cpu/drctester/drctester.h"

namespace {

class drctester_state : public driver_device
{
public:
	drctester_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
	{
	}

	void drctester(machine_config &config) ATTR_COLD
	{
		DRCTESTER(config, "maincpu", 1_MHz_XTAL);
	}
};

static INPUT_PORTS_START( drctester )
INPUT_PORTS_END

ROM_START( drctester )
	ROM_REGION( 4, "maincpu", 0 )
	ROM_FILL( 0, 4, 0 )
ROM_END

} // anonymous namespace

GAME( 2024, drctester, 0,  drctester,  drctester, drctester_state, empty_init, ROT0, "MAME", "DRC Tester", MACHINE_NO_SOUND_HW )

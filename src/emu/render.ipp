// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Vas Crabb
/***************************************************************************

    render.ipp

    Core rendering routines for MAME.

***************************************************************************/

#ifndef MAME_EMU_RENDER_IPP
#define MAME_EMU_RENDER_IPP

#include "render.h"

#include "rendlay.h"


inline layout_view &render_target::current_view() const
{
	return m_views[m_curview].first;
}

#endif // MAME_EMU_RENDER_IPP

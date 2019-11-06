// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Vas Crabb
#ifndef MAME_EMU_DISCREEN_H
#define MAME_EMU_DISCREEN_H

#pragma once

#include <cassert>
#include <utility>


/// \brief Texture formats
enum texture_format
{
	/// Require a format to be specified
	TEXFORMAT_UNDEFINED = 0,
	/// 16bpp palettized, no alpha
	TEXFORMAT_PALETTE16,
	/// 32bpp 8-8-8 RGB
	TEXFORMAT_RGB32,
	/// 32bpp 8-8-8-8 ARGB
	TEXFORMAT_ARGB32,
	/// 16bpp 8-8 Y/Cb, Y/Cr in sequence
	TEXFORMAT_YUY16
};


/// \brief Base class for screen-like devices
///
/// Provides interfaces used by the render system to manage devices
/// that produce video output.
class device_screen_interface : public device_interface
{
public:

	/// \brief Get screen orientation
	///
	/// Gets the screen orientation.  This is the configured orientation
	/// for the screen combined with the system orientation.  Only valid
	/// after configuration is complete.
	/// \return Orientation flags for the screen.
	/// \sa set_orientation
	int orientation() const noexcept
	{
		assert(device().configured());
		return m_orientation;
	}

	/// \brief Get physical aspect ratio
	///
	/// Gets the physical aspect ratio of the screen before rotation is
	/// applied.  This is used when building default layouts.  The
	/// result is only valid after configuration is complete.
	/// \return A pair consisting of the physical X/Y aspect ratio as a
	///   reduced fraction.
	/// \sa set_physical_aspect set_native_aspect
	std::pair<unsigned, unsigned> physical_aspect() const noexcept;

	/// \brief Get visible area
	///
	/// Gets the visible area of the screen's output.  For CRT monitors
	/// this is the active display area excluding horizontal and
	/// vertical blanking intervals.
	/// \return Constant reference to the screen's visible area, in
	///   coordinates.
	rectangle const &visible_area() const noexcept { return m_vis_area; }

	/// \brief Set screen orientation
	///
	/// Sets the orientation of the screen (applied before system
	/// orientation and any user-specified transform).  This is most
	/// useful for systems with multiple screens that are not all
	/// oriented the same way.  Must not be called after configuration
	/// is complete.
	/// \param [in] orientation Orientation flags for the screen.
	/// \return Reference to interface for method chaining.
	/// \sa orientation
	device_screen_interface &set_orientation(int orientation) noexcept
	{
		assert(!device().configured());
		m_orientation = orientation;
		return *this;
	}

	/// \brief Set physical aspect ratio
	///
	/// Sets the physical aspect ratio of the screen before rotation is
	/// applied.  Call this during configuration to override the default
	/// aspect ratio for the screen type.  Must not be called after
	/// configuration is complete.
	/// \param [in] x Physical X aspect.
	/// \param [in] y Physical Y aspect.
	/// \return Reference to interface for method chaining.
	/// \sa physical_aspect set_native_aspect
	device_screen_interface &set_physical_aspect(unsigned x, unsigned y) noexcept
	{
		assert(!device().configured());
		m_phys_aspect = std::make_pair(x, y);
		return *this;
	}

	/// \brief Set aspect ratio for square pixels
	///
	/// Configures the physical aspect ratio to match the pixel aspect
	/// ratio of the visible area.  Must not be called after configuration
	/// is complete.
	/// \return Reference to interface for method chaining.
	/// \sa physical_aspect set_physical_aspect
	device_screen_interface &set_native_aspect() noexcept
	{
		assert(!device().configured());
		m_phys_aspect = std::make_pair(~0U, ~0U);
		return *this;
	}

	/// \brief Set render container
	///
	/// Set the render container that the screen should draw to.  This
	/// is automatically called by the renderer during initialisation.
	/// This must happen before device start time.
	/// \param [in] container Reference to the container the screen
	///   should draw to.  It is the caller's responsibility to ensure
	///   the container remains valid until device stop time.
	/// \sa container
	void set_container(render_container &container) noexcept
	{
		assert(!m_container);
		m_container = &container;
	}

	/// \brief Get render container
	///
	/// Gets the render container that the screen should draw to.  This
	/// is set by the renderer during initialisation before device start
	/// time.  This member must not be called before device start time
	/// or after device stop time.
	/// \return Reference to the render container the screen should draw
	///   to.
	/// \sa set_container
	render_container &container() const noexcept
	{
		assert(m_container);
		return *m_container;
	}

protected:
	device_screen_interface(machine_config const &mconfig, device_t &device);

	virtual void interface_config_complete() override ATTR_COLD;
	virtual void interface_pre_start() override ATTR_COLD;
	virtual void interface_post_start() override ATTR_COLD;
	virtual void interface_post_stop() override ATTR_COLD;

	bool column_visible(s32 x) const noexcept { return (m_vis_area.left() <= x) && (m_vis_area.right() >= x); }
	bool row_visible(s32 y) const noexcept { return (m_vis_area.top() <= y) && (m_vis_area.bottom() >= y); }

	/// \brief Set visible area
	///
	/// Sets the visible area of the screen's output in pixel
	/// coordinates.
	/// \param [in] minx Leftmost visible column (inclusive).
	/// \param [in] maxx Rightmost visible column (inclusive).
	/// \param [in] miny Topmost visible row (inclusive).
	/// \param [in] maxy Bottommost visible row (inclusive).
	void set_visible_area(s16 minx, s16 maxx, s16 miny, s16 maxy) noexcept
	{
		m_vis_area.set(minx, maxx, miny, maxy);
	}

	/// \brief Get default physical aspect ratio
	///
	/// By default, if the physical aspect ratio is not configured, it
	/// will be calculated to produce square pixels.  Implementations
	/// may override this member to change the default.
	/// \return A pair consisting of the default physical X/Y aspect
	///   ratio for the screen before rotation.
	virtual std::pair<unsigned, unsigned> screen_default_aspect() const noexcept;

private:
	int m_orientation;
	std::pair<unsigned, unsigned> m_phys_aspect;
	render_container *m_container;
	rectangle m_vis_area;
};


typedef device_interface_iterator<device_screen_interface> screen_interface_iterator;

#endif // MAME_EMU_DISCREEN_H

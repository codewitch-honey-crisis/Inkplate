#pragma once
#include "uix.hpp"
#include "gfx.hpp"
extern gfx::const_buffer_stream& text_font;
#if defined(INKPLATE10) || defined(INKPLATE10V2) || defined(LOCAL_PC)
constexpr static const gfx::size16 screen_dimensions = {1200, 825};
#endif
// for the grayscale mode
using gsc_pixel_t = gfx::gsc_pixel<8>;
// for the mono mode
using mono_pixel_t = gfx::gsc_pixel<1>;
// screen colors
using scolor_gsc_t = gfx::color<gsc_pixel_t>;
using scolor_mono_t = gfx::color<mono_pixel_t>;
// UI colors (UIX)
using ucolor_t = gfx::color<gfx::rgba_pixel<32>>;
// vector graphics colors
using vcolor_t = gfx::color<gfx::vector_pixel>;

using screen_gsc_t = uix::screen<gsc_pixel_t>;
using surface_gsc_t = screen_gsc_t::control_surface_type;

using screen_mono_t = uix::screen<mono_pixel_t>;
using surface_mono_t = screen_mono_t::control_surface_type;

/// @brief An SVG icon
/// @tparam ControlSurfaceType The UIX surface type to bind to
template <typename ControlSurfaceType>
class icon_box : public uix::control<ControlSurfaceType> {
    using base_type = uix::control<ControlSurfaceType>;

   public:
    typedef void (*on_pressed_changed_callback_type)(bool pressed, void* state);

   private:
    bool m_pressed;
    bool m_dirty;
    gfx::sizef m_svg_size;
    gfx::matrix m_fit;
    on_pressed_changed_callback_type m_on_pressed_changed_callback;
    void* m_on_pressed_changed_callback_state;
    gfx::stream* m_svg;
    static gfx::rectf correct_aspect(gfx::srect16& sr, float aspect) {
        if (aspect>=1.f) {
            sr.y2 /= aspect;
        } else {
            sr.x2 *= aspect;
        }
        return (gfx::rectf)sr;
    }

   public:
    icon_box()
        : base_type(),
          m_pressed(false),
          m_dirty(true),
          m_on_pressed_changed_callback(nullptr),
          m_svg(nullptr) {m_svg_size = {0.f,0.f};}
    gfx::stream& svg() const { return *m_svg; }
    void svg(gfx::stream& value) {
        m_svg = &value;
        m_dirty = true;
        this->invalidate();
    }
    gfx::sizef svg_size() const { return m_svg_size; }
    void svg_size(const gfx::sizef& value) {
        m_svg_size = value;
        m_dirty = true;
        this->invalidate();
    }
    
    bool pressed() const { return m_pressed; }
    void on_pressed_changed_callback(on_pressed_changed_callback_type callback,
                                     void* state = nullptr) {
        m_on_pressed_changed_callback = callback;
        m_on_pressed_changed_callback_state = state;
    }

   protected:
    virtual void on_before_paint() override {
        if (m_dirty) {
            if (m_svg != nullptr) {
                if(m_svg_size.width==0.f || m_svg_size.height==0.f) {
                    m_svg->seek(0);
                    gfx::canvas::svg_dimensions(*m_svg, &m_svg_size);
                }
                gfx::srect16 sr = this->dimensions().bounds();
                gfx::rectf corrected = correct_aspect(sr, m_svg_size.aspect_ratio());
                corrected.center_inplace((gfx::rectf)sr);
                m_fit = gfx::matrix::create_fit_to(m_svg_size,corrected);
            }
            m_dirty = false;
        }
    }
    virtual void on_paint(ControlSurfaceType& dst,
                          const gfx::srect16& clip) override {
        if (m_dirty || m_svg == nullptr) {
           
            return;
        }
        gfx::canvas cvs((gfx::size16)this->dimensions());
        cvs.initialize();
        gfx::draw::canvas(dst, cvs);
        m_svg->seek(0);
        if (gfx::gfx_result::success != cvs.render_svg(*m_svg, m_fit)) {
            puts("SVG render error");
        }
    }
    virtual bool on_touch(size_t locations_size,
                          const gfx::spoint16* locations) override {
        if (!m_pressed) {
            m_pressed = true;
            if (m_on_pressed_changed_callback != nullptr) {
                m_on_pressed_changed_callback(
                    true, m_on_pressed_changed_callback_state);
            }
            m_dirty = true;
            this->invalidate();
        }
        return true;
    }
    virtual void on_release() override {
        if (m_pressed) {
            m_pressed = false;
            if (m_on_pressed_changed_callback != nullptr) {
                m_on_pressed_changed_callback(
                    false, m_on_pressed_changed_callback_state);
            }
            m_dirty = true;
            this->invalidate();
        }
    }
};

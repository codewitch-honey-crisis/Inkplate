#pragma once
#include "uix.hpp"
#include "gfx.hpp"
extern gfx::const_buffer_stream& text_font;
#if defined(INKPLATE10) || defined(INKPLATE10V2) || defined(LOCAL_PC)
constexpr static const gfx::size16 screen_dimensions = {1200, 825};
#endif
// for the grayscale mode
using gsc3_pixel_t = gfx::pixel<
    // unused bit
    gfx::channel_traits<gfx::channel_name::nop,1>, 
    // 3 bits (L)uminosity channel
    gfx::channel_traits<gfx::channel_name::L,3>
>;
// for the mono mode
using gsc1_pixel_t = gfx::gsc_pixel<1>;
// screen colors
using scolor3_t = gfx::color<gsc3_pixel_t>;
using scolor1_t = gfx::color<gsc1_pixel_t>;
// UI colors (UIX)
using ucolor_t = gfx::color<gfx::rgba_pixel<32>>;
// vector graphics colors
using vcolor_t = gfx::color<gfx::vector_pixel>;

using screen3_t = uix::screen<gsc3_pixel_t>;
using surface3_t = screen3_t::control_surface_type;

using screen1_t = uix::screen<gsc1_pixel_t>;
using surface1_t = screen1_t::control_surface_type;

template<typename ControlSurfaceType>
class vlabel : public uix::canvas_control<ControlSurfaceType> {
    using base_type = uix::canvas_control<ControlSurfaceType>;
public:
    using type = vlabel;
    using control_surface_type = ControlSurfaceType;
private:
    gfx::canvas_text_info m_label_text;
    gfx::canvas_path m_label_text_path;
    gfx::rectf m_label_text_bounds;
    bool m_label_text_dirty;
    gfx::vector_pixel m_color;
    gfx::rgba_pixel<32> m_background_color;
    gfx::stream* m_font_stream;
    void build_label_path_untransformed() {
        if(m_font_stream==nullptr) {
            return;
        }
        m_label_text.ttf_font = m_font_stream;
        const float target_width = this->dimensions().width;
        float fsize = this->dimensions().height*.8f;
        m_label_text_path.initialize();
        do {
            m_label_text_path.clear();
            m_label_text.font_size = fsize;
            m_label_text_path.text({0.f,0.f},m_label_text);
            m_label_text_bounds = m_label_text_path.bounds(true);
            --fsize;
            
        } while(fsize>0.f && m_label_text_bounds.width()>=target_width);
    }
public:
    vlabel() : base_type() ,m_label_text_dirty(true) {
        m_font_stream = nullptr;
        m_label_text.text_sz("Label");
        m_label_text.encoding = &gfx::text_encoding::utf8;
        m_label_text.ttf_font_face = 0;
        m_color = gfx::vector_pixel(255,255,255,255);
        m_background_color = gfx::rgba_pixel<32>(0,true);
    }
    virtual ~vlabel() {

    }
    gfx::text_handle text() const {
        return m_label_text.text;
    }
    void text(gfx::text_handle text, size_t text_byte_count) {
        m_label_text.text=text;
        m_label_text.text_byte_count = text_byte_count;
        m_label_text_dirty = true;
        this->invalidate();
    }
    void text(const char* sz) {
        m_label_text.text_sz(sz);
        m_label_text_dirty = true;
        this->invalidate();
    }
        gfx::stream& font() const {
        return *m_font_stream;
    }
    void font(gfx::stream& value) {
        m_font_stream = &value;
        m_label_text_dirty = true;
        this->invalidate();
    }
    gfx::rgba_pixel<32> color() const {
        gfx::rgba_pixel<32> result;
        convert(m_color,&result);
        return result;
    }
    void color(gfx::rgba_pixel<32> value) {
        convert(value,&m_color);
        this->invalidate();
    }
    gfx::rgba_pixel<32> background_color() const {
        return m_background_color;
    }
    void background_color(gfx::rgba_pixel<32> value) {
        m_background_color = value;
        this->invalidate();
    }
protected:
    virtual void on_before_paint() override {
        if(m_label_text_dirty) {
            build_label_path_untransformed();
            m_label_text_dirty = false;
        }
    }
    virtual void on_paint(control_surface_type& destination, const gfx::srect16& clip) {
        if(m_background_color.opacity()!=0) {
            gfx::draw::filled_rectangle(destination,destination.bounds(),m_background_color);
        }
        base_type::on_paint(destination,clip);
    }
    virtual void on_paint(gfx::canvas& destination, const gfx::srect16& clip) override {
        if(m_font_stream==nullptr) {
            return;
        }
        gfx::canvas_style si = destination.style();
        si.fill_paint_type = gfx::paint_type::solid;
        si.stroke_paint_type = gfx::paint_type::none;
        si.fill_color = m_color;
        destination.style(si);
        // save the current transform
        gfx::matrix old = destination.transform();
        gfx::matrix m = gfx::matrix::create_identity();
        m=m.translate(-m_label_text_bounds.x1,(-m_label_text_bounds.y1)+(0/*destination.dimensions().height-m_label_text_bounds.height()*/)*0.5f);
        destination.transform(m);
        destination.path(m_label_text_path);
        destination.render();
        // restore the old transform
        destination.transform(old);
    }

};
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
    }
    gfx::sizef svg_size() const { return m_svg_size; }
    void svg_size(const gfx::sizef& value) {
        m_svg_size = value;
        m_dirty = true;
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

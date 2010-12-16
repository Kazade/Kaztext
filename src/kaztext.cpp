#include <map>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/ftglyph.h>

#include <GL/gl.h>
#include <boost/shared_ptr.hpp>

#include "kaztext.h"

static KTuint next_font_id = 0;

KTuint get_next_font_id() {
    return ++next_font_id;
}

class Font {
public:
    typedef boost::shared_ptr<Font> ptr;

    struct Char {
        uint32_t width;
        uint32_t height;
        uint32_t texture_width;
        uint32_t texture_height;
        float advance_x;
        float advance_y;
        float tex_coord_x;
        float tex_coord_y;
        float left;
        float top;
    };

    Font(const std::string& ttf, const int font_size);
    bool load();
    bool generate_glyph_texture(wchar_t ch);

    int get_size() const { return font_size_; }
    float get_char_advance_y(wchar_t ch) { return char_properties_[ch].advance_y; }
    float get_char_advance_x(wchar_t ch) { return char_properties_[ch].advance_x; }
    float get_char_left(wchar_t ch) { return char_properties_[ch].left; }
    float get_char_top(wchar_t ch) { return char_properties_[ch].top; }

    GLuint get_char_texture(wchar_t ch) { return textures_[ch]; }

    uint32_t get_char_width(wchar_t ch) { return char_properties_[ch].width; }
    uint32_t get_char_height(wchar_t ch) { return char_properties_[ch].height; }
    float get_char_tex_coord_x(wchar_t ch) { return char_properties_[ch].tex_coord_x; }
    float get_char_tex_coord_y(wchar_t ch) { return char_properties_[ch].tex_coord_y; }
private:
    std::string ttf_;
    int font_size_;
    FT_Face face_;

    std::map<wchar_t, GLuint> textures_;
    std::map<wchar_t, Char> char_properties_;
};

static std::map<uint32_t, Font::ptr> fonts_;
static uint32_t current_font_;


struct FreeTypeInitializer {
    FreeTypeInitializer() {
        FT_Init_FreeType(&ftlib);
    }

    ~FreeTypeInitializer() {
        FT_Done_FreeType(ftlib);
    }

    FT_Library ftlib;
};

FreeTypeInitializer ft;

Font::Font(const std::string& ttf, const int font_size):
ttf_(ttf),
font_size_(font_size) {

}

static uint32_t pow2(uint32_t i) {
    if(i == 1) return 2;

    uint32_t result;

    for(result = 1; result < i; result <<= 1);
    return result;
}

bool Font::generate_glyph_texture(wchar_t ch) {
    if(textures_[ch] > 0) return true;

    if(FT_Load_Glyph(face_, FT_Get_Char_Index(face_, ch), FT_LOAD_DEFAULT) != 0) {
        return false;
    }

    FT_Glyph glyph;
    if(FT_Get_Glyph(face_->glyph, &glyph) != 0) {
        return false;
    }

    FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
    FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph) glyph;
    FT_Bitmap& bitmap = bitmap_glyph->bitmap;

    char_properties_[ch].width = bitmap.width;
    char_properties_[ch].height = bitmap.rows;
    char_properties_[ch].texture_width = pow2(bitmap.width);
    char_properties_[ch].texture_height = pow2(bitmap.rows);
    char_properties_[ch].left = bitmap_glyph->left;
    char_properties_[ch].top = bitmap_glyph->top;

    uint32_t tex_w = char_properties_[ch].texture_width;
    uint32_t tex_h = char_properties_[ch].texture_height;

    std::vector<GLubyte> data(tex_w * tex_h * 2);
    for(KTuint j = 0; j < tex_h; ++j) {
        for(KTuint i = 0; i < tex_w; ++i) {
            int idx = 2 * (i + j * tex_w);
            data[idx] = data[idx+1] = (i >= bitmap.width || j >= bitmap.rows) ? 0 : bitmap.buffer[i + bitmap.width * j];
        }
    }

    glGenTextures(1, &textures_[ch]);
    glBindTexture(GL_TEXTURE_2D, textures_[ch]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA16, tex_w, tex_h, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, &data[0]);

    char_properties_[ch].advance_x = (float) (face_->glyph->advance.x >> 6);
    char_properties_[ch].advance_y = (float) ((face_->glyph->metrics.horiBearingY - face_->glyph->metrics.height) >> 6);
    char_properties_[ch].tex_coord_x = float(bitmap.width) / float(tex_w);
    char_properties_[ch].tex_coord_y = float(bitmap.rows) / float(tex_h);

    return true;
}

bool Font::load() {
    if(FT_New_Face(ft.ftlib, ttf_.c_str(), 0, &face_) != 0) {
        return false;
    }

    FT_Set_Char_Size(face_, font_size_ << 6, font_size_ << 6, 96, 96);

    //Cache the common ascii chars to prevent slowdown, perhaps a better way would be
    //to pre-generate strings e.g.
    /*
        ktCacheString(L"This is my string");
    */
    for(wchar_t i = 32; i <= 127; ++i) {
        generate_glyph_texture(i);
    }

    return true;
}

void ktGenFonts(uint32_t n, uint32_t* fonts) {
    for(uint32_t i = 0; i < n; ++i) {
        uint32_t new_id = get_next_font_id();
        fonts_[new_id] = Font::ptr();
        fonts[i] = new_id;
    }
}

void ktBindFont(KTuint n) {
    assert(fonts_.find(n) != fonts_.end() && "Invalid font id");
    current_font_ = n;
}

void ktLoadFont(const char* name, KTsizei size) {
    fonts_[current_font_] = Font::ptr(new Font(name, size));
    fonts_[current_font_]->load();
}

void ktDrawText(float x, float y, const KTwchar* text_in) {
    glPushAttrib(GL_ENABLE_BIT);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    std::wstring text(text_in);

    Font::ptr font = fonts_[current_font_];

    glPushMatrix();
    glScalef(1, -1, 1);
    glTranslatef(x, -y, 0);

    for(std::wstring::const_iterator it = text.begin(); it != text.end(); ++it) {
        wchar_t ch = (*it);

        font->generate_glyph_texture(ch);

        glPushMatrix();
            glTranslatef(font->get_char_left(ch), font->get_char_top(ch) - font->get_char_height(ch), 0);
            glBindTexture(GL_TEXTURE_2D, font->get_char_texture(ch));
            glBegin(GL_QUADS);
                glTexCoord2f(0, 0);
                glVertex2f(0, font->get_char_height(ch));
                glTexCoord2f(0, font->get_char_tex_coord_y(ch));
                glVertex2f(0, 0);
                glTexCoord2f(font->get_char_tex_coord_x(ch), font->get_char_tex_coord_y(ch));
                glVertex2f(font->get_char_width(ch), 0);
                glTexCoord2f(font->get_char_tex_coord_x(ch), 0);
                glVertex2f(font->get_char_width(ch), font->get_char_height(ch));
            glEnd();
        glPopMatrix();
        glTranslatef(font->get_char_advance_x(ch), 0, 0);
    }
    glPopMatrix();

    glPopAttrib();
}

void ktCacheString(const KTwchar* s) {
    std::wstring text(s);

    for(std::wstring::size_type i = 0; i < text.length(); ++i) {
        fonts_[current_font_]->generate_glyph_texture(i);
    }
}

KTfloat ktGetStringWidth(const KTwchar* text_in) {
    std::wstring text(text_in);

    float len = 0;
    for(std::wstring::size_type i = 0; i < text.length(); ++i) {
        len += fonts_[current_font_]->get_char_advance_x(text[i]);
    }
    return len;
}

void ktDrawTextCentred(float x, float y, const KTwchar* text) {
    KTfloat length = ktGetStringWidth(text);
    glPushMatrix();
    glTranslatef(-length / 2.0f, 0.0f, 0.0f);
    ktDrawText(x, y, text);
    glPopMatrix();
}

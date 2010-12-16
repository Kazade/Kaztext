#ifndef KAZTEXT_H_INCLUDED
#define KAZTEXT_H_INCLUDED

#include <string>

typedef unsigned int KTsizei;
typedef unsigned int KTuint;
typedef char KTchar;
typedef wchar_t KTwchar;
typedef float KTfloat;

void ktGenFonts(KTsizei n, KTuint* fonts);
void ktBindFont(KTuint font);
void ktLoadFont(const KTchar* filename, const KTsizei font_size);
void ktDrawText(float x, float y, const KTwchar* text);
void ktDrawTextCentred(float x, float y, const KTwchar* text);
void ktDeleteFonts(KTsizei n, const KTuint* fonts);
void ktCacheString(const KTwchar* string);

KTfloat ktGetStringWidth(const KTwchar* text);

#endif // KAZTEXT_H_INCLUDED

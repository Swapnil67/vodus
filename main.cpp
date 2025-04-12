#include <cstdio>
#include <cassert>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FACE_FILE_PATH "./Comic-Sans-MS.ttf"

int save_bitmap_as_ppm(FT_Bitmap *bitmap, char prefix_char) {
  assert(bitmap->pixel_mode ==  FT_PIXEL_MODE_GRAY);
  char filename[10];
  snprintf(filename, 10, "%c.ppm", prefix_char);
  FILE *f = fopen(filename, "wb");
  if(!f) {
    return -1;
  }

  fprintf(f,
          "P6\n"
          "%d %d\n"
          "255\n",
          bitmap->width,
          bitmap->rows);

  for (unsigned int row = 0; row < bitmap->rows; ++row) {
    for (unsigned int col = 0; col < bitmap->width; ++col) {
      int index = (bitmap->pitch * row) + col;
      unsigned char x = *(bitmap->buffer + index);
      // printf("%c\n", x);
      fputc(x, f);
      fputc(x, f);
      fputc(x, f);
    }
  }

  fclose(f);
  return 1;
}

int main() {
  FT_Library  library;   /* handle to library     */
  FT_Face     face;      /* handle to face object */

  auto error = FT_Init_FreeType(&library);
  if(error) {
    fprintf(stderr, "Could not initialize FreeType2\n");
    exit(1);
  }

  error = FT_New_Face(library, FACE_FILE_PATH, 0, &face);
  if (error == FT_Err_Unknown_File_Format) {
    fprintf(stderr, "the %s could be opened and read, but it appears that its font format is unsupported\n", FACE_FILE_PATH);
    exit(1);
  }
  else if (error) {
    fprintf(stderr, "the %s could be opened and read.\n", FACE_FILE_PATH);
  }
  
  printf("Loaded %s\n", FACE_FILE_PATH);
  printf("\tnum_glyphs = %ld\n", face->num_glyphs);

  error = FT_Set_Pixel_Sizes(
    face,   /* handle to face object */
    0,      /* pixel_width           */
    64);   /* pixel_height          */

  if(error) {
    fprintf(stderr, "could not set font size in pixels\n");
    exit(1);
  }

  const char *text = "a";
  while(*text != '\0') {
    auto glyph_index = FT_Get_Char_Index(face, *text);
    error = FT_Load_Glyph(face, glyph_index, 0);
    if (error) {
      fprintf(stderr, "could not load glyph for %c\n", *text);
      exit(1);
    }
    error = FT_Render_Glyph(face->glyph,            /* glyph slot  */
                            FT_RENDER_MODE_NORMAL); /* render mode */
    if (error) {
      fprintf(stderr, "could not render glyph for %c\n", *text);
      exit(1);
    }
    printf("\t'%c'\n", *text);
    printf("\t\tglyph_index: %u\n",glyph_index);
    printf("\t\trows: %u\n", face->glyph->bitmap.rows);
    printf("\t\twidth: %u\n", face->glyph->bitmap.width);
    printf("\t\tpitch: %u\n", face->glyph->bitmap.pitch);
    printf("\t\tpixel_mode: %u\n", face->glyph->bitmap.pixel_mode);

    save_bitmap_as_ppm(&face->glyph->bitmap, *text);
    text++;
  }

  return 0;
}
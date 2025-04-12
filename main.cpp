#include <cstdio>
#include <cassert>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FACE_FILE_PATH "./Comic-Sans-MS.ttf"

int save_bitmap_as_ppm(FT_Bitmap *bitmap, const char* filename) {
  assert(bitmap->pixel_mode ==  FT_PIXEL_MODE_GRAY);
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

void slap_bitmap_onto_bitmap(FT_Bitmap *dest, FT_Bitmap *src, int x, int y)
{
  assert(dest);
  assert(dest->pixel_mode ==  FT_PIXEL_MODE_GRAY);
  assert(src);
  assert(src->pixel_mode ==  FT_PIXEL_MODE_GRAY);

  for (unsigned int row = 0;
       (row < src->rows) && (row + x < dest->rows);
       ++row) {
    for (unsigned int col = 0;
         (col < src->width) && (col + y < dest->width);
         ++col) {
      dest->buffer[(row + y) * dest->pitch + col + x] = src->buffer[row * src->pitch + col];
    }
  }
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

  FT_GlyphSlot  slot = face->glyph;  /* a small shortcut */

  FT_Bitmap surface;
  surface.width = 800;
  surface.pitch = 800;
  surface.rows = 600;
  surface.buffer = (unsigned char *)malloc(surface.width * surface.pitch);
  surface.pixel_mode = FT_PIXEL_MODE_GRAY;

  const char *text = "Hello, World";
  size_t text_count = strlen(text);  

  int pen_x = 0, pen_y = 100;
  for(int i = 0; i < (int) text_count; ++i) {

    // * retrieve glyph index from character code
    FT_UInt glyph_index = FT_Get_Char_Index(face, text[i]);
  
    // * load glyph image into the slot (erase previous one)
    error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
    if (error) {
      fprintf(stderr, "could not load glyph for %c\n", text[i]);
      exit(1);
    }

    // * convert to an anti-aliased bitmap
    error = FT_Render_Glyph(face->glyph,            /* glyph slot  */
                            FT_RENDER_MODE_NORMAL); /* render mode */
    if (error) {
      fprintf(stderr, "could not render glyph for %c\n", text[i]);
      exit(1);
    }

    // * load glyph image into the slot (erase previous one)
    // error = FT_Load_Char(face, text[i], FT_LOAD_RENDER);
    // if (error) {
    //   fprintf(stderr, "Error during rendering character %c\n", text[i]);
    //   exit(1);
    // }

    printf("\t'%c'\n", text[i]);
    printf("\t\trows: %u\n", slot->bitmap.rows);
    printf("\t\twidth: %u\n", slot->bitmap.width);
    printf("\t\tpitch: %u\n", slot->bitmap.pitch);
    printf("\t\tpixel_mode: %u\n", slot->bitmap.pixel_mode);

    slap_bitmap_onto_bitmap(&surface,
                            &face->glyph->bitmap,
                            pen_x + slot->bitmap_left,
                            pen_y - slot->bitmap_top);

    //* increment pen position
    pen_x += slot->advance.x >> 6;

    save_bitmap_as_ppm(&surface, "output.ppm");
  }
  
  return 0;
}
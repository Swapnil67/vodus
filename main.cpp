#include <cstdio>
#include <cassert>
#include <ft2build.h>
#include <gif_lib.h>
#include FT_FREETYPE_H

#define FACE_FILE_PATH "./Comic-Sans-MS.ttf"

struct Pixels32 {
  uint8_t r, g, b, a;
};

// * Simple custom image fomat
struct Image32 {
  int height, width;
  Pixels32 *pixels;
};

int save_image32_as_ppm(Image32 *image, const char* filename) {
  FILE *f = fopen(filename, "wb");
  if(!f) {
    return -1;
  }

  // * Create a PPM file format
  fprintf(f,
          "P6\n"
          "%d %d\n"
          "255\n",
          (int)image->width,
          (int)image->height);

  for (int row = 0; row < (int)image->height; ++row) {
    for (int col = 0; col < (int)image->width; ++col) {
      int index = ((int)image->width * row) + col;
      Pixels32 p = *(image->pixels + index);
      // printf("%c\n", x);
      fputc(p.r, f);
      fputc(p.g, f);
      fputc(p.b, f);
    }
  }

  fclose(f);
  return 0;
}

// * Slap FreeType bitmap onto Image32
void slap_onto_image32(Image32 *dest, FT_Bitmap *src, int x, int y) {
  for (int row = 0;
       (row < (int)src->rows) && (row + x < (int)dest->height);
       ++row)
  {
    for (int col = 0;
         (col < (int)src->width) && (col + y < (int)dest->width);
         ++col)
    {
      int index = (row + y) * dest->width + col + x;
      dest->pixels[index].r = src->buffer[row * src->pitch + col];
      dest->pixels[index].g = src->buffer[row * src->pitch + col];
      dest->pixels[index].b = src->buffer[row * src->pitch + col];
    }
  }
}

// * Slap giflib single image frame onto Image32
void slap_onto_image32(Image32 *dest, SavedImage *src, ColorMapObject *SColorMap, int x, int y) {
  assert(src);
  assert(SColorMap);
  assert(SColorMap->BitsPerPixel == 8);
  assert(!SColorMap->SortFlag);
  assert(src->ImageDesc.Top == 0);
  assert(src->ImageDesc.Left == 0);

  for (int row = 0;
    (row < (int)src->ImageDesc.Height) && (row + x < (int)dest->height);
    ++row) {
    for (int col = 0;
          (col < (int)src->ImageDesc.Width) && (col + y < (int)dest->width);
          ++col) {
      GifColorType pixel = SColorMap->Colors[src->RasterBits[row * src->ImageDesc.Width + col]];
      dest->pixels[(row + y) * dest->width + col + x].r = pixel.Red;
      dest->pixels[(row + y) * dest->width + col + x].g = pixel.Green;
      dest->pixels[(row + y) * dest->width + col + x].b = pixel.Blue;
    }
  }
}

// * Save FreeType bitmap as a ppm file
// * https://netpbm.sourceforge.net/doc/ppm.html [PPM Specification]
int save_bitmap_as_ppm(FT_Bitmap *bitmap, const char* filename) {
  assert(bitmap->pixel_mode ==  FT_PIXEL_MODE_GRAY);
  FILE *f = fopen(filename, "wb");
  if(!f) {
    return -1;
  }

  // * Create a PPM file format
  fprintf(f,
          "P6\n"
          "%d %d\n"
          "255\n",
          bitmap->width,
          bitmap->rows);

  for (unsigned int row = 0; row < bitmap->rows; ++row) {
    for (unsigned int col = 0; col < bitmap->width; ++col) {
      unsigned int index = (unsigned int)(bitmap->pitch * (int)row) + col;
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

  for (int row = 0;
       (row < (int)src->rows) && (row + x < (int)dest->rows);
       ++row)
  {
    for (int col = 0;
         (col < (int)src->width) && (col + y < (int)dest->width);
         ++col)
    {
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
      face, /* handle to face object */
      0,    /* pixel_width           */
      64);  /* pixel_height          */

  if (error) {
    fprintf(stderr, "could not set font size in pixels\n");
    exit(1);
  }
  FT_GlyphSlot slot = face->glyph;  /* a small shortcut */


  // * Read gif file
  const char* gif_filepath = "cat-swag.gif";
  GifFileType *gif_file = DGifOpenFileName(gif_filepath, &error);
  if(error > 0) {
    fprintf(stderr, "could not read gif file: %s\n", gif_filepath);
  }
  DGifSlurp(gif_file);

  // * Custom Image32 Surface
  Image32 surface;
  surface.width = 800;
  surface.height = 600;
  surface.pixels = (Pixels32 *)malloc((unsigned long)surface.width * (unsigned long)surface.height * sizeof(Pixels32));
  assert(surface.pixels);

  const char *text = "Hello, World";
  size_t text_count = strlen(text);  

  int pen_x = 0, pen_y = 100;
  for(int i = 0; i < (int) text_count; ++i) {

    // * retrieve glyph index from character code
    FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_UInt)text[i]);

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

    slap_onto_image32(&surface,
                      &face->glyph->bitmap,
                      pen_x + slot->bitmap_left,
                      pen_y - slot->bitmap_top);

    //* increment pen position
    pen_x += slot->advance.x >> 6;

  }

  slap_onto_image32(&surface, &gif_file->SavedImages[0], gif_file->SColorMap, 0, 0);

  save_image32_as_ppm(&surface, "output.ppm");

  DGifCloseFile(gif_file, &error);
  
  return 0;
}
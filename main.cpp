#include <cstdio>
#include <cmath>
#include <cassert>
#include <ft2build.h>
#include <gif_lib.h>
#include <png.h>
#include FT_FREETYPE_H

#define FACE_FILE_PATH "./Comic-Sans-MS.ttf"
#define VODUS_WIDTH 690
#define VODUS_HEIGHT 420
#define VODUS_DURATION 3.0f
#define VODUS_FPS 100
#define VODUS_DELTA_TIME (1.0f / VODUS_FPS)

struct Pixels32 {
  uint8_t r, g, b, a;
};

// * Simple custom image fomat
struct Image32 {
  int height, width;
  Pixels32 *pixels;
};

int save_image32_as_png(Image32 *image32, const char *filename) {
  png_image pimage;
  memset(&pimage, 0, sizeof(png_image));
  pimage.opaque = NULL;
  pimage.width = image32->width;
  pimage.height = image32->height;
  pimage.version = PNG_IMAGE_VERSION; 
  pimage.format = PNG_FORMAT_RGBA;

  int convert_to_8bit = 0;
  png_image_write_to_file(&pimage, filename, convert_to_8bit, image32->pixels, 0, nullptr);

  return 0;
}

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

// * Slap image32 onto Image32
void slap_onto_image32(Image32 *dest, Image32 *src, int x, int y) {
  for (int row = 0;
       (row < (int)src->height) && (row + x < (int)dest->height);
       ++row)
  {
    for (int col = 0;
         (col < (int)src->width) && (col + y < (int)dest->width);
         ++col)
    {
      dest->pixels[(row + y) * dest->width + col + x] = src->pixels[row * src->width + col];
    }
  }
}

void fill_image32_with_color(Image32 *image, Pixels32 color)
{
  int n = image->height * image->width;
  for(int i = 0; i < n ;++i) {
    image->pixels[i] = color;
  }
}

// * Slap FreeType bitmap onto Image32
void slap_onto_image32(Image32 *dest, FT_Bitmap *src, Pixels32 color, int x, int y) {
  assert(src->pixel_mode == FT_PIXEL_MODE_GRAY);
  assert(src->num_grays == 256);

  for (int row = 0;
       (row < (int)src->rows) && (row + x < (int)dest->height);
       ++row) {
    for (int col = 0;
         (col < (int)src->width) && (col + y < (int)dest->width);
         ++col) {
      int index = (row + y) * dest->width + col + x;
      float a = src->buffer[row * src->pitch + col] / 255.0f;

      // printf("%d %f\n", src->buffer[row * src->pitch + col], a);
      // dest->pixels[index].r = color.r * a;
      // dest->pixels[index].g = color.g * a;
      // dest->pixels[index].b = color.b * a;
      // dest->pixels[index].a = color.a * a;

      dest->pixels[index].r = color.r * a + (1.0f - a) * dest->pixels[index].r;
      dest->pixels[index].g = color.g * a + (1.0f - a) * dest->pixels[index].g;
      dest->pixels[index].b = color.b * a + (1.0f - a) * dest->pixels[index].b;
      dest->pixels[index].a = color.a * a + (1.0f - a) * dest->pixels[index].a;
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
       ++row)
  {
    for (int col = 0;
         (col < (int)src->ImageDesc.Width) && (col + y < (int)dest->width);
         ++col)
    {
      GifColorType pixel = SColorMap->Colors[src->RasterBits[row * src->ImageDesc.Width + col]];
      int dest_index = (row + y) * dest->width + col + x;
      dest->pixels[dest_index].r = pixel.Red;
      dest->pixels[dest_index].g = pixel.Green;
      dest->pixels[dest_index].b = pixel.Blue;
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

Image32 load_image32_from_png(const char *filepath) {
  png_image png;
  png.version = PNG_IMAGE_VERSION;
  png.opaque = NULL;
  
  png_image_begin_read_from_file(&png, filepath);
  if(png.warning_or_error) {
    fprintf(stderr, "could not read file %s. %s\n", filepath, png.message);
    exit(1);
  }
  
  png.format = PNG_FORMAT_RGBA;
  Pixels32 *buffer = (Pixels32 *)malloc(sizeof(Pixels32) * png.height * png.width);
  
  png_int_32 row_stride = 0;
  png_image_finish_read(&png, NULL, buffer, row_stride, NULL);

  free(buffer);
  png_image_free(&png);

  Image32 result = {
      .width = (int)png.width,
      .height = (int)png.height,
      .pixels = buffer};
  return result;
}

void slap_text_onto_image32(Image32 surface, FT_Face face, const char *text, Pixels32 color, int x, int y) {
  size_t text_count = strlen(text);
  int pen_x = x, pen_y = y;
  FT_GlyphSlot slot = face->glyph; /* a small shortcut */

  for(int i = 0; i < (int) text_count; ++i) {

    // * retrieve glyph index from character code
    FT_UInt glyph_index = FT_Get_Char_Index(face, (FT_UInt)text[i]);

    // * load glyph image into the slot (erase previous one)
    auto error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
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
                      color,
                      pen_x + slot->bitmap_left,
                      pen_y - slot->bitmap_top);

    //* increment pen position
    pen_x += slot->advance.x >> 6;

  }
}

int main(int argc, char *argv[]) {
  if(argc < 4) {
    fprintf(stderr, "Usage: ./vodus <text> <gif_image> <png_image> [font]\n");
    exit(1);
  }

  const char *text = argv[1];
  const char *gif_filepath = argv[2];
  const char *png_filepath = argv[3];
  const char *font_face_file_path = FACE_FILE_PATH;
  if (argc >= 5) {
    font_face_file_path = argv[4];
  }

  // * Freetype library initialization
  FT_Library  library;   /* handle to library     */
  FT_Face     face;      /* handle to face object */
  auto error = FT_Init_FreeType(&library);
  error = FT_New_Face(library, font_face_file_path, 0, &face);
  if(error) {
    fprintf(stderr, "Could not initialize FreeType2\n");
    exit(1);
  }

  if (error == FT_Err_Unknown_File_Format) {
    fprintf(stderr, "the %s could be opened and read, but it appears that its font format is unsupported\n", font_face_file_path);
    exit(1);
  }
  else if (error) {
    fprintf(stderr, "the %s could be opened and read.\n", font_face_file_path);
  }
  printf("Loaded %s\n", font_face_file_path);
  // printf("\tnum_glyphs = %ld\n", face->num_glyphs);

  error = FT_Set_Pixel_Sizes(
      face, /* handle to face object */
      0,    /* pixel_width           */
      64);  /* pixel_height          */

  if (error) {
    fprintf(stderr, "could not set font size in pixels\n");
    exit(1);
  }


  // * Read gif file
  GifFileType *gif_file = DGifOpenFileName(gif_filepath, &error);
  if(error > 0) {
    fprintf(stderr, "could not read gif file: %s\n", gif_filepath);
  }
  DGifSlurp(gif_file);

  // * Custom Image32 Surface
  Image32 surface;
  surface.width = VODUS_WIDTH;
  surface.height = VODUS_HEIGHT;
  surface.pixels = (Pixels32 *)malloc((unsigned long)surface.width * (unsigned long)surface.height * sizeof(Pixels32));
  assert(surface.pixels);
  
  float text_x = 0.0f;
  float text_y = VODUS_HEIGHT;

  #define GIF_DURATION 2.0f
  float gif_dt = GIF_DURATION / gif_file->ImageCount;
  float t = 0.0f;

  char filename[256];
  for (size_t i = 0; text_y > 0.0f; ++i) {
    // * Clean up the surface
    fill_image32_with_color(&surface, {50, 50, 50, 255});

    // * Slap the text onto image32
    Pixels32 color = {255, 0, 0, 255};
    slap_text_onto_image32(surface, face, text, color, (int)text_x, (int)text_y);


    int gif_index = ((int)(t / gif_dt) % gif_file->ImageCount);
    assert(gif_file->ImageCount > 0);
    slap_onto_image32(&surface,
                      &gif_file->SavedImages[gif_index],
                      gif_file->SColorMap,
                      (int)text_x, (int)text_y);

    // Image32 image32_png = load_image32_from_png(png_filepath);
    // slap_onto_image32(&surface,
    //                   &image32_png,
    //                   (int)text_x, (int)text_y);

    // * get text_y position
    text_y -= (VODUS_HEIGHT / VODUS_DURATION) * VODUS_DELTA_TIME;

    // * create a filename
    snprintf(filename, 256, "output/frame-%04zu.png", i);
    printf("Rendering: %s\n",filename);

    // * Create a png image
    save_image32_as_png(&surface, filename);

    t += VODUS_DELTA_TIME;
  }

  DGifCloseFile(gif_file, &error);
  
  return 0;
}

// * ffmpeg -framerate 100 -i 'frame-%04d.png' output.mp4
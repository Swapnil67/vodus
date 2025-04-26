#include <cstdio>
#include <cmath>
#include <cassert>
#include <ft2build.h>
#include <gif_lib.h>
#include <png.h>
#include <pthread.h>
#include <atomic>

#include FT_FREETYPE_H

#define FACE_FILE_PATH "./Comic-Sans-MS.ttf"
#define VODUS_WIDTH 690
#define VODUS_HEIGHT 420
#define VODUS_FPS 100
#define VODUS_DELTA_TIME (1.0f / VODUS_FPS)
#define VODUS_OUTPUT_THREADS_COUNT 4

template <typename F>
struct Defer
{
  Defer(F f): f(f) {}

  ~Defer() {
    f();
  }

  F f;
};

#define CONCAT0(a, b) a##b
#define CONCAT(a, b) CONCAT0(a, b)
#define defer(body) Defer CONCAT(defer, __LINE__)([&]() { body; })

struct Pixels32 {
  uint8_t r, g, b, a;
};

// * Simple custom image fomat
struct Image32 {
  int height, width;
  Pixels32 *pixels;
};


int save_image32_as_png(Image32 image32, const char *filename) {
  png_image pimage;
  memset(&pimage, 0, sizeof(png_image));
  pimage.opaque = NULL;
  pimage.width = (png_uint_32)image32.width;
  pimage.height = (png_uint_32)image32.height;
  pimage.version = PNG_IMAGE_VERSION; 
  pimage.format = PNG_FORMAT_RGBA;

  int convert_to_8bit = 0;
  png_image_write_to_file(&pimage, filename, convert_to_8bit, image32.pixels, 0, nullptr);

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
void slap_onto_image32(Image32 dest, Image32 *src, int x, int y) {
  for (int row = 0; (row < (int)src->height); ++row) {
    if ((row + x < (int)dest.height)) {
      for (int col = 0; (col < (int)src->width); ++col) {
        if((col + y < (int)dest.width)) {
          dest.pixels[(row + y) * dest.width + col + x] = src->pixels[row * src->width + col];
        }
      }
    }
  }
}

void fill_image32_with_color(Image32 image, Pixels32 color)
{
  int n = image.height * image.width;
  for(int i = 0; i < n ;++i) {
    image.pixels[i] = color;
  }
}

// * Slap FreeType bitmap onto Image32
void slap_onto_image32(Image32 dest, FT_Bitmap *src, Pixels32 color, int x, int y) {
  assert(src->pixel_mode == FT_PIXEL_MODE_GRAY);
  assert(src->num_grays == 256);

  for (int row = 0; (row < (int)src->rows); ++row) {
    if((row + x < (int)dest.height)) {
      for (int col = 0; (col < (int)src->width); ++col) {
        if ((col + y < (int)dest.width)) {
          int index = (row + y) * dest.width + col + x;
          float a = src->buffer[row * src->pitch + col] / 255.0f;

          dest.pixels[index].r = (uint8_t)(color.r * a + (1.0f - a) * dest.pixels[index].r);
          dest.pixels[index].g = (uint8_t)(color.g * a + (1.0f - a) * dest.pixels[index].g);
          dest.pixels[index].b = (uint8_t)(color.b * a + (1.0f - a) * dest.pixels[index].b);
          dest.pixels[index].a = (uint8_t)(color.a * a + (1.0f - a) * dest.pixels[index].a);
        }
      }
    }
  }
}

// * Slap giflib single image frame onto Image32
void slap_onto_image32(Image32 dest, SavedImage *src, ColorMapObject *SColorMap, int x, int y) {
  assert(src);
  assert(SColorMap);
  assert(SColorMap->BitsPerPixel == 8);
  assert(!SColorMap->SortFlag);
  assert(src->ImageDesc.Top == 0);
  assert(src->ImageDesc.Left == 0);

  for (int row = 0; (row < (int)src->ImageDesc.Height); ++row) {
    if ((row + x < (int)dest.height)) {
      for (int col = 0; (col < (int)src->ImageDesc.Width); ++col) {
        if ((col + y < (int)dest.width)) {
          GifColorType pixel = SColorMap->Colors[src->RasterBits[row * src->ImageDesc.Width + col]];
          int dest_index = (row + y) * dest.width + col + x;
          dest.pixels[dest_index].r = pixel.Red;
          dest.pixels[dest_index].g = pixel.Green;
          dest.pixels[dest_index].b = pixel.Blue;
        }
      }
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

  // free(buffer);
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

    slap_onto_image32(surface,
                      &face->glyph->bitmap,
                      color,
                      pen_x + slot->bitmap_left,
                      pen_y - slot->bitmap_top);

    //* increment pen position
    pen_x += slot->advance.x >> 6;

  }
}

// * ###################################################################
// * pthreads
// * ###################################################################

// *  0   1   2   3   4
// * [*] [*] [ ] [ ] [ ]
// *  ^ = queue_begin = 0
// *          ^ = queue_size = 2
// *
// * next_queue_frame == queue[2]

constexpr size_t VODUS_QUEUE_CAPACITY = 1024;
Image32 queue[VODUS_QUEUE_CAPACITY];
size_t queue_begin = 0;
size_t queue_size = 0;

Image32 frame_arena[VODUS_QUEUE_CAPACITY];
std::atomic_size_t frame_arena_size = VODUS_QUEUE_CAPACITY;

std::atomic<bool> stop_output_threads(false);
std::atomic<int> frame_count(0);

pthread_mutex_t queue_mutex;
pthread_t output_threads[VODUS_OUTPUT_THREADS_COUNT];

void init_frame_arena(void) {
  for(size_t i = 0; i < VODUS_QUEUE_CAPACITY; ++i) {
    frame_arena[i].height = VODUS_HEIGHT;
    frame_arena[i].width = VODUS_WIDTH;
    frame_arena[i].pixels = (Pixels32 *)malloc(sizeof(Pixels32) * VODUS_WIDTH * VODUS_HEIGHT);
  }
}

// * Returns the frame from the start of queue
Image32 alloc_frame(void) {
  return frame_arena[--frame_arena_size];
}

void dealloc_frame(Image32 frame) {
  frame_arena[frame_arena_size++] = frame;
}

bool enqueue(Image32 frame) {
  pthread_mutex_lock(&queue_mutex); 
  defer(pthread_mutex_unlock(&queue_mutex));

  if(queue_size >= VODUS_QUEUE_CAPACITY) {
    return false;
  }
  queue[(queue_begin + queue_size) % VODUS_QUEUE_CAPACITY] = frame;
  queue_size += 1;
  return true;
}

Image32 dequeue(std::atomic<int> *current_frame_count) {
  pthread_mutex_lock(&queue_mutex); 
  defer(pthread_mutex_unlock(&queue_mutex));

  if(queue_size == 0) {
    return {0, 0, nullptr};
  }
  Image32 result = queue[queue_begin];
  queue_size -= 1;
  queue_begin += 1;
  if(current_frame_count) *current_frame_count = frame_count++;
  return result;
}

void *output_thread_routine(void *) {
  constexpr size_t FILE_PATH_CAPA = 256;
  char file_path[FILE_PATH_CAPA];
  
  for (;;) {
    // * get the next avilable frame from queue
    Image32 frame = dequeue(&frame_count);
    if (frame.pixels == nullptr) {
      if(stop_output_threads.load()) {
        return nullptr;
      }
      continue;
    }
    
    // * build filepath
    snprintf(file_path, FILE_PATH_CAPA, "output/frame-%05d.png", frame_count.fetch_add(1, std::memory_order_relaxed));
    save_image32_as_png(frame, file_path);

    dealloc_frame(frame);
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

  float text_x = 0.0f;
  float text_y = VODUS_HEIGHT;

  constexpr float VODUS_DURATION = 10.0f;
  // constexpr float GIF_DURATION = 2.0f;
  // float gif_dt = GIF_DURATION / gif_file->ImageCount;
  // float t = 0.0f;

  Image32 image32_png = load_image32_from_png(png_filepath);

  // * Initialize queue_mutex
  pthread_mutex_init(&queue_mutex, nullptr);
  init_frame_arena();

  for(int i = 0; i < VODUS_OUTPUT_THREADS_COUNT; ++i) {
    pthread_create(&output_threads[i], nullptr, output_thread_routine, nullptr);
  }

  while (text_y > 0.0f) {
    // * Allocate the frame
    Image32 surface = alloc_frame();
    assert(surface.pixels);

    // * Clean up the surface
    fill_image32_with_color(surface, {50, 50, 50, 255});

    // * Slap the text onto image32
    Pixels32 color = {255, 0, 0, 255};
    slap_text_onto_image32(surface, face, text, color, (int)text_x, (int)text_y);

    // int gif_index = ((int)(t / gif_dt) % gif_file->ImageCount);
    // assert(gif_file->ImageCount > 0);
    // slap_onto_image32(surface,
    //                   &gif_file->SavedImages[gif_index],
    //                   gif_file->SColorMap,
    //                   (int)text_x, (int)text_y);

    slap_onto_image32(surface, &image32_png, (int)text_x, (int)text_y);

    while (!enqueue(surface)) {}

    // * get text_y position
    text_y -= (VODUS_HEIGHT / VODUS_DURATION) * VODUS_DELTA_TIME;

    // t += VODUS_DELTA_TIME;
  }
  
  stop_output_threads.store(true);
  printf("Finished rendering waiting for the output thread.\n");

  for (int i = 0; i < VODUS_OUTPUT_THREADS_COUNT; ++i) {
    pthread_join(output_threads[i], nullptr);
  }

  DGifCloseFile(gif_file, &error);
  
  return 0;
}

// * ffmpeg -framerate 100 -i 'frame-%04d.png' output.mp4
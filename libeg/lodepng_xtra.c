/*
 * Additional functions to support LodePNG for use in rEFInd
 *
 * copyright (c) 2013 by by Roderick W. Smith, and distributed
 * under the terms of the GNU GPL v3.
 *
 * See http://lodev.org/lodepng/ for the original LodePNG.
 *
 */

#include "global.h"
#include "../refind/screen.h"
#include "lodepng.h"

// EFI's equivalent of realloc requires the original buffer's size as an
// input parameter, which the standard libc realloc does not require. Thus,
// I've modified lodepng_malloc() to allocate more memory to store this data,
// and lodepng_realloc() can then read it when required. Because the size is
// stored at the start of the allocated area, these functions are NOT
// interchangeable with the standard EFI functions; memory allocated via
// lodepng_malloc() should be freed via lodepng_free(), and myfree() should
// NOT be used with memory allocated via AllocatePool() or AllocateZeroPool()!

void* lodepng_malloc(size_t size) {
   void *ptr;

   ptr = AllocateZeroPool(size + sizeof(size_t));
   if (ptr) {
      *(size_t *) ptr = size;
      return ((size_t *) ptr) + 1;
   } else {
      return NULL;
   }
} // void* lodepng_malloc()

void lodepng_free (void *ptr) {
   if (ptr) {
      ptr = (void *) (((size_t *) ptr) - 1);
      FreePool(ptr);
   }
} // void lodepng_free()

static size_t report_size(void *ptr) {
   if (ptr)
      return * (((size_t *) ptr) - 1);
   else
      return 0;
} // size_t report_size()

void* lodepng_realloc(void *ptr, size_t new_size) {
   size_t *new_pool;
   size_t old_size;

   new_pool = lodepng_malloc(new_size);
   if (new_pool && ptr) {
      old_size = report_size(ptr);
      CopyMem(new_pool, ptr, (old_size < new_size) ? old_size : new_size);
   }
   return new_pool;
} // lodepng_realloc()

// Finds length of ASCII string, which MUST be NULL-terminated.
int MyStrlen(const char *InString) {
   int Length = 0;

   if (InString) {
      while (InString[Length] != '\0')
         Length++;
   }
   return Length;
} // int MyStrlen()

typedef struct _lode_color {
   UINT8 red;
   UINT8 green;
   UINT8 blue;
   UINT8 alpha;
} lode_color;

EG_IMAGE * egDecodePNG(IN UINT8 *FileData, IN UINTN FileDataLength, IN UINTN IconSize, IN BOOLEAN WantAlpha) {
   EG_IMAGE *NewImage = NULL;
   unsigned Error, Width, Height;
   EG_PIXEL *PixelData;
   lode_color *LodeData;
   UINTN i;

   Error = lodepng_decode_memory((unsigned char **) &PixelData, &Width, &Height, (unsigned char*) FileData,
                                 (size_t) FileDataLength, LCT_RGBA, 8);

   if (Error) {
      return NULL;
   }

   // allocate image structure and buffer
   NewImage = egCreateImage(Width, Height, WantAlpha);
   if ((NewImage == NULL) || (NewImage->Width != Width) || (NewImage->Height != Height))
      return NULL;

   LodeData = (lode_color *) PixelData;

   // Annoyingly, EFI and LodePNG use different ordering of RGB values in
   // their pixel data representations, so we've got to adjust them....
   for (i = 0; i < (NewImage->Height * NewImage->Width); i++) {
      NewImage->PixelData[i].r = LodeData[i].red;
      NewImage->PixelData[i].g = LodeData[i].green;
      NewImage->PixelData[i].b = LodeData[i].blue;
      if (WantAlpha)
         NewImage->PixelData[i].a = LodeData[i].alpha;
   }
   lodepng_free(PixelData);

   return NewImage;
} // EG_IMAGE * egDecodePNG()
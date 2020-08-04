/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2020, Blender Foundation.
 */

#include "COM_Bmp.h"
#include "COM_Pixels.h"
#include "COM_Rect.h"

const int fileHeaderSize = 14;
const int infoHeaderSize = 40;

static unsigned char *createBitmapFileHeader(int height,
                                             int width,
                                             int bytesPerPixel,
                                             int paddingSize)
{
  int fileSize = fileHeaderSize + infoHeaderSize + (bytesPerPixel * width + paddingSize) * height;

  static unsigned char fileHeader[] = {
      0,
      0,  /// signature
      0,
      0,
      0,
      0,  /// image file size in bytes
      0,
      0,
      0,
      0,  /// reserved
      0,
      0,
      0,
      0,  /// start of pixel array
  };

  fileHeader[0] = (unsigned char)('B');
  fileHeader[1] = (unsigned char)('M');
  fileHeader[2] = (unsigned char)(fileSize);
  fileHeader[3] = (unsigned char)(fileSize >> 8);
  fileHeader[4] = (unsigned char)(fileSize >> 16);
  fileHeader[5] = (unsigned char)(fileSize >> 24);
  fileHeader[10] = (unsigned char)(fileHeaderSize + infoHeaderSize);

  return fileHeader;
}

static unsigned char *createBitmapInfoHeader(int height, int width, int bytesPerPixel)
{
  static unsigned char infoHeader[] = {
      0, 0, 0, 0,  /// header size
      0, 0, 0, 0,  /// image width
      0, 0, 0, 0,  /// image height
      0, 0,        /// number of color planes
      0, 0,        /// bits per pixel
      0, 0, 0, 0,  /// compression
      0, 0, 0, 0,  /// image size
      0, 0, 0, 0,  /// horizontal resolution
      0, 0, 0, 0,  /// vertical resolution
      0, 0, 0, 0,  /// colors in color table
      0, 0, 0, 0,  /// important color count
  };

  infoHeader[0] = (unsigned char)(infoHeaderSize);
  infoHeader[4] = (unsigned char)(width);
  infoHeader[5] = (unsigned char)(width >> 8);
  infoHeader[6] = (unsigned char)(width >> 16);
  infoHeader[7] = (unsigned char)(width >> 24);
  infoHeader[8] = (unsigned char)(height);
  infoHeader[9] = (unsigned char)(height >> 8);
  infoHeader[10] = (unsigned char)(height >> 16);
  infoHeader[11] = (unsigned char)(height >> 24);
  infoHeader[12] = (unsigned char)(1);
  infoHeader[14] = (unsigned char)(bytesPerPixel * 8);

  return infoHeader;
}

void COM_Bmp::generateBitmapImage(PixelsRect &rect, std::string filename)
{
  auto read = rect.pixelsImg();
  int width = rect.xmax - rect.xmin;
  int height = rect.ymax - rect.ymin;

  int buffer_size = read.elem_chs * width * height;
  unsigned char *img = (unsigned char *)malloc(buffer_size);

  /*Convert float buffer to unsigned char*/
  int img_offset = 0;
  float *read_cur = read.start;
  float *read_row_end = read.start + read.row_chs;
  while (read_cur < read.end) {
    while (read_cur < read_row_end) {
      for (int i = 0; i < read.elem_chs; i++) {
        int result = *read_cur * 255;
        if (result > 255) {
          result = 255;
        }
        if (result < 0) {
          result = 0;
        }
        BLI_assert(result >= 0 && result <= 255);
        img[img_offset] = (unsigned char)result;
        img_offset++;
        read_cur++;
      }
    }
    read_row_end += read.brow_chs;
    read_cur += read.row_jump;
  }

  generateBitmapImage(img, width, height, read.elem_chs, filename);
}

void COM_Bmp::generateBitmapImage(
    const unsigned char *img_buffer, int width, int height, int n_channels, std::string filename)
{
  unsigned char padding[3] = {0, 0, 0};
  int paddingSize = (4 - (width * n_channels) % 4) % 4;

  unsigned char *fileHeader = createBitmapFileHeader(height, width, paddingSize, n_channels);
  unsigned char *infoHeader = createBitmapInfoHeader(height, width, n_channels);

  FILE *imageFile = fopen((filename + ".bmp").c_str(), "wb");

  fwrite(fileHeader, 1, fileHeaderSize, imageFile);
  fwrite(infoHeader, 1, infoHeaderSize, imageFile);

  for (size_t i = 0; i < height; i++) {
    fwrite(img_buffer + (i * width * n_channels), n_channels, width, imageFile);
    fwrite(padding, 1, paddingSize, imageFile);
  }

  fclose(imageFile);
}

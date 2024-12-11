#ifndef TIMAGE_H
#define TIMAGE_H

struct tImage {
    const uint32_t* data;
    uint16_t width;
    uint16_t height;
    uint32_t size;
};

#endif // TIMAGE_H
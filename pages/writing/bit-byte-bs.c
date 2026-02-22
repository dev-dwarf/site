#include <stdint.h>
typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

u64 read_bits_le_lsb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  u8 n = 0;

  u8 ir = start & 0x7;
  if (ir) {
    n = 8 - ir;
    v = buf[i++] >> ir;
    if (len < n) {
      return v & ((1<<len)-1);
    }
  }

  while (n+8 < len) {
    v |= buf[i++] << n;
    n += 8;
  }

  u8 r = len - n;
  if (r) {
    v |= (buf[i++] & ((1<<r)-1)) << n;
  }

  return v;
}

u64 read_bits_le_msb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  u8 n = 0;

  u8 ir = start & 0x7;
  if (ir) {
    n = 8-ir;
    v = buf[i++] & ((1<<n)-1);
    if (len <= n) {
      return v >> (n-len); 
    }
  }

  while (n+8 <= len) {
    v |= buf[i++] << n;
    n += 8;
  }

  u8 r = len - n;
  if (r) {
    v |= (buf[i] >> (8-r)) << n;
  }

  return v;
}

u64 read_bits_be_lsb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  
  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8 - ir;
    v = buf[i++] >> ir;
    if (len > n) {
      len -= n;
      v = v << len;
    } else {
      return v & ((1<<len)-1);
    }
  }

  while (len >= 8) {
    len -= 8;
    v |= buf[i++] << len;
  }

  if (len) {
    v |= buf[i] & ((1<<len)-1);
  }

  return v;
}

u64 read_bits_be_msb0(u8 *buf, u32 start, u8 len) {
  u64 v = 0;
  u8 i = start >> 3;
  
  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8-ir;
    v = buf[i++] & ((1<<n)-1);
    if (len > n) {
      len -= n;
      v = v << len;
    } else {
      return v >> (n-len); 
    }
  }

  while (len >= 8) {
    len -= 8;
    v |= buf[i++] << len;
  }

  if (len) {
    v |= buf[i] >> (8-len);
  }

  return v;
}

void write_bits_le_lsb0(u8 *buf, u32 start, u8 len, u64 value) {
  u8 i = start >> 3;

  u8 ir = start & 0x7;
  if (ir) {
    u8 n = 8 - ir;
    if (n < len) {
      buf[i] = (buf[i] & ((1<<ir)-1)) | (((u8)value) << ir);
      i++;
      len -= n;
      value >>= n;
    } else {
      u8 mask = ((1<<len)-1) << ir;
      buf[i] = (buf[i] & ~mask) | ((value << ir) & mask);
      return;
    }
  }

  while (len >= 8) {
    buf[i++] = ((u8)value);
    len -= 8;
    value >>= 8;
  }

  if (len > 0) {
    u8 mask = ((1<<len)-1);
    buf[i] = (buf[i] & ~mask) | (value & mask);
  }
}

#include <stdio.h>
#include <string.h> // memcpy, memset
int main() {
    u8 buf[16];

    /* NOTE(lf) the code to generate the different bit patterns
      for these tests really sucks for most of the variants.
    */

    for (u32 i = 1; i <= 16; i++) { // BE MSB0
      for (u32 s = 0; s < 16; s++) {
        memset(buf, 0xFF, sizeof(buf));

        u32 V;
        V = (1<<i)-1; // full bit pattern
        // V = 1<<(i-1); // msb only
        // V = 0xAAAA >> (16-i); // every other bit, msb=1
        // V = 0xCCCC >> (16-i); // every 2 bits, msb=1
        // V = 0x3333 >> (16-i); // every 2 bits, msb=0
        // V = 0x5555 >> (16-i); // every other bit, msb=0

        u32 Vf = V | ((0xFFFF << i) & 0xFFFF);
        u32 B = 0;
        for (u32 b = 0; b < i; b++) {
          u32 n = 8*(3-(b>>3)) + (7-(b&7));
          B |= !!(Vf & (1<<(i-(b+1)))) << n;
        }
        B = B >> s;
        memcpy(buf, &B, sizeof(B));
        for (u32 b = 0; b < sizeof(B)/2; b++) {
          u8 t = buf[b];
          buf[b] = buf[sizeof(B)-1-b];
          buf[sizeof(B)-1-b] = t;
        }
        memcpy(buf+12, buf, sizeof(B));
        memset(buf, 0, sizeof(B));

        u32 S = s + 12*8;

        u32 y = read_bits_be_msb0(buf, S, i);
        if (y != V) {
          y = read_bits_be_msb0(buf, S, i);
        }
        if (y != V) {
          printf("(BE, MSB0) %08X : %X ? %X \n", B, V, y);
        }
      }
    }
    printf("\n");

    for (u32 i = 1; i <= 16; i++) { // BE LSB0
      for (u32 s = 0; s < 16; s++) {
        memset(buf, 0xFF, sizeof(buf));

        u32 V;
        // V = (1<<i)-1; // full bit pattern
        // V = 1<<(i-1); // msb only
        // V = 0xAAAA >> (16-i); // every other bit, msb=1
        // V = 0xCCCC >> (16-i); // every 2 bits, msb=1
        // V = 0x3333 >> (16-i); // every 2 bits, msb=0
        V = 0x5555 >> (16-i); // every other bit, msb=0

        u32 Vf = V | ((0xFFFF << i) & 0xFFFF);
        u32 B = 0;
        {
          u8 j = s >> 3;
          u8 ir = s & 0x7;
          u8 n = 0;
          if (ir) {
            n = 8-ir;
            if (i >= n) {
              buf[j++] = (Vf >> (i-n)) << ir;
            } else {
              buf[j++] = Vf << ir;
            }
          }
          while ((n+8u) < i) {
            n += 8;
            buf[j++] = (Vf >> (i-n)) & 0xFF;
          }
          if (n<i) {
            buf[j] = Vf & ((1<<(i-n))-1);
          }

        }

        for (u32 b = 0; b < sizeof(B)/2; b++) {
          u8 t = buf[b];
          buf[b] = buf[sizeof(B)-1-b];
          buf[sizeof(B)-1-b] = t;
        }
        memcpy(&B, buf, sizeof(B));
        for (u32 b = 0; b < sizeof(B)/2; b++) {
          u8 t = buf[b];
          buf[b] = buf[sizeof(B)-1-b];
          buf[sizeof(B)-1-b] = t;
        }
        memcpy(buf+12, buf, sizeof(B));
        memset(buf, 0, sizeof(B));

        u32 S = s + 12*8;
        u32 y = read_bits_be_lsb0(buf, S, i);
        if (y != V) {
          y = read_bits_be_lsb0(buf, S, i);
        }
        if (y != V) {
          printf("%X, %X ", i, s);
          printf("(BE, LSB0) %08X : %X ? %X \n", B, V, y);
        }
      }
    }
    printf("\n");

    for (u32 i = 1; i <= 16; i++) { // LE LSB0
      for (u32 s = 0; s < 16; s++) {
        memset(buf, 0xFF, sizeof(buf));

        u32 V;
        V = (1<<i)-1; // full bit pattern
        V = 1<<(i-1); // msb only
        V = 0xAAAA >> (16-i); // every other bit, msb=1
        V = 0xCCCC >> (16-i); // every 2 bits, msb=1
        // V = 0x3333 >> (16-i); // every 2 bits, msb=0
        // V = 0x5555 >> (16-i); // every other bit, msb=0

        u32 Vf = V | ((0xFFFF << i) & 0xFFFF);
        u32 B = Vf << s;
        u32 S = s + 12*8;

        // memcpy(buf+12, &B, sizeof(B));
        write_bits_le_lsb0(buf, S, i, Vf);

        u32 y = read_bits_le_lsb0(buf, S, i);
        if (y != V) {
          y = read_bits_le_lsb0(buf, S, i);
        }
        if (y != V) {
          printf("%X, %X ", i, s);
          printf("(LE, LSB0) %08X : %X ? %X \n", B, V, y);
        }
      }
    }
    printf("\n");

    for (u32 i = 1; i <= 16; i++) { // LE MSB0
      for (u32 s = 0; s < 16; s++) {
        memset(buf, 0xFF, sizeof(buf));

        u32 V;
        V = (1<<i)-1; // full bit pattern
        // V = 1<<(i-1); // msb only
        // V = 0xAAAA >> (16-i); // every other bit, msb=1
        // V = 0xCCCC >> (16-i); // every 2 bits, msb=1
        // V = 0x3333 >> (16-i); // every 2 bits, msb=0
        // V = 0x5555 >> (16-i); // every other bit, msb=0

        u32 Vf = V | ((0xFFFF << i) & 0xFFFF);
        u32 B;
        {
          u8 j = s>>3;
          u8 ir = s&7;
          u8 n = 0;

          if (ir) {
            n = 8-ir;
            if (n < i) {
              buf[j++] = (Vf & ((1<<n)-1));
            } else {
              buf[j++] = (Vf & ((1<<i)-1)) << (n-i);
            }
          }

          while (n+8u < i) {
            buf[j++] = (Vf >> n);
            n += 8;
          }

          if (n < i) {
            buf[j] = (Vf >> n) << (8-(i-n));
          }
        }
        
        memcpy(&B, buf, sizeof(B));
        memcpy(buf+12, buf, sizeof(B));
        memset(buf, 0, sizeof(B));

        u32 S = s + 12*8;
        u32 y = read_bits_le_msb0(buf, S, i);
        if (y != V) {
          y = read_bits_le_msb0(buf, S, i);
        }
        if (y != V) {
          printf("%X, %X ", i, s);
          printf("(LE, MSB0) %08X : %X ? %X \n", B, V, y);
        }
      }
    }
    printf("\n");

}
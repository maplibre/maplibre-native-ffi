// The frames that rendering cases keep, as PNGs on stdout, which every device
// runner returns the way it returns the results.
//
// The harness links only the library and tests/graphics, so it encodes PNGs
// itself, small rather than fast: each row predicts from the row above (PNG
// filter 2), and one deflate block with the fixed Huffman codes holds LZ77
// matches from a hash of the next three bytes.

#include <array>
#include <cstdio>
#include <map>
#include <string>

#include "bench.hpp"

namespace bench {
namespace {

class BitWriter {
 public:
  explicit BitWriter(std::string& out) : out_(out) {}

  // Writes `count` bits of `value`, least significant first.
  void bits(uint32_t value, uint32_t count) {
    buffer_ |= static_cast<uint64_t>(value) << filled_;
    filled_ += count;
    while (filled_ >= 8) {
      out_.push_back(static_cast<char>(buffer_ & 0xFFU));
      buffer_ >>= 8U;
      filled_ -= 8;
    }
  }

  // Writes a Huffman code, which deflate stores most significant bit first.
  void code(uint32_t code, uint32_t length) {
    uint32_t reversed = 0;
    for (uint32_t bit = 0; bit < length; ++bit) {
      reversed |= ((code >> bit) & 1U) << (length - 1 - bit);
    }
    bits(reversed, length);
  }

  void flush() {
    if (filled_ > 0) out_.push_back(static_cast<char>(buffer_ & 0xFFU));
    buffer_ = 0;
    filled_ = 0;
  }

 private:
  std::string& out_;
  uint64_t buffer_ = 0;
  uint32_t filled_ = 0;
};

void literal(BitWriter& writer, uint32_t symbol) {
  if (symbol < 144) {
    writer.code(0x30 + symbol, 8);
  } else if (symbol < 256) {
    writer.code(0x190 + symbol - 144, 9);
  } else if (symbol < 280) {
    writer.code(symbol - 256, 7);
  } else {
    writer.code(0xC0 + symbol - 280, 8);
  }
}

constexpr std::array<uint16_t, 29> kLengthBase{
  3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
  31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
};
constexpr std::array<uint8_t, 29> kLengthExtra{0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                               1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                               4, 4, 4, 4, 5, 5, 5, 5, 0};
constexpr std::array<uint16_t, 30> kDistanceBase{
  1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
  33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
  1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
constexpr std::array<uint8_t, 30> kDistanceExtra{0,  0,  0,  0,  1,  1, 2,  2,
                                                 3,  3,  4,  4,  5,  5, 6,  6,
                                                 7,  7,  8,  8,  9,  9, 10, 10,
                                                 11, 11, 12, 12, 13, 13};

void match(BitWriter& writer, uint32_t length, uint32_t distance) {
  size_t code = kLengthBase.size() - 1;
  while (kLengthBase[code] > length) --code;
  literal(writer, 257 + static_cast<uint32_t>(code));
  writer.bits(length - kLengthBase[code], kLengthExtra[code]);
  size_t dcode = kDistanceBase.size() - 1;
  while (kDistanceBase[dcode] > distance) --dcode;
  writer.code(static_cast<uint32_t>(dcode), 5);
  writer.bits(distance - kDistanceBase[dcode], kDistanceExtra[dcode]);
}

auto deflate(const std::string& data) -> std::string {
  constexpr uint32_t kWindow = 32768;
  constexpr uint32_t kMaxMatch = 258;
  constexpr uint32_t kHashBits = 16;
  std::string out;
  out.reserve(data.size() / 4);
  BitWriter writer{out};
  writer.bits(1, 1);  // The final block.
  writer.bits(1, 2);  // Fixed Huffman codes.
  std::vector<int64_t> head(1U << kHashBits, -1);
  const auto* bytes = reinterpret_cast<const uint8_t*>(data.data());
  const auto size = data.size();
  const auto hash = [&](size_t at) {
    const uint32_t value =
      bytes[at] | (bytes[at + 1] << 8U) | (bytes[at + 2] << 16U);
    return (value * 2654435761U) >> (32 - kHashBits);
  };
  size_t at = 0;
  while (at < size) {
    uint32_t best = 0;
    size_t best_from = 0;
    if (at + 3 <= size) {
      const auto slot = hash(at);
      const auto candidate = head[slot];
      head[slot] = static_cast<int64_t>(at);
      if (candidate >= 0 && at - static_cast<size_t>(candidate) <= kWindow) {
        const auto from = static_cast<size_t>(candidate);
        const auto limit = std::min<size_t>(kMaxMatch, size - at);
        uint32_t length = 0;
        while (length < limit && bytes[from + length] == bytes[at + length])
          ++length;
        if (length >= 3) {
          best = length;
          best_from = from;
        }
      }
    }
    if (best >= 3) {
      match(writer, best, static_cast<uint32_t>(at - best_from));
      // Index the matched bytes too, so the next match can start inside them.
      for (size_t next = at + 1; next < at + best && next + 3 <= size; ++next) {
        head[hash(next)] = static_cast<int64_t>(next);
      }
      at += best;
    } else {
      literal(writer, bytes[at]);
      ++at;
    }
  }
  literal(writer, 256);
  writer.flush();
  return out;
}

auto crc32(const std::string& data, size_t from) -> uint32_t {
  static const auto table = [] {
    std::array<uint32_t, 256> values{};
    for (uint32_t index = 0; index < 256; ++index) {
      uint32_t value = index;
      for (int bit = 0; bit < 8; ++bit) {
        value = (value & 1U) != 0 ? 0xEDB88320U ^ (value >> 1U) : value >> 1U;
      }
      values[index] = value;
    }
    return values;
  }();
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t index = from; index < data.size(); ++index) {
    crc =
      table[(crc ^ static_cast<uint8_t>(data[index])) & 0xFFU] ^ (crc >> 8U);
  }
  return crc ^ 0xFFFFFFFFU;
}

void big_endian(std::string& out, uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    out.push_back(
      static_cast<char>((value >> static_cast<uint32_t>(shift)) & 0xFFU)
    );
  }
}

void chunk(std::string& out, const char* type, const std::string& body) {
  big_endian(out, static_cast<uint32_t>(body.size()));
  const auto start = out.size();
  out.append(type, 4);
  out += body;
  big_endian(out, crc32(out, start));
}

auto images() -> std::map<std::string, std::string>& {
  static std::map<std::string, std::string> value;
  return value;
}

auto encode_png(const Image& image) -> std::string {
  const size_t stride = static_cast<size_t>(image.width) * 4;
  std::string filtered;
  filtered.reserve((stride + 1) * image.height);
  for (uint32_t row = 0; row < image.height; ++row) {
    filtered.push_back(2);  // Up: each byte less the byte above it.
    const auto* line = image.rgba.data() + row * stride;
    for (size_t index = 0; index < stride; ++index) {
      const uint8_t above = row == 0 ? 0 : line[index - stride];
      filtered.push_back(
        static_cast<char>(static_cast<uint8_t>(line[index] - above))
      );
    }
  }
  uint32_t a = 1;
  uint32_t b = 0;
  for (const char byte : filtered) {
    a = (a + static_cast<uint8_t>(byte)) % 65521U;
    b = (b + a) % 65521U;
  }
  std::string zlib{"\x78\x01", 2};
  zlib += deflate(filtered);
  big_endian(zlib, (b << 16U) | a);

  std::string png{"\x89PNG\r\n\x1a\n", 8};
  std::string header;
  big_endian(header, image.width);
  big_endian(header, image.height);
  header += std::string{"\x08\x06\x00\x00\x00", 5};  // 8-bit RGBA.
  chunk(png, "IHDR", header);
  chunk(png, "IDAT", zlib);
  chunk(png, "IEND", {});
  return png;
}

}  // namespace

void keep_image(const std::string& name, const Image& image) {
  if (
    image.width == 0 ||
    image.rgba.size() < static_cast<size_t>(image.width) * image.height * 4
  ) {
    fail("the frame's readback is empty");
  }
  images()[name] = encode_png(image);
}

// Each image is a line that names it, its PNG in base64 lines of 76
// characters, and an end line.
void print_images() {
  static const char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  for (const auto& [name, png] : images()) {
    std::printf("MLN_FFI_BENCH_IMAGE_BEGIN %s\n", name.c_str());
    std::string line;
    for (size_t at = 0; at < png.size(); at += 3) {
      const uint32_t chunk_value =
        (static_cast<uint8_t>(png[at]) << 16U) |
        (at + 1 < png.size() ? static_cast<uint8_t>(png[at + 1]) << 8U : 0U) |
        (at + 2 < png.size() ? static_cast<uint8_t>(png[at + 2]) : 0U);
      line += alphabet[(chunk_value >> 18U) & 0x3FU];
      line += alphabet[(chunk_value >> 12U) & 0x3FU];
      line += at + 1 < png.size() ? alphabet[(chunk_value >> 6U) & 0x3FU] : '=';
      line += at + 2 < png.size() ? alphabet[chunk_value & 0x3FU] : '=';
      if (line.size() >= 76) {
        std::printf("%s\n", line.c_str());
        line.clear();
      }
    }
    if (!line.empty()) std::printf("%s\n", line.c_str());
    std::printf("MLN_FFI_BENCH_IMAGE_END\n");
  }
  std::fflush(stdout);
}

}  // namespace bench

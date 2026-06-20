#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include <mbgl/util/image.hpp>

extern "C" {

struct MlnRustDecodedImage {
  uint32_t width;
  uint32_t height;
  uint8_t* data;
  std::size_t data_len;
  char* error;
};

auto mlnffi_rust_decode_image(const uint8_t* data, std::size_t dataLen)
  -> MlnRustDecodedImage;
void mlnffi_rust_decoded_image_free(MlnRustDecodedImage image);

}  // extern "C"

namespace {

class RustDecodedImage {
 public:
  explicit RustDecodedImage(MlnRustDecodedImage image_) : image(image_) {}
  RustDecodedImage(const RustDecodedImage&) = delete;
  auto operator=(const RustDecodedImage&) -> RustDecodedImage& = delete;
  RustDecodedImage(RustDecodedImage&&) = delete;
  auto operator=(RustDecodedImage&&) -> RustDecodedImage& = delete;

  ~RustDecodedImage() { mlnffi_rust_decoded_image_free(image); }

  [[nodiscard]] auto get() const -> const MlnRustDecodedImage& { return image; }

 private:
  MlnRustDecodedImage image;
};

}  // namespace

namespace mbgl {

auto decodeImage(const std::string& encoded) -> PremultipliedImage {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  const auto* data = reinterpret_cast<const uint8_t*>(encoded.data());
  auto decoded = RustDecodedImage{
    mlnffi_rust_decode_image(data, encoded.size()),
  };

  const auto& rustImage = decoded.get();
  if (rustImage.error != nullptr) {
    throw std::runtime_error(rustImage.error);
  }

  if (rustImage.data == nullptr) {
    throw std::runtime_error("Rust image decoder returned no pixel data");
  }

  auto image = PremultipliedImage{{rustImage.width, rustImage.height}};
  if (rustImage.data_len != image.bytes()) {
    throw std::runtime_error(
      "Rust image decoder returned mismatched pixel data"
    );
  }

  std::copy_n(rustImage.data, rustImage.data_len, image.data.get());
  return image;
}

}  // namespace mbgl

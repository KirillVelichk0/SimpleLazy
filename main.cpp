#include <array>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

template <bool AgressiveCopyMode, class T, class... Args>
auto GenerateFactory(Args &&...args) {
  if constexpr (AgressiveCopyMode) {
    return [... args = std::forward<Args>(
                args)]() mutable -> std::unique_ptr<std::decay_t<T>> {
      return std::make_unique<std::decay_t<T>>(std::move<Args>(args)...);
    };
  } else {
    return [... args = (std::forward<Args>(
                args))]() mutable -> std::unique_ptr<std::decay_t<T>> {
      return std::make_unique<std::decay_t<T>>(std::forward<Args>(args)...);
    };
  }
}
template <class T> class SimpleLazy final {
private:
  std::function<std::unique_ptr<std::decay_t<T>>()> factory;
  std::shared_ptr<std::decay_t<T>> internal;
  bool cracked;
  void CreateInternal() {
    try {
      if (cracked) {
        throw std::runtime_error("Cant create object. There was some error "
                                 "while creating factory\n");
      }
      this->internal = this->factory();
    } catch (std::exception &e) {
      this->cracked = true;
      throw std::runtime_error(
          std::string("Cant create object using factory for lazy because:\n") +
          e.what());
    } catch (...) {
      this->cracked = true;
      throw;
    }
  }

public:
  template <bool AgressiveCopyMode, class... Args>
  static SimpleLazy Create(Args &&...args) {
    SimpleLazy result;
    result.cracked = false;
    result.factory = GenerateFactory<AgressiveCopyMode, T, Args...>(
        std::forward<Args>(args)...);
    return result;
  }
  template <class... Args> SimpleLazy(Args &&...args) : cracked(false) {
    try {
      this->factory =
          GenerateFactory<false, T, Args...>(std::forward<Args>(args)...);
    } catch (std::exception &e) {
      this->cracked = true;
      throw std::runtime_error(
          std::string("Cant create factory for lazy because:\n") + e.what());
    } catch (...) {
      this->cracked = true;
      throw;
    }
  }
  SimpleLazy(const SimpleLazy<T> &another) = default;
  SimpleLazy &operator=(const SimpleLazy<T> &another) = default;
  SimpleLazy(SimpleLazy<T> &&another) = default;
  SimpleLazy &operator=(SimpleLazy &&another) = default;
  [[nodiscard]] bool IsInited() const noexcept {
    return this->internal != nullptr && !this->cracked;
  }
  [[nodiscard]] bool IsCracked() const noexcept { return this->cracked; }
  T *operator->() {
    if (this->cracked) {
      throw std::runtime_error("Object dont created correctly\n");
    }
    if (this->factory != nullptr) {
      this->internal = this->factory();
      this->factory = nullptr;
    }
    return this->internal.get();
  }
};

int main(int argc, char *argv[]) {
  int x = 7;
  SimpleLazy<std::array<int, 7>> l1(std::array<int, 7>{1, 2, 6, 1, 3, 2, 1});
  std::cout << l1->at(0) << "\n";
  return 0;
}

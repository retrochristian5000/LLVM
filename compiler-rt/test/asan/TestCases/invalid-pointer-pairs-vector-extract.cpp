// RUN: %clangxx_asan -O2 %s -o %t -mllvm -asan-detect-invalid-pointer-pair

// RUN: %env_asan_opts=detect_invalid_pointer_pairs=1:halt_on_error=0 %run %t

int main() {
  const char *begins[2] = {"abc", "def"};
  const char *ends[2] = {"abcde", "defgh"};
  long lengths[2]{0};
  // CHECK: ERROR: AddressSanitizer: invalid-pointer-pair
  // CHECK: #{{[0-9]+ .*}} in main {{.*}}invalid-pointer-pairs-vector-extract.cpp:[[@LINE+1]]
  lengths[0] = ends[0] - begins[0];
  // CHECK: ERROR: AddressSanitizer: invalid-pointer-pair
  // CHECK: #{{[0-9]+ .*}} in main {{.*}}invalid-pointer-pairs-vector-extract.cpp:[[@LINE+1]]
  lengths[1] = ends[1] - begins[1];
  return 0;
}

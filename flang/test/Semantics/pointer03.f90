! RUN: %python %S/test_errors.py %s %flang_fc1
module m
  integer :: t
  parameter(i=1)
  !ERROR: 'i' was previously initialized
  integer, pointer :: i => t
end module

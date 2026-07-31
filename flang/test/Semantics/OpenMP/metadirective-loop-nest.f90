!RUN: %python %S/../test_errors.py %s %flang -fopenmp -fopenmp-version=51

subroutine collapse_too_deep(n, a)
  integer :: n, a(n, n), i, j
  !ERROR: This construct requires a nest of depth 3, but the associated nest is a nest of depth 2
  !BECAUSE: COLLAPSE clause was specified with argument 3
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(3)) default(nothing)
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine ordered_too_deep(n, a)
  integer :: n, a(n, n), i, j
  !ERROR: This construct requires a perfect nest of depth 3, but the associated nest is a perfect nest of depth 2
  !BECAUSE: ORDERED clause was specified with argument 3
  !$omp metadirective when(implementation={vendor(llvm)}: do ordered(3)) default(nothing)
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine collapse_too_deep_exec(n, a)
  integer :: n, a(n, n), i, j
  a = 0
  !ERROR: This construct requires a nest of depth 3, but the associated nest is a nest of depth 2
  !BECAUSE: COLLAPSE clause was specified with argument 3
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(3)) default(nothing)
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine collapse_too_deep_compiler_directive(n, a)
  integer :: n, a(n, n), i, j
  a = 0
  !ERROR: This construct requires a nest of depth 3, but the associated nest is a nest of depth 2
  !BECAUSE: COLLAPSE clause was specified with argument 3
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(3)) default(nothing)
  !dir$ ivdep
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine noncanonical_do_while(n)
  integer :: n, i
  i = 0
  !ERROR: This construct requires a canonical loop nest
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !BECAUSE: DO WHILE loop is not a valid affected loop
  do while (i < n)
    i = i + 1
  end do
end subroutine

subroutine noncanonical_do_concurrent(n, a)
  integer :: n, a(n), i
  !ERROR: This construct requires a canonical loop nest
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !BECAUSE: DO CONCURRENT loop is not a valid affected loop
  do concurrent(i=1:n)
    a(i) = i
  end do
end subroutine

subroutine noncanonical_no_control(n)
  integer :: n, i
  i = 0
  !ERROR: This construct requires a canonical loop nest
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !BECAUSE: DO loop without loop control is not a valid affected loop
  do
    i = i + 1
    if (i >= n) exit
  end do
end subroutine

subroutine collapse_too_deep_interface(n, a)
  integer :: n, a(n, n), i, j
  !ERROR: This construct requires a nest of depth 3, but the associated nest is a nest of depth 2
  !BECAUSE: COLLAPSE clause was specified with argument 3
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(3)) default(nothing)
  interface
    subroutine ext()
    end subroutine
  end interface
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine tile_non_rectangular(n, a)
  integer :: n, a(n, n), i, j
  !ERROR: This construct requires a rectangular loop nest, but the associated nest is not
  !BECAUSE: None of the loops affected by TILE can be non-rectangular
  !$omp metadirective when(implementation={vendor(llvm)}: tile sizes(2, 2)) default(nothing)
  do i = 1, n
    !BECAUSE: The upper bound of the affected loop uses iteration variables of enclosing loops: 'i'
    do j = 1, i
      a(j, i) = i
    end do
  end do
end subroutine

subroutine collapse_valid(n, a)
  integer :: n, a(n, n), i, j
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(2)) default(nothing)
  do i = 1, n
    do j = 1, n
      a(j, i) = i
    end do
  end do
end subroutine

subroutine collapse_non_rectangular_valid(n, a)
  integer :: n, a(n, n), i, j
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(2)) default(nothing)
  do i = 1, n
    do j = 1, i
      a(j, i) = i
    end do
  end do
end subroutine

! A loop-associated variant with no loop nest to associate with is in error,
! whether the metadirective is the last construct in the execution part ...
subroutine no_loop_at_end()
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
end subroutine

! ... or is followed by a non-loop construct.
subroutine no_loop_before_stmt(a)
  integer :: a
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: parallel do) default(nothing)
  a = 0
end subroutine

! An OpenMP declarative directive also interrupts the loop association.
subroutine no_loop_before_declarative_directive(n, a)
  integer :: n, a(n), i
  integer, save :: x
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !$omp threadprivate(x)
  do i = 1, n
    a(i) = i
  end do
end subroutine

! A variant that cannot be selected on this target needs no loop nest.
subroutine no_loop_dead_variant()
  !$omp metadirective when(device={kind(nohost)}: do) default(nothing)
end subroutine

! A loop-associated variant in a declarative context (e.g. a module
! specification part) also has no loop nest to associate with.
module no_loop_in_module
  implicit none
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
end module

! Starting a contained procedure must not discard a pending variant from the
! enclosing module specification part.
module no_loop_in_module_with_contains
  implicit none
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
contains
  subroutine contained()
  end subroutine
end module

! A later specification-part metadirective must not discard an earlier one.
module no_loop_before_another_metadirective
  implicit none
  !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !$omp metadirective when(implementation={vendor(llvm)}: parallel) default(nothing)
end module

! A loop in the enclosing subprogram cannot satisfy a variant from an
! interface body, which has no execution part of its own.
subroutine no_loop_in_interface_body(n, a)
  integer :: n, a(n), i
  interface
    subroutine iface()
      !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
      !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
    end subroutine
  end interface
  do i = 1, n
    a(i) = i
  end do
end subroutine

! Checking an interface body must preserve variants pending in the enclosing
! subprogram for its execution part.
subroutine no_loop_in_interface_body_preserves_outer(n, a)
  integer :: n, a(n), i
  !ERROR: This construct requires a nest of depth 2, but the associated nest is a nest of depth 1
  !BECAUSE: COLLAPSE clause was specified with argument 2
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(2)) default(nothing)
  interface
    subroutine iface()
      !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
      !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
    end subroutine
  end interface
  do i = 1, n
    a(i) = i
  end do
end subroutine

! A loop after an empty BLOCK cannot satisfy a variant from the BLOCK's
! specification part.
subroutine no_loop_in_empty_block(n, a)
  integer :: n, a(n), i
  block
    !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
    !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  end block
  do i = 1, n
    a(i) = i
  end do
end subroutine

! A loop in the BLOCK itself remains its associated loop.
subroutine loop_in_block(n, a)
  integer :: n, a(n), i
  block
    !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
    do i = 1, n
      a(i) = i
    end do
  end block
end subroutine

! A loop in a mutually exclusive IF branch cannot satisfy the variant.
subroutine no_loop_across_if_branch(n, a, flag)
  integer :: n, a(n), i
  logical :: flag
  if (flag) then
    !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
    !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  else
    do i = 1, n
      a(i) = i
    end do
  end if
end subroutine

! A loop after a nested executable block cannot satisfy its variant.
subroutine no_loop_after_if(n, a, flag)
  integer :: n, a(n), i
  logical :: flag
  if (flag) then
    !ERROR: This construct should contain a DO-loop or a loop-nest-generating construct
    !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  end if
  do i = 1, n
    a(i) = i
  end do
end subroutine

! A loop in the same IF branch remains the associated loop.
subroutine loop_in_if_branch(n, a, flag)
  integer :: n, a(n), i
  logical :: flag
  if (flag) then
    !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
    do i = 1, n
      a(i) = i
    end do
  end if
end subroutine

subroutine noninteger_iteration_variable(n, a)
  integer :: n, a(n)
  real :: i
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !ERROR: The DO loop iteration variable must be of integer type
  do i = 1, n
    a(int(i)) = int(i)
  end do
end subroutine

subroutine noninteger_collapsed_iteration_variables(n, a)
  integer :: n, a(n, n)
  real :: i, j
  !$omp metadirective when(implementation={vendor(llvm)}: do collapse(2)) default(nothing)
  !ERROR: The DO loop iteration variable must be of integer type
  do i = 1, n
    !ERROR: The DO loop iteration variable must be of integer type
    do j = 1, n
      a(int(j), int(i)) = int(i)
    end do
  end do
end subroutine

subroutine threadprivate_iteration_variable(n, a)
  integer :: n, a(n)
  integer, save :: i
  !$omp threadprivate(i)
  !$omp metadirective when(implementation={vendor(llvm)}: do) default(nothing)
  !ERROR: Loop iteration variable of an affected loop cannot be THREADPRIVATE
  do i = 1, n
    a(i) = i
  end do
end subroutine

subroutine invalid_iteration_variable_dsa(n, a)
  integer :: n, a(n), i
  !ERROR: Loop iteration variable with a predetermined data sharing attribute cannot appear in a FIRSTPRIVATE clause
  !$omp metadirective when(implementation={vendor(llvm)}: do firstprivate(i)) default(nothing)
  !BECAUSE: 'i' is an iteration variable of an affected loop
  do i = 1, n
    a(i) = i
  end do
end subroutine

! An invalid iteration variable in a variant that cannot be selected on this
! target does not constrain the program.
subroutine dead_variant_noninteger_iteration_variable(n, a)
  integer :: n, a(n)
  real :: i
  !$omp metadirective when(device={kind(nohost)}: do) default(nothing)
  do i = 1, n
    a(int(i)) = int(i)
  end do
end subroutine

subroutine dead_variant_invalid_iteration_variable_dsa(n, a)
  integer :: n, a(n), i
  !$omp metadirective when(device={kind(nohost)}: do firstprivate(i)) default(nothing)
  do i = 1, n
    a(i) = i
  end do
end subroutine

subroutine unreachable_ranked_collapse(n, a)
  integer :: n, a(n), i
  !$omp metadirective &
  !$omp& when(user={condition(score(2): .true.)}: do) &
  !$omp& when(user={condition(score(1): .true.)}: do collapse(2)) &
  !$omp& default(nothing)
  do i = 1, n
    a(i) = i
  end do
end subroutine

subroutine unreachable_ranked_iteration_variable(n, a)
  integer :: n, a(n)
  real :: i
  !$omp metadirective &
  !$omp& when(user={condition(score(2): .true.)}: nothing) &
  !$omp& when(user={condition(score(1): .true.)}: do) &
  !$omp& default(nothing)
  do i = 1, n
    a(int(i)) = int(i)
  end do
end subroutine

subroutine unreachable_ranked_interrupted_association(n, a)
  integer :: n, a(n), i
  integer, save :: x
  !$omp metadirective &
  !$omp& when(user={condition(score(2): .true.)}: nothing) &
  !$omp& when(user={condition(score(1): .true.)}: do) &
  !$omp& default(nothing)
  !$omp threadprivate(x)
  do i = 1, n
    a(i) = i
  end do
end subroutine

! Repeating the same runtime guard does not make a lower-ranked candidate
! reachable. If FLAG is true the higher-ranked candidate wins; if it is false
! neither candidate matches.
subroutine unreachable_same_runtime_condition_collapse(n, a, flag)
  integer :: n, a(n), i
  logical :: flag
  !$omp metadirective &
  !$omp& when(user={condition(score(2): flag)}: do) &
  !$omp& when(user={condition(score(1): flag)}: do collapse(2)) &
  !$omp& default(nothing)
  do i = 1, n
    a(i) = i
  end do
end subroutine

subroutine unreachable_same_runtime_condition_iteration_variable(n, a, flag)
  integer :: n, a(n)
  logical :: flag
  real :: i
  !$omp metadirective &
  !$omp& when(user={condition(score(2): flag)}: nothing) &
  !$omp& when(user={condition(score(1): flag)}: do) &
  !$omp& default(nothing)
  do i = 1, n
    a(int(i)) = int(i)
  end do
end subroutine

subroutine unreachable_same_runtime_condition_interruption(n, a, flag)
  integer :: n, a(n), i
  integer, save :: x
  logical :: flag
  !$omp metadirective &
  !$omp& when(user={condition(score(2): flag)}: nothing) &
  !$omp& when(user={condition(score(1): flag)}: do) &
  !$omp& default(nothing)
  !$omp threadprivate(x)
  do i = 1, n
    a(i) = i
  end do
end subroutine

! A statically selected enclosing replacement contributes its construct traits
! to nested selection. The inner DO is reachable because PARALLEL is known to
! be present.
subroutine construct_from_enclosing_metadirective(n, a)
  integer :: n, a(n)
  real :: i
  !$omp begin metadirective &
  !$omp& when(implementation={vendor(llvm)}: parallel) &
  !$omp& default(nothing)
  !$omp metadirective &
  !$omp& when(construct={parallel}: do) &
  !$omp& default(nothing)
  !ERROR: The DO loop iteration variable must be of integer type
  do i = 1, n
    a(int(i)) = int(i)
  end do
  !$omp end metadirective
end subroutine

! CRITICAL is statically selected and does not provide a PARALLEL construct
! trait. The inner DO is therefore unreachable and must not constrain the
! THREADPRIVATE iteration variable.
subroutine impossible_construct_from_enclosing_metadirective(n, a)
  integer :: n, a(n)
  integer, save :: i
  !$omp threadprivate(i)
  !$omp begin metadirective &
  !$omp& when(implementation={vendor(llvm)}: critical) &
  !$omp& default(nothing)
  !$omp metadirective &
  !$omp& when(construct={parallel}: do) &
  !$omp& default(nothing)
  do i = 1, n
    a(i) = i
  end do
  !$omp end metadirective
end subroutine

! A runtime condition leaves the enclosing PARALLEL/NOTHING choice unresolved.
! Keep the inner DO conservatively because it is reachable on the PARALLEL
! path.
subroutine unresolved_construct_from_enclosing_metadirective(n, a, flag)
  integer :: n, a(n)
  integer, save :: i
  logical :: flag
  !$omp threadprivate(i)
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: parallel) &
  !$omp& default(nothing)
  !$omp metadirective &
  !$omp& when(construct={parallel}: do) &
  !$omp& default(nothing)
  !ERROR: Loop iteration variable of an affected loop cannot be THREADPRIVATE
  do i = 1, n
    a(i) = i
  end do
  !$omp end metadirective
end subroutine

! Selected traits occur at the enclosing metadirective's position, before the
! source PARALLEL trait. TARGET,PARALLEL therefore selects SIMD and diagnoses
! the noncanonical loop.
subroutine ordered_enclosing_metadirective_traits(n)
  integer :: n, i
  i = 0
  !$omp begin metadirective &
  !$omp& when(implementation={vendor(llvm)}: target) &
  !$omp& default(nothing)
  !$omp parallel
  !$omp metadirective &
  !ERROR: This construct requires a canonical loop nest
  !$omp& when(construct={target, parallel}: simd) &
  !$omp& default(nothing)
  !BECAUSE: DO WHILE loop is not a valid affected loop
  do while (i < n)
    i = i + 1
  end do
  !$omp end parallel
  !$omp end metadirective
end subroutine

! Reversing those traits does not match the effective TARGET,PARALLEL nesting,
! so the same noncanonical loop is unaffected by SIMD.
subroutine reversed_enclosing_metadirective_traits(n)
  integer :: n, i
  i = 0
  !$omp begin metadirective &
  !$omp& when(implementation={vendor(llvm)}: target) &
  !$omp& default(nothing)
  !$omp parallel
  !$omp metadirective &
  !$omp& when(construct={parallel, target}: simd) &
  !$omp& default(nothing)
  do while (i < n)
    i = i + 1
  end do
  !$omp end parallel
  !$omp end metadirective
end subroutine

! The high-scored constant-true NOTHING shadows the inner DO in every possible
! outer context. It must be pruned even though the enclosing PARALLEL/NOTHING
! selection is dynamic.
subroutine shadowed_construct_in_unresolved_metadirective(n, a, flag)
  integer :: n, a(n)
  integer, save :: i
  logical :: flag
  !$omp threadprivate(i)
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: parallel) &
  !$omp& default(nothing)
  !$omp metadirective &
  !$omp& when(user={condition(score(100): .true.)}: nothing) &
  !$omp& when(construct={parallel}: do) &
  !$omp& default(nothing)
  do i = 1, n
    a(i) = i
  end do
  !$omp end metadirective
end subroutine

! The same FLAG controls both enclosing selections. PARALLEL requires FLAG and
! TARGET requires .NOT.FLAG, so the combined construct context is impossible.
subroutine correlated_nested_metadirective_paths(flag, n)
  logical, intent(in) :: flag
  integer :: n, i
  i = 0
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: parallel) &
  !$omp& default(nothing)
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: nothing) &
  !$omp& default(target)
  !$omp metadirective &
  !$omp& when(construct={parallel, target}: simd) &
  !$omp& default(nothing)
  do while (i < n)
    i = i + 1
  end do
  !$omp end metadirective
  !$omp end metadirective
end subroutine

! Independent flags can select PARALLEL and TARGET together, so the inner SIMD
! remains reachable and constrains the associated loop.
subroutine independent_nested_metadirective_paths(flag1, flag2, n)
  logical, intent(in) :: flag1, flag2
  integer :: n, i
  i = 0
  !$omp begin metadirective &
  !$omp& when(user={condition(flag1)}: parallel) &
  !$omp& default(nothing)
  !$omp begin metadirective &
  !$omp& when(user={condition(flag2)}: nothing) &
  !$omp& default(target)
  !$omp metadirective &
  !ERROR: This construct requires a canonical loop nest
  !$omp& when(construct={parallel, target}: simd) &
  !$omp& default(nothing)
  !BECAUSE: DO WHILE loop is not a valid affected loop
  do while (i < n)
    i = i + 1
  end do
  !$omp end metadirective
  !$omp end metadirective
end subroutine

! A mutable condition can change between nested selection points. Do not
! correlate equal expressions in that case: PARALLEL and TARGET are reachable
! together when FLAG is initially true and then assigned false.
subroutine mutable_nested_metadirective_paths(flag, n)
  logical :: flag
  integer :: n, i
  i = 0
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: parallel) &
  !$omp& default(nothing)
  flag = .false.
  !$omp begin metadirective &
  !$omp& when(user={condition(flag)}: nothing) &
  !$omp& default(target)
  !$omp metadirective &
  !ERROR: This construct requires a canonical loop nest
  !$omp& when(construct={parallel, target}: simd) &
  !$omp& default(nothing)
  !BECAUSE: DO WHILE loop is not a valid affected loop
  do while (i < n)
    i = i + 1
  end do
  !$omp end metadirective
  !$omp end metadirective
end subroutine

subroutine dynamic_ranked_collapse(n, a, flag)
  integer :: n, a(n), i
  logical :: flag
  !$omp metadirective &
  !$omp& when(user={condition(score(2): flag)}: nothing) &
  !ERROR: This construct requires a nest of depth 2, but the associated nest is a nest of depth 1
  !BECAUSE: COLLAPSE clause was specified with argument 2
  !$omp& when(user={condition(score(1): .true.)}: do collapse(2)) &
  !$omp& default(nothing)
  do i = 1, n
    a(i) = i
  end do
end subroutine

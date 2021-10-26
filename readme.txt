MCALC: A Matrix Calculator

Usage:
  mcalc ["expression"/filename]

Syntax:
  [a2, a1, a0]             is for polynomials: (a2 x^2 + a1 x + a0)
  [[a1, a2, a3]]           is a row vector
  [[a11, a12;  a21, a22]]  is a 2x2 matrix

  A B  is multiplication, except:
       when A is a polynomial or a fraction object and B is a square matrix
       it is substitution of B into A: A(B)

  Expressions end with semicolon ;
  Elements MUST be separated by commas ,
  Matrix rows are separated by semicolons ;
  Expressions ending with double semicolon ;; don't produce output
  Nested matrices and/or polynomials are not supported

Operators by precedence (first to last):
  ^
  + - (unary)
  '
  / \ % * o - .- + .+
  = += -= *= /= \= %=
  ,
  ( )
  [ ] ; [[ ]]
  ;;
Notes:
  '     is transpose (eg: M')
  o     is the tensor product
  \     is for square matrices (eg: A\B)
  .+ .- are element-wise operations

Keywords:
  help: Print this info (works in cmd)
  clear: Clears all variables from memory
  vars: Shows all variables & answer count
General functions:
  ans(n): The n-th latest answer
Vectors:
  cross(3D vec, 3D vec) -> 3D vec
  cross(2D vec, 2D vec) -> scalar
Matrices:
  ref: Row Echelon Form
  rref: Reduced Row Echelon Form (Gaussian Elimination)
Square matrices:
  I n: n-square identity matrix
  det: determinant of square matrix
  inv: inverse of square matrix
  diag: diagonal factorization of square matrix
  lu: LU factorization of square matrix
  tr: trace of square matrix
Fraction objects:
  sample(F): s to z domain
  ldiv(F, S): long division of fraction F by S steps
Utility:
  cmd(w, h): set console buffer size (in characters)

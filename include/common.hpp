#pragma once

#include <Eigen/Dense>
#include <gtsam/geometry/Rot3.h>

template<int R, int C>
using Mat = Eigen::Matrix<double, R, C>;

template<int R>
using Vect = Eigen::Matrix<double, R, 1>;

enum class Axis{x=0,y=1,z=2};

template<Axis a1, Axis a2>
constexpr Axis missing()
{
  return Axis{3 - static_cast<std::underlying_type_t<Axis>>(a1) -
    static_cast<std::underlying_type_t<Axis>>(a2)};
}

template<Axis a1, Axis a2, Axis a3>
constexpr bool isTaitBryan()
{
  return 
    (static_cast<std::underlying_type_t<Axis>>(a1) +
    static_cast<std::underlying_type_t<Axis>>(a2) +
    static_cast<std::underlying_type_t<Axis>>(a3)) == 3;
}

template<Axis a1, Axis a2, Axis a3>
constexpr bool isProperEuler()
{
  return (a1 == a3) and (a1 != a2);
}

template<Axis a1, Axis a2, Axis a3>
constexpr bool isMalformed()
{
  return (a1 == a2) or (a2 == a3);
}

inline Mat<3,3> skew(double wx, double wy, double wz) {
  return (Mat<3,3>() << 0.0, -wz, +wy, +wz, 0.0, -wx, -wy, +wx, 0.0).finished();
}

template <class Derived>
inline Mat<3,3> skew(const Eigen::MatrixBase<Derived>& w) {
  return skew(w(0), w(1), w(2));
}

template <class Derived>
inline Vect<3> vectorFromSkewSymmetric(const Eigen::MatrixBase<Derived>& M)
{
  return (Vect<3>() << - M(1,2), M(0,2), - M(0,1)).finished();
}

struct Elem 
{
  int r, c;
};

template<typename Mat>
class Wrap
{
public:
  Wrap(Mat const& _m):m{_m}{};
  auto operator()(Elem const& e) const
  {
    return sign(e) * uncorrected(e);
  }
  auto uncorrected(Elem const& e) const
  {
    return m(e.r, e.c);
  }
  auto sign(Elem const& e) const
  {
    return sign_mat(e.r, e.c);
  }

private:
  gtsam::Matrix3 sign_mat = 
    gtsam::Matrix3::Identity() 
    + skew(gtsam::Vector3::Ones());
  Mat const& m;
};

template<Axis>
struct Rot;

#define MakeRotMat(name) \
  template<> struct Rot<Axis::name> { \
    static gtsam::Matrix3 Mat(double a){ return gtsam::Rot3::R##name(a).matrix();}; \
  }

MakeRotMat(x);
MakeRotMat(y);
MakeRotMat(z);

template<Axis a1, Axis a2, Axis a3>
Mat<3,3> toR(Vect<3> const& v)
{
  return Rot<a1>::Mat(v[0]) * Rot<a2>::Mat(v[1]) * Rot<a3>::Mat(v[2]);
}

template<Axis axis>
Mat<3,3> make_generator()
{
  Mat<3,3> M = Mat<3,3>::Zero();
  if constexpr(axis == Axis::x){
    M(1,2) = -1;
    M(2,1) = 1;
  }
  else if constexpr(axis == Axis::y){
    M(0,2) = 1;
    M(2,0) = -1;
  }
  else {
    M(0,1) = -1;
    M(1,0) = 1;
  }
  return M;
}

template<Axis axis>
static const auto generator = make_generator<axis>();

#include <OptionalRef.hpp>
template<int N>
Vect<N> normalize(Vect<N> const& v, OptionalRef<Mat<N,N>> H = {})
{
  double n2 = v.dot(v);
  if( n2 < std::numeric_limits<double>::epsilon()){
    if(H)
      *H = Mat<N,N>::Constant(std::nan(""));
    return Vect<N>::Constant(std::nan(""));
  }
  double n = std::sqrt(n2);
  if(H){
    *H = - v * v.transpose() / ( n * n2) + Mat<N,N>::Identity() * 1.0 / n;
  }
  return v / n;
}

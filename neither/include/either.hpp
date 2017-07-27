#ifndef NEITHER_EITHER_HPP
#define NEITHER_EITHER_HPP

#include <neither/maybe.hpp>
#include <memory>
#include <type_traits>

namespace neither {

template<class T>
constexpr T max(T x, T y) {
  return x>y ? x : y;
}

template<class T>
struct Left {
  T value;
};

template<class T>
constexpr Left<T> left(T const& x) {
  return {x};
}

template<class T>
Left<T> left(T&& x) {
  return {std::move(x)};
}

template<class T>
struct Right {
  T value;
};

template<class T>
constexpr Right<T> right(T const& x) {
  return {x};
}


template<class T>
Right<T> right(T&& x) {
  return {std::move(x)};
}


template<class L, class R>
struct Either;

template<class L,class...Xs>
auto isCopyable (L l, Xs...) -> L {
  return l;
}

template<class L, class R>
auto ensureEither ( Either<L,R> const& e) -> Either<L,R> {
  return e;
}

template<class L, class R>
auto ensureEither ( Either<L,R> && e) -> Either<L,R> {
  return e;
}

template<class L, class R>
auto ensureEitherRight ( Either<L,R> const& e, R) -> Either<L, R> {
  return e;
}


template<class L, class R>
auto ensureEitherRight ( Either<L,R>&& e, R&&) -> Either<L, R> {
  return e;
}


template<class L, class R>
auto ensureEitherLeft ( Either<L,R> const& e, L) -> Either<L, R> {
  return e;
}

template<class L, class R>
auto ensureEitherLeft ( Either<L,R>&& e, L&&  ) -> Either<L, R> {
  return e;
}







template<class L, class R>
struct Either {

  union {
    L leftValue;
    R rightValue;
  };

  bool const isLeft = 0;

  constexpr Either( Left<L> const& l )
    : leftValue{l.value}
    , isLeft(1)
  {}

  constexpr Either( Right<R> const& r )
    : rightValue{r.value}
    , isLeft(0)
  {}

  Either(Left<L> && l )
    : leftValue{std::move(l.value)}
    , isLeft(1)
  {}


  Either( Right<R> && r )
    : rightValue{std::move(r.value)}
    , isLeft(0)
  {}

  constexpr Either( Either<L, R> const& e )
    : isLeft(e.isLeft) {
    if (isLeft) {
      new (&leftValue)L(e.leftValue);
    } else {
      new (&rightValue)R(e.rightValue);
    }
  }


  Either( Either<L, R>&& e )
    : isLeft(e.isLeft) {
    if (isLeft) {
      new (&leftValue)L(std::move(e.leftValue));
    } else {
      new (&rightValue)R(std::move(e.rightValue));
    }
  }


  ~Either() {
    if (isLeft) {
      leftValue.~L();
    } else {
      rightValue.~R();
    }
  }

  constexpr auto left() const -> Maybe<L> {
    if (!isLeft)
      return maybe();
    return maybe(leftValue);
  }

  constexpr auto right() const -> Maybe<R> {
    if(isLeft)
      return maybe();
    return maybe(rightValue);
  }


  static constexpr auto leftOf( L const& l ) {
    return Either<L, R>{ neither::left(l) };
  }

  static constexpr auto rightOf( R const& r ) {
    return Either<L, R>{ neither::right(r) };
  }


  static constexpr auto leftOf( L && l ) {
    return Either<L, R>{ neither::left(std::move(l)) };
  }

  static constexpr auto rightOf( R && r ) {
    return Either<L, R>{ neither::right(std::move(r)) };
  }


  template<
    class L2 = L,
    class R2 = R>
    constexpr auto join() const
    -> decltype(
      isCopyable(leftValue, rightValue),
      std::declval<std::common_type_t<L2, R2>>()
    ) {
    return isLeft?  leftValue : rightValue;
  }


  template<
    class L2 = L,
    class R2 = R>
    auto join()&&
    -> std::common_type_t<L2, R2> {
    return isLeft? std::move(leftValue) : std::move(rightValue);
  }

  template<class LeftF, class RightF>
  constexpr auto join(LeftF const& leftCase, RightF const&  rightCase) const
    -> decltype( isLeft? leftCase( leftValue ) : rightCase( rightValue ) ) {
    return isLeft? leftCase( leftValue ) : rightCase( rightValue );
  }

  template<class F>
  constexpr auto leftMap(F const& leftCase) const& -> Either<decltype(leftCase( isCopyable(leftValue, rightValue) )), R> {
    using NextEither = Either<decltype(leftCase(leftValue)), R>;
    return isLeft ?
      NextEither::leftOf( leftCase( leftValue ) ) :
      NextEither::rightOf( rightValue );
  }

  template<class F>
  auto leftMap(F const& leftCase)&& -> Either<decltype(leftCase(std::move(leftValue))), R> {
    using NextEither = Either<decltype(leftCase(std::move(leftValue))), R>;
    return isLeft ?
      NextEither::leftOf(leftCase(std::move(leftValue))) :
      NextEither::rightOf( std::move(rightValue) );
  }

  template<class F>
  constexpr auto rightMap(F const& rightCase) const& -> Either<L, decltype(rightCase(isCopyable(rightValue, leftValue)))> {
    using NextEither = Either<L, decltype(rightCase(rightValue))>;
    return isLeft ?
      NextEither::leftOf( leftValue ) :
      NextEither::rightOf( rightCase( rightValue ) );
  }

  template<class F>
  auto rightMap(F const& rightCase)&& -> Either<L, decltype(rightCase(std::move(rightValue)))> {
    using NextEither = Either<L, decltype(rightCase(std::move(rightValue)))>;
    return isLeft ?
      NextEither::leftOf( std::move(leftValue) ) :
      NextEither::rightOf( rightCase( std::move(rightValue) ) );
  }

  template<class LeftCase>
  constexpr auto leftFlatMap(LeftCase const& leftCase) const&
    -> decltype( ensureEitherRight(leftCase(isCopyable(leftValue)), isCopyable(rightValue))) {
    using NextEither = decltype(leftCase(leftValue));

    if (!*this) {
      return leftCase( leftValue );
    }

    return NextEither::rightOf(rightValue);
  }

  template<class RightCase>
  constexpr auto rightFlatMap(RightCase const& rightCase) const&
    -> decltype( ensureEitherLeft(rightCase(isCopyable(rightValue)), isCopyable(leftValue))) {
    using NextEither = decltype(rightCase(rightValue));

    if (*this) {
      return rightCase( rightValue );
    }

    return NextEither::leftOf(leftValue);
  }



  template<class LeftCase>
  auto leftFlatMap(LeftCase const& leftCase)&&
    -> decltype( ensureEitherRight(leftCase(std::move(leftValue)), std::move(rightValue))) {
    using NextEither = decltype(leftCase(std::move(leftValue)));

    if (!*this) {
      return leftCase( std::move(leftValue) );
    }

    return NextEither::rightOf(std::move(rightValue));
  }

  template<class RightCase>
  auto rightFlatMap(RightCase const& rightCase)&&
    -> decltype( ensureEitherLeft(rightCase(std::move(rightValue)), std::move(leftValue))) {
    using NextEither = decltype(rightCase(std::move(rightValue)));

    if (*this) {
      return rightCase(std::move(rightValue));
    }

    return NextEither::leftOf(std::move(leftValue));
  }


  constexpr operator bool()const { return !isLeft; }

};

}

#endif

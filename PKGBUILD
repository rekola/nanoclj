pkgname=nanoclj-git
pkgver=r604.97c66d9
pkgrel=1
pkgdesc="A tiny Clojure interpreter"
arch=('x86_64')
url="https://github.com/rekola/nanoclj"
license=('custom:nanoclj')
depends=('libsixel' 'shapelib' 'pcre2' 'libutf8proc' 'cairo' 'libcurl-gnutls' 'libxml2' 'zlib')
options=()
install=
source=("git+https://github.com/rekola/nanoclj")
noextract=()
md5sums=('SKIP')

pkgver() {
	cd "$srcdir/nanoclj"
  printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short=7 HEAD)"
}

build() {
	cd "$srcdir/nanoclj"
  mkdir -p build
  cd build 
  cmake .. 
  make
}

package() {
	cd "$srcdir/nanoclj"
	install -Dm755 "$srcdir/nanoclj/build/nanoclj" "$pkgdir/usr/bin/nanoclj"
  install -Dm644 COPYING "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
  cd build
  for f in *.clj; do
     install -Dm644 ${f} "$pkgdir/usr/share/nanoclj/${f}"
  done
  cd tests
  for f in *.clj; do
     install -Dm644 ${f} "$pkgdir/usr/share/nanoclj/tests/${f}"
  done
}

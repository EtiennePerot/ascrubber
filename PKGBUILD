# Maintainer: Etienne Perot <etienne@perot.me>
pkgname=ascrubber-git
pkgver=20120721
pkgrel=1
pkgdesc="Scrub audio files from potential fingerprints."
arch=('i686' 'x86_64')
url='https://github.com/EtiennePerot/ascrubber'
license=('BSD' 'MIT')
depends=('flac')
makedepends=('git' 'cmake')

_gitroot='git://perot.me/ascrubber'
_gitname='ascrubber'

build() {
  cd "$srcdir"
  msg "Connecting to GIT server...."

  if [[ -d "$_gitname" ]]; then
    cd "$_gitname" && git pull origin
    msg "The local files are updated."
  else
    git clone "$_gitroot" "$_gitname"
  fi

  msg "GIT checkout done or server timeout"
  msg "Starting build..."

  rm -rf "$srcdir/$_gitname-build"
  git clone "$srcdir/$_gitname" "$srcdir/$_gitname-build"
  cd "$srcdir/$_gitname-build"

  mkdir build
  cd build
  cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX:PATH=/usr ..
  make
}

package() {
  cd "$srcdir/$_gitname-build"
  install -D -m644 LICENSE "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE"
  install -D -m644 LICENSE.optionparser "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE.optionparser"
  install -D -m644 CMake/LICENSE.FindFLAC "${pkgdir}/usr/share/licenses/${pkgname}/LICENSE.FindFLAC"
  cd build
  make DESTDIR="$pkgdir/" install
}

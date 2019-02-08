pkgname=jtuner
pkgver=0.1
pkgrel=1
arch=('x86_64')
depends=('alsa-lib' 'gtk2')

build() {
    cd ..
    make
}

package() {
    cd ..
    make DESTDIR="${pkgdir}" install
}
